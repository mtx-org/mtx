/* SCSI.C - Digital Unix-specific SCSI routines.
**
** TECSys Development, Inc., April 1998
**
** This module began life as a part of XMCD, an X-windows CD player
** program for many platforms. No real functionality from the original XMCD
** is present in this module, but in the interest of making certain that
** proper credit is given where it may be due, the copyrights and inclusions
** from the XMCD module OS_DEC.C are included below.
**
** The portions of coding in this module ascribable to TECSys Development
** are hereby also released under the terms and conditions of version 2
** of the GNU General Public License as described below....
*/

/*
 *   libdi - scsipt SCSI Device Interface Library
 *
 *   Copyright (C) 1993-1997  Ti Kan
 *   E-mail: ti@amb.org
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free
 *   Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *   Digital UNIX (OSF/1) and Ultrix support
 *
 *   Contributing author: Matt Thomas
 *   E-Mail: thomas@lkg.dec.com
 *
 *   This software fragment contains code that interfaces the
 *   application to the Digital UNIX and Ultrix operating systems.
 *   The term Digital, Ultrix and OSF/1 are used here for identification
 *   purposes only.
 */

static int	bus = -1,
			target = -1,
			lun = -1;

static int SCSI_OpenDevice(char *DeviceName)
{
	int		fd;
	struct stat	stbuf;
	int		saverr;

	/* Check for validity of device node */
	if (stat(DeviceName, &stbuf) < 0)
	{
		FatalError("cannot stat SCSI device '%s' - %m\n", DeviceName);
	}
	if (!S_ISCHR(stbuf.st_mode))
	{
		FatalError("device '%s': not appropriate device type - %m\n", DeviceName);
	}

	if ((fd = open(DeviceName, O_RDONLY | O_NDELAY, 0)) >= 0)
	{
		struct devget	devget;

		if (ioctl(fd, DEVIOCGET, &devget) >= 0)
		{
#ifdef __osf__
			lun = devget.slave_num % 8;
			devget.slave_num /= 8;
#else
			lun = 0;
#endif
			target = devget.slave_num % 8;
			devget.slave_num /= 8;
			bus = devget.slave_num % 8;
			(void) close(fd);

			if ((fd = open(DEV_CAM, O_RDWR, 0)) >= 0 ||
				(fd = open(DEV_CAM, O_RDONLY, 0)) >= 0)
			{
				return (fd);
			}
			fd = bus = target = lun = -1;
			FatalError("error %d opening SCSI device '%s' - %m\n",
					 errno, DEV_CAM);
		}
		else
		{
			(void) close(fd);
			fd = bus = target = lun = -1;
			FatalError("error %d on DEVIOCGET ioctl for '%s' - %m\n",
					 errno, DeviceName);
		}
	}
	else
	{
		saverr = errno;
		fd = bus = target = lun = -1;
		FatalError("cannot open SCSI device '%s', error %d - %m\n",
				DeviceName, saverr);
	}

	fd = bus = target = lun = -1;
	return -1;
}


static void SCSI_CloseDevice(char *DeviceName, int DeviceFD)
{
	(void) close(DeviceFD);
	bus = target = lun = -1;
}


static int SCSI_ExecuteCommand(int DeviceFD,
								Direction_T Direction,
								CDB_T *CDB,
								int CDB_Length,
								void *DataBuffer,
								int DataBufferLength,
								RequestSense_T *RequestSense)
{
	UAGT_CAM_CCB	uagt;
	CCB_SCSIIO	ccb;

	if (DeviceFD < 0)
		return -1;

	(void) memset(&uagt, 0, sizeof(uagt));
	(void) memset(&ccb, 0, sizeof(ccb));

	/* Setup the user agent ccb */
	uagt.uagt_ccb = (CCB_HEADER *) &ccb;
	uagt.uagt_ccblen = sizeof(CCB_SCSIIO);

	/* Setup the scsi ccb */
	(void) memcpy((unsigned char  *) ccb.cam_cdb_io.cam_cdb_bytes,
				CDB, CDB_Length);
	ccb.cam_cdb_len = CDB_Length;
	ccb.cam_ch.my_addr = (CCB_HEADER *) &ccb;
	ccb.cam_ch.cam_ccb_len = sizeof(CCB_SCSIIO);
	ccb.cam_ch.cam_func_code = XPT_SCSI_IO;

	if (DataBuffer != NULL && DataBufferLength > 0)
	{
		ccb.cam_ch.cam_flags |= (Direction == Input) ?
			CAM_DIR_IN : CAM_DIR_OUT;
		uagt.uagt_buffer = (u_char *) DataBuffer;
		uagt.uagt_buflen = DataBufferLength;
	}
	else
		ccb.cam_ch.cam_flags |= CAM_DIR_NONE;
	
	ccb.cam_ch.cam_flags |= CAM_DIS_AUTOSENSE;
	ccb.cam_data_ptr = uagt.uagt_buffer;
	ccb.cam_dxfer_len = uagt.uagt_buflen;
	ccb.cam_timeout = 300; /* Timeout set to 5 minutes */

	ccb.cam_sense_ptr = (u_char *) RequestSense;
	ccb.cam_sense_len = sizeof(RequestSense_T);

	ccb.cam_ch.cam_path_id = bus;
	ccb.cam_ch.cam_target_id = target;
	ccb.cam_ch.cam_target_lun = lun;

	if (ioctl(DeviceFD, UAGT_CAM_IO, (caddr_t) &uagt) < 0)
	{
		return -1;
	}

	/* Check return status */
	if ((ccb.cam_ch.cam_status & CAM_STATUS_MASK) != CAM_REQ_CMP)
	{
		if (ccb.cam_ch.cam_status & CAM_SIM_QFRZN)
		{
			(void) memset(&ccb, 0, sizeof(ccb));
			(void) memset(&uagt, 0, sizeof(uagt));

			/* Setup the user agent ccb */
			uagt.uagt_ccb = (CCB_HEADER  *) &ccb;
			uagt.uagt_ccblen = sizeof(CCB_RELSIM);

			/* Setup the scsi ccb */
			ccb.cam_ch.my_addr = (struct ccb_header *) &ccb;
			ccb.cam_ch.cam_ccb_len = sizeof(CCB_RELSIM);
			ccb.cam_ch.cam_func_code = XPT_REL_SIMQ;

			ccb.cam_ch.cam_path_id = bus;
			ccb.cam_ch.cam_target_id = target;
			ccb.cam_ch.cam_target_lun = lun;

			if (ioctl(DeviceFD, UAGT_CAM_IO, (caddr_t) &uagt) < 0)
				return -1;
		}

		printf(	"mtx: %s:\n%s=0x%x %s=0x%x\n",
				"SCSI command fault",
				"Opcode",
				CDB[0],
				"Status",
				ccb.cam_scsi_status);
		return -1;
	}

	return 0;
}
