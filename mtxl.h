/* 
  MTX -- SCSI Tape Attached Medium Changer Control Program

  Copyright 1997-1998 Leonard N. Zubkoff <lnz@dandelion.com>
  Copyright 2007-2008 by Robert Nelson <robertn@the-nelsons.org>
  This file created by Eric Lee Green <eric@badtux.org>
  
  This program is free software; you may redistribute and/or modify it under
  the terms of the GNU General Public License Version 2 as published by the
  Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY, without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for complete details.

  $Date: 2008-08-19 03:03:38 -0700 (Tue, 19 Aug 2008) $
  $Revision: 193 $
*/

/* Much of the guts of mtx.c has been extracted to mtxl.c, a library file
 * full of utility routines. This file is the header file for that library.
 *   -E
 */

#ifndef MTXL_H
#define MTXL_H 1

#include "mtx.h"

#undef min
#undef max

void FatalError(char *ErrorMessage, ...);
void *xmalloc(size_t Size);
void *xzmalloc(size_t Size);
void slow_bzero(char *buffer, int numchars);

DEVICE_TYPE SCSI_OpenDevice(char *DeviceName);
void SCSI_CloseDevice(char *DeviceName, DEVICE_TYPE DeviceFD);
int SCSI_ExecuteCommand(DEVICE_TYPE DeviceFD,
			       Direction_T Direction,
			       CDB_T *CDB,
			       int CDB_Length,
			       void *DataBuffer,
			       int DataBufferLength,
			       RequestSense_T *RequestSense);

void PrintRequestSense(RequestSense_T *RequestSense);

int BigEndian16(unsigned char *BigEndianData);
int BigEndian24(unsigned char *BigEndianData);
int min(int x, int y);
int max(int x, int y);

void PrintHex(int Indent, unsigned char *Buffer, int Length);
int ClearUnitAttention(DEVICE_TYPE fd, RequestSense_T *RequestSense);

ElementStatus_T *ReadElementStatus(	DEVICE_TYPE MediumChangerFD,
									RequestSense_T *RequestSense,
									Inquiry_T *inquiry_info,
									SCSI_Flags_T *flags);

Inquiry_T *RequestInquiry(	DEVICE_TYPE fd,
							RequestSense_T *RequestSense);

RequestSense_T *MoveMedium(	DEVICE_TYPE MediumChangerFD,
							int SourceAddress,
							int DestinationAddress, 
							ElementStatus_T *ElementStatus, 
							Inquiry_T *inquiry_info,
							SCSI_Flags_T *flags);

RequestSense_T *ExchangeMedium(	DEVICE_TYPE MediumChangerFD,
								int SourceAddress,
								int DestinationAddress,
								int Dest2Address,
								ElementStatus_T *ElementStatus, 
								SCSI_Flags_T *flags);

RequestSense_T *PositionElement(DEVICE_TYPE MediumChangerFD,
								int DestinationAddress,
								ElementStatus_T *ElementStatus);

int Inventory(DEVICE_TYPE MediumChangerFD);  /* inventory library */
int LoadUnload(DEVICE_TYPE fd, int bLoad); /* load/unload tape, magazine or disc */
int StartStop(DEVICE_TYPE fd, int bStart); /* start/stop device */
int LockUnlock(DEVICE_TYPE fd, int bLock); /* lock/unlock medium in device */
RequestSense_T *Erase(DEVICE_TYPE fd);        /* send SHORT erase to drive */

void SCSI_Set_Timeout(int secs); /* set the SCSI timeout */
void SCSI_Default_Timeout(void);  /* go back to default timeout */

/* we may not have this function :-(. */
#ifdef HAVE_GET_ID_LUN
   scsi_id_t *SCSI_GetIDLun(DEVICE_TYPE fd);
#endif

/* These two hacks are so that I can stick the tongue out on an
 * NSM optical jukebox. 
 */ 
NSM_Result_T *RecNSMHack(DEVICE_TYPE MediumChangerFD, 
			 int param_len, int timeout);

int SendNSMHack(DEVICE_TYPE MediumChangerFD, NSM_Param_T *nsm_command, 
		int param_len, int timeout);

#endif

