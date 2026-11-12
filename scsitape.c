/* Copyright 2001 Enhanced Software Technologies Inc.
 * Copyright 2007-2008 by Robert Nelson <robertn@the-nelsons.org>
 *   Released under terms of the GNU General Public License as
 * required by the license on 'mtxl.c'.
 * $Date: 2008-08-19 03:03:38 -0700 (Tue, 19 Aug 2008) $
 * $Revision: 193 $
 */

/* This is a generic SCSI tape control program. It operates by
 * directly sending commands to the tape drive. If you are going
 * through your operating system's SCSI tape driver, do *NOT* use 
 * this program! If, on the other hand, you are using raw READ and WRITE
 * commands through your operating system's generic SCSI interface (or
 * through our built-in 'read' and 'write'), this is the place for you. 
 */

/*#define DEBUG_PARTITION */
/*#define DEBUG 1 */

/* 
	Commands:
		setblk <n> -- set the block size to <n>
		fsf <n> -- go forward by <n> filemarks
		bsf <n> -- go backward by <n> filemarks
		eod  -- go to end of data
		rewind -- rewind back to start of data
		eject  -- rewind, then eject the tape. 
		erase  -- (short) erase the tape (we have no long erase)
		mark <n> -- write <n> filemarks.
		seek <n> -- seek to position <n>.

		write <blksize> <-- write blocks from stdin to the tape 
		read  [<blksize>] [<#blocks/#bytes>] -- read blocks from tape, write to stdout. 

	See the 'tapeinfo' program for status info about the tape drive.

 */

#include <stdio.h>
#include <string.h>

#include "mtx.h"
#include "mtxl.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_SYS_MTIO_H
#include <sys/mtio.h> /* will try issuing some ioctls for Solaris, sigh. */
#endif

#ifdef _MSC_VER
#include <io.h>
#endif

void Usage(void) {
  FatalError("Usage: scsitape -f <generic-device> <command> where <command> is:\n setblk <n> | fsf <n> | bsf <n> | eod | rewind | eject | mark <n> |\n  seek <n> | read [<blksize> [<numblocks]] | write [<blocksize>] \n");
}

#define arg1 (arg[0])  /* for backward compatibility, sigh */
static int arg[4];  /* the argument for the command, sigh. */

/* the device handle we're operating upon, sigh. */
static char *device;  /* the text of the device thingy. */
static DEVICE_TYPE MediumChangerFD = (DEVICE_TYPE) -1;



static int S_setblk(void);
static int S_fsf(void);
static int S_bsf(void);
static int S_eod(void);
static int S_rewind(void);
static int S_eject(void);
static int S_mark(void);
static int S_seek(void);
static int S_reten(void);
static int S_erase(void);

static int S_read(void);
static int S_write(void);


struct command_table_struct {
	int num_args;
	char *name;
	int (*command)(void);
} command_table[] = {
	{ 1, "setblk", S_setblk },
	{ 1, "fsf", S_fsf },
	{ 1, "bsf", S_bsf },
	{ 0, "eod", S_eod },
	{ 0, "rewind", S_rewind },
	{ 0, "eject", S_eject },
	{ 0, "reten", S_reten },
	{ 0, "erase", S_erase },
	{ 1, "mark", S_mark },
	{ 1, "seek", S_seek },
	{ 2, "read", S_read },
	{ 2, "write",S_write },
	{ 0, NULL, NULL } /* terminate list */
};

char *argv0;


/* open_device() -- set the 'fh' variable.... */
void open_device(void)
{
	if (MediumChangerFD != -1)
	{
		SCSI_CloseDevice("Unknown",MediumChangerFD);  /* close it, sigh...  new device now! */
	}

	MediumChangerFD = SCSI_OpenDevice(device);
}

static int get_arg(char *arg)
{
	int retval=-1;

	if (*arg < '0' || *arg > '9')
	{
		return -1;  /* sorry! */
	}

	retval=atoi(arg);
	return retval;
}


/* we see if we've got a file open. If not, we open one :-(. Then
 * we execute the actual command. Or not :-(. 
 */ 
int execute_command(struct command_table_struct *command)
{
	/* if the device is not already open, then open it from the 
	* environment.
	*/
	if (!MediumChangerFD == -1)
	{
		/* try to get it from STAPE or TAPE environment variable... */
		device = getenv("STAPE");
		if (device == NULL)
		{
			device = getenv("TAPE");
			if (device == NULL)
			{
				Usage();
			}
		}
		open_device();
	}

	/* okay, now to execute the command... */
	return command->command();
}


/* parse_args():
 *   Basically, we are parsing argv/argc. We can have multiple commands
 * on a line now, such as "unload 3 0 load 4 0" to unload one tape and
 * load in another tape into drive 0, and we execute these commands one
 * at a time as we come to them. If we don't have a -f at the start, we
 * barf. If we leave out a drive #, we default to drive 0 (the first drive
 * in the cabinet). 
 */ 

int parse_args(int argc, char **argv)
{
	int i, cmd_tbl_idx,retval,arg_idx;
	struct command_table_struct *command;

	i=1;
	arg_idx = 0;
	while (i < argc)
	{
		if (strcmp(argv[i],"-f") == 0)
		{
			i++;
			if (i >= argc)
			{
				Usage();
			}
			device = argv[i++];
			open_device(); /* open the device and do a status scan on it... */
		}
		else
		{
			cmd_tbl_idx=0;
			command = &command_table[0]; /* default to the first command... */
			command = &command_table[cmd_tbl_idx];
			while (command->name)
			{
				if (strcmp(command->name,argv[i]) == 0)
				{
					/* we have a match... */
					break;
				}
				/* otherwise we don't have a match... */
				cmd_tbl_idx++;
				command = &command_table[cmd_tbl_idx];
			}
			/* if it's not a command, exit.... */
			if (command->name == NULL)
			{
				Usage();
			}
			i++;  /* go to the next argument, if possible... */
			/* see if we need to gather arguments, though! */
			arg1 = -1; /* default it to something */
			for (arg_idx=0;arg_idx < command->num_args ; arg_idx++)
			{
				if (i < argc)
				{
					arg[arg_idx] = get_arg(argv[i]);
					if (arg[arg_idx] !=  -1)
					{
						i++; /* increment i over the next cmd. */
					}
				}
				else
				{
					arg[arg_idx] = 0; /* default to 0 setmarks or whatever */
				}
			}
			retval=execute_command(command);  /* execute_command handles 'stuff' */
			exit(retval);
		}
	}
	return 0; /* should never get here */
}

/* For Linux, this allows us to do a short erase on a tape (sigh!).
 * Note that you'll need to do a 'mt status' on the tape afterwards in
 * order to get the tape driver in sync with the tape drive again. Also
 * note that on other OS's, this might do other evil things to the tape
 * driver. Note that to do an erase, you must first rewind!

 */
static int S_erase(void)
{
	int retval;
	RequestSense_T *RequestSense;

	retval=S_rewind();
	if (retval)
	{
		return retval;  /* we have an exit status :-(. */
	} 

	RequestSense=Erase(MediumChangerFD);
	if (RequestSense)
	{
		PrintRequestSense(RequestSense);
		exit(1);  /* exit with an error status. */
	}
	return 0;
}

/* This should eject a tape or magazine, depending upon the device sent
 * to.
 */
static int S_eject(void)
{
	int i;
	i = LoadUnload(MediumChangerFD, 0);
	if ( i < 0)
	{
		fprintf(stderr,"scsitape:eject failed\n");
		fflush(stderr);
	}
	return i;
}



/* We write a filemarks of 0 before going to grab position, in order
 * to insure that data in the buffer is not a problem. 
 */

static int S_mark(void)
{
	RequestSense_T RequestSense; /* for result of ReadElementStatus */
	CDB_T CDB;
	unsigned char buffer[6];
	int count = arg1; /* voila! */

	CDB[0] = 0x10;  /* SET_MARK */
	CDB[1] = 0;
	CDB[2] = (unsigned char)(count >> 16);
	CDB[3] = (unsigned char)(count >> 8);
	CDB[4] = (unsigned char)count;
	CDB[5] = 0; 

	/* we really don't care if this command works or not, sigh.  */
	slow_bzero((char *)&RequestSense, sizeof(RequestSense_T));

	if (SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 6, buffer, 0, &RequestSense)!= 0)
	{
		PrintRequestSense(&RequestSense);
		return 1;
	}
	return 0;
}
/* let's rewind to bod! 
 */

static int S_rewind(void)
{
	RequestSense_T sense;
	CDB_T CDB;
	unsigned char buffer[6];

	CDB[0] = 0x01;  /* REWIND */
	CDB[1] = 0;
	CDB[2] = 0;
	CDB[3] = 0;
	CDB[4] = 0;
	CDB[5] = 0; 

	/* we really don't care if this command works or not, sigh.  */
	slow_bzero((char *)&sense,sizeof(RequestSense_T));
	if (SCSI_ExecuteCommand(MediumChangerFD,Input,&CDB,6,buffer,0,&sense)!=0)
	{
		PrintRequestSense(&sense);
		return 1;
	}
	return 0;
}




/* This is used for fsf and bsf. */
static int Space(int count, char code)
{
	RequestSense_T sense;
	CDB_T CDB;
	unsigned char buffer[6];

	CDB[0] = 0x11;  /* SET_MARK */
	CDB[1] = code;
	CDB[2] = (unsigned char)(count >> 16);
	CDB[3] = (unsigned char)(count >> 8);
	CDB[4] = (unsigned char)count;
	CDB[5] = 0; 

	/* we really don't care if this command works or not, sigh.  */
	slow_bzero((char *)&sense,sizeof(RequestSense_T));
	if (SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 6, buffer, 0, &sense) != 0)
	{
		PrintRequestSense(&sense);
		return 1;
	}
	return 0;
}


/* Let's try a fsf: */
/* We write a filemarks of 0 before going to grab position, in order
 * to insure that data in the buffer is not a problem. 
 */

static int S_fsf(void)
{
	return Space(arg1,1); /* go forward! */
}

static int S_bsf(void)
{
	return Space(-arg1,1); /* go backward! */
}

static int S_eod(void)
{
	return Space(0,3); /* go to eod! */
}

/* sigh, abuse of the LOAD command...

 */
static int S_reten(void)
{
	RequestSense_T sense;
	CDB_T CDB;
	unsigned char buffer[6];

	CDB[0] = 0x1B;  /* START_STOP */
	CDB[1] = 0; /* wait */
	CDB[2] = 0;
	CDB[3] = 0;
	CDB[4] = 3; /* reten. */ 
	CDB[5] = 0; 

	/* we really don't care if this command works or not, sigh.  */
	slow_bzero((char *)&sense, sizeof(RequestSense_T));
	if (SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 6, buffer, 0, &sense) != 0)
	{
		PrintRequestSense(&sense);
		return 1;
	}
	return 0;
}

/* seek a position on the tape (sigh!) */
static int S_seek(void)
{
	RequestSense_T sense;
	CDB_T CDB;
	unsigned char buffer[6];
	int count =  arg1;

	/* printf("count=%d\n",arg1); */

	CDB[0] = 0x2B;  /* LOCATE */
	CDB[1] = 0;  /* Logical */
	CDB[2] = 0; /* padding */
	CDB[3] = (unsigned char)(count >> 24);
	CDB[4] = (unsigned char)(count >> 16);
	CDB[5] = (unsigned char)(count >> 8);
	CDB[6] = (unsigned char)count;
	CDB[7] = 0; 
	CDB[8] = 0; 
	CDB[9] = 0; 

	/* we really don't care if this command works or not, sigh.  */
	slow_bzero((char *)&sense,sizeof(RequestSense_T));
	if (SCSI_ExecuteCommand(MediumChangerFD, Input, &CDB, 10, buffer, 0, &sense) != 0)
	{
		PrintRequestSense(&sense);
		return 1;
	}
	return 0;
}

#ifdef MTSRSZ
static int Solaris_setblk(int fh,int count)
{
	/* we get here only if we have a MTSRSZ, which means Solaris. */
	struct mtop mt_com;  /* the struct used for the MTIOCTOP ioctl */
	int result;

	/* okay, we have fh and count.... */

	/* Now to try the ioctl: */
	mt_com.mt_op=MTSRSZ;
	mt_com.mt_count=count;

	/* surround the actual ioctl to enable threading, since fsf/etc. can be
	 * big time consumers and we want other threads to be able to run too. 
	 */

	result=ioctl(fh, MTIOCTOP, (char *)&mt_com);

	if (result < 0)
	{
		return errno;
	}

	/* okay, we did okay. Return a value of None... */
	return 0;
}
#endif


/* okay, this is a write: we need to set the block size to something: */
static int S_setblk(void)
{
	RequestSense_T sense;
	CDB_T CDB;
	char buffer[12];
	unsigned int count = (unsigned int) arg1;

	CDB[0] = 0x15;  /* MODE SELECT */
	CDB[1] = 0x10;  /* scsi2 */
	CDB[2] = 0; 
	CDB[3] = 0;
	CDB[4] = 12; /* length of data */
	CDB[5] = 0;

	slow_bzero((char *)&sense, sizeof(RequestSense_T));
	slow_bzero(buffer, 12);

	/* Now to set the mode page header: */
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0x10; /* we are in buffered mode now, people! */
	buffer[3] = 8; /* block descriptor length. */ 
	buffer[4] = 0; /* reset to default density, sigh. */ /* 0 */
	buffer[5] = 0; /* 1 */
	buffer[6] = 0; /* 2 */
	buffer[7] = 0; /* 3 */
	buffer[8] = 0; /* 4 */
	buffer[9] = (unsigned char)(count >> 16); /* 5 */
	buffer[10] = (unsigned char)(count >> 8); /* 6 */
	buffer[11] = (unsigned char)count;  /* 7 */

	if (SCSI_ExecuteCommand(MediumChangerFD,Output,&CDB,6,buffer,12,&sense)!=0)
	{
		PrintRequestSense(&sense);
		return 1;
	}
#ifdef MTSRSZ
	/*   Solaris_setblk(MediumChangerFD,count);   */
#endif

	return 0;
}

/*************************************************************************/
/* SCSI read/write calls. These are mostly pulled out of BRU 16.1, 
 * modified to work within the mtxl.h framework rather than the
 * scsi_lowlevel.h framework. 
 *************************************************************************/ 

#define MAX_READ_SIZE 128*1024  /* max size of a variable-block read */

#define READ_OK 0
#define READ_FILEMARK 1
#define READ_EOD 2
#define READ_EOP 3
#define READ_SHORT 5
#define READ_ERROR 255

#define WRITE_OK 0
#define WRITE_ERROR 1
#define WRITE_EOM 2
#define WRITE_EOV 3


/* These are copied out of BRU 16.1, with all the boolean masks changed
 * to our bitmasks.
*/
#define S_NO_SENSE(s) ((s).SenseKey == 0x0)
#define S_RECOVERED_ERROR(s) ((s).SenseKey == 0x1)

#define S_NOT_READY(s) ((s).SenseKey == 0x2)
#define S_MEDIUM_ERROR(s) ((s).SenseKey == 0x3)
#define S_HARDWARE_ERROR(s) ((s).SenseKey == 0x4)
#define S_UNIT_ATTENTION(s) ((s).SenseKey == 0x6)
#define S_BLANK_CHECK(s) ((s).SenseKey == 0x8)
#define S_VOLUME_OVERFLOW(s) ((s).SenseKey == 0xd)

#define DEFAULT_TIMEOUT 3*60  /* 3 minutes here */

#define HIT_FILEMARK(s) (S_NO_SENSE((s)) && (s).Filemark && (s).Valid)
/* Sigh, the T-10 SSC spec says all of the following is needed to
 * detect a short read while in variable block mode. We'll see.
 */
#define SHORT_READ(s) (S_NO_SENSE((s)) && (s).ILI && (s).Valid &&  (s).AdditionalSenseCode==0  && (s).AdditionalSenseCodeQualifier==0)

#define HIT_EOD(s) (S_BLANK_CHECK((s)) && (s).Valid)
#define HIT_EOP(s) (S_MEDIUM_ERROR((s)) && (s).EOM && (s).Valid)
#define HIT_EOM(s) ((s).EOM && (s).Valid)
#define BECOMING_READY(s) (S_UNIT_ATTENTION((s)) && (s).AdditionalSenseCode == 0x28 && (s).AdditionalSenseCodeQualifier == 0)

/* Reading is a problem. We can hit a filemark, hit an EOD, or hit an
 * EOP. Our caller may do something about that. Note that we assume that
 * our caller has already put us into fixed block mode. If he has not, then
 * we are in trouble anyhow. 
 */
int SCSI_readt(DEVICE_TYPE fd, char * buf, unsigned int bufsize, unsigned int *len, unsigned int timeout) {
	int rtnval;
	CDB_T cmd;

	int blockCount;
	int info;

	RequestSense_T RequestSense;

	if (bufsize==0)
	{
		/* we are in variable block mode */
		blockCount=MAX_READ_SIZE; /* variable block size. */
	}
	else
	{
		blockCount= *len / bufsize;
		if ((*len % bufsize) != 0)
		{
			fprintf(stderr,"Error: Data (%d bytes) not even multiple of block size (%d bytes).\n",*len,bufsize);
			exit(1); /* we're finished, sigh. */
		}
	}

	if (timeout == 0)
	{
		timeout = 1 * 60; /* 1 minutes */
	}

	memset(&cmd, 0, sizeof(CDB_T));
	cmd[0] = 0x08; /* READ */
	cmd[1] = (bufsize) ? 1 : 0; /* fixed length or var length blocks */
	cmd[2] = (unsigned char)(blockCount >> 16); /* MSB */
	cmd[3] = (unsigned char)(blockCount >> 8);
	cmd[4] = (unsigned char)blockCount; /* LSB */

	/* okay, let's read, look @ the result code: */
	rtnval=READ_OK;
	if (SCSI_ExecuteCommand(fd,Input,&cmd,6,buf,(bufsize) ? *len : MAX_READ_SIZE,&RequestSense))
	{
		rtnval=READ_ERROR;
		if (HIT_EOP(RequestSense))
		{
			cmd[0]=0x08;
			rtnval=READ_EOP;
		}

		if (HIT_FILEMARK(RequestSense))
		{
			rtnval=READ_FILEMARK;
		}

		if (HIT_EOD(RequestSense))
		{
			rtnval=READ_EOD;
		}

		if ( (bufsize==0) && SHORT_READ(RequestSense))
		{
			rtnval=READ_SHORT; /* we only do short reads for variable block mode */
		}

		if (rtnval != READ_ERROR)
		{
			/* info contains number of blocks or bytes *not* read. May be 
			negative if the block we were trying to read was too big. So
			we will have to account for that and set it to zero if so, so that
			we return the proper # of blocks read. 
			*/
			info = ((RequestSense.Information[0] << 24) +
					(RequestSense.Information[1] << 16) +
					(RequestSense.Information[2] << 8) +
					 RequestSense.Information[3]);

			/* on 64-bit platforms, we may need to turn 'info' into a negative # */
			if (info > 0x7fffffff)
				info = 0;

			if (info < 0)
				info=0;  /* make sure  we don't return too big len read. */

			/* Now set *len to # of bytes read. */
			*len= bufsize ? (blockCount-info) * bufsize : MAX_READ_SIZE-info ;
		}
		else
		{
			PrintRequestSense(&RequestSense);
			exit(1);  /* foo. */
		}
	}

	return rtnval;
}

/* Low level SCSI write. Modified from BRU 16.1,  with much BRU smarts
 * taken out and with the various types changed to mtx types rather than
 * BRU types.
 */ 
int SCSI_write(DEVICE_TYPE fd, char * buf, unsigned int blocksize,
				unsigned int *len)
{
	CDB_T cmd;

	int blockCount;
	int rtnval=0;
	RequestSense_T RequestSense;

	if (blocksize == 0)
	{
		/* we are in variable block mode */
		blockCount = *len; /* variable block size. */
	}
	else
	{
		blockCount= *len / blocksize ;
		if ((*len % blocksize) != 0)
		{
			fprintf(stderr,"Error: Data (%d bytes) not even multiple of block size (%d bytes).\n",*len,blocksize);
			exit(1); /* we're finished, sigh. */
		}
	}

	fprintf(stderr,"Writing %d blocks\n",blockCount);

	memset(&cmd, 0, sizeof(CDB_T));
	cmd[0] = 0x0a; /* WRITE */
	cmd[1] = (blocksize) ? 1 : 0; /* fixed length or var length blocks */
	cmd[2] = (unsigned char)(blockCount >> 16); /* MSB */
	cmd[3] = (unsigned char)(blockCount >> 8);
	cmd[4] = (unsigned char)blockCount; /* LSB */


	if (SCSI_ExecuteCommand(fd,Output,&cmd,6,buf, *len, &RequestSense))
	{
		if (HIT_EOM(RequestSense))
		{
			/* we hit end of media. Return -1. */
			if (S_VOLUME_OVERFLOW(RequestSense))
			{
				exit(WRITE_EOV);
			}
			exit(WRITE_EOM); /* end of media! */
		}
		else
		{
			/* it was plain old write error: */
			PrintRequestSense(&RequestSense);
			exit(WRITE_ERROR);
		}
	}
	else
	{
		rtnval = *len; /* worked! */
	}
	return rtnval;
}

/* S_write is not implemented yet! */
static int S_write(void)
{
	char *buffer; /* the buffer we're gonna read/write out of. */
	int buffersize;
	int len; /* the length of the data in the buffer */
	int blocksize = arg[0];
	int numblocks = arg[1];
	int varsize=0; /* variable size block flag */
	int result;
	int eof_input;
	int infile=fileno(stdin); /* sigh */

	if (blocksize == 0)
	{
		varsize = 1;
		buffersize = MAX_READ_SIZE;
		len = MAX_READ_SIZE;
	}
	else
	{
		varsize = 0; /* fixed block mode */
		buffersize = blocksize;
		len = blocksize;
	}

	/* sigh, make it oversized just to have some */  
	buffer = malloc(buffersize+8); 

	eof_input = 0;
	while (!eof_input)
	{
		/* size_t could be 64 bit on a 32 bit platform, so do casts. */
		len=0;
		/* If it is a pipe, we could read 4096 bytes rather than the full
		* 128K bytes or whatever, so we must gather multiple reads into
		* the buffer.
		*/
		while (len < buffersize)
		{
			result=(int)read(infile, buffer + len, (size_t)(buffersize - len));
			if (!result)
			{
				eof_input = 1;
				if (!len)
				{
					/* if we have no deata in our buffer, exit */
					return 0; /* we're at end of file! */
				}
				break;		/* otherwise, break and write the data */
			}
			len += result;	/* add the result input to our length. */
		}

		result = SCSI_write(MediumChangerFD, buffer, blocksize, (unsigned int *)&len);
		if (!result)
		{
			return 1; /* at end of tape! */
		}

		/* Now see if we have numbytes or numblocks. If so, we may wish to exit
		this loop.
		*/
		if (arg[1])
		{
			if (varsize)
			{
				/***BUG***/
				return 0; /* we will only write one block in variable size mode :-( */
			}
			else
			{
				if (numblocks)
				{
					numblocks--;
				}
				else
				{
					return 0; /* we're done. */
				}
			}
		}
	}
	/* and done! */
	return 0;
}

/* Okay, the read thingy: */

/* We have a device opened (we hope!) by the parser. 
 * we will have arg[0] and arg[1] being the blocksize and # of blocks
 * (respectively).
 */ 


static int S_read(void)
{
	char *buffer; /* the buffer we're going to be reading out of */
	int buffersize;
	unsigned int len; /* the length of the data in the buffer */
	int blocksize = arg[0];
	int numblocks = arg[1];
	int varsize = 0; /* variable size block flag. */

	int result;

	int outfile=fileno(stdout); /* sigh. */

	if (blocksize == 0)
	{
		varsize=1;
		buffersize=MAX_READ_SIZE;
		len=MAX_READ_SIZE;
	}
	else
	{
		varsize=0; /* fixed block mode */
		buffersize=blocksize;
		len=blocksize;
	}

	/* sigh, make it oversized just to have some */  
	buffer = malloc(buffersize + 8); 

	for ( ; ; )
	{
		if (varsize)
		{
			/* it could have gotten reset by prior short read... */
			len=MAX_READ_SIZE; 
		}
		result=SCSI_readt(MediumChangerFD,buffer,blocksize, &len, DEFAULT_TIMEOUT);

		if (result==READ_FILEMARK || result==READ_EOD || result==READ_EOP)
		{
			/* okay, normal end of file? */
			if (len > 0)
			{
				write(outfile,buffer,len);
			}

#ifdef NEED_TO_GO_PAST_FILEMARK
			/* Now, let's try to go past the filemark if that's what we hit: */
			if (result==READ_FILEMARK)
			{
				arg1 = 1;	/* arg for S_fsf. */
				S_fsf();	/* and go forward 1 filemark, we hope! */
			}
#endif
			return 0; /* hit normal end of file. */
		}
		else if (result == READ_SHORT)
		{
			/* short reads are only valid in variable block mode. */
			if (varsize)
			{
				if (len > 0)
				{
					write(outfile,buffer,len);
				}
			}
			else
			{
				fprintf(stderr,"scsitape:Short Read encountered on input. Aborting.\n");
				fflush(stderr);
				exit(1); /* error exit! */
			}
		}
		else if (result == READ_OK)
		{
			write(outfile,buffer,len);
		}
		else
		{
			fprintf(stderr,"scsitape:Read Error\n");
			fflush(stderr);
			exit(1);
		}

		/* Now see if we have numbytes or numblocks: if so, we may wish to
		 * exit this loop.
		 */
		if (arg[1])
		{
			if (varsize)
			{
				/****BUG****/ 
				return 0; /* we're only reading one block in var size mode! */
			}
			else if (numblocks)
			{
				numblocks--;
			}
			else
			{
				return 0; /* we're done. */
			}
		}
	}
}


/* See parse_args for the scoop. parse_args does all. */
int main(int argc, char **argv)
{
	argv0 = argv[0];
	parse_args(argc, argv);

	if (device)
		SCSI_CloseDevice(device,MediumChangerFD);

	exit(0);
}
