/* Copyright 2007-2008, Robert Nelson <robertn@the-nelsons.org>
 *   Released under terms of the GNU General Public License as
 * required by the license on 'mtxl.c'.
 * $Date: 2007-01-28 19:23:33 -0800 (Sun, 28 Jan 2007) $
 * $Revision: 125 $
 */

/* This is a generic SCSI device control program. It operates by
 * directly sending commands to the device.
 */

/* 
 *	Commands:
 *		load -- Load medium
 *		unload -- Unload medium
 *		start -- Start device
 *		stop -- Stop device
 *		lock -- Lock medium
 *		unlock -- Unlock medium
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

#ifdef _MSC_VER
#include <io.h>
#endif

char *argv0;

/* the device handle we're operating upon. */
static char *device;  /* the device name. */
static DEVICE_TYPE DeviceFD = (DEVICE_TYPE) -1;

static int S_load(void);
static int S_unload(void);
static int S_start(void);
static int S_stop(void);
static int S_lock(void);
static int S_unlock(void);

struct command_table_struct
{
	char *name;
	int (*command)(void);
}
	command_table[] =
{
	{ "load", S_load },
	{ "unload", S_unload },
	{ "start", S_start },
	{ "stop", S_stop },
	{ "lock", S_lock },
	{ "unlock", S_unlock },
	{ NULL, NULL } /* terminate list */
};

void Usage(void)
{
	FatalError("Usage: scsieject -f <generic-device> <command> where <command> is:\n load | unload | start | stop | lock | unlock\n");
}

/* open_device() -- set the 'DeviceFD' variable.... */
void open_device(void)
{
	if (DeviceFD != -1)
	{
		SCSI_CloseDevice("Unknown", DeviceFD);
	}

	DeviceFD = SCSI_OpenDevice(device);
}

/* we see if we've got a file open. If not, we open one :-(. Then
 * we execute the actual command. Or not :-(. 
 */ 
int execute_command(struct command_table_struct *command)
{
	/*
	 * If the device is not already open, then open it from the 
	 * environment.
	 */
	if (DeviceFD == -1)
	{
		/* try to get it from STAPE or TAPE environment variable... */
		if ((device = getenv("STAPE")) == NULL &&
			(device = getenv("TAPE")) == NULL)
		{
			Usage();	/* Doesn't return */
		}

		open_device();
	}

	/* okay, now to execute the command... */
	return command->command();
}


/* parse_args():
 * Basically, we are parsing argv/argc. We can have multiple commands
 * on a line, such as "load start" to load a tape and start the device.
 * We execute these commands one at a time as we come to them. If we don't 
 * have a -f at the start and the default device isn't defined in a TAPE or 
 * STAPE environment variable, we exit.
 */ 

int parse_args(int argc, char **argv)
{
	int index, retval;
	struct command_table_struct *command;

	argv0 = argv[0];

	for (index = 1; index < argc; index++)
	{
		if (strcmp(argv[index], "-f") == 0)
		{
			index++;
			if (index >= argc)
			{
				Usage();	/* Doesn't return */
			}
			device = argv[index];
			open_device();
		}
		else
		{
			for (command = &command_table[0]; command->name != NULL; command++)
			{
				if (strcmp(command->name, argv[index]) == 0)
				{
					break;
				}
			}

			if (command->name == NULL)
			{
				Usage();	/* Doesn't return */
			}

			retval = execute_command(command);

			if (retval < 0)
			{
				/* Command failed, we probably shouldn't continue */
				return retval;
			}
		}
	}

	return 0;
}

int S_load(void)
{
	int result = LoadUnload(DeviceFD, 1);

	if (result < 0)
	{
		fputs("scsieject: load failed\n", stderr);
		fflush(stderr);
	}

	return result;
}

int S_unload(void)
{
	int result = LoadUnload(DeviceFD, 0);

	if (result < 0)
	{
		fputs("scsieject: unload failed\n", stderr);
		fflush(stderr);
	}

	return result;
}

int S_start(void)
{
	int result = StartStop(DeviceFD, 1);

	if (result < 0)
	{
		fputs("scsieject: start failed\n", stderr);
		fflush(stderr);
	}

	return result;
}

int S_stop(void)
{
	int result = StartStop(DeviceFD, 0);

	if (result < 0)
	{
		fputs("scsieject: stop failed\n", stderr);
		fflush(stderr);
	}

	return result;
}

int S_lock(void)
{
	int result = LockUnlock(DeviceFD, 1);

	if (result < 0)
	{
		fputs("scsieject: lock failed\n", stderr);
		fflush(stderr);
	}

	return result;
}

int S_unlock(void)
{
	int result = LockUnlock(DeviceFD, 0);

	if (result < 0)
	{
		fputs("scsieject: unlock failed\n", stderr);
		fflush(stderr);
	}

	return result;
}

/* See parse_args for the scoop. parse_args does all. */
int main(int argc, char **argv)
{
	parse_args(argc, argv);

	if (device)
	{
		SCSI_CloseDevice(device, DeviceFD);
	}

	exit(0);
}
