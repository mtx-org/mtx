#ifdef __DECC
#pragma module MTX "V01-00"
#else
#module MTX "V01-00"
#endif

typedef int DEVICE_TYPE;

#include <ssdef.h>
#include <lib$routines.h>
#include <ots$routines.h>
#include <stdio.h>
#include <ctype.h>
#include <iodef.h>
#include <descrip.h>
#include <dcdef.h>
#include <devdef.h>
#include <dvidef.h>
#include <starlet.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <prvdef.h>

#if defined(__DECC)
#pragma member_alignment save
#pragma nomember_alignment
#endif

/*
  Define either of the following symbols as 1 to enable checking of
  the LDR flag for specified devices.  DO NOT set these bits if you
  do not 1) have a DEC-recognized autoloader, or 2) use the LDRSET
  utility to set the LDR flag for the target devices.
*/

#define USING_DEC_DRIVE		0
#define USING_LDRSET		0

static unsigned long VMS_ExitCode = SS$_ABORT;
