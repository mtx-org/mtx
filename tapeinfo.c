/* Copyright 2000 Enhanced Software Technologies Inc.
 * Copyright 2007-2008 by Robert Nelson <robertn@the-nelsons.org>
 *   Released under terms of the GNU General Public License as
 * required by the license on 'mtxl.c'.
 * $Date: 2008-08-19 03:03:38 -0700 (Tue, 19 Aug 2008) $
 * $Revision: 193 $
 */

/*#define DEBUG_PARTITION */
/*#define DEBUG 1 */

/* What this does: This basically dumps out the contents of the following
 * pages:
 *
 * Inquiry -- prints full inquiry info. If it's not a tape drive, this is
 * the end of things.
 *    DeviceType:
 *    Manufacturer:
 *    ProdID:  
 *    ProdRevision:
 *
 * Log Sense: TapeAlert Page (if supported):
 *    TapeAlert:[message#]<Message>  e.g. "TapeAlert:[22]Cleaning Cartridge Worn Out"
 *  
 * Mode Sense: 
 *  Data Compression Page:
 *    DataCompEnabled:<yes|no>
 *    DataCompCapable:<yes|no>
 *    DataDeCompEnabled:<yes|no>
 *    CompType:<number>
 *    DeCompType:<number>
 *
 *  Device Configuration Page:
 *    ActivePartition:<#>
 *    DevConfigComp:<#>    -- the compression byte in device config page.
 *    EarlyWarningSize:<#> -- size of early warning buffer?
 *
 *  Medium Partition Page:
 *    NumPartitions:<#>
 *    MaxPartitions:<#>
 *    Partition[0]:<size>
 *    Partition[1]:<size>...
 *
 * Read Block Limits command:
 *    MinBlock:<#>  -- Minimum block size.
 *    MaxBlock:<#>  -- Maximum block size. 
 */

#include <stdio.h>
#include <string.h>
#include "mtx.h"
#include "mtxl.h"

char	*argv0;

void usage(void)
{
	FatalError("Usage: tapeinfo -f <generic-device>\n");
}

/* A table for printing out the peripheral device type as ASCII. */ 
static char *PeripheralDeviceType[32] =
{
	"Disk Drive",
	"Tape Drive",
	"Printer",
	"Processor",
	"Write-once",
	"CD-ROM",
	"Scanner",
	"Optical",
	"Medium Changer",
	"Communications",
	"ASC IT8",
	"ASC IT8",
	"RAID Array",
	"Enclosure Services",
	"OCR/W",
	"Bridging Expander",	/* 0x10 */
	"Reserved",				/* 0x11 */
	"Reserved",				/* 0x12 */
	"Reserved",				/* 0x13 */
	"Reserved",				/* 0x14 */
	"Reserved",				/* 0x15 */
	"Reserved",				/* 0x16 */
	"Reserved",				/* 0x17 */
	"Reserved",				/* 0x18 */
	"Reserved",				/* 0x19 */
	"Reserved",				/* 0x1a */
	"Reserved",				/* 0x1b */
	"Reserved",				/* 0x1c */
	"Reserved",				/* 0x1d */
	"Reserved",				/* 0x1e */
	"Unknown"				/* 0x1f */
};



/* we call it MediumChangerFD for history reasons, sigh. */

/* now to print inquiry information: Copied from other one.... */

static void ReportInquiry(DEVICE_TYPE MediumChangerFD)
{
	RequestSense_T RequestSense;
	Inquiry_T *Inquiry;
	int i;

	Inquiry = RequestInquiry(MediumChangerFD, &RequestSense);
	if (Inquiry == NULL)
	{
		PrintRequestSense(&RequestSense);
		FatalError("INQUIRY Command Failed\n");
	}

	printf("Product Type: %s\n", PeripheralDeviceType[Inquiry->PeripheralDeviceType]);

	printf("Vendor ID: '");
	for (i = 0; i < sizeof(Inquiry->VendorIdentification); i++)
		printf("%c", Inquiry->VendorIdentification[i]);

	printf("'\nProduct ID: '");
	for (i = 0; i < sizeof(Inquiry->ProductIdentification); i++)
		printf("%c", Inquiry->ProductIdentification[i]);

	printf("'\nRevision: '");
	for (i = 0; i < sizeof(Inquiry->ProductRevisionLevel); i++)
		printf("%c", Inquiry->ProductRevisionLevel[i]);
	printf("'\n");

	if (Inquiry->MChngr)
	{
		/* check the attached-media-changer bit... */
		printf("Attached Changer API: Yes\n");
	}
	else
	{
		printf("Attached Changer API: No\n");
	}

	free(Inquiry);		/* well, we're about to exit, but ... */
}




/* Okay, now for the Log Sense Tape Alert Page (if supported): */
#define TAPEALERT_SIZE 2048  /* max size of tapealert buffer. */ 
#define MAX_TAPE_ALERT 0x41

static char *tapealert_messages[] =
{
	"Undefined", /* 0 */
	"         Read: Having problems reading (slowing down)", /* 1 */
	"        Write: Having problems writing (losing capacity)", /* 2 */
	"   Hard Error: Uncorrectable read/write error", /* 3 */
	"        Media: Media Performance Degraded, Data Is At Risk", /* 4 */
	" Read Failure: Tape faulty or tape drive broken", /* 5 */
	"Write Failure: Tape faulty or tape drive broken", /* 6 */
	"   Media Life: The tape has reached the end of its useful life", /* 7 */
	"Not Data Grade:Replace cartridge with one  containing data grade tape",/*8*/
	"Write Protect: Attempted to write to a write-protected cartridge",/*9 */
	"   No Removal: Cannot unload, initiator is preventing media removal", /*a*/
	"Cleaning Media:Cannot back up or restore to a cleaning cartridge", /* b */
	"   Bad Format: The loaded tape contains data in an unsupported format", /*c */
	" Snapped Tape: The data cartridge contains a broken tape", /* d */
	"Undefined", /* e */
	"Undefined", /* f */
	"Undefined", /* 10 */
	"Undefined", /* 11 */
	"Undefined", /* 12 */
	"Undefined", /* 13 */
	"    Clean Now: The tape drive neads cleaning NOW", /* 0x14 */
	"Clean Periodic:The tape drive needs to be cleaned at next opportunity", /* 0x15 */
	"Cleaning Media:Cannot clean because cleaning cartridge used up, insert new cleaning cartridge to clean the drive", /* 0x16 */
	"Undefined", /* 0x17 */
	"Undefined", /* 0x18 */
	"Undefined", /* 0x19 */
	"Undefined", /* 0x1a */
	"Undefined", /* 0x1b */
	"Undefined", /* 0x1c */
	"Undefined", /* 0x1d */
	"   Hardware A: Tape drive has a problem not read/write related", /* 0x1e */
	"   Hardware B: Tape drive has a problem not read/write related", /* 0x1f */
	"    Interface: Problem with SCSI interface between tape drive and initiator", /* 0x20 */
	"  Eject Media: The current operation has failed. Eject and reload media", /* 0x21 */
	"Download Fail: Attempt to download new firmware failed", /* 0x22 */
	"Undefined", /* 0x23 */
	"Undefined", /* 0x24 */
	"Undefined", /* 0x25 */
	"Undefined", /* 0x26 */
	"Undefined", /* 0x27 */
	"Loader Hardware A: Changer having problems communicating with tape drive", /* 0x28   40 */
	"Loader Stray Tape: Stray tape left in drive from prior error", /* 0x29 41 */
	"Loader Hardware B: Autoloader mechanism has a fault", /* 0x2a 42 */
	"  Loader Door: Loader door is open, please close it", /* 0x2b 43 */
	"Undefined", /* 0x2c */
	"Undefined", /* 0x2d */
	"Undefined", /* 0x2e */
	"Undefined", /* 0x2f */
	"Undefined", /* 0x30 */
	"Undefined", /* 0x31 */
	"Undefined", /* 0x32 */
	"Undefined", /* 0x33 */
	"Undefined", /* 0x34 */
	"Undefined", /* 0x35 */
	"Undefined", /* 0x36 */
	"Undefined", /* 0x37 */
	"Undefined", /* 0x38 */
	"Undefined", /* 0x39 */
	"Undefined", /* 0x3a */
	"Undefined", /* 0x3b */
	"Undefined", /* 0x3c */
	"Undefined", /* 0x3d */
	"Undefined", /* 0x3e */
	"Undefined", /* 0x3f */
	"Undefined" /* 0x40 */
};

typedef struct TapeCapacityStruct
{
	unsigned int partition0_remaining;
	unsigned int partition1_remaining;
	unsigned int partition0_size;
	unsigned int partition1_size;
}	TapeCapacity;

#if defined(DEBUG)
/* DEBUG */
static void dump_data(unsigned char *data, int len)
{
	if (len != 0)
	{
		fprintf(stderr,"DATA:");
		PrintHex(1, data, len);
	}
	else
	{
		fprintf(stderr, "**NO DATA**\n");
	}
}
#endif


/* Request the tape capacity page defined by some DAT autoloaders. */

static TapeCapacity *RequestTapeCapacity(DEVICE_TYPE fd, RequestSense_T *sense)
{
	CDB_T CDB;
	TapeCapacity *result;
	int result_len;

	unsigned char buffer[TAPEALERT_SIZE]; /* Overkill, but ... */

	slow_bzero((char *)buffer,TAPEALERT_SIZE); /*zero it... */

	/* now to create the CDB block: */
	CDB[0] = 0x4d;   /* Log Sense */
	CDB[1] = 0;   
	CDB[2] = 0x31;   /* Tape Capacity Page. */
	CDB[3] = 0;
	CDB[4] = 0;
	CDB[5] = 0;
	CDB[6] = 0;
	CDB[7] = TAPEALERT_SIZE >> 8 & 0xff;	/* hi byte, allocation size */
	CDB[8] = TAPEALERT_SIZE & 0xff;			/* lo byte, allocation size */
	CDB[9] = 0;								/* reserved */ 

	if (SCSI_ExecuteCommand(fd, Input, &CDB, 10, buffer, TAPEALERT_SIZE, sense) != 0)
	{
		/*    fprintf(stderr,"RequestTapeCapacity: Command failed: Log Sense\n"); */
		return NULL;
	}

	/* dump_data(buffer,64); */

	/* okay, we have stuff in the result buffer: the first 4 bytes are a header:
	* byte 0 should be 0x31, byte 1 == 0, bytes 2,3 tell how long the
	* log page is. 
	*/
	if ((buffer[0]&0x3f) != 0x31)
	{
		/*    fprintf(stderr,"RequestTapeCapacity: Invalid header for page (not 0x31).\n"); */
		return NULL;
	}

	result_len = ((int)buffer[2] << 8) + buffer[3];

	if (result_len != 32)
	{
		/*   fprintf(stderr,"RequestTapeCapacity: Page was %d bytes long, not 32 bytes\n",result_len); */
		return NULL; /* This Is Not The Page You're Looking For */
	}

	result = xmalloc(sizeof(TapeCapacity));

	/* okay, now allocate data and move the buffer over there: */

		/*	0  1  2  3  4  5  6  7  8  9
	DATA:	31 00 00 20 00 01 4c 04 01 3a
			10 11 12 13 14 15 16 17 18 19
	DATA:	81 0c 00 02 4c 04 00 00 00 00
			20 21 22 23 24 25 26 27 28 29
	DATA:	00 03 4c 04 01 3f 4b 1f 00 04
			30 31 32 33 34 35
	DATA:	4c 04 00 00 00 00 00 00 00 00
	DATA:	00 00 00 00 00 00 00 00 00 00
	DATA:	00 00 00 00 00 00 00 00 00 00
	DATA:	00 00 00 00
	*/

	result->partition0_remaining =
		((unsigned int)buffer[8]  << 24) +
		((unsigned int)buffer[9]  << 16) +
		((unsigned int)buffer[10] <<  8) + 
		buffer[11];

	result->partition1_remaining =
		((unsigned int)buffer[16] << 24) +
		((unsigned int)buffer[17] << 16) +
		((unsigned int)buffer[18] <<  8) +
		buffer[19];

	result->partition0_size =
		((unsigned int)buffer[24] << 24) +
		((unsigned int)buffer[25] << 16) +
		((unsigned int)buffer[26] <<  8) +
		buffer[27];

	result->partition1_size =
		((unsigned int)buffer[32] << 24) +
		((unsigned int)buffer[33] << 16) +
		((unsigned int)buffer[34] <<  8) +
		buffer[35]; 

	return result;
}



struct tapealert_struct
{
	int length;
	unsigned char *data;
};

static struct tapealert_struct *RequestTapeAlert(DEVICE_TYPE fd, RequestSense_T *sense)
{
	CDB_T CDB;

	struct tapealert_struct *result;
	int i, tapealert_len, result_idx;

	unsigned char buffer[TAPEALERT_SIZE];
	unsigned char *walkptr;

	slow_bzero((char *)buffer, TAPEALERT_SIZE); /*zero it... */

	/* now to create the CDB block: */
	CDB[0] = 0x4d;	/* Log Sense */
	CDB[1] = 0;
	CDB[2] = 0x2e;	/* Tape Alert Page. */
	CDB[3] = 0;
	CDB[4] = 0;
	CDB[5] = 0;
	CDB[6] = 0;
	CDB[7] = TAPEALERT_SIZE >> 8 & 0xff;	/* hi byte, allocation size */
	CDB[8] = TAPEALERT_SIZE & 0xff;			/* lo byte, allocation size */
	CDB[9] = 0;								/* reserved */

	if (SCSI_ExecuteCommand(fd,Input,&CDB,10,buffer,TAPEALERT_SIZE,sense)!=0)
	{
		return NULL;
	}

	result = xmalloc(sizeof(struct tapealert_struct));

	/* okay, we have stuff in the result buffer: the first 4 bytes are a header:
	 * byte 0 should be 0x2e, byte 1 == 0, bytes 2,3 tell how long the
	 * tapealert page is. 
	 */
	if ((buffer[0]&0x3f) != 0x2e)
	{
		result->data = NULL;
		result->length = 0;
		return result;
	}

	tapealert_len = ((int)buffer[2] << 8) + buffer[3];

	if (!tapealert_len)
	{
		result->length = 0;
		result->data = NULL;
		return result;
	}

	/* okay, now allocate data and move the buffer over there: */
	result->length = MAX_TAPE_ALERT;
	result->data = xzmalloc(MAX_TAPE_ALERT); /* alloc & zero. */

	walkptr = &buffer[4];
	i = 0;

	while (i < tapealert_len)
	{
		result_idx=(((int)walkptr[0])<<8) + walkptr[1]; /* the parameter #. */
		if (result_idx > 0 && result_idx < MAX_TAPE_ALERT)
		{
			if (walkptr[4])
			{
				result->data[result_idx] = 1; 
			}
			else
			{
				result->data[result_idx] = 0;
			}
#ifdef DEBUGOLD1
			fprintf(stderr,"Alert[0x%x]=%d\n",result_idx,result->data[result_idx]);
			fflush(stderr);
#endif
		}
		else
		{
			FatalError("Invalid tapealert page: %d\n",result_idx);
		}

		i = i + 4 + walkptr[3]; /* length byte! */
		walkptr = walkptr + 4 + walkptr[3]; /* next! */
	}
	return result;
}

static void ReportTapeCapacity(DEVICE_TYPE fd)
{
	/* we actually ignore a bad sense reading, like might happen if the 
	 * tape drive does not support the tape capacity page. 
	 */

	RequestSense_T RequestSense;

	TapeCapacity *result;

	result=RequestTapeCapacity(fd,&RequestSense);

	if (!result)
		return;

	printf("Partition 0 Remaining Kbytes: %d\n", result->partition0_remaining);
	printf("Partition 0 Size in Kbytes: %d\n", result->partition0_size);

	if (result->partition1_size)
	{
		printf("Partition 1 Remaining Kbytes: %d\n", result->partition1_remaining);
		printf("Partition 1 Size in Kbytes: %d\n", result->partition1_size);
	}

	free(result);
}



static void ReportTapeAlert(DEVICE_TYPE fd)
{
	/* we actually ignore a bad sense reading, like might happen if the 
	 * tape drive does not support the tapealert page. 
	 */

	RequestSense_T RequestSense;

	struct tapealert_struct *result;
	int i;

	result=RequestTapeAlert(fd,&RequestSense);

	if (!result)
		return; /* sorry. Don't print sense here. */

	if (!result->length)
		return; /* sorry, no alerts valid. */

	for (i = 0; i < result->length; i++)
	{
		if (result->data[i])
		{
			printf("TapeAlert[%d]: %s.\n", i, tapealert_messages[i]);
		}
	}

	free(result->data);
	free(result);
}

static unsigned char
*mode_sense(DEVICE_TYPE fd, char pagenum, int alloc_len,  RequestSense_T *RequestSense)
{
	CDB_T CDB;

	unsigned char *input_buffer;
	unsigned char *tmp;
	unsigned char *retval;
	int i, pagelen;

	if (alloc_len > 255)
	{
		FatalError("mode_sense(6) can only read up to 255 characters!\n");
	}

	input_buffer = (unsigned char *)xzmalloc(256); /* overdo it, eh? */

	/* clear the sense buffer: */
	slow_bzero((char *)RequestSense, sizeof(RequestSense_T));

	/* returns an array of bytes in the page, or, if not possible, NULL. */
	CDB[0] = 0x1a; /* Mode Sense(6) */
	CDB[1] = 0; 
	CDB[2] = pagenum; /* the page to read. */
	CDB[3] = 0;
	CDB[4] = 255; /* allocation length. This does max of 256 bytes! */
	CDB[5] = 0;

	if (SCSI_ExecuteCommand(fd, Input, &CDB, 6, input_buffer, 255, RequestSense) != 0)
	{
#ifdef DEBUG_MODE_SENSE
		fprintf(stderr,"Could not execute mode sense...\n");
		fflush(stderr);
#endif
		return NULL; /* sorry, couldn't do it. */
	}

	/* Oh hell, write protect is the only thing I have: always print
	 * it if our mode page was 0x0fh, before skipping past buffer: 
	 * if the media is *NOT* write protected, just skip, sigh. 
	 *
	 * Oh poops, the blocksize is reported in the block descriptor header
	 * <   * too. Again, just print if our mode page was 0x0f...
	 */
	if (pagenum == 0x0f)
	{
		int blocklen;

		if (input_buffer[2] & 0x80)
		{
			printf("WriteProtect: yes\n");
		}

		if (input_buffer[2] & 0x70)
		{
			printf("BufferedMode: yes\n");
		}

		if (input_buffer[1] )
		{
			printf("Medium Type: 0x%x\n", input_buffer[1]);
		}
		else
		{
			printf("Medium Type: Not Loaded\n");
		}

		printf("Density Code: 0x%x\n", input_buffer[4]);
		/* Put out the block size: */

		blocklen =	((int)input_buffer[9]  << 16)+
					((int)input_buffer[10] <<  8)+
					input_buffer[11];

		printf("BlockSize: %d\n", blocklen);
	}

	/* First skip past any header.... */
	tmp = input_buffer + 4 + input_buffer[3];

	/* now find out real length of page... */
	pagelen = tmp[1] + 2;
	retval = xmalloc(pagelen);

	/* and copy our data to the new page. */
	for (i=0;i<pagelen;i++)
	{
		retval[i]=tmp[i];
	}

	/* okay, free our input buffer: */
	free(input_buffer);
	return retval;
}


#define DCE_MASK 0x80
#define DCC_MASK 0x40
#define DDE_MASK 0x80

static void ReportCompressionPage(DEVICE_TYPE fd)
{
	/* actually ignore a bad sense reading, like might happen if the tape
	* drive does not support the mode sense compression page. 
	*/

	RequestSense_T RequestSense;

	unsigned char *compression_page;

	compression_page=mode_sense(fd,0x0f,16,&RequestSense);

	if (!compression_page)
	{
		return;  /* sorry! */
	}

	/* Okay, we now have the compression page. Now print stuff from it: */
	printf("DataCompEnabled: %s\n", (compression_page[2] & DCE_MASK)? "yes" : "no");
	printf("DataCompCapable: %s\n", (compression_page[2] & DCC_MASK)? "yes" : "no");
	printf("DataDeCompEnabled: %s\n", (compression_page[3] & DDE_MASK)? "yes" : "no");
	printf("CompType: 0x%x\n",
		(compression_page[4] << 24) +
		(compression_page[5] << 16) +
		(compression_page[6] <<  8) +
		 compression_page[7]);

	printf("DeCompType: 0x%x\n",
		(compression_page[8]  << 24) +
		(compression_page[9]  << 16) +
		(compression_page[10] <<  8) +
		 compression_page[11]);

	free(compression_page);
}

/* Now for the device configuration mode page: */

static void ReportConfigPage(DEVICE_TYPE fd)
{
	RequestSense_T RequestSense;
	unsigned char *config_page;

	config_page = mode_sense(fd, 0x10, 16, &RequestSense);
	if (!config_page)
		return;

	/* Now to print the stuff: */
	printf("ActivePartition: %d\n", config_page[3]);

	/* The following does NOT work accurately on any tape drive I know of... */
	/*  printf("DevConfigComp: %s\n", config_page[14] ? "yes" : "no"); */
	printf("EarlyWarningSize: %d\n",
		(config_page[11] << 16) +
		(config_page[12] <<  8) +
		 config_page[13]);
}

/* ***************************************
 * Medium Partition Page:
 * 
 * The problem here, as we oh so graphically demonstrated during debugging
 * of the Linux 'st' driver :-), is that there are actually *TWO* formats for
 * the Medium Partition Page: There is the "long" format, where there is a
 * partition size word for each partition on the page, and there is a "short"
 * format, beloved of DAT drives, which only has a partition size word for
 * partition #1 (and no partition size word for partition #0, and no
 * provisions for any more partitions). So we must look at the size and
 * # of partitions defined to know what to report as what. 
 *
 ********************************************/

static void ReportPartitionPage(DEVICE_TYPE fd)
{
	RequestSense_T RequestSense;
	unsigned char *partition_page;

	int num_parts,max_parts;
	int i;

	partition_page=mode_sense(fd,0x11,255,&RequestSense);
	if (!partition_page)
		return;

	/* Okay, now we have either old format or new format: */
	num_parts = partition_page[3];
	max_parts = partition_page[2];

	printf("NumPartitions: %d\n", num_parts);
	printf("MaxPartitions: %d\n", max_parts);

	if (!num_parts)
	{
		/* if no additional partitions, then ... */ 
		free(partition_page);
		return;
	}

	/* we know we have at least one partition if we got here. Check the
	 * page size field. If it is 8 or below, then we are the old format....
	 */

#ifdef DEBUG_PARTITION
	fprintf(stderr,"partition_page[1]=%d\n",partition_page[1]);
	fflush(stderr);
#endif
	if (partition_page[1]==8)
	{
		/* old-style! */
		printf("Partition1: %d\n",(partition_page[8]<<8)+partition_page[9]);
	}
	else
	{
		/* new-style! */
		for (i=0;i<=max_parts;i++)
		{
#ifdef DEBUG_PARTITION
			fprintf(stderr,"partition%d:[%d]%d [%d]%d\n", i, 8 + i * 2,
			partition_page[8+i*2]<<8, 9+i*2,partition_page[9 + i * 2]);
			fflush(stderr);
#endif
			printf("Partition%d: %d\n", i,
					(partition_page[8 + i * 2] << 8) + partition_page[9 + i * 2]);
		}
	}
	free(partition_page);
}

static void ReportSerialNumber(DEVICE_TYPE fd)
{
	/*	Actually ignore a bad sense reading, like might happen if the
		tape drive does not support the inquiry page 0x80. 
	*/

	RequestSense_T sense;
	CDB_T CDB;

#define WILD_SER_SIZE 30
unsigned char buffer[WILD_SER_SIZE]; /* just wildly overestimate serial# length! */

	int i, lim;
	char *bufptr; 

	CDB[0] = 0x12;			/* INQUIRY */
	CDB[1] = 1;				/* EVPD = 1 */
	CDB[2] = 0x80;			/* The serial # page, hopefully. */
	CDB[3] = 0;				/* reserved */
	CDB[4] = WILD_SER_SIZE;
	CDB[5] = 0;

	if (SCSI_ExecuteCommand(fd, Input, &CDB, 6, &buffer, sizeof(buffer), &sense) != 0)
	{
		/* PrintRequestSense(&sense); */ /* zap debugging output :-) */
		/* printf("No Serial Number: None\n"); */
		return; 
	}

	/* okay, we have something in our buffer. Byte 3 should be the length of
	the sernum field, and bytes 4 onward are the serial #. */

	lim = (int)buffer[3];
	bufptr = (char *)&(buffer[4]);

	printf("SerialNumber: '");
	for (i=0;i<lim;i++)
	{
		putchar(*bufptr++);
	}
	printf("'\n");
}

/*  Read Block Limits! */

void ReportBlockLimits(DEVICE_TYPE fd)
{
	RequestSense_T sense;
	CDB_T CDB;
	unsigned char buffer[6];

	CDB[0] = 0x05;	/* READ_BLOCK_LIMITS */
	CDB[1] = 0;
	CDB[2] = 0;
	CDB[3] = 0;		/* 1-5 all unused. */
	CDB[4] = 0;
	CDB[5] = 0; 

	slow_bzero((char *)&sense,sizeof(RequestSense_T));
	if (SCSI_ExecuteCommand(fd, Input, &CDB, 6, buffer, 6, &sense) != 0)
	{
		return;
	}

	/* okay, but if we did get a result, print it: */
	printf("MinBlock: %d\n", (buffer[4] << 8) + buffer[5]);
	printf("MaxBlock: %d\n", (buffer[1] << 16) + (buffer[2]<<8) + buffer[3]);
}

/* Do a READ_POSITION. This may not be always valid, but (shrug). */
void ReadPosition(DEVICE_TYPE fd)
{
	RequestSense_T sense;
	CDB_T CDB;
	unsigned char buffer[20];
	unsigned int position;

	CDB[0] = 0x34;		/* READ_POSITION */
	CDB[1] = 0;
	CDB[2] = 0;
	CDB[3] = 0;			/* 1-9 all unused. */
	CDB[4] = 0;
	CDB[5] = 0;
	CDB[6] = 0;
	CDB[7] = 0;
	CDB[8] = 0;
	CDB[9] = 0;

	slow_bzero((char *)&sense, sizeof(RequestSense_T));

	SCSI_Set_Timeout(2); /* set timeout to 2 seconds! */

	/* if we don't get a result (e.g. we issue this to a disk drive), punt. */
	if (SCSI_ExecuteCommand(fd, Input, &CDB, 10, buffer, 20, &sense) != 0)
	{
		return;
	}

	SCSI_Default_Timeout(); /* reset it to 5 minutes, sigh! */
	/* okay, but if we did get a result, print it: */

#define RBL_BOP 0x80
#define RBL_EOP 0x40
#define RBL_BCU 0x20
#define RBL_BYCU  0x10
#define RBL_R1 0x08
#define RBL_BPU 0x04
#define RBL_PERR 0x02

	/* If we have BOP, go ahead and print that. */
	if (buffer[0]&RBL_BOP)
	{
		printf("BOP: yes\n");
	}

	/* if we have valid data, print it: */
	if (buffer[0]&RBL_BPU)
	{
		printf("Block Position: -1");
	}
	else
	{
		position = (unsigned int)(((unsigned int)buffer[4] << 24) +
								  ((unsigned int)buffer[5] << 16) +
								  ((unsigned int)buffer[6] <<  8) +
									buffer[7]);

		printf("Block Position: %d\n",position);
	}
}

/* Test unit ready: This will tell us whether the tape drive
 * is currently ready to read or write.
 */

int TestUnitReady(DEVICE_TYPE fd)
{
	RequestSense_T sense;
	CDB_T CDB;
	unsigned char buffer[6];

	CDB[0] = 0x00;		/* TEST_UNIT_READY */
	CDB[1] = 0;
	CDB[2] = 0;
	CDB[3] = 0;			/* 1-5 all unused. */
	CDB[4] = 0;
	CDB[5] = 0;

	slow_bzero((char *)&sense,sizeof(RequestSense_T));
	if (SCSI_ExecuteCommand(fd,Input,&CDB,6,buffer,0,&sense)!=0)
	{
		printf("Ready: no\n");
		return 0;
	}

	printf("Ready: yes\n");
	return 1;
}

/* We write a filemarks of 0 before going to grab position, in order
 * to insure that data in the buffer is not a problem. 
 */

int WriteFileMarks(DEVICE_TYPE fd,int count)
{
	RequestSense_T sense;
	CDB_T CDB;
	unsigned char buffer[6];

	CDB[0] = 0x10;  /* WRITE_FILE_MARKS */
	CDB[1] = 0;
	CDB[2] = (unsigned char)(count >> 16);
	CDB[3] = (unsigned char)(count >> 8);
	CDB[4] = (unsigned char)count;
	CDB[5] = 0; 

	/* we really don't care if this command works or not, sigh.  */
	slow_bzero((char *)&sense, sizeof(RequestSense_T));
	if (SCSI_ExecuteCommand(fd, Input, &CDB, 6, buffer, 0, &sense) != 0)
	{
		return 1;
	}

	return 0;
}


/* This will get the SCSI ID and LUN of the target device, if such
 * is available from the OS. Currently only Linux supports this,
 * but other drivers could, if someone wants to write a 
 * SCSI_GetIDLun function for them. 
 */
#ifdef HAVE_GET_ID_LUN

static void ReportIDLun(DEVICE_TYPE fd)
{
	scsi_id_t *scsi_id;

	scsi_id = SCSI_GetIDLun(fd);
	printf("SCSI ID: %d\nSCSI LUN: %d\n", scsi_id->id, scsi_id->lun);
}

#endif

/* we only have one argument: "-f <device>". */
int main(int argc, char **argv)
{
	DEVICE_TYPE fd;
	char *filename;

	argv0=argv[0];

	if (argc != 3)
	{
		fprintf(stderr,"argc=%d",argc);
		usage();
	}

	if (strcmp(argv[1],"-f")!=0)
	{
		usage();
	}
	filename=argv[2];

	fd=SCSI_OpenDevice(filename);

	/* Now to call the various routines: */
	ReportInquiry(fd);
	ReportSerialNumber(fd);
	ReportTapeAlert(fd);
	ReportBlockLimits(fd); 

#ifdef HAVE_GET_ID_LUN
	ReportIDLun(fd);
#endif

	/* okay, we should only report position if the unit is ready :-(. */
	if (TestUnitReady(fd))
	{
		ReportCompressionPage(fd); 
		ReadPosition(fd); 
		ReportTapeCapacity(fd);		/* only if we have it */
		ReportConfigPage(fd);		/* only valid if unit is ready. */
		ReportPartitionPage(fd); 
	}

	exit(0);
}
