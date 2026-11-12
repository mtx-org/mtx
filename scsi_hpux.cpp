/* Copyright 1997, 1998 Leonard Zubkoff <lnz@dandelion.com>
   Changes copyright 2000 Eric Green <eric@badtux.org>

  This program is free software; you may redistribute and/or modify it under
  the terms of the GNU General Public License Version 2 as published by the
  Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY, without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for complete details.

struct sctl_io
{
	unsigned flags;                 // IN: SCTL_READ
	unsigned cdb_length;            // IN
	unsigned char cdb[16];          // IN 
	void *data;                     // IN 
	unsigned data_length;           // IN 
	unsigned max_msecs;             // IN: milli-seconds before abort 
	unsigned data_xfer;             // OUT 
	unsigned cdb_status;            // OUT: SCSI status 
	unsigned char sense[256];       // OUT 
	unsigned sense_status;          // OUT: SCSI status 
	unsigned sense_xfer;            // OUT: bytes of sense data received 
	unsigned reserved[16];          // IN: Must be zero; OUT: undefined 
};

*/


/* Hockey Pux may define these. If so, *UN*define them. */
#ifdef ILI
#undef ILI
#endif

#ifdef EOM
#undef EOM
#endif

/* This is the SCSI commands for HPUX. */

#define LONG_PRINT_REQUEST_SENSE  /* Sigh! */

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

#define MTX_HZ 1000
#define DEFAULT_HZ (5*60*MTX_HZ)

static int sctl_io_timeout=DEFAULT_HZ;		/* default timeout is 5 minutes. */


void SCSI_Set_Timeout(int to)
{
	sctl_io_timeout=to*60*MTX_HZ;
}

void SCSI_Default_Timeout(void)
{
	sctl_io_timeout=DEFAULT_HZ;
}


int SCSI_ExecuteCommand(DEVICE_TYPE DeviceFD,
						Direction_T Direction,
						CDB_T *CDB,
						int CDB_Length,
						void *DataBuffer,
						int DataBufferLength,
						RequestSense_T *RequestSense)
{
	int ioctl_result;
	struct sctl_io Command;

	int i;

	memset(&Command, 0, sizeof(struct sctl_io));
	memset(RequestSense, 0, sizeof(RequestSense_T));

	switch (Direction)
	{
	case Input:
		if (DataBufferLength > 0)
			memset(DataBuffer, 0, DataBufferLength);
		Command.flags =  SCTL_READ | SCTL_INIT_SDTR;
		break;

	case Output:
		Command.flags = SCTL_INIT_WDTR | SCTL_INIT_SDTR;
		break;
	}

	Command.max_msecs = sctl_io_timeout;	/* Set timeout to <n> minutes. */
	memcpy(Command.cdb, CDB, CDB_Length);
	Command.cdb_length = CDB_Length;
	Command.data = DataBuffer;
	Command.data_length = DataBufferLength;
	ioctl_result=ioctl(DeviceFD, SIOC_IO, &Command);
	SCSI_Default_Timeout();			/* change the default back to 5 minutes */

	if (ioctl_result < 0)
	{
		perror("mtx");
		return ioctl_result;
	}

	if (Command.sense_xfer > sizeof(RequestSense_T))
	{
		Command.sense_xfer=sizeof(RequestSense_T);
	}

	if (Command.sense_xfer)
	{
		memcpy(RequestSense, Command.sense, Command.sense_xfer);
	}

	return Command.sense_status;
}
