/* Mammoth 2 Debug Buffer Dumper
   Copyright 2000 Enhanced Software Technologies Inc.
   Copyright 2007-2008 by Robert Nelson <robertn@the-nelsons.org>

$Date: 2008-08-19 03:03:38 -0700 (Tue, 19 Aug 2008) $
$Revision: 193 $

   Written by Eric Lee Green <eric@badtux.org>
   Released under the terms of the GNU General Public License v2 or
    above.

   This is an example of how to use the mtx library file 'mtxl.c' to
   do a special-purpose task -- dump the Mammoth2 debug buffer, in this case. 
   Note that this debug buffer is 1M-4M in size, thus may overwhelm the
   SCSI generic subsystem on some supported platforms...

   syntax:

   mam2debug generic-filename output-filename.


*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mtx.h"
#include "mtxl.h"

/* This is a TOTALLY UNDOCUMENTED feature to read the debug data buffer
 * in an Exabyte Mammoth II and dump it to a file:
 */

static RequestSense_T *DumpM2DebugBuff(DEVICE_TYPE MediumChangerFD, int outfile) 
{
  RequestSense_T *RequestSense = xmalloc(sizeof(RequestSense_T));
  CDB_T CDB;

  unsigned char *databuffer;
  unsigned char buff_descriptor[4];
  int numbytes;
  int testbytes;

  CDB[0]=0x3c; /* command. */ 
  CDB[1]=0x03;  /* mode - read buff_descriptor! */ 
  CDB[2]=0x01;    /* page. */ 
  CDB[3]=0;   /* offset. */
  CDB[4]=0;
  CDB[5]=0;  
  CDB[6]=0;    /* length. */
  CDB[7]=0;
  CDB[8]=4;   /* the descriptor is 4 long. */ 
  CDB[9]=0;
  
  if ((testbytes=SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 10,
			  buff_descriptor, 4, RequestSense)) != 0){
    fprintf(stderr,"mam2debug: could not read buff_descriptor. [%d]\n",testbytes);
    return RequestSense;  /* couldn't do it. */ 
  }

  /* okay, read numbytes: */
  numbytes=(buff_descriptor[1]<<16) + (buff_descriptor[2]<<8) + buff_descriptor[3];
  databuffer=(unsigned char *) xmalloc(numbytes+1000000); /* see if this helps :-(. */
  CDB[6]=buff_descriptor[1];
  CDB[7]=buff_descriptor[2];
  CDB[8]=buff_descriptor[3];

  CDB[1]=0x02; /* mode -- read buffer! */
  
  if (SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 10,
			  databuffer, numbytes, RequestSense) != 0){
    fprintf(stderr,"mam2debug: could not read buffer.\n");
    free(databuffer);
    return RequestSense;  /* couldn't do it. */ 
  }
  
  write(outfile,databuffer,numbytes);
  close(outfile);
  free(databuffer);
  free(RequestSense);
  return NULL;  /* okay! */
}

static void usage(void) {
  fprintf(stderr,"Usage: mam2debug scsi-generic-file output-file-name\n");
  exit(1);
}

/* Now for the actual main() routine: */

int main(int argc,char** argv) {
  DEVICE_TYPE changer_fd;
  static RequestSense_T *result;
  int outfile;

  if (argc != 3) {
    usage();
  }
  
  changer_fd=SCSI_OpenDevice(argv[1]);
  
  if (changer_fd <= 0) {
    fprintf(stderr,"Could not open input device\n");
    usage();
  }

  outfile=open(argv[2],O_CREAT|O_WRONLY);
  if (outfile <=0) {
    fprintf(stderr,"Could not open output file\n");
    usage();
  }

  result=DumpM2DebugBuff(changer_fd, outfile);

  if (result) {
    PrintRequestSense(result);
    exit(1);
  }

  exit(0);
}

