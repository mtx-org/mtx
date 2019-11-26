/* SCSI.C - VMS-specific SCSI routines.
**
** TECSys Development, Inc., April 1998
**
** This module began life as a program called CDWRITE20, a CD-R control
** program for VMS. No real functionality from the original CDWRITE20
** is present in this module, but in the interest of making certain that
** proper credit is given where it may be due, the copyrights and inclusions
** from the CDWRITE20 program are included below.
**
** The portions of coding in this module ascribable to TECSys Development
** are hereby also released under the terms and conditions of version 2
** of the GNU General Public License as described below....
*/

/* The remainder of these credits are included directly from the CDWRITE20
** sources. */

/* Copyright 1994 Yggdrasil Computing, Inc. */
/* Written by Adam J. Richter (adam@yggdrasil.com) */

/* Rewritten February 1997 by Eberhard Heuser-Hofmann*/
/* using the OpenVMS generic scsi-interface */
/* see the README-file, how to setup your machine */

/*
**  Modified March 1997 by John Vottero to use overlapped, async I/O
**  and lots of buffers to help prevent buffer underruns.
**  Also improved error reporting.
*/

/* This file may be copied under the terms and conditions of version 2
   of the GNU General Public License, as published by the Free
   Software Foundation (Cambridge, Massachusetts).

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* The second notice comes from sys$examples:gktest.c (OpenVMS 7.0)*/

/*
** COPYRIGHT (c) 1993 BY
** DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASSACHUSETTS.
** ALL RIGHTS RESERVED.
**
** THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED AND COPIED
** ONLY  IN  ACCORDANCE  OF  THE  TERMS  OF  SUCH  LICENSE  AND WITH THE
** INCLUSION OF THE ABOVE COPYRIGHT NOTICE. THIS SOFTWARE OR  ANY  OTHER
** COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY
** OTHER PERSON.  NO TITLE TO AND  OWNERSHIP OF THE  SOFTWARE IS  HEREBY
** TRANSFERRED.
**
** THE INFORMATION IN THIS SOFTWARE IS	SUBJECT TO CHANGE WITHOUT NOTICE
** AND	SHOULD	NOT  BE  CONSTRUED  AS A COMMITMENT BY DIGITAL EQUIPMENT
** CORPORATION.
**
** DIGITAL ASSUMES NO RESPONSIBILITY FOR THE USE  OR  RELIABILITY OF ITS
** SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DIGITAL.
*/

/*
  Define the Generic SCSI Command Descriptor.
*/

typedef struct scsi$desc
{
	unsigned int SCSI$L_OPCODE;	    /* SCSI Operation Code */
	unsigned int SCSI$L_FLAGS;	    /* SCSI Flags Bit Map */
	unsigned char *SCSI$A_CMD_ADDR;   /* ->SCSI Command Buffer */
	unsigned int SCSI$L_CMD_LEN;	    /* SCSI Command Length (bytes) */
	unsigned char *SCSI$A_DATA_ADDR;  /* ->SCSI Data Buffer */
	unsigned int SCSI$L_DATA_LEN;	    /* SCSI Data Length (bytes) */
	unsigned int SCSI$L_PAD_LEN;	    /* SCSI Pad Length (bytes) */
	unsigned int SCSI$L_PH_CH_TMOUT;  /* SCSI Phase Change Timeout (seconds) */
	unsigned int SCSI$L_DISCON_TMOUT; /* SCSI Disconnect Timeout (seconds) */
	unsigned int SCSI$L_RES_1;	    /* Reserved */
	unsigned int SCSI$L_RES_2;	    /* Reserved */
	unsigned int SCSI$L_RES_3;	    /* Reserved */
	unsigned int SCSI$L_RES_4;	    /* Reserved */
	unsigned int SCSI$L_RES_5;	    /* Reserved */
	unsigned int SCSI$L_RES_6;	    /* Reserved */
}
SCSI$DESC;


/*
  Define the SCSI Input/Output Status Block.
*/

typedef struct scsi$iosb
{
	unsigned short SCSI$W_VMS_STAT;		/* VMS Status Code */
	unsigned long SCSI$L_IOSB_TFR_CNT;	/* Actual Byte Count Transferred */
	unsigned char SCSI$B_IOSB_FILL_1;	/* Unused */
	unsigned char SCSI$B_IOSB_STS;		/* SCSI Device Status */
}
SCSI$IOSB;


/*
  Define the VMS symbolic representation for a successful SCSI command.
*/

#define SCSI$K_GOOD		0


/*
  Define the SCSI Flag Field Constants.
*/

#define SCSI$K_WRITE        	0x0 /* Data Transfer Direction: Write */
#define SCSI$K_READ         	0x1 /* Data Transfer Direction: Read */
#define SCSI$K_FL_ENAB_DIS  	0x2 /* Enable Disconnect/Reconnect */


/*
  Define DESCR_CNT.  It must be a power of two and large enough
  for the maximum concurrent usage of descriptors.
*/

#define DESCR_CNT 16


#define MK_EFN			0   /* Event Flag Number */
#define FailureStatusP(Status)	(~(Status) & 1)


static struct dsc$descriptor_s *descr(char *String)
{
	static struct dsc$descriptor_s d_descrtbl[DESCR_CNT];
	static unsigned short descridx = 0;
	struct dsc$descriptor_s *d_ret = &d_descrtbl[descridx];
	descridx = (descridx + 1) & (DESCR_CNT - 1);
	d_ret->dsc$w_length = strlen((const char *) String);
	d_ret->dsc$a_pointer = String;
	d_ret->dsc$b_class = 0;
	d_ret->dsc$b_dtype = 0;

	return d_ret;
}


static int SCSI_OpenDevice(char *DeviceName)
{
	unsigned long d_dev[2], iosb[2], Status;
	union prvdef setprivs, newprivs;
	unsigned long ismnt = 0;
	unsigned long dvcls = 0;
	unsigned long dchr2 = 0;
	int DeviceFD = 0;
	struct itmlst_3
	{
		unsigned short ilen;
		unsigned short code;
		unsigned long *returnP;
		unsigned long ignored;
	}
	dvi_itmlst[] = {
		{ 4, DVI$_MNT, 0 /*&ismnt*/, 0 },
		{ 4, DVI$_DEVCLASS, 0 /*&dvcls*/, 0 },
		{ 4, DVI$_DEVCHAR2, 0 /*&dchr2*/, 0 },
		{ 0, 0, 0, 0 }
	};

	dvi_itmlst[0].returnP = &ismnt;
	dvi_itmlst[1].returnP = &dvcls;
	dvi_itmlst[2].returnP = &dchr2;

	Status = sys$alloc(descr(DeviceName), 0, 0, 0, 0);

	if (FailureStatusP(Status))
	{
		VMS_ExitCode = Status;
		FatalError("cannot allocate device '%s' - %X\n", DeviceName, Status);
	}

	Status = sys$assign(descr(DeviceName), &DeviceFD, 0, 0);
	if (FailureStatusP(Status))
	{
		VMS_ExitCode = Status;
		FatalError("cannot open device '%s' - %X\n", DeviceName, Status);
	}

	Status = sys$getdviw(0, DeviceFD, 0, &dvi_itmlst, &iosb, 0, 0, 0);
	if (FailureStatusP(Status))
	{
		VMS_ExitCode = Status;
		FatalError("cannot $getdvi(1) on device '%s' - %X\n", DeviceName, Status);
	}

	if (FailureStatusP(Status = iosb[0]))
	{
		VMS_ExitCode = Status;
		FatalError("cannot $getdvi(2) on device '%s' - %X\n", DeviceName, Status);
	}

	if (dvcls != DC$_TAPE)
	{
		VMS_ExitCode = SS$_IVDEVNAM;
		FatalError("specified device is NOT a magtape: operation denied\n");
	}
#ifndef __DECC
#ifndef DEV$M_SCSI
#define DEV$M_SCSI 0x1000000
#endif
#endif
	if (~dchr2 & DEV$M_SCSI)
	{
		VMS_ExitCode = SS$_IVDEVNAM;
		FatalError("specified magtape is NOT a SCSI device: operation denied\n");
	}
#if USING_DEC_DRIVE | USING_LDRSET
#ifndef __DECC
#ifndef DEV$M_LDR
#define DEV$M_LDR 0x100000
#endif
#endif
	if (~dchr2 & DEV$M_LDR)
	{
		VMS_ExitCode = SS$_IVDEVNAM;
		FatalError("specified SCSI magtape does not have a loader: operation denied\n");
	}
#endif
	if (ismnt)
	{
		VMS_ExitCode = SS$_DEVMOUNT;
		FatalError("specified device is mounted: operation denied\n");
	}

	ots$move5(0, 0, 0, sizeof(newprivs), &newprivs);
	newprivs.prv$v_diagnose = 1;
	newprivs.prv$v_log_io = 1;
	newprivs.prv$v_phy_io = 1;
	Status = sys$setprv(1, &newprivs, 0, 0);

	if (FailureStatusP(Status))
	{
		VMS_ExitCode = Status;
		FatalError("error enabling privs (diagnose,log_io,phy_io): operation denied\n");
	}

	Status = sys$setprv(1, 0, 0, &setprivs);
	if (FailureStatusP(Status))
	{
		VMS_ExitCode = Status;
		FatalError("error retrieving current privs: operation denied\n");
	}

	if (!setprivs.prv$v_diagnose)
	{
		VMS_ExitCode = SS$_NODIAGNOSE;
		FatalError("DIAGNOSE privilege is required: operation denied\n");
	}

	if (!setprivs.prv$v_phy_io && !setprivs.prv$v_log_io)
	{
		VMS_ExitCode = SS$_NOPHY_IO;
		FatalError("PHY_IO or LOG_IO privilege is required: operation denied\n");
	}

	return DeviceFD;
}


static void SCSI_CloseDevice(char *DeviceName, int DeviceFD)
{
	unsigned long Status;

	Status = sys$dassgn(DeviceFD);
	if (FailureStatusP(Status))
		FatalError("cannot close SCSI device '%s' - %X\n", DeviceName, Status);
}


static int SCSI_ExecuteCommand(	int DeviceFD,
								Direction_T Direction,
								CDB_T *CDB,
								int CDB_Length,
								void *DataBuffer,
								int DataBufferLength,
								RequestSense_T *RequestSense)
{
	SCSI$DESC cmd_desc;
	SCSI$IOSB cmd_iosb;
	unsigned long Status;
	int Result;
	memset(RequestSense, 0, sizeof(RequestSense_T));
	/* Issue the QIO to send the SCSI Command. */
	ots$move5(0, 0, 0, sizeof(cmd_desc), &cmd_desc);
	cmd_desc.SCSI$L_OPCODE = 1; /* Only defined SCSI opcode... a VMS thing */
	cmd_desc.SCSI$A_CMD_ADDR = CDB;
	cmd_desc.SCSI$L_CMD_LEN = CDB_Length;
	cmd_desc.SCSI$A_DATA_ADDR = DataBuffer;
	cmd_desc.SCSI$L_DATA_LEN = DataBufferLength;
	cmd_desc.SCSI$L_PAD_LEN = 0;
	cmd_desc.SCSI$L_PH_CH_TMOUT = 180; /* SCSI Phase Change Timeout (seconds) */
	cmd_desc.SCSI$L_DISCON_TMOUT = 180; /* SCSI Disconnect Timeout (seconds) */

	switch (Direction)
	{
	/*
	NOTE: Do NOT include flag SCSI$K_FL_ENAB_SYNC.
	It does NOT work for this case.
	*/
	case Input:
		cmd_desc.SCSI$L_FLAGS = SCSI$K_READ | SCSI$K_FL_ENAB_DIS;
		break;

	case Output:
		cmd_desc.SCSI$L_FLAGS = SCSI$K_WRITE | SCSI$K_FL_ENAB_DIS;
		break;
	}

	/* Issue the SCSI Command. */
	Status = sys$qiow(MK_EFN, DeviceFD, IO$_DIAGNOSE, &cmd_iosb, 0, 0,
	&cmd_desc, sizeof(cmd_desc), 0, 0, 0, 0);
	Result = SCSI$K_GOOD;

	if (Status & 1)
		Status = cmd_iosb.SCSI$W_VMS_STAT;

	if (Status & 1)
		Result = cmd_iosb.SCSI$B_IOSB_STS;

	if (Result != SCSI$K_GOOD)
	{
		unsigned char RequestSenseCDB[6] =
			{ 0x03 /* REQUEST_SENSE */, 0, 0, 0, sizeof(RequestSense_T), 0 };

		printf("SCSI command error: %d - requesting sense data\n", Result);

		/* Execute Request Sense to determine the failure reason. */
		ots$move5(0, 0, 0, sizeof(cmd_desc), &cmd_desc);
		cmd_desc.SCSI$L_OPCODE = 1; /* Only defined SCSI opcode... a VMS thing */
		cmd_desc.SCSI$L_FLAGS = SCSI$K_READ | SCSI$K_FL_ENAB_DIS;
		cmd_desc.SCSI$A_CMD_ADDR = RequestSenseCDB;
		cmd_desc.SCSI$L_CMD_LEN = 6;
		cmd_desc.SCSI$A_DATA_ADDR = RequestSense;
		cmd_desc.SCSI$L_DATA_LEN = sizeof(RequestSense_T);
		cmd_desc.SCSI$L_PH_CH_TMOUT = 180;
		cmd_desc.SCSI$L_DISCON_TMOUT = 180;

		/* Issue the QIO to send the Request Sense Command. */
		Status = sys$qiow(MK_EFN, DeviceFD, IO$_DIAGNOSE, &cmd_iosb, 0, 0,
			&cmd_desc, sizeof(cmd_desc), 0, 0, 0, 0);
		if (FailureStatusP(Status))
		{
			printf("?Error returned from REQUEST_SENSE(1): %%X%08lX\n", Status);
			sys$exit(Status);
		}

		/* Check the VMS Status from QIO. */
		Status = cmd_iosb.SCSI$W_VMS_STAT;
		if (FailureStatusP(Status))
		{
			printf("?Error returned from REQUEST_SENSE(2): %%X%08lX\n", Status);
			sys$exit(Status);
		}
	}
	return Result;
}


static void VMS_DefineStatusSymbols(void)
{
	char SymbolName[32], SymbolValue[32];
	int StorageElementNumber;

	if (DataTransferElementFull)
	{
		/* Define MTX_DTE Symbol (environment variable) as 'FULL:n'. */
		sprintf(SymbolValue, "FULL:%d",
		DataTransferElementSourceStorageElementNumber);
		lib$set_symbol(descr("MTX_DTE"), descr(SymbolValue), &2);
	}
	else
	{
		/* Define MTX_DTE Symbol (environment variable) as 'EMPTY'. */
		lib$set_symbol(descr("MTX_DTE"), descr("EMPTY"), &2);
	}

	/* Define MTX_MSZ Symbol (environment variable) "Magazine SiZe" as 'n'. */
	sprintf(SymbolValue, "%d", StorageElementCount);
	lib$set_symbol(descr("MTX_MSZ"), descr(SymbolValue), &2);
	for (StorageElementNumber = 1;
		 StorageElementNumber <= StorageElementCount;
		 StorageElementNumber++)
	{
		sprintf(SymbolName, "MTX_STE%02d", StorageElementNumber);
		strcpy(SymbolValue,
			(StorageElementFull[StorageElementNumber] ? "FULL" : "EMPTY"));
		lib$set_symbol(descr(SymbolName), descr(SymbolValue), &2);
	}
}
