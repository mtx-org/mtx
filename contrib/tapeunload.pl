#!/usr/bin/perl
##########################  tapeunload   ###########################
# This script uses mtx 1.2.9pre2 to unload a tape 
# based on its volume tag.  You can
# specify a slot into which the tape should go, but if you don`t, it puts the
# tape into the slot from which it was taken.  This slot number
# is returned 
# both from the standard output and as the exit value.
# A negative exit value indicates an error.
# If volume tags are missing from any full slot or drive, 
# bar codes are rescanned automatically.
# Note: This script assumes that the tape is ready to be removed from
# the drive.  That means you may have to unload the tape from the drive
# with  "mt offline" before the tape can be moved to a storage slot.
# 

# usage: 
# tapeunload TAPE_LABEL_1  
# Removes tape with label TAPE_LABEL_1 from a drive and puts it
# back into its storage slot. Or, 
# tapeunload TAPE_LABEL_1 40 
# Removes tape with label TAPE_LABEL_1 from a drive and puts it
# into its storage slot 40. 
#

# Set this variable to your mtx binary and proper scsi library device.
$MTXBIN="/usr/local/bin/mtx -f /dev/sga" ;  

# Additions and corrections are welcome.
# This software comes with absolutely no warranty and every other imaginable
# disclaimer.
#   --  Frank Samuelson sam@milagro.lanl.gov

##################################################################

@wt= &mdo("status");  #slurp in the list of slots

# Check to be certain that every full slot has a volume tag
# Rescanning probably will not help.  I haven`t seen any bar code
# readers that read tapes that are in the drives.  But you never know...
for ($i=0; $i< $#wt; $i++) {  # look through every line
    if ( $wt[$i] =~ /Full/  && $wt[$i] !~ /VolumeTag/ ) {
        # if the element is full, but there is no volume tag, do inventory
        @wt= &mdo("inventory status");
        break;
    }
}

#try to find our tape
$drivein=-1;
for ($i=0; $i< $#wt; $i++) {  # look through every line
                              # for a full tape drive 
    if ($wt[$i] =~ / *Data Transfer Element (d*):Full (Storage Element
(d*) Loaded):VolumeTag = (.*)/ ){
        if ($ARGV[0] eq  $3) { # We found our tape
            $drivein=$1;          # set the drive number
            $slottogo=$2;       # set the slot number
            break;             # stop reading the rest of the file.
        }
    }
}

if ( $drivein>=0) {         # we found the tape you wanted.
    if ($#ARGV==1) {         #If an alternative slot was requested, set it.
        $slottogo=$ARGV[1];  # and let mtx handle the errors.
    }
    
    # Now we unload the tape.
    @dump=&mdo(" unload $slottogo $drivein ");
    print "$slottogo";
    exit($slottogo);
    # The end.

    
} else {
    print STDERR " Ug. Tape $ARGV[0] is not in a tape drive.";
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
