/*  MTX -- SCSI Tape Attached Medium Changer Control Program

	Copyright 1997-1998 by Leonard N. Zubkoff.
	Copyright 1999-2006 by Eric Lee Green.
	Copyright 2007-2008 by Robert Nelson <robertn@the-nelsons.org>

	$Date: 2008-08-19 03:03:38 -0700 (Tue, 19 Aug 2008) $
	$Revision: 193 $

	This file created Feb 2000 by Eric Lee Green <eric@badtux.org> from pieces
	extracted from mtx.c, plus a near total re-write of most of the beast.

	This program is free software; you may redistribute and/or modify it under
	the terms of the GNU General Public License Version 2 as published by the
	Free Software Foundation.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY, without even the implied warranty of MERCHANTABILITY
	or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
	for complete details.
*/


/*
 *	FatalError: changed Feb. 2000 elg@badtux.org to eliminate a buffer
 *	overflow :-(. That could be important if mtxl is SUID for some reason. 
*/

#include "mtx.h"
#include "mtxl.h"

/* #define DEBUG_NSM 1 */

/* #define DEBUG_MODE_SENSE 1 */
/* #define DEBUG */
/* #define DEBUG_SCSI */
#define __WEIRD_CHAR_SUPPRESS 1 

/* zap the following define when we finally add real import/export support */
#define IMPORT_EXPORT_HACK 1 /* for the moment, import/export == storage */

/* First, do some SCSI routines: */

/* the camlib is used on FreeBSD. */
#if HAVE_CAMLIB_H
#	include "scsi_freebsd.c"
#endif

/* the scsi_ctl interface is used on HP/UX. */
#if HAVE_SYS_SCSI_CTL_H
#	include "scsi_hpux.c"
#endif

/* the 'sg' interface is used on Linux. */
#if HAVE_SCSI_SG_H
#	include "scsi_linux.c"
#endif

/* the IOCTL_SCSI_PASSTHROUGH interface is used on Windows. */
#if HAVE_DDK_NTDDSCSI_H || defined(_MSC_VER)
#	include "scsi_win32.c"
#endif

/* The 'uscsi' interface is used on Solaris. */
#if HAVE_SYS_SCSI_IMPL_USCSI_H
#	include "scsi_sun.c"
#endif

/* The 'gsc' interface, is used on AIX. */
#if HAVE_SYS_GSCDDS_H
#	include "scsi_aix.c"
#endif

/* The 'dslib' interface is used on SGI. */
#if HAVE_DSLIB_H
#	include "scsi_sgi.c"
#endif

/* Hmm, dunno what to do about Digital Unix at the moment. */
#ifdef DIGITAL_UNIX
#	include "du/scsi.c"
#endif

#ifdef VMS
#	include "[.vms]scsi.c"
#endif

extern char *argv0; /* something to let us do good error messages. */

/* create a global RequestSenseT value. */
RequestSense_T scsi_error_sense;

/* Now for some low-level SCSI stuff: */

Inquiry_T *RequestInquiry(DEVICE_TYPE fd, RequestSense_T *RequestSense)
{
	Inquiry_T *Inquiry;
	CDB_T CDB;

	Inquiry = (Inquiry_T *) xmalloc(sizeof(Inquiry_T));  

	CDB[0] = 0x12;		/* INQUIRY */
	CDB[1] = 0;			/* EVPD = 0 */
	CDB[2] = 0;			/* Page Code */
	CDB[3] = 0;			/* Reserved */
	CDB[4] = sizeof(Inquiry_T);	/* Allocation Length */
	CDB[5] = 0;			/* Control */

	/* set us a very short timeout, sigh... */
	SCSI_Set_Timeout(30); /* 30 seconds, sigh! */

	if (SCSI_ExecuteCommand(fd, Input, &CDB, 6, Inquiry, sizeof(Inquiry_T), RequestSense) != 0)
	{
#ifdef DEBUG
		fprintf(stderr, "SCSI Inquiry Command failed\n");
#endif
		free(Inquiry);
		return NULL;  /* sorry! */
	}
	return Inquiry;
}


#if defined(DEBUG_NSM) || defined(DEBUG_EXCHANGE)
/* DEBUG */
static void dump_cdb(unsigned char *CDB, int len)
{
	fprintf(stderr,"CDB:");
	PrintHex(1, CDB, len);
}
#endif


#if defined(DEBUG_NSM) || defined(DEBUG_ADIC)
/* DEBUG */
static void dump_data(unsigned char *data, int len)
{
	if (!len)
	{
		fprintf(stderr,"**NO DATA**\n");
		return;
	}

	fprintf(stderr,"DATA:");
	PrintHex(1, data, len);
}
#endif


int BigEndian16(unsigned char *BigEndianData)
{
	return (BigEndianData[0] << 8) + BigEndianData[1];
}


int BigEndian24(unsigned char *BigEndianData)
{
	return (BigEndianData[0] << 16) + (BigEndianData[1] << 8) + BigEndianData[2];
}


void FatalError(char *ErrorMessage, ...)
{
#define FORMAT_BUF_LEN 1024

	char	FormatBuffer[FORMAT_BUF_LEN];
	char	*SourcePointer;
	char	*TargetPointer = FormatBuffer;
	char	Character, LastCharacter = '\0';
	int		numchars = 0;

	va_list ArgumentPointer;
	va_start(ArgumentPointer, ErrorMessage);
	/*  SourcePointer = "mtx: "; */
	sprintf(TargetPointer,"%s: ",argv0);
	numchars=strlen(TargetPointer);

	while ((Character = *ErrorMessage++) != '\0')
	{
		if (LastCharacter == '%')
		{
			if (Character == 'm')
			{
				SourcePointer = strerror(errno);
				while ((Character = *SourcePointer++) != '\0')
				{
					*TargetPointer++ = Character;
					numchars++;
					if (numchars == FORMAT_BUF_LEN - 1)
					{
						break;
					}
				}
				if (numchars == FORMAT_BUF_LEN - 1)
				{
					break;  /* break outer loop... */
				}
			}
			else
			{
				*TargetPointer++ = '%';
				*TargetPointer++ = Character;
				numchars++;
				if (numchars == FORMAT_BUF_LEN - 1)
				{
					break;
				}
			}
			LastCharacter = '\0';
		}
		else
		{
			if (Character != '%')
			{
				*TargetPointer++ = Character;
				numchars++;
				if (numchars == FORMAT_BUF_LEN-1)
				{
					break;
				}
			}
			LastCharacter = Character;
		}
	}

	*TargetPointer = '\0';  /* works even if we had to break from above... */
	vfprintf(stderr, FormatBuffer, ArgumentPointer);
	va_end(ArgumentPointer);

#ifndef VMS
	exit(1);
#else
	sys$exit(VMS_ExitCode);
#endif
}

/* This is a really slow and stupid 'bzero' implementation'... */
void slow_bzero(char *buffer, int numchars)
{
	while (numchars--)
	{
		*buffer++ = 0;
	}
}

/* malloc some memory while checking for out-of-memory conditions. */
void *xmalloc(size_t Size)
{
	void *Result = (void *) malloc(Size);
	if (Result == NULL)
	{
		FatalError("cannot allocate %d bytes of memory\n", Size);
	}
	return Result;
}

/* malloc some memory, zeroing it too: */
void *xzmalloc(size_t Size)
{
	void *Result = (void *)xmalloc(Size);

	slow_bzero(Result, Size);
	return Result;
}


int min(int x, int y)
{
	return (x < y ? x : y);
}


int max(int x, int y)
{
	return (x > y ? x : y);
}


/* Okay, this is a hack for the NSM modular jukebox series, which
 * uses the "SEND DIAGNOSTIC" command to do shit. 
 */

int SendNSMHack(DEVICE_TYPE MediumChangerFD, NSM_Param_T *nsm_command, 
		int param_len, int timeout)
{
	CDB_T CDB;
	int list_len = param_len + sizeof(NSM_Param_T) - 2048;

	/* Okay, now for the command: */
	CDB[0] = 0x1D;
	CDB[1] = 0x10;
	CDB[2] = 0;
	CDB[3] = (unsigned char)(list_len >> 8);
	CDB[4] = (unsigned char)list_len;
	CDB[5] = 0;

#ifdef DEBUG_NSM
	dump_cdb(&CDB,6);
	dump_data(nsm_command,list_len);
#endif
	fflush(stderr);
	/* Don't set us any timeout unless timeout is > 0 */
	if (timeout > 0)
	{
		SCSI_Set_Timeout(timeout); /* 30 minutes, sigh! */
	}

	/* Now send the command: */
	if (SCSI_ExecuteCommand(MediumChangerFD, Output, &CDB, 6, nsm_command, list_len, &scsi_error_sense))
	{
		return -1; /* we failed */
	}
	return 0; /* Did it! */
}

NSM_Result_T *RecNSMHack(	DEVICE_TYPE MediumChangerFD,
							int param_len, int timeout)
{
	CDB_T CDB;

	NSM_Result_T *retval = (NSM_Result_T *)xzmalloc(sizeof(NSM_Result_T));

	int list_len = param_len + sizeof(NSM_Result_T) - 0xffff;

	/* Okay, now for the command: */
	CDB[0] = 0x1C;
	CDB[1] = 0x00;
	CDB[2] = 0;
	CDB[3] = (unsigned char)(list_len >> 8);
	CDB[4] = (unsigned char)list_len;
	CDB[5] = 0;

#ifdef DEBUG_NSM
	dump_cdb(&CDB,6);
#endif

	/* Don't set us any timeout unless timeout is > 0 */
	if (timeout > 0)
	{
		SCSI_Set_Timeout(timeout);
	}

	/* Now send the command: */
	if (SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 6, retval, list_len, &scsi_error_sense))
	{
		return NULL; /* we failed */
	}

#ifdef DEBUG_NSM
	fprintf(stderr,
			"page_code=%02x page_len=%d command_code=%s\n",
			retval->page_code,
			(int) ((retval->page_len_msb << 8) + retval->page_len_lsb),
	retval->command_code);
#endif

	return retval; /* Did it! (maybe!)*/
}

/* Routine to inventory the library. Needed by, e.g., some Breece Hill
 * loaders. Sends an INITIALIZE_ELEMENT_STATUS command. This command
 * has no parameters, such as a range to scan :-(. 
 */

int Inventory(DEVICE_TYPE MediumChangerFD)
{
	CDB_T	CDB;

	/* okay, now for the command: */
	CDB[0] = 0x07; 
	CDB[1] = CDB[2] = CDB[3] = CDB[4] = CDB[5] = 0;

	/* set us a very long timeout, sigh... */
	SCSI_Set_Timeout(30 * 60); /* 30 minutes, sigh! */

	if (SCSI_ExecuteCommand(MediumChangerFD,Input,&CDB,6,NULL,0,&scsi_error_sense) != 0)
	{
		/* If error is UNIT ATTENTION then retry the request */
		if (scsi_error_sense.ErrorCode != 0x70 || scsi_error_sense.SenseKey != 6 ||
			ClearUnitAttention(MediumChangerFD, &scsi_error_sense) != 0 ||
			SCSI_ExecuteCommand(MediumChangerFD,Input,&CDB,6,NULL,0,&scsi_error_sense) != 0)
		{
			PrintRequestSense(&scsi_error_sense);
			fprintf(stderr, "Initialize Element Status (0x07) failed\n");
			return -1;  /* could not do! */
		}
	}
	return 0; /* did do! */
}

/* Routine to read the Mode Sense Element Address Assignment Page */
/* We try to read the page. If we can't read the page, we return NULL.
 * Our caller really isn't too worried about why we could not read the
 * page, it will simply default to some kind of default values. 
 */ 
ElementModeSense_T *ReadAssignmentPage(DEVICE_TYPE MediumChangerFD)
{
	CDB_T CDB;
	ElementModeSense_T *retval;  /* processed SCSI. */
	unsigned char input_buffer[136];
	ElementModeSensePage_T *sense_page; /* raw SCSI. */

	/* okay, now for the command: */
	CDB[0] = 0x1A; /* Mode Sense(6) */
	CDB[1] = 0x08; 
	CDB[2] = 0x1D; /* Mode Sense Element Address Assignment Page */
	CDB[3] = 0;
	CDB[4] = 136; /* allocation_length... */
	CDB[5] = 0;

	/* clear the data buffer: */
	slow_bzero((char *)&scsi_error_sense, sizeof(RequestSense_T));
	slow_bzero((char *)input_buffer, sizeof(input_buffer));

	if (SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 6,
							&input_buffer, sizeof(input_buffer), &scsi_error_sense) != 0)
	{
		/* If error is UNIT ATTENTION then retry the request */
		if (scsi_error_sense.ErrorCode != 0x70 || scsi_error_sense.SenseKey != 6 ||
			ClearUnitAttention(MediumChangerFD, &scsi_error_sense) != 0 ||
			SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 6,
								&input_buffer, sizeof(input_buffer), &scsi_error_sense) != 0)
		{
			PrintRequestSense(&scsi_error_sense);
			fprintf(stderr,"Mode sense (0x1A) for Page 0x1D failed\n");
			fflush(stderr);
			return NULL; /* sorry, couldn't do it. */
		}
	}

	/* Could do it, now build return value: */

#ifdef DEBUG_MODE_SENSE
	PrintHex(0, input_buffer, 30);
#endif

	/* first, skip past: mode data header, and block descriptor header if any */
	sense_page=(ElementModeSensePage_T *)(input_buffer+4+input_buffer[3]);

#ifdef DEBUG_MODE_SENSE
	fprintf(stderr,"*sense_page=%x  %x\n",*((unsigned char *)sense_page),sense_page->PageCode);
	fflush(stderr);
#endif

	retval = (ElementModeSense_T *)xzmalloc(sizeof(ElementModeSense_T));

	/* Remember that all SCSI values are big-endian: */
	retval->MediumTransportStart =
		(((int)sense_page->MediumTransportStartHi) << 8) +
		sense_page->MediumTransportStartLo;

	retval->NumMediumTransport =
		(((int)(sense_page->NumMediumTransportHi))<<8) +
		sense_page->NumMediumTransportLo;

	/* HACK! Some HP autochangers don't report NumMediumTransport right! */
	/* ARG! TAKE OUT THE %#@!# HACK! */
#ifdef STUPID_DUMB_IDIOTIC_HP_LOADER_HACK
	if (!retval->NumMediumTransport)
	{
		retval->NumMediumTransport = 1;
	}
#endif

#ifdef DEBUG_MODE_SENSE
	fprintf(stderr, "rawNumStorage= %d %d    rawNumImportExport= %d %d\n",
			sense_page->NumStorageHi, sense_page->NumStorageLo,
			sense_page->NumImportExportHi, sense_page->NumImportExportLo);
	fprintf(stderr, "rawNumTransport=%d %d  rawNumDataTransfer=%d %d\n",
			sense_page->NumMediumTransportHi, sense_page->NumMediumTransportLo,
			sense_page->NumDataTransferHi, sense_page->NumDataTransferLo);
	fflush(stderr);
#endif

	retval->StorageStart =
		((int)sense_page->StorageStartHi << 8) + sense_page->StorageStartLo;

	retval->NumStorage =
		((int)sense_page->NumStorageHi << 8) + sense_page->NumStorageLo;

	retval->ImportExportStart =
		((int)sense_page->ImportExportStartHi << 8) + sense_page->ImportExportStartLo; 

	retval->NumImportExport =
		((int)sense_page->NumImportExportHi << 8) + sense_page->NumImportExportLo;

	retval->DataTransferStart =
		((int)sense_page->DataTransferStartHi << 8) + sense_page->DataTransferStartLo;

	retval->NumDataTransfer =
		((int)sense_page->NumDataTransferHi << 8) + sense_page->NumDataTransferLo;

	/* allocate a couple spares 'cause some HP autochangers and maybe others
	* don't properly report the robotics arm(s) count here... 
	*/
	retval->NumElements =
		retval->NumStorage+retval->NumImportExport +
		retval->NumDataTransfer+retval->NumMediumTransport;

	retval->MaxReadElementStatusData =
		(sizeof(ElementStatusDataHeader_T) +
		4 * sizeof(ElementStatusPage_T) +
		retval->NumElements * sizeof(TransportElementDescriptor_T));

#ifdef IMPORT_EXPORT_HACK
	retval->NumStorage = retval->NumStorage+retval->NumImportExport;
#endif

#ifdef DEBUG_MODE_SENSE
	fprintf(stderr, "NumMediumTransport=%d\n", retval->NumMediumTransport);
	fprintf(stderr, "NumStorage=%d\n", retval->NumStorage);
	fprintf(stderr, "NumImportExport=%d\n", retval->NumImportExport);
	fprintf(stderr, "NumDataTransfer=%d\n", retval->NumDataTransfer);
	fprintf(stderr, "MaxReadElementStatusData=%d\n", retval->MaxReadElementStatusData);
	fprintf(stderr, "NumElements=%d\n", retval->NumElements);
	fflush(stderr);
#endif

	return retval;
}

static void FreeElementData(ElementStatus_T *data)
{
	free(data->DataTransferElementAddress);
	free(data->DataTransferElementSourceStorageElementNumber);
	free(data->DataTransferPrimaryVolumeTag);
	free(data->DataTransferAlternateVolumeTag);
	free(data->PrimaryVolumeTag);
	free(data->AlternateVolumeTag);
	free(data->StorageElementAddress);
	free(data->StorageElementIsImportExport);
	free(data->StorageElementFull);
	free(data->DataTransferElementFull);
}


/* allocate data  */

static ElementStatus_T *AllocateElementData(ElementModeSense_T *mode_sense)
{
	ElementStatus_T *retval;

	retval = (ElementStatus_T *)xzmalloc(sizeof(ElementStatus_T));

	/* now for the invidual component arrays.... */

	retval->DataTransferElementAddress =
		(int *)xzmalloc(sizeof(int) * (mode_sense->NumDataTransfer + 1));
	retval->DataTransferElementSourceStorageElementNumber = 
		(int *)xzmalloc(sizeof(int) * (mode_sense->NumDataTransfer + 1));
	retval->DataTransferPrimaryVolumeTag =
		(barcode *)xzmalloc(sizeof(barcode) * (mode_sense->NumDataTransfer + 1));
	retval->DataTransferAlternateVolumeTag =
		(barcode *)xzmalloc(sizeof(barcode) * (mode_sense->NumDataTransfer + 1));
	retval->PrimaryVolumeTag =
		(barcode *)xzmalloc(sizeof(barcode) * (mode_sense->NumStorage + 1));
	retval->AlternateVolumeTag =
		(barcode *)xzmalloc(sizeof(barcode) * (mode_sense->NumStorage + 1));
	retval->StorageElementAddress =
		(int *)xzmalloc(sizeof(int) * (mode_sense->NumStorage + 1));
	retval->StorageElementIsImportExport =
		(boolean *)xzmalloc(sizeof(boolean) * (mode_sense->NumStorage + 1));
	retval->StorageElementFull =
		(boolean *)xzmalloc(sizeof(boolean) * (mode_sense->NumStorage + 1));
	retval->DataTransferElementFull =
		(boolean *)xzmalloc(sizeof(boolean) * (mode_sense->NumDataTransfer + 1));

	return retval; /* sigh! */
}


void copy_barcode(unsigned char *src, unsigned char *dest)
{
	int i;

	for (i=0; i < 36; i++)
	{
		*dest = *src++;

		if ((*dest < 32) || (*dest > 127))
		{
			*dest = '\0';
		}

		dest++;
	}
	*dest = 0; /* null-terminate */ 
}

/* This #%!@# routine has more parameters than I can count! */
static unsigned char *SendElementStatusRequestActual(
					DEVICE_TYPE MediumChangerFD,
					RequestSense_T *RequestSense,
					Inquiry_T *inquiry_info, 
					SCSI_Flags_T *flags,
					int ElementStart,
					int NumElements,
					int NumBytes
					)
{
	CDB_T CDB;
	boolean is_attached = false;

	unsigned char *DataBuffer; /* size of data... */

#ifdef HAVE_GET_ID_LUN
	scsi_id_t *scsi_id;
#endif
	if (inquiry_info->MChngr && 
		inquiry_info->PeripheralDeviceType != MEDIUM_CHANGER_TYPE)
	{
		is_attached = true;
	}

	if (flags->no_attached)
	{
		/* override, sigh */ 
		is_attached = false;
	}

	DataBuffer = (unsigned char *)xzmalloc(NumBytes + 1);

	slow_bzero((char *)RequestSense, sizeof(RequestSense_T));

#ifdef HAVE_GET_ID_LUN
	scsi_id = SCSI_GetIDLun(MediumChangerFD);
#endif

	CDB[0] = 0xB8;		/* READ ELEMENT STATUS */

	if (is_attached)
	{
		CDB[0] = 0xB4;  /* whoops, READ_ELEMENT_STATUS_ATTACHED! */ 
	}

#ifdef HAVE_GET_ID_LUN
	CDB[1] = (scsi_id->lun << 5) | ((flags->no_barcodes) ? 
		0 : 0x10) | flags->elementtype;  /* Lun + VolTag + Type code */
	free(scsi_id);
#else
	/* Element Type Code = 0, VolTag = 1 */
	CDB[1] = (unsigned char)((flags->no_barcodes ? 0 : 0x10) | flags->elementtype);
#endif
	/* Starting Element Address */
	CDB[2] = (unsigned char)(ElementStart >> 8);
	CDB[3] = (unsigned char)ElementStart;

	/* Number Of Elements */
	CDB[4]= (unsigned char)(NumElements >> 8);
	CDB[5]= (unsigned char)NumElements;

	CDB[6] = 0;			/* Reserved */

	/* allocation length */
	CDB[7]= (unsigned char)(NumBytes >> 16);
	CDB[8]= (unsigned char)(NumBytes >> 8);
	CDB[9]= (unsigned char)NumBytes;

	CDB[10] = 0;			/* Reserved */
	CDB[11] = 0;			/* Control */

#ifdef DEBUG_BARCODE
	fprintf(stderr,"CDB:\n");
	PrintHex(2, CDB, 12);
#endif

	if (SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 12,
		DataBuffer,NumBytes, RequestSense) != 0)
	{

#ifdef DEBUG
		fprintf(stderr, "Read Element Status (0x%02X) failed\n", CDB[0]);
#endif

		/*
			First see if we have sense key of 'illegal request',
			additional sense code of '24', additional sense qualfier of 
			'0', and field in error of '4'. This means that we issued a request
			w/bar code reader and did not have one, thus must re-issue the request
			w/out barcode :-(.
		*/

		/*
			Most autochangers and tape libraries set a sense key here if
			they do not have a bar code reader. For some reason, the ADIC DAT
			uses a different sense key? Let's retry w/o bar codes for *ALL*
			sense keys.
		*/

		if (RequestSense->SenseKey > 1)
		{
			/* we issued a code requesting bar codes, there is no bar code reader? */
			/* clear out our sense buffer first... */
			slow_bzero((char *)RequestSense, sizeof(RequestSense_T));

			CDB[1] &= ~0x10; /* clear bar code flag! */

#ifdef DEBUG_BARCODE
			fprintf(stderr,"CDB:\n");
			PrintHex(2, CDB, 12);
#endif

			if (SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 12,
									DataBuffer, NumBytes, RequestSense) != 0)
			{
				free(DataBuffer);
				return NULL;
			}
		}
		else
		{
			free(DataBuffer);
			return NULL;
		}
	}

#ifdef DEBUG_BARCODE
	/* print a bunch of extra debug data :-(.  */
	PrintRequestSense(RequestSense); /* see what it sez :-(. */
	fprintf(stderr,"Data:\n");
	PrintHex(2, DataBuffer, 40);
#endif  
	return DataBuffer; /* we succeeded! */
}


unsigned char *SendElementStatusRequest(DEVICE_TYPE MediumChangerFD,
										RequestSense_T *RequestSense,
										Inquiry_T *inquiry_info, 
										SCSI_Flags_T *flags,
										int ElementStart,
										int NumElements,
										int NumBytes
										)
{
	unsigned char *DataBuffer; /* size of data... */
	int real_numbytes;

	DataBuffer = SendElementStatusRequestActual(MediumChangerFD,
												RequestSense,
												inquiry_info,
												flags,
												ElementStart,
												NumElements,
												NumBytes
												);
	/*
		One weird loader wants either 8 or BYTE_COUNT_OF_REPORT
		values for the ALLOCATION_LENGTH. Give it what it wants
		if we get an Sense Key of 05 Illegal Request with a 
		CDB position of 7 as the field in error.
	*/

	if (DataBuffer == NULL &&
		RequestSense->SenseKey == 5 &&
		RequestSense->CommandData &&
		RequestSense->BitPointer == 7)
	{
		NumBytes=8; /* send an 8 byte request */
		DataBuffer=SendElementStatusRequestActual(	MediumChangerFD,
													RequestSense,
													inquiry_info,
													flags,
													ElementStart,
													NumElements,
													NumBytes
													);
	}

	/* the above code falls thru into this: */
	if (DataBuffer != NULL)
	{
		/* see if we need to retry with a bigger NumBytes: */
		real_numbytes = ((int)DataBuffer[5] << 16) +
						((int)DataBuffer[6] << 8) +
						 (int)DataBuffer[7] + 8;

		if (real_numbytes > NumBytes)
		{
			/* uh-oh, retry! */
			free(DataBuffer); /* solve memory leak */
			DataBuffer = SendElementStatusRequestActual(MediumChangerFD,
														RequestSense,
														inquiry_info,
														flags,
														ElementStart,
														NumElements,
														real_numbytes
														);
		}
	}

	return DataBuffer;
}



/******************* ParseElementStatus ***********************************/
/* This does the actual grunt work of parsing element status data. It fills
 * in appropriate pieces of its input data. It may be called multiple times
 * while we are gathering element status.
 */

static void ParseElementStatus(	int *EmptyStorageElementAddress,
								int *EmptyStorageElementCount,
								unsigned char *DataBuffer,
								ElementStatus_T *ElementStatus,
								ElementModeSense_T *mode_sense,
								int *pNextElement
								)
{
	unsigned char *DataPointer = DataBuffer;
	TransportElementDescriptor_T TEBuf;
	TransportElementDescriptor_T *TransportElementDescriptor;
	ElementStatusDataHeader_T *ElementStatusDataHeader;
	Element2StatusPage_T *ElementStatusPage;
	Element2StatusPage_T ESBuf;
	int ElementCount;
	int TransportElementDescriptorLength;
	int BytesAvailable;
	int ImportExportIndex;

	ElementStatusDataHeader = (ElementStatusDataHeader_T *) DataPointer;
	DataPointer += sizeof(ElementStatusDataHeader_T);
	ElementCount =
		BigEndian16(ElementStatusDataHeader->NumberOfElementsAvailable);

#ifdef DEBUG
	fprintf(stderr,"ElementCount=%d\n",ElementCount);
	fflush(stderr);
#endif

	while (ElementCount > 0)
	{
#ifdef DEBUG
		int got_element_num=0;

		fprintf(stderr,"Working on element # %d Element Count %d\n",got_element_num,ElementCount);
		got_element_num++;
#endif

		memcpy(&ESBuf, DataPointer, sizeof(ElementStatusPage_T));
		ElementStatusPage = &ESBuf;
		DataPointer += sizeof(ElementStatusPage_T);

		TransportElementDescriptorLength =
			BigEndian16(ElementStatusPage->ElementDescriptorLength);

		if (TransportElementDescriptorLength <
			sizeof(TransportElementDescriptorShort_T))
		{
			/* Foo, Storage Element Descriptors can be 4 bytes?! */
			if ((ElementStatusPage->ElementTypeCode != MediumTransportElement &&
				ElementStatusPage->ElementTypeCode != StorageElement &&
				ElementStatusPage->ElementTypeCode != ImportExportElement ) ||
				TransportElementDescriptorLength < 4)
			{
#ifdef DEBUG
				fprintf(stderr,"boom! ElementTypeCode=%d\n",ElementStatusPage->ElementTypeCode);
#endif
				FatalError("Transport Element Descriptor Length too short: %d\n", TransportElementDescriptorLength);
			}
		}
#ifdef DEBUG
		fprintf(stderr,"Transport Element Descriptor Length=%d\n",TransportElementDescriptorLength);
#endif
		BytesAvailable =
			BigEndian24(ElementStatusPage->ByteCountOfDescriptorDataAvailable);
#ifdef DEBUG
		fprintf(stderr,"%d bytes of descriptor data available in descriptor\n",
				BytesAvailable);
#endif
		/* work around a bug in ADIC DAT loaders */
		if (BytesAvailable <= 0)
		{
			ElementCount--; /* sorry :-( */
		}
		while (BytesAvailable > 0)
		{
			/* TransportElementDescriptor =
			(TransportElementDescriptor_T *) DataPointer; */
			memcpy(&TEBuf, DataPointer, 
				(TransportElementDescriptorLength <= sizeof(TEBuf)) ?
					TransportElementDescriptorLength  :
					sizeof(TEBuf));
			TransportElementDescriptor = &TEBuf;

			if (pNextElement != NULL)
			{
				if (BigEndian16(TransportElementDescriptor->ElementAddress) != 0 || *pNextElement == 0)
				{
					(*pNextElement) = BigEndian16(TransportElementDescriptor->ElementAddress) + 1;
				}
				else
				{
					return;
				}
			}

			DataPointer += TransportElementDescriptorLength;
			BytesAvailable -= TransportElementDescriptorLength;
			ElementCount--;

			switch (ElementStatusPage->ElementTypeCode)
			{
			case MediumTransportElement:
				ElementStatus->TransportElementAddress = BigEndian16(TransportElementDescriptor->ElementAddress);
#ifdef DEBUG
				fprintf(stderr,"TransportElementAddress=%d\n",ElementStatus->TransportElementAddress); 
#endif
				break;

			/* we treat ImportExport elements as if they were normal
			* storage elements now, sigh...
			*/
			case ImportExportElement:
#ifdef DEBUG
				fprintf(stderr,"ImportExportElement=%d\n",ElementStatus->StorageElementCount);
#endif
				if (ElementStatus->ImportExportCount >= mode_sense->NumImportExport)
				{
					fprintf(stderr,"Warning:Too Many Import/Export Elements Reported (expected %d, now have %d\n",
					mode_sense->NumImportExport,
					ElementStatus->ImportExportCount + 1);
					fflush(stderr);
					return; /* we're done :-(. */
				}

				ImportExportIndex = mode_sense->NumStorage - mode_sense->NumImportExport + ElementStatus->ImportExportCount;

				ElementStatus->StorageElementAddress[ImportExportIndex] =
					BigEndian16(TransportElementDescriptor->ElementAddress);
				ElementStatus->StorageElementFull[ImportExportIndex] =
					TransportElementDescriptor->Full;

				if ( (TransportElementDescriptorLength > 11) && 
					(ElementStatusPage->VolBits & E2_AVOLTAG))
				{
					copy_barcode(TransportElementDescriptor->AlternateVolumeTag,
						ElementStatus->AlternateVolumeTag[ImportExportIndex]);
				}
				else
				{
					ElementStatus->AlternateVolumeTag[ImportExportIndex][0] = 0;  /* null string. */;
				} 
				if ((TransportElementDescriptorLength > 11) && 
					(ElementStatusPage->VolBits & E2_PVOLTAG))
				{
					copy_barcode(TransportElementDescriptor->PrimaryVolumeTag,
						ElementStatus->PrimaryVolumeTag[ImportExportIndex]);
				}
				else
				{
					ElementStatus->PrimaryVolumeTag[ImportExportIndex][0]=0; /* null string. */
				}

				ElementStatus->StorageElementIsImportExport[ImportExportIndex] = 1;

				ElementStatus->ImportExportCount++;
				break;

			case StorageElement:
#ifdef DEBUG
				fprintf(stderr,"StorageElementCount=%d  ElementAddress = %d ",ElementStatus->StorageElementCount,BigEndian16(TransportElementDescriptor->ElementAddress));
#endif
				/* ATL/Exabyte kludge -- skip slots that aren't installed :-( */
				if (TransportElementDescriptor->AdditionalSenseCode==0x83 && 
					TransportElementDescriptor->AdditionalSenseCodeQualifier==0x02) 
					continue;

				ElementStatus->StorageElementAddress[ElementStatus->StorageElementCount] =
					BigEndian16(TransportElementDescriptor->ElementAddress);
				ElementStatus->StorageElementFull[ElementStatus->StorageElementCount] =
					TransportElementDescriptor->Full;
#ifdef DEBUG
				if (TransportElementDescriptor->Except)
					fprintf(stderr,"ASC,ASCQ = 0x%x,0x%x ",TransportElementDescriptor->AdditionalSenseCode,TransportElementDescriptor->AdditionalSenseCodeQualifier);
				fprintf(stderr,"TransportElement->Full = %d\n",TransportElementDescriptor->Full);
#endif
				if (!TransportElementDescriptor->Full)
				{
					EmptyStorageElementAddress[(*EmptyStorageElementCount)++] =
						ElementStatus->StorageElementCount; /* slot idx. */
					/*   ElementStatus->StorageElementAddress[ElementStatus->StorageElementCount]; */
				}
				if ((TransportElementDescriptorLength >  11) && 
					(ElementStatusPage->VolBits & E2_AVOLTAG))
				{
					copy_barcode(TransportElementDescriptor->AlternateVolumeTag,
						ElementStatus->AlternateVolumeTag[ElementStatus->StorageElementCount]);
				}
				else
				{
					ElementStatus->AlternateVolumeTag[ElementStatus->StorageElementCount][0]=0;  /* null string. */;
				} 
				if ((TransportElementDescriptorLength > 11) && 
					(ElementStatusPage->VolBits & E2_PVOLTAG))
				{
					copy_barcode(TransportElementDescriptor->PrimaryVolumeTag,
						ElementStatus->PrimaryVolumeTag[ElementStatus->StorageElementCount]);
				}
				else
				{
					ElementStatus->PrimaryVolumeTag[ElementStatus->StorageElementCount][0]=0; /* null string. */
				}

				ElementStatus->StorageElementCount++;
				/*
					Note that the original mtx had no check here for 
					buffer overflow, though some drives might mistakingly
					do one... 
				*/

				if (ElementStatus->StorageElementCount > mode_sense->NumStorage)
				{
					fprintf(stderr,"Warning:Too Many Storage Elements Reported (expected %d, now have %d\n",
							mode_sense->NumStorage,
							ElementStatus->StorageElementCount);
					fflush(stderr);
					return; /* we're done :-(. */
				}
				break;

			case DataTransferElement:
				/* tape drive not installed, go back to top of loop */

				/* if (TransportElementDescriptor->Except) continue ; */

				/* Note: This is for Exabyte tape libraries that improperly
				report that they have a 2nd tape drive when they don't. We
				could generalize this in an ideal world, but my attempt to
				do so failed with dual-drive Exabyte tape libraries that
				*DID* have the second drive. Sigh. 
				*/
				if (TransportElementDescriptor->AdditionalSenseCode==0x83 && 
					TransportElementDescriptor->AdditionalSenseCodeQualifier==0x04)
				{
					continue;
				}

				/*	generalize it. Does it work? Let's try it! */
				/*	
					No, dammit, following does not work on dual-drive Exabyte
					'cause if a tape is in the drive, it sets the AdditionalSense
					code to something (sigh).
				*/
				/* if (TransportElementDescriptor->AdditionalSenseCode!=0)
					continue;
				*/ 

				ElementStatus->DataTransferElementAddress[ElementStatus->DataTransferElementCount] =
					BigEndian16(TransportElementDescriptor->ElementAddress);
				ElementStatus->DataTransferElementFull[ElementStatus->DataTransferElementCount] = 
					TransportElementDescriptor->Full;
				ElementStatus->DataTransferElementSourceStorageElementNumber[ElementStatus->DataTransferElementCount] =
					BigEndian16(TransportElementDescriptor->SourceStorageElementAddress);

#if DEBUG
				fprintf(stderr, "%d: ElementAddress = %d, Full = %d, SourceElement = %d\n", 
						ElementStatus->DataTransferElementCount,
						ElementStatus->DataTransferElementAddress[ElementStatus->DataTransferElementCount],
						ElementStatus->DataTransferElementFull[ElementStatus->DataTransferElementCount],
						ElementStatus->DataTransferElementSourceStorageElementNumber[ElementStatus->DataTransferElementCount]);
#endif
				if (ElementStatus->DataTransferElementCount >= mode_sense->NumDataTransfer)
				{
					FatalError("Too many Data Transfer Elements Reported\n");
				}

				if (ElementStatusPage->VolBits & E2_PVOLTAG)
				{
					copy_barcode(TransportElementDescriptor->PrimaryVolumeTag,
					ElementStatus->DataTransferPrimaryVolumeTag[ElementStatus->DataTransferElementCount]);
				}
				else
				{
					ElementStatus->DataTransferPrimaryVolumeTag[ElementStatus->DataTransferElementCount][0]=0; /* null string */
				}

				if (ElementStatusPage->VolBits & E2_AVOLTAG)
				{
					copy_barcode(TransportElementDescriptor->AlternateVolumeTag,
					ElementStatus->DataTransferAlternateVolumeTag[ElementStatus->DataTransferElementCount]);
				}
				else
				{
					ElementStatus->DataTransferAlternateVolumeTag[ElementStatus->DataTransferElementCount][0]=0; /* null string */
				}

				ElementStatus->DataTransferElementCount++;

				/* 0 actually is a usable element address */
				/* if (DataTransferElementAddress == 0) */
				/*	FatalError( */
				/*  "illegal Data Transfer Element Address %d reported\n", */
				/* DataTransferElementAddress); */
				break;

			default:
				FatalError("illegal Element Type Code %d reported\n",
							ElementStatusPage->ElementTypeCode);
			}
		}
	}

#ifdef DEBUG
	if (pNextElement)
		fprintf(stderr,"Next start element will be %d\n",*pNextElement);
#endif
}


/********************* Real ReadElementStatus ********************* */

/*
 * We no longer do the funky trick to figure out ALLOCATION_LENGTH.
 * Instead, we use the SCSI Generic command rather than SEND_SCSI_COMMAND
 * under Linux, which gets around the @#%@ 4k buffer size in Linux. 
 * We still have the restriction that Linux cuts off the last two
 * bytes of the SENSE DATA (Q#@$%@#$^ Linux!). Which means that the
 * verbose widget won't work :-(. 
 
 * We now look for that "attached" bit in the inquiry_info to see whether
 * to use READ_ELEMENT_ATTACHED or plain old READ_ELEMENT. In addition, we
 * look at the device type in the inquiry_info to see whether it is a media
 * changer or tape device, and if it's a media changer device, we ignore the
 * attached bit (one beta tester found an old 4-tape DAT changer that set
 * the attached bit for both the tape device AND the media changer device). 

*/

ElementStatus_T *ReadElementStatus(DEVICE_TYPE MediumChangerFD, RequestSense_T *RequestSense, Inquiry_T *inquiry_info, SCSI_Flags_T *flags)
{
	ElementStatus_T *ElementStatus;

	unsigned char *DataBuffer; /* size of data... */

	int EmptyStorageElementCount=0;
	int *EmptyStorageElementAddress; /* [MAX_STORAGE_ELEMENTS]; */

	int empty_idx = 0;
	boolean is_attached = false;
	int i,j;

	ElementModeSense_T *mode_sense = NULL;

	if (inquiry_info->MChngr && inquiry_info->PeripheralDeviceType != MEDIUM_CHANGER_TYPE)
	{
		is_attached = true;
	}

	if (flags->no_attached)
	{
		/* override, sigh */ 
		is_attached = false;
	}

	if (!is_attached)
	{
		mode_sense = ReadAssignmentPage(MediumChangerFD);
	}

	if (!mode_sense)
	{
		mode_sense = (ElementModeSense_T *)xmalloc(sizeof(ElementModeSense_T));
		mode_sense->NumMediumTransport = MAX_TRANSPORT_ELEMENTS;
		mode_sense->NumStorage = MAX_STORAGE_ELEMENTS;
		mode_sense->NumDataTransfer = MAX_TRANSFER_ELEMENTS;
		mode_sense->MaxReadElementStatusData =
			(sizeof(ElementStatusDataHeader_T) + 3 * sizeof(ElementStatusPage_T) +
			(MAX_STORAGE_ELEMENTS+MAX_TRANSFER_ELEMENTS+MAX_TRANSPORT_ELEMENTS) *
				sizeof(TransportElementDescriptor_T));

		/* don't care about the others anyhow at the moment... */
	}

	ElementStatus = AllocateElementData(mode_sense);

	/* Now to initialize it (sigh).  */
	ElementStatus->StorageElementCount = 0;
	ElementStatus->DataTransferElementCount = 0;

	/* first, allocate some empty storage stuff: Note that we pass this
	* down to ParseElementStatus (sigh!) 
	*/

	EmptyStorageElementAddress = (int *)xzmalloc((mode_sense->NumStorage+1)*sizeof(int));
	for (i = 0; i < mode_sense->NumStorage; i++)
	{
		EmptyStorageElementAddress[i] = -1;
	}

	/* Okay, now to send some requests for the various types of stuff: */

	/* -----------STORAGE ELEMENTS---------------- */
	/* Let's start with storage elements: */

	for (i = 0; i < mode_sense->NumDataTransfer; i++)
	{
		/* initialize them to an illegal # so that we can fix later... */
		ElementStatus->DataTransferElementSourceStorageElementNumber[i] = -1; 
	}

	if (flags->querytype == MTX_ELEMENTSTATUS_ORIGINAL)
	{
#ifdef DEBUG
		fprintf(stderr,"Using original element status polling method (storage, import/export, drivers etc independantly)\n");
#endif
		flags->elementtype = StorageElement; /* sigh! */
		DataBuffer = SendElementStatusRequest(	MediumChangerFD, RequestSense,
												inquiry_info, flags,
												mode_sense->StorageStart,
												/* adjust for import/export. */
												mode_sense->NumStorage - mode_sense->NumImportExport,
												mode_sense->MaxReadElementStatusData);

		if (!DataBuffer)
		{
#ifdef DEBUG
			fprintf(stderr,"Had no elements!\n");
#endif
			/* darn. Free up stuff and return. */
#ifdef DEBUG_MODE_SENSE
			PrintRequestSense(RequestSense);
#endif
			FreeElementData(ElementStatus);
			return NULL; 
		}

#ifdef DEBUG
		fprintf(stderr, "Parsing storage elements\n");
#endif
		ParseElementStatus(EmptyStorageElementAddress, &EmptyStorageElementCount,
			DataBuffer,ElementStatus,mode_sense,NULL);

		free(DataBuffer); /* sigh! */

		/* --------------IMPORT/EXPORT--------------- */
		/* Next let's see if we need to do Import/Export: */
		if (mode_sense->NumImportExport > 0)
		{
#ifdef DEBUG
			fprintf(stderr,"Sending request for Import/Export status\n");
#endif
			flags->elementtype = ImportExportElement;
			DataBuffer = SendElementStatusRequest(	MediumChangerFD,RequestSense,
													inquiry_info, flags,
													mode_sense->ImportExportStart,
													mode_sense->NumImportExport,
													mode_sense->MaxReadElementStatusData);

			if (!DataBuffer)
			{
#ifdef DEBUG
				fprintf(stderr,"Had no input/export element!\n");
#endif
				/* darn. Free up stuff and return. */
#ifdef DEBUG_MODE_SENSE
				PrintRequestSense(RequestSense);
#endif
				FreeElementData(ElementStatus);
				return NULL;
			}
#ifdef DEBUG
			fprintf(stderr,"Parsing inport/export element status\n");
#endif
#ifdef DEBUG_ADIC
			dump_data(DataBuffer, 100);		/* dump some data :-(. */
#endif
			ParseElementStatus(	EmptyStorageElementAddress, &EmptyStorageElementCount,
								DataBuffer, ElementStatus, mode_sense, NULL);

			ElementStatus->StorageElementCount += ElementStatus->ImportExportCount;
		}

		/* ----------------- DRIVES ---------------------- */

#ifdef DEBUG
		fprintf(stderr,"Sending request for data transfer element (drive) status\n");
#endif
		flags->elementtype = DataTransferElement; /* sigh! */
		DataBuffer = SendElementStatusRequest(	MediumChangerFD, RequestSense,
												inquiry_info, flags,
												mode_sense->DataTransferStart,
												mode_sense->NumDataTransfer,
												mode_sense->MaxReadElementStatusData);
		if (!DataBuffer)
		{
#ifdef DEBUG
			fprintf(stderr,"No data transfer element status.");
#endif
			/* darn. Free up stuff and return. */
#ifdef DEBUG_MODE_SENSE
			PrintRequestSense(RequestSense);
#endif
			FreeElementData(ElementStatus);
			return NULL; 
		}

#ifdef DEBUG
		fprintf(stderr,"Parsing data for data transfer element (drive) status\n");
#endif
		ParseElementStatus(	EmptyStorageElementAddress, &EmptyStorageElementCount,
							DataBuffer,ElementStatus, mode_sense, NULL);

		free(DataBuffer); /* sigh! */

		/* ----------------- Robot Arm(s) -------------------------- */

		/* grr, damned brain dead HP doesn't report that it has any! */
		if (!mode_sense->NumMediumTransport)
		{ 
			ElementStatus->TransportElementAddress = 0; /* default it sensibly :-(. */
		}
		else
		{
#ifdef DEBUG
			fprintf(stderr,"Sending request for robot arm status\n");
#endif
			flags->elementtype = MediumTransportElement; /* sigh! */
			DataBuffer = SendElementStatusRequest(	MediumChangerFD, RequestSense,
													inquiry_info, flags,
													mode_sense->MediumTransportStart,
													1, /* only get 1, sigh. */
													mode_sense->MaxReadElementStatusData);
			if (!DataBuffer)
			{
#ifdef DEBUG
				fprintf(stderr,"Loader reports no robot arm!\n");
#endif
				/* darn. Free up stuff and return. */
#ifdef DEBUG_MODE_SENSE
				PrintRequestSense(RequestSense);
#endif
				FreeElementData(ElementStatus);
				return NULL; 
			} 
#ifdef DEBUG
			fprintf(stderr,"Parsing robot arm data\n");
#endif
			ParseElementStatus(	EmptyStorageElementAddress, &EmptyStorageElementCount,
								DataBuffer, ElementStatus, mode_sense, NULL);

			free(DataBuffer);
		}
	}
	else
	{
		int nLastEl=-1, nNextEl=0;

#ifdef DEBUG
		fprintf(stderr,"Using alternative element status polling method (all elements)\n");
#endif
		/* ----------------- ALL Elements ---------------------- */
		/*	Just keep asking for elements till no more are returned 
			- increment our starting address as we go acording to the 
			number of elements returned from the last call
		*/

		while(nLastEl!=nNextEl)
		{
			flags->elementtype = AllElementTypes;//StorageElement; /* sigh! */ /*XL1B2 firewire changer does not seem to respond to specific types so just use all elements*/
			DataBuffer = SendElementStatusRequest(	MediumChangerFD,
													RequestSense,
													inquiry_info,
													flags,
													nNextEl,//mode_sense->StorageStart,
													/* adjust for import/export. */
													mode_sense->NumStorage - mode_sense->NumImportExport,//FIX ME:this should be a more sensible value
													mode_sense->MaxReadElementStatusData);
			if (!DataBuffer)
			{
				if (RequestSense->AdditionalSenseCode == 0x21 && 
				RequestSense->AdditionalSenseCodeQualifier == 0x01)
				{
					/* Error is invalid element address, we've probably just hit the end */
					break;
				}

				/* darn. Free up stuff and return. */
				FreeElementData(ElementStatus);
				return NULL; 
			} 

			nLastEl = nNextEl;

			ParseElementStatus(	EmptyStorageElementAddress, &EmptyStorageElementCount,
								DataBuffer, ElementStatus, mode_sense, &nNextEl);

			free(DataBuffer); /* sigh! */
		}

		ElementStatus->StorageElementCount += ElementStatus->ImportExportCount;
	}

	/*---------------------- Sanity Checking ------------------- */

	if (ElementStatus->DataTransferElementCount == 0)
		FatalError("no Data Transfer Element reported\n");

	if (ElementStatus->StorageElementCount == 0)
		FatalError("no Storage Elements reported\n");


	/* ---------------------- Reset SourceStorageElementNumbers ------- */

	/*
	 * Re-write the SourceStorageElementNumber code  *AGAIN*.
	 *
	 * Pass1:
	 *	Translate from raw element # to our translated # (if possible).
	 *	First, check the SourceStorageElementNumbers against the list of 
	 *	filled slots. If the slots indicated are empty, we accept that list as
	 *	valid. Otherwise decide the SourceStorageElementNumbers are invalid.
	 *
	 * Pass2:
	 *	If we had some invalid (or unknown) SourceStorageElementNumbers
	 *	then we must search for free slots, and assign SourceStorageElementNumbers
	 *	to those free slots. We happen to already built a list of free
	 *	slots as part of the process of reading the storage element numbers
	 *	from the tape. So that's easy enough to do! 
	 */

#ifdef DEBUG_TAPELIST
	fprintf(stderr, "empty slots: %d\n", EmptyStorageElementCount);
	if (EmptyStorageElementCount)
	{
		for (i = 0; i < EmptyStorageElementCount; i++)
		{
			fprintf(stderr, "empty: %d\n", EmptyStorageElementAddress[i]);
		}
	}
#endif

	/*
	 *	Now we re-assign origin slots if the "real" origin slot
	 *	is obviously defective: 
	 */
	/* pass one: */
	for (i = 0; i < ElementStatus->DataTransferElementCount; i++)
	{
		int elnum;

		/* if we have an element, then ... */
		if (ElementStatus->DataTransferElementFull[i])
		{
			elnum = ElementStatus->DataTransferElementSourceStorageElementNumber[i];
			/* if we have an element number, then ... */
			if (elnum >= 0)
			{
				/* Now to translate the elnum: */
				ElementStatus->DataTransferElementSourceStorageElementNumber[i] = -1;
				for (j = 0; j < ElementStatus->StorageElementCount; j++)
				{
					if (elnum == ElementStatus->StorageElementAddress[j])
					{
						/* now see if the element # is already occupied:*/
						if (!ElementStatus->StorageElementFull[j])
						{
							/* properly set the source... */
							ElementStatus->DataTransferElementSourceStorageElementNumber[i] = j;
						}
					}
				}
			}
		}
	}

	/* Pass2: */
	/*	We have invalid sources, so let's see what they should be: */
	/*	Note: If EmptyStorageElementCount is < # of drives, the leftover
	 *	drives will be assigned a -1 (see the initialization loop for
	 *	EmptyStorageElementAddress above), which will be reported as "slot 0"
	 *	by the user interface. This is an invalid value, but more useful for us
	 *	to have than just crapping out here :-(. 
	*/
	empty_idx=0;
	for (i = 0; i < ElementStatus->DataTransferElementCount; i++)
	{
		if (ElementStatus->DataTransferElementFull[i] && 
			ElementStatus->DataTransferElementSourceStorageElementNumber[i] < 0)
		{
#ifdef DEBUG_TAPELIST
			fprintf(stderr,"for drive %d, changing to %d (empty slot #%d)\n",
					i,
					EmptyStorageElementAddress[empty_idx],
					empty_idx);
#endif
			ElementStatus->DataTransferElementSourceStorageElementNumber[i] =
				EmptyStorageElementAddress[empty_idx++];
		}
	}

	/* and done! */
	free(mode_sense);
	free(EmptyStorageElementAddress);
	return ElementStatus;
}

/*************************************************************************/

RequestSense_T *PositionElement(DEVICE_TYPE MediumChangerFD,
		int DestinationAddress,
		ElementStatus_T *ElementStatus)
{
	RequestSense_T *RequestSense = xmalloc(sizeof(RequestSense_T));
	CDB_T CDB;

	CDB[0] = 0x2b;
	CDB[1] = 0;
	CDB[2] = (unsigned char)(ElementStatus->TransportElementAddress >> 8);
	CDB[3] = (unsigned char)ElementStatus->TransportElementAddress;
	CDB[4] = (unsigned char)(DestinationAddress >> 8);
	CDB[5] = (unsigned char)DestinationAddress;
	CDB[6] = 0;
	CDB[7] = 0;
	CDB[8] = 0;
	CDB[9] = 0;

	if(SCSI_ExecuteCommand(	MediumChangerFD, Output, &CDB, 10,
							NULL, 0, RequestSense) != 0)
	{
		return RequestSense;
	}
	free(RequestSense);
	return NULL; /* success */
}


/* Now the actual media movement routine! */
RequestSense_T *MoveMedium(	DEVICE_TYPE MediumChangerFD, int SourceAddress,
							int DestinationAddress, 
							ElementStatus_T *ElementStatus,
							Inquiry_T *inquiry_info, SCSI_Flags_T *flags)
{
	RequestSense_T *RequestSense = xmalloc(sizeof(RequestSense_T));
	CDB_T CDB;

	if (inquiry_info->MChngr && inquiry_info->PeripheralDeviceType != MEDIUM_CHANGER_TYPE)
	{
		/* if using the ATTACHED API */
		CDB[0] = 0xA7;		/* MOVE_MEDIUM_ATTACHED */
	}
	else
	{
		CDB[0] = 0xA5;		/* MOVE MEDIUM */
	}

	CDB[1] = 0;			/* Reserved */

	/* Transport Element Address */
	CDB[2] = (unsigned char)(ElementStatus->TransportElementAddress >> 8);
	CDB[3] = (unsigned char)ElementStatus->TransportElementAddress;

	/* Source Address */
	CDB[4] = (unsigned char)(SourceAddress >> 8);
	CDB[5] = (unsigned char)SourceAddress;

	/* Destination Address */
	CDB[6] = (unsigned char)(DestinationAddress >> 8);
	CDB[7] = (unsigned char)DestinationAddress;

	CDB[8] = 0;			/* Reserved */
	CDB[9] = 0;			/* Reserved */

	if (flags->invert)
	{
		CDB[10] = 1;			/* Reserved */
	}
	else
	{
		CDB[10] = 0;
	}
	/* eepos controls the tray for import/export elements, sometimes. */
	CDB[11] = flags->eepos << 6;			/* Control */

	if (SCSI_ExecuteCommand(MediumChangerFD, Output, &CDB, 12,
							NULL, 0, RequestSense) != 0)
	{
#ifdef DEBUG
		fprintf(stderr, "Move Medium (0x%02X) failed\n", CDB[0]);
#endif
		return RequestSense;
	}

	free(RequestSense);
	return NULL; /* success! */
}


/* Now the actual Exchange Medium routine! */
RequestSense_T *ExchangeMedium(	DEVICE_TYPE MediumChangerFD, int SourceAddress,
								int DestinationAddress, int Dest2Address,
								ElementStatus_T *ElementStatus,
								SCSI_Flags_T *flags)
{
	RequestSense_T *RequestSense = xmalloc(sizeof(RequestSense_T));
	CDB_T CDB;

	CDB[0] = 0xA6;		/* EXCHANGE MEDIUM */
	CDB[1] = 0;			/* Reserved */

	/* Transport Element Address */
	CDB[2] = (unsigned char)(ElementStatus->TransportElementAddress >> 8);
	CDB[3] = (unsigned char)ElementStatus->TransportElementAddress;

	/* Source Address */
	CDB[4] = (unsigned char)(SourceAddress >> 8);
	CDB[5] = (unsigned char)SourceAddress;

	/* Destination Address */
	CDB[6] = (unsigned char)(DestinationAddress >> 8);
	CDB[7] = (unsigned char)DestinationAddress;

	/* move destination back to source? */
	CDB[8] = (unsigned char)(Dest2Address >> 8);
	CDB[9] = (unsigned char)Dest2Address;
	CDB[10] = 0;

	if (flags->invert)
	{
		CDB[10] |= 2;			/* INV2 */
	}

	if (flags->invert2)
	{
		CDB[1] |= 1;			/* INV1 */
	}

	/* eepos controls the tray for import/export elements, sometimes. */
	CDB[11] = flags->eepos << 6;			/* Control */

#ifdef DEBUG_EXCHANGE
	dump_cdb(&CDB,12);
#endif  

	if (SCSI_ExecuteCommand(MediumChangerFD, Output, &CDB, 12,
							NULL, 0, RequestSense) != 0)
	{
		return RequestSense;
	}
	free(RequestSense);
	return NULL; /* success! */
}


/*
 * for Linux, this creates a way to do a short erase... the @#$%@ st.c
 * driver defaults to doing a long erase!
 */

RequestSense_T *Erase(DEVICE_TYPE MediumChangerFD)
{
	RequestSense_T *RequestSense = xmalloc(sizeof(RequestSense_T));
	CDB_T CDB;

	CDB[0] = 0x19;
	CDB[1] = 0;  /* Short! */
	CDB[2] = CDB[3] = CDB[4] = CDB[5] = 0;

	if (SCSI_ExecuteCommand(MediumChangerFD, Output, &CDB, 6, NULL, 0, RequestSense) != 0)
	{
		/* If error is UNIT ATTENTION then retry the request */
		if (RequestSense->ErrorCode != 0x70 || RequestSense->SenseKey != 6 ||
			ClearUnitAttention(MediumChangerFD, RequestSense) != 0 ||
			SCSI_ExecuteCommand(MediumChangerFD, Output, &CDB, 6, NULL, 0, RequestSense) != 0)
		{
#ifdef DEBUG
			fprintf(stderr, "Erase (0x19) failed\n");
#endif
			return RequestSense;
		}
	}

	free(RequestSense);
	return NULL;		/* Success! */
}

/* Routine to send an LOAD/UNLOAD from the MMC/SSC spec to a device. 
 * For tapes and changers this can be used either to eject a tape 
 * or to eject a magazine (on some Seagate changers, when sent to LUN 1 ).
 * For CD/DVDs this is used to Load or Unload a disc which is required by
 * some media changers.
 */

int LoadUnload(DEVICE_TYPE fd, int bLoad)
{
	CDB_T CDB;
	/* okay, now for the command: */

	CDB[0] = 0x1B;
	CDB[4] = bLoad ? 3 : 2;
	CDB[1] = CDB[2] = CDB[3] = CDB[5] = 0;

	if (SCSI_ExecuteCommand(fd, Input, &CDB, 6, NULL, 0, &scsi_error_sense) != 0)
	{
		/* If error is UNIT ATTENTION then retry the request */
		if (scsi_error_sense.ErrorCode != 0x70 || scsi_error_sense.SenseKey != 6 ||
			ClearUnitAttention(fd, &scsi_error_sense) != 0 ||
			SCSI_ExecuteCommand(fd, Input, &CDB, 6, NULL, 0, &scsi_error_sense) != 0)
		{
			PrintRequestSense(&scsi_error_sense);
			fprintf(stderr, "Eject (0x1B) failed\n");
			return -1;  /* could not do! */
		}
	}
	return 0; /* did do! */
}

/* Routine to send an START/STOP from the MMC/SSC spec to a device. 
 * For tape drives this may be required prior to using the changer 
 * Load or Unload commands.
 * For CD/DVD drives this is used to Load or Unload a disc which may be
 * required by some media changers.
 */

int StartStop(DEVICE_TYPE fd, int bStart)
{
	CDB_T CDB;
	/* okay, now for the command: */

	CDB[0] = 0x1B;
	CDB[4] = bStart ? 1 : 0;
	CDB[1] = CDB[2] = CDB[3] = CDB[5] = 0;

	if (SCSI_ExecuteCommand(fd, Input, &CDB, 6,NULL, 0, &scsi_error_sense) != 0)
	{
		PrintRequestSense(&scsi_error_sense);
		fprintf(stderr, "Eject (0x1B) failed\n");
		return -1;  /* could not do! */
	}
	return 0; /* did do! */
}

/* Routine to send a LOCK/UNLOCK from the SSC/MMC spec to a device. 
 * This can be used to prevent or allow the Tape or CD/DVD from being
 * removed. 
 */

int LockUnlock(DEVICE_TYPE fd, int bLock)
{
	CDB_T CDB;
	/* okay, now for the command: */

	CDB[0] = 0x1E;
	CDB[1] = CDB[2] = CDB[3] = CDB[5] = 0;
	CDB[4] = (char)bLock;

	if (SCSI_ExecuteCommand(fd, Input, &CDB, 6, NULL, 0, &scsi_error_sense) != 0)
	{
		PrintRequestSense(&scsi_error_sense);
		fprintf(stderr, "Lock/Unlock (0x1E) failed\n");
		return -1;  /* could not do! */
	}
	return 0; /* did do! */
}

int ClearUnitAttention(DEVICE_TYPE fd, RequestSense_T *RequestSense)
{
	CDB_T CDB;
	int RetryCount = 10;	/* Unit Attentions may be stacked */
	RequestSense_T	unit_attention_sense;

	CDB[0] = 0x03;	/* Request Sense */
	CDB[4] = (char)sizeof(*RequestSense);
	CDB[1] = CDB[2] = CDB[3] = CDB[5] = 0;

	while (RetryCount-- > 0)
	{
		if (SCSI_ExecuteCommand(fd, Input, &CDB, 6,
								&unit_attention_sense, sizeof(unit_attention_sense),
								RequestSense) != 0)
		{
			fprintf(stderr, "RequestSense (0x03) failed\n");
			return -1;  /* could not do! */
		}

		if (unit_attention_sense.SenseKey == 0)
		{
			/* If SenseKey is NO SENSE then we are done. */
			return 0;
		}
	}
	return -1; /* did do! */

}

static char Spaces[] = "                                                            ";

void PrintHex(int Indent, unsigned char *Buffer, int Length)
{
	int		idxBuffer;
	int		idxAscii;
	int		PadLength;
	char	cAscii;

	for (idxBuffer = 0; idxBuffer < Length; idxBuffer++)
	{
		if ((idxBuffer % 16) == 0)
		{
			if (idxBuffer > 0)
			{
				fputc('\'', stderr);

				for (idxAscii = idxBuffer - 16; idxAscii < idxBuffer; idxAscii++)
				{
					cAscii = Buffer[idxAscii] >= 0x20 && Buffer[idxAscii] < 0x7F ? Buffer[idxAscii] : '.';
					fputc(cAscii, stderr);
				}
				fputs("'\n", stderr);
			}
			fprintf(stderr, "%.*s%04X: ", Indent, Spaces, idxBuffer);
		}
		fprintf(stderr, "%02X ", (unsigned char)Buffer[idxBuffer]);
	}

	PadLength = 16 - (Length % 16);

	if (PadLength > 0)
	{
		fprintf(stderr, "%.*s'", 3 * PadLength, Spaces);

		for (idxAscii = idxBuffer - (16 - PadLength); idxAscii < idxBuffer; idxAscii++)
		{
			cAscii = Buffer[idxAscii] >= 0x20 && Buffer[idxAscii] < 0x80 ? Buffer[idxAscii] : '.';
			fputc(cAscii, stderr);
		}
		fputs("'\n", stderr);
	}

	fflush(stderr);
}

static char *sense_keys[] =
{
	"No Sense",			/* 00 */
	"Recovered Error",	/* 01 */
	"Not Ready",		/* 02 */
	"Medium Error",		/* 03 */
	"Hardware Error",	/* 04 */
	"Illegal Request",	/* 05 */
	"Unit Attention",	/* 06 */
	"Data Protect",		/* 07 */
	"Blank Check",		/* 08 */
	"0x09",				/* 09 */
	"0x0a",				/* 0a */
	"Aborted Command",	/* 0b */
	"0x0c",				/* 0c */
	"Volume Overflow",	/* 0d */
	"Miscompare",		/* 0e */
	"0x0f"				/* 0f */
};

static char Yes[] = "yes";
static char No[] = "no";

void PrintRequestSense(RequestSense_T *RequestSense)
{
	char *msg;

	fprintf(stderr, "mtx: Request Sense: Long Report=yes\n");
	fprintf(stderr, "mtx: Request Sense: Valid Residual=%s\n", RequestSense->Valid ? Yes : No);

	if (RequestSense->ErrorCode == 0x70)
	{ 
		msg = "Current" ;
	}
	else if (RequestSense->ErrorCode == 0x71)
	{
		msg = "Deferred" ;
	}
	else
	{
		msg = "Unknown?!" ;
	}

	fprintf(stderr, "mtx: Request Sense: Error Code=%0x (%s)\n", RequestSense->ErrorCode, msg);
	fprintf(stderr, "mtx: Request Sense: Sense Key=%s\n", sense_keys[RequestSense->SenseKey]);
	fprintf(stderr, "mtx: Request Sense: FileMark=%s\n", RequestSense->Filemark ? Yes : No);
	fprintf(stderr, "mtx: Request Sense: EOM=%s\n", RequestSense->EOM ? Yes : No);
	fprintf(stderr, "mtx: Request Sense: ILI=%s\n", RequestSense->ILI ? Yes : No);

	if (RequestSense->Valid)
	{
		fprintf(stderr, "mtx: Request Sense: Residual = %02X %02X %02X %02X\n",RequestSense->Information[0],RequestSense->Information[1],RequestSense->Information[2],RequestSense->Information[3]);
	}

	fprintf(stderr,"mtx: Request Sense: Additional Sense Code = %02X\n", RequestSense->AdditionalSenseCode);
	fprintf(stderr,"mtx: Request Sense: Additional Sense Qualifier = %02X\n", RequestSense->AdditionalSenseCodeQualifier);

	if (RequestSense->SKSV)
	{
		fprintf(stderr,"mtx: Request Sense: Field in Error = %02X\n", RequestSense->BitPointer);
	}

	fprintf(stderr, "mtx: Request Sense: BPV=%s\n", RequestSense->BPV ? Yes : No);
	fprintf(stderr, "mtx: Request Sense: Error in CDB=%s\n", RequestSense->CommandData ? Yes : No);
	fprintf(stderr, "mtx: Request Sense: SKSV=%s\n", RequestSense->SKSV ? Yes : No);

	if (RequestSense->BPV || RequestSense -> SKSV)
	{
		fprintf(stderr, "mtx: Request Sense: Field Pointer = %02X %02X\n",
				RequestSense->FieldData[0], RequestSense->FieldData[1]);
	}

	fflush(stderr);
}
