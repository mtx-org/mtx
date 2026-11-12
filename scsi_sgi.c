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

/* This is the SCSI commands for SGI Iris */
DEVICE_TYPE SCSI_OpenDevice(char *DeviceName)
{
	dsreq_t *DeviceFD = dsopen(DeviceName, O_RDWR | O_EXCL);
	if (DeviceFD == 0)
		FatalError("cannot open SCSI device '%s' - %m\n", DeviceName);
	return (DEVICE_TYPE) DeviceFD;
}


void SCSI_CloseDevice(char *DeviceName, DEVICE_TYPE DeviceFD)
{
	dsclose((dsreq_t *) DeviceFD);
}

#define MTX_HZ 1000 
#define MTX_DEFAULT_SCSI_TIMEOUT 60*5*MTX_HZ /* 5 minutes! */

static int mtx_default_timeout = MTX_DEFAULT_SCSI_TIMEOUT ;
void SCSI_Set_Timeout(int sec)
{
	mtx_default_timeout=sec*MTX_HZ;
}

void SCSI_Default_Timeout()
{
	mtx_default_timeout=MTX_DEFAULT_SCSI_TIMEOUT;
}

int SCSI_ExecuteCommand(DEVICE_TYPE DeviceFD,
						Direction_T Direction,
						CDB_T *CDB,
						int CDB_Length,
						void *DataBuffer,
						int DataBufferLength,
						RequestSense_T *RequestSense)
{
	dsreq_t *dsp = (dsreq_t *) DeviceFD;
	int Result;
	memset(RequestSense, 0, sizeof(RequestSense_T));
	memcpy(CMDBUF(dsp), CDB, CDB_Length);

	if (Direction == Input)
	{
		memset(DataBuffer, 0, DataBufferLength);
		filldsreq(dsp, (unsigned char *) DataBuffer, DataBufferLength, DSRQ_READ | DSRQ_SENSE);
	}
	else
		filldsreq(dsp, (unsigned char *) DataBuffer, DataBufferLength, DSRQ_WRITE | DSRQ_SENSE);

	/* Set 5 minute timeout. */
	/* TIME(dsp) = 300 * 1000; */
	TIME(dsp) = mtx_default_timeout;
	Result = doscsireq(getfd((dsp)), dsp);
	
	if (SENSESENT(dsp) > 0)
	{
		memcpy(RequestSense, SENSEBUF(dsp), min(sizeof(RequestSense_T), SENSESENT(dsp)));
	}

	SCSI_Default_Timeout(); /* reset the mtx default timeout */
	return Result;
}
