#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef int DEVICE_TYPE;

#ifdef __osf__
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <io/common/iotypes.h>
#else /* must be ultrix */
#include <sys/devio.h>
#endif
#include <io/cam/cam.h>
#include <io/cam/uagt.h> 
#include <io/cam/dec_cam.h>
#include <io/cam/scsi_all.h>  
#define DEV_CAM		"/dev/cam"	/* CAM device path */


#define DIGITAL_UNIX
