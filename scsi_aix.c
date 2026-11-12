/* Changes 2003 Steve Heck <steve.heck@am.sony.com>
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


/* This is the SCSI commands for AIX using GSC Generic SCSI Interface. */

#define LONG_PRINT_REQUEST_SENSE  /* sigh! */

DEVICE_TYPE SCSI_OpenDevice(char *DeviceName)
{
	int DeviceFD = open(DeviceName, 0); 

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

static int timeout = 9 * 60;

void SCSI_Set_Timeout(int to)
{
	timeout = to;
}

void SCSI_Default_Timeout(void)
{
	timeout = 9 * 60; /* the default */
}

#ifdef DEBUG
int SCSI_DumpBuffer(int DataBufferLength, unsigned char *DataBuffer)
{
	int i, j;
	j = 0;

	for (i = 0; i < DataBufferLength; i++)
	{
		if (j == 25)
		{
			fprintf(stderr, "\n");
			j = 0;
		}

		if (j == 0)
		{
			fprintf(stderr, "%04x:", i);
		}

		if (j > 0)
		{
			fprintf(stderr, " ");
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
	char sbyte;
	scmd_t scmd;

#ifdef DEBUG_SCSI
	fprintf(stderr,"------CDB--------\n");
	SCSI_DumpBuffer(CDB_Length,(char *)CDB);
#endif

	/* memset(&scmd, 0, sizeof(struct scmd_t)); */
	/* memset(RequestSense, 0, sizeof(RequestSense_T)); */
	switch (Direction)
	{
	case Input:
		scmd.rw = 1;
		if (DataBufferLength > 0)
		{
			memset(DataBuffer, 0, DataBufferLength);
		}
		break;

	case Output:
		scmd.rw = 2;
		break;
	}
	/* Set timeout to 5 minutes. */
#ifdef DEBUG_TIMEOUT
	fprintf(stderr,"timeout=%d\n",timeout);
	fflush(stderr);
#endif

	scmd.timeval = timeout;

	scmd.cdb = (caddr_t) CDB;
	scmd.cdblen = CDB_Length;
	scmd.data_buf = DataBuffer;
	scmd.datalen = DataBufferLength;
	scmd.sense_buf = (caddr_t) RequestSense;
	scmd.senselen = sizeof(RequestSense_T);
	scmd.statusp = &sbyte;
	ioctl_result = ioctl(DeviceFD, GSC_CMD, (caddr_t) &scmd);

	SCSI_Default_Timeout(); /* set it back to default, sigh. */

	if (ioctl_result < 0)
	{
#ifdef DEBUG
		perror("mtx");
#endif
		return ioctl_result;
	}

	if (sbyte != 0)
	{
		return -1;
	}
#ifdef DEBUG_SCSI
	if (Direction==Input)
	{
		fprintf(stderr,"--------input data-----------\n");
		SCSI_DumpBuffer(DataBufferLength,DataBuffer);
	}
#endif
	return 0;
}
