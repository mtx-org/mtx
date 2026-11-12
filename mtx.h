/* MTX -- SCSI Tape Attached Medium Control Program

	Copyright 1997-1998 Leonard N. Zubkoff <lnz@dandelion.com>

	Changes 1999 Eric Lee Green to add support for multi-drive tape changers.
	Copyright 2007-2008 by Robert Nelson <robertn@the-nelsons.org>

	$Date: 2008-08-19 03:03:38 -0700 (Tue, 19 Aug 2008) $
	$Revision: 193 $
	See mtx.c for licensing information. 

*/

#ifndef MTX_H  /* protect against multiple includes... */
#define MTX_H 1

/* surround all the Unix-stuff w/ifndef VMS */
#ifdef VMS
#include "[.vms]defs.h"
#else /* all the Unix stuff:  */

#ifdef _MSC_VER
#include "msvc/config.h"  /* all the autoconf stuff. */
#else
#include "config.h"  /* all the autoconf stuff. */
#endif

/* all the general Unix includes: */

#include <stdio.h>
#include <errno.h>

#if HAVE_STDLIB_H
#  include <stdlib.h>
#endif

#if HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#if HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#if HAVE_STDARG_H
#  include <stdarg.h>
#endif

#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#if HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif

#if HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif

/* Now greatly modified to use GNU Autoconf stuff: */
/* If we use the 'sg' interface, like Linux, do this: */
#if HAVE_SCSI_SG_H
#  include <scsi/scsi.h>
#  include <scsi/scsi_ioctl.h>
#  include <scsi/sg.h>
typedef int DEVICE_TYPE; /* the sg interface uses this. */
#  define HAVE_GET_ID_LUN 1  /* signal that we have it... */
#endif

/* Windows Native programs built using MinGW */
#if HAVE_DDK_NTDDSCSI_H
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <ddk/ntddscsi.h>
#  undef DEVICE_TYPE

typedef int DEVICE_TYPE;
#endif

/* Windows Native programs built using Microsoft Visual C */
#ifdef _MSC_VER
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winioctl.h>
#  include <ntddscsi.h>
#  undef DEVICE_TYPE

typedef int DEVICE_TYPE;
#endif

/* The 'cam' interface, like FreeBSD: */
#if HAVE_CAMLIB_H
#  include <camlib.h> /* easy (?) access to the CAM user library. */
#  include <cam/cam_ccb.h>
#  include <cam/scsi/scsi_message.h> /* sigh sigh sigh! */
typedef struct cam_device *DEVICE_TYPE;
#endif


/* the 'uscsi' interface, as used on Solaris: */
#if HAVE_SYS_SCSI_IMPL_USCSI_H
#include <sys/scsi/impl/uscsi.h>
typedef int DEVICE_TYPE;
#endif

/* the scsi_ctl interface, as used on HP/UX: */
#if HAVE_SYS_SCSI_CTL_H
#  include <sys/wsio.h>
#  include <sys/spinlock.h>
#  include <sys/scsi.h>
#  include <sys/scsi_ctl.h>
  typedef int DEVICE_TYPE;
#  ifndef VERSION
#     define VERSION "1.2.12 hbb"
#  endif
#endif

/* the 'gsc' interface, as used on AIX: */
#if HAVE_SYS_GSCDDS_H
#   include <sys/gscdds.h>
    typedef int DEVICE_TYPE;
#endif

   /* the 'dslib' interface, as used on SGI.  */
#if HAVE_DSLIB_H
#include <dslib.h>
typedef dsreq_t *DEVICE_TYPE; /* 64-bit pointers/32bit int on later sgi? */
#endif


#if ((defined(__alpha) && defined(__osf__)) || \
     defined(ultrix) || defined(__ultrix))
#include "du/defs.h"
#endif


#endif /* VMS protect. */

/* Do a test for LITTLE_ENDIAN_BITFIELDS. Use WORDS_BIGENDIAN as set
 * by configure: 
 */

#if WORDS_BIGENDIAN
# define BIG_ENDIAN_BITFIELDS
#else
# define LITTLE_ENDIAN_BITFIELDS
#endif

/* Get rid of some Hocky Pux defines: */
#ifdef S_NO_SENSE
#undef S_NO_SENSE
#endif
#ifdef S_RECOVERED_ERROR
#undef S_RECOVERED_ERROR
#endif
#ifdef S_NOT_READY
#undef S_NOT_READY
#endif
#ifdef S_MEDIUM_ERROR
#undef S_MEDIUM_ERROR
#endif
#ifdef S_HARDWARE_ERROR
#undef S_HARDWARE_ERROR
#endif
#ifdef S_UNIT_ATTENTION
#undef S_UNIT_ATTENTION
#endif
#ifdef S_BLANK_CHECK
#undef S_BLANK_CHECK
#endif
#ifdef S_VOLUME_OVERFLOW
#undef S_VOLUME_OVERFLOW
#endif

/* Note: These are only used for defaults for when we don't have 
 * the element assignment mode page to tell us real amount...
 */
#define MAX_STORAGE_ELEMENTS 64 /* for the BIG jukeboxes! */
#define MAX_TRANSFER_ELEMENTS 2  /* we just do dual-drive for now :-} */
#define MAX_TRANSPORT_ELEMENTS 1 /* we just do one arm for now... */

#define MTX_ELEMENTSTATUS_ORIGINAL 0
#define MTX_ELEMENTSTATUS_READALL 1

/* These are flags used for the READ_ELEMENT_STATUS and MOVE_MEDIUM
 * commands:
 */
typedef struct SCSI_Flags_Struct
{
	unsigned char eepos;
	unsigned char invert;
	unsigned char no_attached; /* ignore _attached bit */
	unsigned char no_barcodes;  /* don't try to get barcodes. */
	int numbytes;
	int elementtype;
	int numelements;
	int attached;
	int has_barcodes;
	int querytype; //MTX_ELEMENTSTATUS
	unsigned char invert2; /* used for EXCHANGE command, sigh. */
}	SCSI_Flags_T;

#ifdef _MSC_VER
typedef unsigned char boolean;

#define false   0
#define true    1

typedef unsigned char Direction_T;

#define Input   0
#define Output  1
#else
typedef enum { false, true } boolean;

typedef enum { Input, Output } Direction_T;
#endif


typedef unsigned char CDB_T[12];


typedef struct Inquiry
{
#ifdef LITTLE_ENDIAN_BITFIELDS
  unsigned char PeripheralDeviceType:5;			/* Byte 0 Bits 0-4 */
  unsigned char PeripheralQualifier:3;			/* Byte 0 Bits 5-7 */
  unsigned char DeviceTypeModifier:7;			/* Byte 1 Bits 0-6 */
  boolean RMB:1;					/* Byte 1 Bit 7 */
  unsigned char ANSI_ApprovedVersion:3;			/* Byte 2 Bits 0-2 */
  unsigned char ECMA_Version:3;				/* Byte 2 Bits 3-5 */
  unsigned char ISO_Version:2;				/* Byte 2 Bits 6-7 */
  unsigned char ResponseDataFormat:4;			/* Byte 3 Bits 0-3 */
  unsigned char :2;					/* Byte 3 Bits 4-5 */
  boolean TrmIOP:1;					/* Byte 3 Bit 6 */
  boolean AENC:1;					/* Byte 3 Bit 7 */
#else
  unsigned char PeripheralQualifier:3;			/* Byte 0 Bits 5-7 */
  unsigned char PeripheralDeviceType:5;			/* Byte 0 Bits 0-4 */
  boolean RMB:1;					/* Byte 1 Bit 7 */
  unsigned char DeviceTypeModifier:7;			/* Byte 1 Bits 0-6 */
  unsigned char ISO_Version:2;				/* Byte 2 Bits 6-7 */
  unsigned char ECMA_Version:3;				/* Byte 2 Bits 3-5 */
  unsigned char ANSI_ApprovedVersion:3;			/* Byte 2 Bits 0-2 */
  boolean AENC:1;					/* Byte 3 Bit 7 */
  boolean TrmIOP:1;					/* Byte 3 Bit 6 */
  unsigned char :2;					/* Byte 3 Bits 4-5 */
  unsigned char ResponseDataFormat:4;			/* Byte 3 Bits 0-3 */
#endif
  unsigned char AdditionalLength;			/* Byte 4 */
  unsigned char :8;					/* Byte 5 */
#ifdef LITTLE_ENDIAN_BITFIELDS
  boolean ADDR16:1;                                    /* Byte 6 bit 0 */
  boolean Obs6_1:1;                                    /* Byte 6 bit 1 */
  boolean Obs6_2:1; /* obsolete */                     /* Byte 6 bit 2 */
  boolean MChngr:1; /* Media Changer */                /* Byte 6 bit 3 */
  boolean MultiP:1;                                    /* Byte 6 bit 4 */
  boolean VS:1;                                        /* Byte 6 bit 5 */
  boolean EncServ:1;                                   /* Byte 6 bit 6 */
  boolean BQue:1;                                      /* Byte 6 bit 7 */
#else
  boolean BQue:1;                                      /* Byte 6 bit 7 */
  boolean EncServ:1;                                   /* Byte 6 bit 6 */
  boolean VS:1;                                        /* Byte 6 bit 5 */
  boolean MultiP:1;                                    /* Byte 6 bit 4 */
  boolean MChngr:1; /* Media Changer */                /* Byte 6 bit 3 */
  boolean Obs6_2:1; /* obsolete */                     /* Byte 6 bit 2 */
  boolean Obs6_1:1;                                    /* Byte 6 bit 1 */
  boolean ADDR16:1;                                    /* Byte 6 bit 0 */
#endif
#ifdef LITTLE_ENDIAN_BITFIELDS
  boolean SftRe:1;					/* Byte 7 Bit 0 */
  boolean CmdQue:1;					/* Byte 7 Bit 1 */
  boolean :1;						/* Byte 7 Bit 2 */
  boolean Linked:1;					/* Byte 7 Bit 3 */
  boolean Sync:1;					/* Byte 7 Bit 4 */
  boolean WBus16:1;					/* Byte 7 Bit 5 */
  boolean WBus32:1;					/* Byte 7 Bit 6 */
  boolean RelAdr:1;					/* Byte 7 Bit 7 */
#else
  boolean RelAdr:1;					/* Byte 7 Bit 7 */
  boolean WBus32:1;					/* Byte 7 Bit 6 */
  boolean WBus16:1;					/* Byte 7 Bit 5 */
  boolean Sync:1;					/* Byte 7 Bit 4 */
  boolean Linked:1;					/* Byte 7 Bit 3 */
  boolean :1;						/* Byte 7 Bit 2 */
  boolean CmdQue:1;					/* Byte 7 Bit 1 */
  boolean SftRe:1;					/* Byte 7 Bit 0 */
#endif
  unsigned char VendorIdentification[8];		/* Bytes 8-15 */
  unsigned char ProductIdentification[16];		/* Bytes 16-31 */
  unsigned char ProductRevisionLevel[4];		/* Bytes 32-35 */
  unsigned char FullProductRevisionLevel[19];           /* bytes 36-54 */
  unsigned char VendorFlags;                            /* byte 55 */
}
Inquiry_T;

/* Hockey Pux may define these. If so, *UN*define them. */
#ifdef ILI
#undef ILI
#endif

#ifdef EOM
#undef EOM
#endif

typedef struct RequestSense
{
#ifdef LITTLE_ENDIAN_BITFIELDS
  unsigned char ErrorCode:7;				/* Byte 0 Bits 0-6 */
  boolean Valid:1;					/* Byte 0 Bit 7 */
#else
  boolean Valid:1;					/* Byte 0 Bit 7 */
  unsigned char ErrorCode:7;				/* Byte 0 Bits 0-6 */
#endif
  unsigned char SegmentNumber;				/* Byte 1 */
#ifdef LITTLE_ENDIAN_BITFIELDS
  unsigned char SenseKey:4;				/* Byte 2 Bits 0-3 */
  unsigned char :1;					/* Byte 2 Bit 4 */
  boolean ILI:1;					/* Byte 2 Bit 5 */
  boolean EOM:1;					/* Byte 2 Bit 6 */
  boolean Filemark:1;					/* Byte 2 Bit 7 */
#else
  boolean Filemark:1;					/* Byte 2 Bit 7 */
  boolean EOM:1;					/* Byte 2 Bit 6 */
  boolean ILI:1;					/* Byte 2 Bit 5 */
  unsigned char :1;					/* Byte 2 Bit 4 */
  unsigned char SenseKey:4;				/* Byte 2 Bits 0-3 */
#endif
  unsigned char Information[4];				/* Bytes 3-6 */
  unsigned char AdditionalSenseLength;			/* Byte 7 */
  unsigned char CommandSpecificInformation[4];		/* Bytes 8-11 */
  unsigned char AdditionalSenseCode;			/* Byte 12 */
  unsigned char AdditionalSenseCodeQualifier;		/* Byte 13 */
  unsigned char :8;					/* Byte 14 */
#ifdef LITTLE_ENDIAN_BITFIELDS
  unsigned char BitPointer:3;                           /* Byte 15 */
  boolean BPV:1;
  unsigned char :2;
  boolean CommandData :1;
  boolean SKSV:1;      
#else
  boolean SKSV:1;      
  boolean CommandData :1;
  unsigned char :2;
  boolean BPV:1;
  unsigned char BitPointer:3;                           /* Byte 15 */
#endif
  unsigned char FieldData[2];       	 		/* Byte 16,17 */
}
RequestSense_T;

/* Okay, now for the element status mode sense page (0x1d): */

typedef struct ElementModeSensePageHeader {
  unsigned char PageCode;  /* byte 0 */
  unsigned char ParameterLengthList;  /* byte 1; */
  unsigned char MediumTransportStartHi; /* byte 2,3 */
  unsigned char MediumTransportStartLo;
  unsigned char NumMediumTransportHi;   /* byte 4,5 */
  unsigned char NumMediumTransportLo;   /* byte 4,5 */
  unsigned char StorageStartHi;         /* byte 6,7 */
  unsigned char StorageStartLo;         /* byte 6,7 */
  unsigned char NumStorageHi;           /* byte 8,9 */
  unsigned char NumStorageLo;           /* byte 8,9 */
  unsigned char ImportExportStartHi;    /* byte 10,11 */
  unsigned char ImportExportStartLo;    /* byte 10,11 */
  unsigned char NumImportExportHi;      /* byte 12,13 */
  unsigned char NumImportExportLo;      /* byte 12,13 */
  unsigned char DataTransferStartHi;    /* byte 14,15 */
  unsigned char DataTransferStartLo;    /* byte 14,15 */
  unsigned char NumDataTransferHi;      /* byte 16,17 */
  unsigned char NumDataTransferLo;      /* byte 16,17 */
  unsigned char Reserved1;             /* byte 18, 19 */
  unsigned char Reserved2;             /* byte 18, 19 */
} ElementModeSensePage_T;

typedef struct ElementModeSenseHeader {
  int MaxReadElementStatusData; /* 'nuff for all of below. */
  int NumElements;              /* total # of elements. */
  int MediumTransportStart;
  int NumMediumTransport;
  int StorageStart;
  int NumStorage;
  int ImportExportStart;
  int NumImportExport;
  int DataTransferStart;
  int NumDataTransfer;
} ElementModeSense_T;


#ifdef _MSC_VER
typedef char ElementTypeCode_T;

#define AllElementTypes	        0
#define MediumTransportElement  1
#define StorageElement	        2
#define ImportExportElement     3
#define DataTransferElement     4
#else
typedef enum ElementTypeCode
{
  AllElementTypes =		0,
  MediumTransportElement =	1,
  StorageElement =		2,
  ImportExportElement =		3,
  DataTransferElement =		4
}
ElementTypeCode_T;
#endif


typedef struct ElementStatusDataHeader
{
  unsigned char FirstElementAddressReported[2];		/* Bytes 0-1 */
  unsigned char NumberOfElementsAvailable[2];		/* Bytes 2-3 */
  unsigned char :8;					/* Byte 4 */
  unsigned char ByteCountOfReportAvailable[3];		/* Bytes 5-7 */
}
ElementStatusDataHeader_T;


typedef struct ElementStatusPage
{
  ElementTypeCode_T ElementTypeCode:8;			/* Byte 0 */
#ifdef LITTLE_ENDIAN_BITFIELDS
  unsigned char :6;					/* Byte 1 Bits 0-5 */
  boolean AVolTag:1;					/* Byte 1 Bit 6 */
  boolean PVolTag:1;					/* Byte 1 Bit 7 */
#else
  boolean PVolTag:1;					/* Byte 1 Bit 7 */
  boolean AVolTag:1;					/* Byte 1 Bit 6 */
  unsigned char :6;					/* Byte 1 Bits 0-5 */
#endif
  unsigned char ElementDescriptorLength[2];		/* Bytes 2-3 */
  unsigned char :8;					/* Byte 4 */
  unsigned char ByteCountOfDescriptorDataAvailable[3];	/* Bytes 5-7 */
}
ElementStatusPage_T;

typedef struct Element2StatusPage
{
  ElementTypeCode_T ElementTypeCode:8;			/* Byte 0 */
  unsigned char VolBits ; /* byte 1 */
#define E2_PVOLTAG 0x80
#define E2_AVOLTAG 0x40
  unsigned char ElementDescriptorLength[2];		/* Bytes 2-3 */
  unsigned char :8;					/* Byte 4 */
  unsigned char ByteCountOfDescriptorDataAvailable[3];	/* Bytes 5-7 */
}
Element2StatusPage_T;



typedef struct TransportElementDescriptorShort
{
  unsigned char ElementAddress[2];			/* Bytes 0-1 */
#ifdef LITTLE_ENDIAN_BITFIELDS
  boolean Full:1;					/* Byte 2 Bit 0 */
  unsigned char :1;					/* Byte 2 Bit 1 */
  boolean Except:1;					/* Byte 2 Bit 2 */
  unsigned char :5;					/* Byte 2 Bits 3-7 */
#else
  unsigned char :5;					/* Byte 2 Bits 3-7 */
  boolean Except:1;					/* Byte 2 Bit 2 */
  unsigned char :1;					/* Byte 2 Bit 1 */
  boolean Full:1;					/* Byte 2 Bit 0 */
#endif
  unsigned char :8;					/* Byte 3 */
  unsigned char AdditionalSenseCode;			/* Byte 4 */
  unsigned char AdditionalSenseCodeQualifier;		/* Byte 5 */
  unsigned char :8;					/* Byte 6 */
  unsigned char :8;					/* Byte 7 */
  unsigned char :8;					/* Byte 8 */
#ifdef LITTLE_ENDIAN_BITFIELDS
  unsigned char :6;					/* Byte 9 Bits 0-5 */
  boolean SValid:1;					/* Byte 9 Bit 6 */
  boolean Invert:1;					/* Byte 9 Bit 7 */
#else
  boolean Invert:1;					/* Byte 9 Bit 7 */
  boolean SValid:1;					/* Byte 9 Bit 6 */
  unsigned char :6;					/* Byte 9 Bits 0-5 */
#endif
  unsigned char SourceStorageElementAddress[2];		/* Bytes 10-11 */
#ifdef HAS_LONG_DESCRIPTORS
  unsigned char Reserved[4];                            /* Bytes 12-15 */
#endif
}
TransportElementDescriptorShort_T;


typedef struct TransportElementDescriptor
{
  unsigned char ElementAddress[2];			/* Bytes 0-1 */
#ifdef LITTLE_ENDIAN_BITFIELDS
  boolean Full:1;					/* Byte 2 Bit 0 */
  unsigned char :1;					/* Byte 2 Bit 1 */
  boolean Except:1;					/* Byte 2 Bit 2 */
  unsigned char :5;					/* Byte 2 Bits 3-7 */
#else
  unsigned char :5;					/* Byte 2 Bits 3-7 */
  boolean Except:1;					/* Byte 2 Bit 2 */
  unsigned char :1;					/* Byte 2 Bit 1 */
  boolean Full:1;					/* Byte 2 Bit 0 */
#endif
  unsigned char :8;					/* Byte 3 */
  unsigned char AdditionalSenseCode;			/* Byte 4 */
  unsigned char AdditionalSenseCodeQualifier;		/* Byte 5 */
  unsigned char :8;					/* Byte 6 */
  unsigned char :8;					/* Byte 7 */
  unsigned char :8;					/* Byte 8 */
#ifdef LITTLE_ENDIAN_BITFIELDS
  unsigned char :6;					/* Byte 9 Bits 0-5 */
  boolean SValid:1;					/* Byte 9 Bit 6 */
  boolean Invert:1;					/* Byte 9 Bit 7 */
#else
  boolean Invert:1;					/* Byte 9 Bit 7 */
  boolean SValid:1;					/* Byte 9 Bit 6 */
  unsigned char :6;					/* Byte 9 Bits 0-5 */
#endif
  unsigned char SourceStorageElementAddress[2];		/* Bytes 10-11 */
  unsigned char PrimaryVolumeTag[36];          /* barcode */
  unsigned char AlternateVolumeTag[36];   
#ifdef HAS_LONG_DESCRIPTORS
  unsigned char Reserved[4];				/* 4 extra bytes? */
#endif
     
}
TransportElementDescriptor_T;




/* Now for element status data; */

typedef unsigned char barcode[37];

typedef struct ElementStatus {

  int StorageElementCount;
  int ImportExportCount;
  int DataTransferElementCount;
  int *DataTransferElementAddress;  /* array. */
  int *DataTransferElementSourceStorageElementNumber; /* array */
  barcode *DataTransferPrimaryVolumeTag; /* array. */
  barcode *DataTransferAlternateVolumeTag; /* array. */
  barcode *PrimaryVolumeTag;  /* array */
  barcode *AlternateVolumeTag; /* array */
  int *StorageElementAddress; /* array */
  boolean *StorageElementIsImportExport; /* array */

  int TransportElementAddress;  /* assume only one of those... */

  boolean *DataTransferElementFull; /* array */
  boolean *StorageElementFull;  /* array */

} ElementStatus_T;


/* Now for the SCSI ID and LUN information: */
typedef struct scsi_id {
  int id;
  int lun;
} scsi_id_t;

#define MEDIUM_CHANGER_TYPE 8  /* what type bits are set for medium changers. */

/* The following two structs are used for the brain-dead functions of the
 * NSM jukebox. 
 */

typedef struct NSM_Param {
  unsigned char page_code;
  unsigned char reserved;
  unsigned char page_len_msb;
  unsigned char page_len_lsb;
  unsigned char allocation_msb;
  unsigned char allocation_lsb;
  unsigned char reserved2[2];
  unsigned char command_code[4];
  unsigned char command_params[2048]; /* egregious overkill. */
} NSM_Param_T;

extern RequestSense_T scsi_error_sense; 

typedef struct NSM_Result {
  unsigned char page_code;
  unsigned char reserved;
  unsigned char page_len_msb;
  unsigned char page_len_lsb;
  unsigned char command_code[4];
  unsigned char ces_code[2]; 
  unsigned char return_data[0xffff]; /* egregioius overkill */
} NSM_Result_T;

#endif  /* of multi-include protection. */
