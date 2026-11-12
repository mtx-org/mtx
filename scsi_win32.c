/* Copyright 2006-2008 Robert Nelson <robertn@the-nelsons.org>

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

/*
 * This is the SCSI commands for Windows.
 */

#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef _MSC_VER
#include <ntddscsi.h>
#else
#include <ddk/ntddscsi.h>
#endif

#define SCSI_DEFAULT_TIMEOUT	300		/* 1 minutes */
#define SCSI_MAX_TIMEOUT		108000	/* 30 hours */

typedef	struct	_HANDLE_ENTRY
{
	HANDLE  hDevice;
	UCHAR   PortId;
	UCHAR   PathId;
	UCHAR   TargetId;
	UCHAR   Lun;
} HANDLE_ENTRY, *PHANDLE_ENTRY;

PHANDLE_ENTRY	HandleTable = NULL;
int				nEntries = 0;

DEVICE_TYPE SCSI_OpenDevice(char *DeviceName)
{
	int		DeviceIndex;
	TCHAR	szDevicePath[256];

	int		nColons = 0;
	int		index;

	int		port, path, target, lun;

	for (DeviceIndex = 0; DeviceIndex < nEntries; DeviceIndex++)
	{
		if (HandleTable[DeviceIndex].hDevice == INVALID_HANDLE_VALUE)
			break;
	}

	if (DeviceIndex >= nEntries)
	{
		PHANDLE_ENTRY pNewTable;

		nEntries += 4;

		if (HandleTable == NULL)
		{
			pNewTable = (PHANDLE_ENTRY)malloc(nEntries * sizeof(HANDLE_ENTRY));
		}
		else
		{
			pNewTable = (PHANDLE_ENTRY)realloc(HandleTable, nEntries * sizeof(HANDLE_ENTRY));
		}

		if (pNewTable == NULL)
		{
			FatalError("cannot open SCSI device '%s' - %m\n", DeviceName);
		}

		HandleTable = pNewTable;
	}

	for (index = 0; DeviceName[index] != '\0'; index++)
	{
		if (DeviceName[index] == ':')
			nColons++;
		else if (DeviceName[index] < '0' || DeviceName[index] > '9')
			break;
	}

	if (DeviceName[index] == '\0' && nColons == 3 && 
		sscanf(DeviceName, "%d:%d:%d:%d", &port, &path, &target, &lun) == 4)
	{
		HandleTable[DeviceIndex].PortId = (UCHAR)port;
		HandleTable[DeviceIndex].PathId = (UCHAR)path;
		HandleTable[DeviceIndex].TargetId = (UCHAR)target;
		HandleTable[DeviceIndex].Lun = (UCHAR)lun;

		sprintf(szDevicePath, "\\\\.\\scsi%d:", port);
	}
	else 
	{
		int nPrefixLength = 0;

		if (DeviceName[0] != '\\') {
			memcpy(szDevicePath, "\\\\.\\", 4 * sizeof(TCHAR));
			nPrefixLength = 4;
		}

		HandleTable[DeviceIndex].PortId = 0;
		HandleTable[DeviceIndex].PathId = 0;
		HandleTable[DeviceIndex].TargetId = 0;
		HandleTable[DeviceIndex].Lun = 0;

		strncpy(&szDevicePath[nPrefixLength], 
				DeviceName, 
				sizeof(szDevicePath) / sizeof(TCHAR) - nPrefixLength - 1);

		szDevicePath[sizeof(szDevicePath) / sizeof(TCHAR) - 1] = '\0';
	}

	HandleTable[DeviceIndex].hDevice = CreateFile(szDevicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (HandleTable[DeviceIndex].hDevice == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();

#if DEBUG
		LPSTR lpszMessage;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, (LPSTR)&lpszMessage, 0, NULL);
		fputs(lpszMessage, stderr);
#endif

		switch (dwError)
		{
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			errno = ENOENT;
			break;

		case ERROR_TOO_MANY_OPEN_FILES:
			errno =  EMFILE;
			break;

		default:
		case ERROR_ACCESS_DENIED:
		case ERROR_SHARING_VIOLATION:
		case ERROR_LOCK_VIOLATION:
		case ERROR_INVALID_NAME:
			errno = EACCES;
			break;

		case ERROR_FILE_EXISTS:
			errno = EEXIST;
			break;

		case ERROR_INVALID_PARAMETER:
			errno = EINVAL;
			break;
		}

		FatalError("cannot open SCSI device '%s' - %m\n", DeviceName);
	}

	return DeviceIndex;
}

static int scsi_timeout = SCSI_DEFAULT_TIMEOUT;

void SCSI_Set_Timeout(int secs)
{
	if (secs > SCSI_MAX_TIMEOUT)
	{
		secs = SCSI_MAX_TIMEOUT;
	}

	scsi_timeout = secs;
}
 
void SCSI_Default_Timeout(void)
{
	scsi_timeout = SCSI_DEFAULT_TIMEOUT;
}

void SCSI_CloseDevice(char *DeviceName, DEVICE_TYPE DeviceFD)
{
	if (DeviceFD < nEntries)
	{
		CloseHandle(HandleTable[DeviceFD].hDevice);
		HandleTable[DeviceFD].hDevice = INVALID_HANDLE_VALUE;
	}
	else
	{
		errno = EBADF;
		FatalError("cannot close SCSI device '%s' - %m\n", DeviceName);
	}
}


/* Get the SCSI ID and LUN... */
scsi_id_t *SCSI_GetIDLun(DEVICE_TYPE fd)
{
	scsi_id_t *		retval;

	SCSI_ADDRESS	ScsiAddress;
	BOOL			bResult;
	DWORD			dwBytesReturned;

	if (fd < nEntries)
	{
		retval = (scsi_id_t *)xmalloc(sizeof(scsi_id_t));
		retval->id = HandleTable[fd].TargetId;
		retval->lun = HandleTable[fd].Lun;

#ifdef DEBUG
		fprintf(stderr,"SCSI:ID=%d LUN=%d\n", retval->id, retval->lun);
#endif
		return retval;
	}
	else
	{
		errno = EBADF;
		FatalError("cannot close SCSI device - %m\n");
	}

	memset(&ScsiAddress, 0, sizeof(ScsiAddress));

	ScsiAddress.Length = sizeof(ScsiAddress);

	bResult = DeviceIoControl(	HandleTable[fd].hDevice, 
								IOCTL_SCSI_GET_ADDRESS, 
								&ScsiAddress, sizeof(ScsiAddress), 
								&ScsiAddress, sizeof(ScsiAddress), 
								&dwBytesReturned, 
								NULL);

	if (!bResult)
	{
		return NULL;
	}

	retval = (scsi_id_t *)xmalloc(sizeof(scsi_id_t));
	retval->id = ScsiAddress.TargetId;
	retval->lun = ScsiAddress.Lun;

#ifdef DEBUG
	fprintf(stderr,"SCSI:ID=%d LUN=%d\n",retval->id,retval->lun);
#endif
	return retval;
}

int SCSI_ExecuteCommand(DEVICE_TYPE DeviceFD,
						Direction_T Direction,
						CDB_T *CDB,
						int CDB_Length,
						void *DataBuffer,
						int DataBufferLength,
						RequestSense_T *RequestSense)
{
	PSCSI_PASS_THROUGH ScsiPassThrough;

	const DWORD	dwDataBufferOffset = sizeof(SCSI_PASS_THROUGH) + (sizeof(RequestSense_T) + 3) / 4 * 4;
	const DWORD	dwBufferSize = dwDataBufferOffset + DataBufferLength;

	BOOL		bResult;
	DWORD		dwBytesReturned;
	DWORD		dwInputLength;
	DWORD		dwOutputLength;

	if (DeviceFD >= nEntries || HandleTable[DeviceFD].hDevice == INVALID_HANDLE_VALUE)
	{
		errno = EBADF;
		return -1;
	}

	ScsiPassThrough = (PSCSI_PASS_THROUGH)malloc(dwBufferSize);

	memset(ScsiPassThrough, 0, dwDataBufferOffset);

	ScsiPassThrough->Length = sizeof(SCSI_PASS_THROUGH);

	ScsiPassThrough->PathId = HandleTable[DeviceFD].PathId;
	ScsiPassThrough->TargetId = HandleTable[DeviceFD].TargetId;
	ScsiPassThrough->Lun = HandleTable[DeviceFD].Lun;
	ScsiPassThrough->CdbLength = (UCHAR)CDB_Length;
	ScsiPassThrough->DataIn = Direction == Input;
	ScsiPassThrough->DataBufferOffset = dwDataBufferOffset;
	ScsiPassThrough->DataTransferLength = DataBufferLength;
	ScsiPassThrough->SenseInfoOffset = sizeof(SCSI_PASS_THROUGH);
	ScsiPassThrough->SenseInfoLength = sizeof(RequestSense_T);
	ScsiPassThrough->TimeOutValue = scsi_timeout;

	memcpy(ScsiPassThrough->Cdb, CDB, CDB_Length);
	dwBytesReturned = 0;

	if (Direction == Output)
	{
		memcpy((void *)(((char *)ScsiPassThrough) + dwDataBufferOffset), DataBuffer, DataBufferLength);
		dwInputLength = dwBufferSize;
		dwOutputLength = dwDataBufferOffset;
	}
	else
	{
		dwInputLength = sizeof(SCSI_PASS_THROUGH);
		dwOutputLength = dwBufferSize;
	}

	bResult = DeviceIoControl(	HandleTable[DeviceFD].hDevice, 
								IOCTL_SCSI_PASS_THROUGH, 
								ScsiPassThrough, dwInputLength, 
								ScsiPassThrough, dwOutputLength, 
								&dwBytesReturned, 
								NULL);
	if (bResult)
	{
		if (ScsiPassThrough->ScsiStatus != 0)
		{
			memcpy(RequestSense, &ScsiPassThrough[1], sizeof(RequestSense_T));
#if DEBUG
			fprintf(stderr, "Command failed - ScsiStatus = %d\n", ScsiPassThrough->ScsiStatus);
			PrintRequestSense(RequestSense);
#endif
			bResult = false;
		}
		else
		{
			if (Direction == Input)
			{
				memcpy(	DataBuffer, 
						(void *)(((char *)ScsiPassThrough) + dwDataBufferOffset),
						DataBufferLength);
			}
		}
	}
	else
	{
#if DEBUG
		DWORD   dwError = GetLastError();
		LPSTR   lpszMessage;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, (LPSTR)&lpszMessage, 0, NULL);
		fputs(lpszMessage, stderr);
		LocalFree(lpszMessage);
#endif

		memset(RequestSense, 0, sizeof(RequestSense_T));
	}

	free(ScsiPassThrough);

	return bResult ? 0 : -1;
}
