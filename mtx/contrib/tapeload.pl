!/usr/bin/perl
##########################  tapeload   ###########################
# This script uses mtx 1.2.9pre2 to load a tape based 
# on its volume tag.  You can
# specify a tape drive by number, but if you don`t, it puts the
# tape in the first available drive and returns the number of that drive,
# both from the standard output and as the exit value.
# A negative exit value indicates an error.
# If volume tags are missing from any full slot, bar codes are rescanned
# automatically.  
#
# usage: 
# tapeload TAPE_LABEL_1    # Loads tape with label TAPE_LABEL_1 into a drive
# or
# tapeload TAPE_LABEL_1 1  # Loads tape with label TAPE_LABEL_1 into drive #1
#

# Set this variable to your mtx binary and proper scsi library device.
$MTXBIN="/usr/local/bin/mtx -f /dev/sga" ;  

# Additions and corrections are welcome.
# This software comes with absolutely no warranty and every other imaginable
# disclaimer.
#   -- Frank Samuelson sam@milagro.lanl.gov
##################################################################

@wt= &mdo("status");  #slurp in the list of slots

# Check to be certain that every full slot has a volume tag
for ($i=0; $i< $#wt; $i++) {  # look through every line
    if ( $wt[$i] =~ /Full/  && $wt[$i] !~ /VolumeTag/ ) {
        # if the element is full, but there is no volume tag, do inventory
        @wt= &mdo("inventory status");
        break;
    }
}

#try to find our tape
$slot=-1;
for ($i=0; $i< $#wt; $i++) {  # look through every line
    if ($wt[$i] =~ / *Storage Element (d*):Full :VolumeTag=(.*)/ ) {
        if ($ARGV[0] eq  $2) { # We found the tape
            $slot=$1;          # set the slot number
            break;             # stop reading the rest of the file.
        }
    }
}

if ( $slot>0) {         # we found the tape you wanted.

    $drivefound=-1;          # set flag to bad value
    for ($i=0; $i< $#wt; $i++) {  # look through every line
        # if this is a tape drive
        if ($wt[$i] =~ / *Data Transfer Element (d*):(.*)/ ) { #parse the line
            $drive=$1;
            $state=$2;
#           print STDERR "$wt[$i] $drive $state";
            if ($state =~ /Full/) {   # This drive is full.
                # if we are looking for a particular drive and this is it
                if ( $#ARGV==1 && $drive == $ARGV[1]) { 
                    print STDERR " ERROR: Specified drive $ARGV[1] is full.";
                    print STDERR @wt;
                    exit(-6);
                }
            } elsif ($state =~ /Empty/) { #This is a tape drive and it`s empty.
                if ( $#ARGV==1 ) {          # If we want a particular drive
                    if ($drive == $ARGV[1]) {   # and this is it,
                        $drivefound=$drive;      # mark it so.
                        break;
                    }
                } else {              # If any old drive will do
                    $drivefound=$drive;    # Mark it.
                    break;
                }
            } else {         # This is a tape drive, but what the heck is it?
                print STDERR " Cannot assess drive status in line";
                print STDERR $wt[$i];
                exit(-7);
            }
        }
    }

    if ( $drivefound < 0 ) {  # specified drive was not found
        print STDERR "Error: Specified drive $ARGV[1] was not found";
        print STDERR @wt;
        exit(-8);
    }
    # Now we actually load the tape.
    @dump=&mdo(" load $slot $drivefound ");
    print "$drivefound";
    exit($drivefound);
    # The end.

    
} else {
    print STDERR " Ug. Tape $ARGV[0] is not in the library.";
    print STDERR @wt;
    exit(-4);
}


sub mdo             # a subroutine to call mtx ;
{
#    print STDERR "$_[0]";
    if (!open(FD,"$MTXBIN $_[0] |")) {    #call mtx function 
        print STDERR " ERRKK.  Could not start mtx ";
        exit (-9);
    }

    @twt= <FD>;        # slurp in the output

    if (! close(FD)) {        # if mtx exited with a nonzero value...
        print STDERR " Mtx gave an error. Tapeload is exiting... ";
        exit (-10);
    }

    @twt;
}
