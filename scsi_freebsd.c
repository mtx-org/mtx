/* Copyright 2000 Enhanced Software Technologies Inc. (http://www.estinc.com)
   Copyright 2007-2008 by Robert Nelson <robertn@the-nelsons.org>
   Written by Eric Lee Green <eric@badtux.org>

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

/* This is the SCSI commands for FreeBSD */
DEVICE_TYPE SCSI_OpenDevice(char *DeviceName)
{
	struct cam_device *DeviceFD = cam_open_pass(DeviceName, O_RDWR | O_EXCL, NULL);
	if (DeviceFD == 0)
		FatalError("cannot open SCSI device '%s' - %m\n", DeviceName);
	return (DEVICE_TYPE) DeviceFD;
}


void SCSI_CloseDevice(char *DeviceName, DEVICE_TYPE DeviceFD)
{
	cam_close_device((struct cam_device *) DeviceFD);
}

#define PASS_HZ 1000*60
#define PASS_DEFAULT_TIMEOUT 5*PASS_HZ
static int pass_timeout = PASS_DEFAULT_TIMEOUT;

void SCSI_Set_Timeout(int secs)
{
	pass_timeout=secs*PASS_HZ;
}

void SCSI_Default_Timeout(void) {
	pass_timeout=5*PASS_HZ;
}

int SCSI_ExecuteCommand(DEVICE_TYPE DeviceFD,
						Direction_T Direction,
						CDB_T *CDB,
						int CDB_Length,
						void *DataBuffer,
						int DataBufferLength,
						RequestSense_T *RequestSense)
{
	struct cam_device *dsp = (struct cam_device *) DeviceFD;
	int retval;
	union ccb *ccb;
	CDB_T *cdb;
	int Result;

	ccb = cam_getccb(dsp);
	cdb = (CDB_T *) &ccb->csio.cdb_io.cdb_bytes; /* pointer to actual cdb. */

	/* cam_getccb() zeros the CCB header only. So now clear the
	* payload portion of the ccb.
	*/
	bzero(&(&ccb->ccb_h)[1], sizeof(struct ccb_scsiio) - sizeof(struct ccb_hdr));

	/* copy the CDB... */
	memcpy(cdb,CDB,CDB_Length);

	/* set the command control block stuff.... the rather involved
	* conditional expression sets the direction to NONE if there is no
	* data to go in or out, and IN or OUT if we want data. Movement
	* commands will have no data buffer, just a CDB, while INQUIRY and
	* READ_ELEMENT_STATUS will have input data, and we don't have any 
	* stuff that outputs data -- yet -- but we may eventually. 
	*/
	cam_fill_csio(	&ccb->csio,
					1,							/* retries */
					NULL,						/* cbfcnp*/
					(DataBufferLength ? 
						(Direction == Input ? CAM_DIR_IN : CAM_DIR_OUT) : 
						CAM_DIR_NONE),			/* flags */
					MSG_SIMPLE_Q_TAG,			/* tag action */
					DataBuffer,					/* data ptr */
					DataBufferLength,			/* xfer_len */
					SSD_FULL_SIZE,				/* sense_len */
					CDB_Length,					/* cdb_len */
					pass_timeout				/* timeout */ /* should be 5 minutes or more?! */
					); 

	pass_timeout = PASS_DEFAULT_TIMEOUT; /* make sure it gets reset. */
	memset(RequestSense, 0, sizeof(RequestSense_T)); /* clear sense buffer... */

	if (Direction == Input)
	{
		memset(DataBuffer, 0, DataBufferLength);
	}

	Result = cam_send_ccb(DeviceFD,ccb);
	if (Result < 0 || 
		(ccb->ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP)
	{
		/* copy our sense data, sigh... */
		memcpy(RequestSense,(void *) &ccb->csio.sense_data,
		min(sizeof(RequestSense_T), sizeof(struct scsi_sense_data)));

		cam_freeccb(ccb);
		return -1; /* sorry!  */
	}

	/* okay, we did good, maybe? */
	cam_freeccb(ccb);
	return 0; /* and done? */
}
