/* Copyright 1997, 1998 Leonard Zubkoff <lnz@dandelion.com>
   Changes copyright 2000 Eric Green <eric@badtux.org>
   Copyright 2007-2008 by Robert Nelson <robertn@the-nelsons.org>

$Date: 2008-08-19 03:03:38 -0700 (Tue, 19 Aug 2008) $
$Revision: 193 $

  This program is free software; you may redistribute and/or modify it under
  the terms of the GNU General Public License Version 2 as published by the
  Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY, without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for complete details.

*/

/* This is the SCSI commands for Sun Solaris. */

#define LONG_PRINT_REQUEST_SENSE  /* sigh! */

DEVICE_TYPE SCSI_OpenDevice(char *DeviceName)
{
	int DeviceFD = open(DeviceName, O_RDWR | O_NDELAY);
	if (DeviceFD < 0)
		FatalError("cannot open SCSI device '%s' - %m\n", DeviceName);
	return (DEVICE_TYPE) DeviceFD;
}


void SCSI_CloseDevice(char *DeviceName, DEVICE_TYPE DeviceFD)
{
	if (close(DeviceFD) < 0)
		FatalError("cannot close SCSI device '%s' - %m\n", DeviceName);
}


#define HAS_SCSI_TIMEOUT

static int uscsi_timeout=5*60; 

void SCSI_Set_Timeout(int to)
{
	uscsi_timeout = to;
}

void SCSI_Default_Timeout(void)
{
	uscsi_timeout=5*60; /* the default */
}

#ifdef DEBUG
int SCSI_DumpBuffer(int DataBufferLength, unsigned char *DataBuffer)
{
	int i,j;
	j = 0;

	for (i = 0; i < DataBufferLength; i++)
	{
		if (j == 25)
		{
			fprintf(stderr,"\n");
			j = 0;
		}

		if (j == 0)
		{
			fprintf(stderr, "%04x:", i);
		}

		if (j > 0)
		{
			fprintf(stderr," ");
		}

		fprintf(stderr, "%02x", (int)DataBuffer[i]);
		j++;
	}
	fprintf(stderr, "\n");
}
#endif


int SCSI_ExecuteCommand(DEVICE_TYPE DeviceFD,
						Direction_T Direction,
						CDB_T *CDB,
						int CDB_Length,
						void *DataBuffer,
						int DataBufferLength,
						RequestSense_T *RequestSense)
{
	int ioctl_result;
	struct uscsi_cmd Command;

#ifdef DEBUG_SCSI
	fprintf(stderr,"------CDB--------\n");
	SCSI_DumpBuffer(CDB_Length,(char *)CDB);
#endif

	memset(&Command, 0, sizeof(struct uscsi_cmd));
	memset(RequestSense, 0, sizeof(RequestSense_T));
	switch (Direction)
	{
	case Input:
		Command.uscsi_flags = USCSI_DIAGNOSE | USCSI_ISOLATE | USCSI_RQENABLE;
		if (DataBufferLength > 0)
		{
			memset(DataBuffer, 0, DataBufferLength);
			Command.uscsi_flags |= USCSI_READ;
		}
		break;
	case Output:
		Command.uscsi_flags = USCSI_DIAGNOSE | USCSI_ISOLATE |
								USCSI_WRITE | USCSI_RQENABLE;
		break;
	}
  /* Set timeout to 5 minutes. */
#ifdef DEBUG_TIMEOUT
	fprintf(stderr,"uscsi_timeout=%d\n",uscsi_timeout);
	fflush(stderr);
#endif
	Command.uscsi_timeout = uscsi_timeout;

	Command.uscsi_cdb = (caddr_t) CDB;
	Command.uscsi_cdblen = CDB_Length;
	Command.uscsi_bufaddr = DataBuffer;
	Command.uscsi_buflen = DataBufferLength;
	Command.uscsi_rqbuf = (caddr_t) RequestSense;
	Command.uscsi_rqlen = sizeof(RequestSense_T);
	ioctl_result = ioctl(DeviceFD, USCSICMD, &Command);

	SCSI_Default_Timeout(); /* set it back to default, sigh. */

	if (ioctl_result < 0)
	{
#ifdef DEBUG
		perror("mtx");
#endif
		return ioctl_result;
	}

	if (RequestSense->ErrorCode > 1)
	{
		return -1;
	}

#ifdef DEBUG_SCSI
	if (Direction==Input)
	{
		fprintf(stderr,"--------input data-----------\n");
		SCSI_DumpBuffer(DataBufferLength, DataBuffer);
	}
#endif
	return 0;
}
