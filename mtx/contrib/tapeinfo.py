# Copyright 2000 Enhanced Software Technologies Inc.
# All Rights Reserved
# Released under Free Software Foundation's General Public License,
# Version 2 or above

# Routine to call 'tapeinfo' and read status for a node. This is an
# example of how to parse the 'tapeinfo' output from a scripting language.
#

import os
import string
import sys


configdir="/opt/brupro/bin"  # sigh.

def inquiry(device):
    retval={}

    # okay, now do the thing:

    command="%s/tapeinfo -f %s" % (configdir,device)

    # Now to read:

    infile=os.popen(command,"r")

    try:
        s=infile.readline()
    except:
        s=""
        pass
    if not s:
        return None # did not get anything.
    while s:
        s=string.strip(s)
        idx,val=string.split(s,':',1)
        val=string.strip(val)
        if val[0]=="'":
            val=val[1:-1] # strip off single quotes, sigh.
            val=string.strip(val)
            pass
        while "\0" in val:
            # zapo!
            val=string.replace(val,"\0","")
            pass
        retval[idx]=val
        try:
            s=infile.readline()
        except:
            s=""
            pass
        continue # to top of loop!
    return retval

