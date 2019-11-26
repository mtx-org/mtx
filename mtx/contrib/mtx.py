# Copyright 2000 Enhanced Software Technologies Inc.
# Released under Free Software Foundation's General Public License,
# Version 2 or above
#
# This is an example of how to parse the 'mtx' output from a scripting
# language. 
#
# Routine to call 'mtx' and read status, or
# whatever.
#
# We do this here rather than creating a Python "C" module because:
#  1) Reduces complexity of compile environment
#  2) Easier debugging.
#  3) More in keeping with the Unix philosophy of things.
#  4) Easier to add new functionality. 
#
#

import string
import os
import time # we can do some waiting here...

def readpipe(command):
    result=""
    
    infile=os.popen(command,"r")

    try:
        s=infile.read()
    except:
        s=""
        pass
    if not s:
        return None  # didn't get anything :-(. 
    result=result+s
    return result




# these are returned by the mtx.status routine:
class slotstatus:
    def __init__(self,slotnum,middle,barcode=None):
        middle=string.strip(middle)
        try:
            left,right=string.split(middle," ")
        except:
            left=middle
            right=None
            pass
        # Note: We will not be able to test this at the moment since the
        # 220 has no import/export port!
        if right=="IMPORT/EXPORT":
            self.importexport=1
        else:
            self.importexport=0
            pass
        self.slotnum=int(slotnum) # make sure it's an integer...
        if left=="Full":
            self.full=1
        else:
            self.full=None
            pass
        if not barcode:
            self.voltag=None
        else:
            l=string.split(barcode,"=",1)
            self.voltag=l[1]
            pass
        return
    pass

# Drive status lines have this format:
#Data Transfer Element 0:Full (Storage Element 10 Loaded):VolumeTag = B0000009H
#Data Transfer Element 1:Empty

class drivestatus:
    def __init__(self,slotnum,middle,barcode=None):
        self.slotnum=slotnum
        if middle=="Empty":
            self.full=None
            self.origin=None
            self.voltag=None
            return
        else:
            self.full=1
            pass

        # okay, we know we have a tape in the drive. 
        # split and find out our origin: we will always have
        # an origin, even if the #$@% mtx program had to dig one
        # out of the air:

        l=string.split(middle," ")
        self.origin=int(l[3])

        if not barcode:
            self.voltag=None # barcode of this element.
        else:
            l=string.split(barcode," ")
            self.voltag=string.strip(l[2])
            pass
        return
    pass

# This is the return value for mtx.status. 
class mtxstatus:
    def __init__(self):
        self.numdrives=None
        self.numslots=None
        self.numexport=None
        self.drives=[]  # a list of drivestatus instances.
        self.slots=[]   # a list of slotstatus instances
        self.export=[]  # another list of slotstatus instances
        return
    pass

# call 'mtx' and get barcode info, if possible:
# okay, we now have a string that consists of a number of lines.
# we want to explode this string into its component parts.
# Example format:
# Storage Changer /dev/sgd:2 Drives, 21 Slots
# Data Transfer Element 0:Full (Storage Element '5' Loaded) 
# Data Transfer Element 1:Empty
# Storage Element 1:Full :VolumeTag=CLNA0001
# Storage Element 2:Full :VolumeTag=B0000009
# Storage Element 3:Full :VolumeTag=B0000010
# ....
# What we want to do, then, is:
# 1) Turn it into lines by doing a string.split on newline.
# 2) Split the 1st line on ":" to get left and right.
# 3) Split the right half on space to get #drives "Drives," #slots
# 4) process the drives (Full,Empty, etc.)
# 5) For each of the remaining lines: Split on ':'
# 6) Full/Empty status is in 2)
#
configdir="/opt/brupro/bin"  # sigh. 

def status(device):
    retval=mtxstatus()
    command="%s/mtx -f %s status" % (configdir,device)
    result=readpipe(command)
    if not result:
        return None  # sorry! 
    # now to parse:

    try:
        lines=string.split(result,"\n")
    except:
        return None # sorry, no status!
    
    # print "DEBUG:lines=",lines

    try:
        l=string.split(lines[0],":")
    except:
        return None # sorry, no status!
    
    # print "DEBUG:l=",l
    leftside=l[0]
    rightside=l[1]
    if len(l) > 2:
        barcode=l[3]
    else:
        barcode=None
        pass
    t=string.split(rightside)
    retval.numdrives=int(t[0])
    retval.numslots=int(t[2])
    
    for s in lines[1:]:
        if not s:
            continue # skip blank lines!
        #print "DEBUG:s=",s
        parts=string.split(s,":")
        leftpart=string.split(parts[0])
        rightpart=parts[1]
        try:
            barcode=parts[2]
        except:
            barcode=None
            pass
        #print "DEBUG:leftpart=",leftpart
        if leftpart[0]=="Data" and leftpart[1]=="Transfer":
            retval.drives.append(drivestatus(leftpart[3],rightpart,barcode))
            pass
        if leftpart[0]=="Storage" and leftpart[1]=="Element":
            element=slotstatus(leftpart[2],rightpart,barcode)
            if element.importexport:
                retval.export.append(element)
            else:
                retval.slots.append(element)
                pass
            pass
        continue

    return retval

# Output of a mtx inquiry looks like:
#
#Product Type: Medium Changer
#Vendor ID: 'EXABYTE '
#Product ID: 'Exabyte EZ17    '
#Revision: '1.07'
#Attached Changer: No
#
# We simply return a hash table with these values { left:right } format. 

def mtxinquiry(device):
    command="%s/mtx -f %s inquiry" % (configdir,device)
    str=readpipe(command) # calls the command, returns all its data.

    str=string.strip(str)
    lines=string.split(str,"\n")
    retval={}
    for l in lines:
        # DEBUG #
        l=string.strip(l)
        print "splitting line: '",l,"'"
        idx,val=string.split(l,':',1)
        val=string.strip(val)
        if val[0]=="'":
            val=val[1:-1] # strip off single quotes, sigh. 
            pass 
        retval[idx]=val
        continue
    return retval

# Now for the easy part:

def load(device,slot,drive=0):
    command="%s/mtx -f %s load %s %s >/dev/null " % (configdir,device,slot,drive)
    status=os.system(command)
    return status

def unload(device,slot,drive=0):
    command="%s/mtx -f %s unload %s %s >/dev/null " % (configdir,device,slot,drive)
    return os.system(command)

def inventory(device):
    command="%s/mtx -f %s inventory >/dev/null " % (configdir,device)
    return os.system(command)

def wait_for_inventory(device):
    # loop while we have an error return...
    errcount=0
    while inventory(device):
        if errcount==0:
            print "Waiting for loader '%s'" % device
            pass
        time.sleep(1)
        try:
            s=status(device)
        except:
            s=None
            pass
        if s:
            return 0 # well, whatever we're doing, we're inventoried :-(
        errcount=errcount+1
        if errcount==600:   # we've been waiting for 10 minutes :-(
            return 1 # sorry!
        continue
    return 0  # we succeeded!

# RCS REVISION LOG:
# $Log$
# Revision 1.1  2001/06/05 17:10:51  elgreen
# Initial revision
#
# Revision 1.2  2000/12/22 14:17:19  eric
# mtx 1.2.11pre1
#
# Revision 1.14  2000/11/12 20:35:29  eric
# do string.strip on the voltag
#
# Revision 1.13  2000/11/04 00:33:38  eric
# if we can get an inventory on the loader after we send it 'mtx inventory',
# then obviously we managed to do SOMETHING.
#
# Revision 1.12  2000/10/28 00:04:34  eric
# added wait_for_inventory command
#
# Revision 1.11  2000/10/27 23:27:58  eric
# Added inventory command...
#
# Revision 1.10  2000/10/01 01:06:29  eric
# evening checkin
#
# Revision 1.9  2000/09/29 02:49:29  eric
# evening checkin
#
# Revision 1.8  2000/09/02 01:05:33  eric
# Evening Checkin
#
# Revision 1.7  2000/09/01 00:08:11  eric
# strip lines in mtxinquiry
#
# Revision 1.6  2000/09/01 00:05:33  eric
# debugging
#
# Revision 1.5  2000/08/31 23:46:01  eric
# fix def:
#
# Revision 1.4  2000/08/31 23:44:06  eric
# =->==
#
