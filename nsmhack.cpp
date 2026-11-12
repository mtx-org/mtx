/* Copyright 2001 DISC Inc. 
 * Released under terms of the GNU General Public License as required
 * by the license on the file "mtxl.c". See file "LICENSE" for details.
 */

#define DEBUG_NSM 1

/* This is a hack to make the NSM modular series jukeboxes stick out
 * their tongue, then retract tongue, so we can import media. They
 * automatically stick out their tongue when exporting media, but
 * importing media is not working, you try to do a MOVE_MEDIUM and
 * it says "What medium?" before even sticking out its tongue. 
 * My manager has turned in a change request to NSM engineering to direct 
 * their firmware guys to add EEPOS support to the NSM modular jukeboxes so 
 * that we have tongue firmware that's compatible with Exabyte, Sony, Breece
 *  Hill, etc., but until that new firmware is here, this hack will work.
 */

/* Note: Perhaps "hack" is an overstatement, since this will also
 * eventually add pack management and other things of that nature
 * that are extremely loader dependent.
 */

/* Commands:
   -f <devicenode>
   tongue_out <sourceslot>
   tongue_in
   tongue_button_wait
   tongue_button_enable
   tongue_button_disable
*/
   

#include "mtxl.h"  /* get the SCSI routines out of the main file */

/****************************************************************/
/* Variables:  */
/****************************************************************/   

/* the device handle we're operating upon, sigh. */
static char *device;  /* the text of the device thingy. */
static DEVICE_TYPE MediumChangerFD = (DEVICE_TYPE) -1;
char *argv0;
int arg[4]; /* arguments for the command. */
#define arg1 (arg[0])  /* for backward compatibility, sigh */
static SCSI_Flags_T SCSI_Flags = { 0, 0, 0,0 };

static ElementStatus_T *ElementStatus = NULL;

/* Okay, now let's do the main routine: */

void Usage(void) {
  FatalError("Usage: nsmhack -f <generic-device> <command> where <command> is:\n [tongue_out] | [tongue_in] | [tongue_button_wait] | [tongue_button_enable]\n    | tongue_button_disable. \n");
}

static int S_tongue_out(void);
static int S_tongue_in(void);
static int S_slotinfo(void);
static int S_jukeinfo(void);

struct command_table_struct {
  int num_args;
  char *name;
  int (*command)(void);
} command_table[] = {
  { 1, "tongue_out", S_tongue_out },
  { 0, "tongue_in", S_tongue_in },
  { 0, "slotinfo", S_slotinfo },
  { 0, "jukeinfo", S_jukeinfo },
  { 0, NULL, NULL }
};


/* open_device() -- set the 'fh' variable.... */
void open_device(void) {

  if (MediumChangerFD != -1) {
    SCSI_CloseDevice("Unknown",MediumChangerFD);  /* close it, sigh...  new device now! */
  }

  MediumChangerFD = SCSI_OpenDevice(device);

}

static int get_arg(char *arg) {
  int retval=-1;

  if (*arg < '0' || *arg > '9') {
    return -1;  /* sorry! */
  }

  retval=atoi(arg);
  return retval;
}

/* we see if we've got a file open. If not, we open one :-(. Then
 * we execute the actual command. Or not :-(. 
 */ 
int execute_command(struct command_table_struct *command) {

  /* if the device is not already open, then open it from the 
   * environment.
   */
  if (MediumChangerFD == -1) {
    /* try to get it from STAPE or TAPE environment variable... */
    device=getenv("STAPE");
    if (device==NULL) {
      device=getenv("TAPE");
      if (device==NULL) {
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

int parse_args(int argc,char **argv) {
  int i,cmd_tbl_idx,retval,arg_idx;
  struct command_table_struct *command;

  i=1;
  arg_idx=0;
  while (i<argc) {
    if (strcmp(argv[i],"-f") == 0) {
      i++;
      if (i>=argc) {
	Usage();
      }
      device=argv[i++];
      open_device(); /* open the device and do a status scan on it... */
    } else {
      cmd_tbl_idx=0;
      command=&command_table[0]; /* default to the first command... */
      command=&command_table[cmd_tbl_idx];
      while (command->name) {
	if (!strcmp(command->name,argv[i])) {
	  /* we have a match... */
	  break;
	}
	/* otherwise we don't have a match... */
	cmd_tbl_idx++;
	command=&command_table[cmd_tbl_idx];
      }
      /* if it's not a command, exit.... */
      if (!command->name) {
	Usage();
      }
      i++;  /* go to the next argument, if possible... */
      /* see if we need to gather arguments, though! */
      arg1=-1; /* default it to something */
      for (arg_idx=0;arg_idx < command->num_args ; arg_idx++) {
	if (i < argc) {
	  arg[arg_idx]=get_arg(argv[i]);
	  if (arg[arg_idx] !=  -1) {
	    i++; /* increment i over the next cmd. */
	  }
	} else {
	  arg[arg_idx]=0; /* default to 0 setmarks or whatever */
	} 
      }
      retval=execute_command(command);  /* execute_command handles 'stuff' */
      exit(retval);
    }
  }
  return 0; /* should never get here */
}

static void init_param(NSM_Param_T *param, char *command, int paramlen, int resultlen) {
  int i;

  /* zero it out first: */
  memset((char *)param,0,sizeof(NSM_Param_T));

  resultlen=resultlen+sizeof(NSM_Result_T)-0xffff;

  
  param->page_code=0x80;
  param->reserved=0;
  param->page_len_msb=((paramlen+8)>>8) & 0xff;
  param->page_len_lsb=(paramlen+8) & 0xff;
  param->allocation_msb=((resultlen + 10) >> 8) & 0xff;
  param->allocation_lsb= (resultlen+10) & 0xff;
  param->reserved2[0]=0;
  param->reserved2[1]=0;

  for (i=0;i<4;i++) {
    param->command_code[i]=command[i];
  }

}

static NSM_Result_T *SendRecHack(NSM_Param_T *param,int param_len, 
				 int read_len) {
  NSM_Result_T *result;
  /* send the command: */
  if (SendNSMHack(MediumChangerFD,param,param_len,0)) {
    PrintRequestSense(&scsi_error_sense);                   
    FatalError("SendNSMHack failed.\n");    
  }

  /* Now read the result: */
  result=RecNSMHack(MediumChangerFD,read_len,0);
  if (!result) {
    PrintRequestSense(&scsi_error_sense);                   
    FatalError("RecNSMHack failed.\n");    
  }  

  return result;
}


/* Print some info about the NSM jukebox. */
static int S_jukeinfo(void) {
  NSM_Result_T *result;
  NSM_Param_T param;

  if (!device)
    Usage();

  /* okay, we have a device: Let's get vendor ID: */
  init_param(&param,"1010",0,8);
  result=SendRecHack(&param,0,8);
  /* Okay, we got our result, print out the vendor ID: */
  result->return_data[8]=0;
  printf("Vendor ID: %s\n",result->return_data);
  free(result);
  
  /* Get our product ID: */
  init_param(&param,"1011",0,16);
  result=SendRecHack(&param,0,16);
  result->return_data[16]=0;
  printf("Product ID: %s\n",result->return_data);
  free(result);

  init_param(&param,"1012",0,4);
  result=SendRecHack(&param,0,4);
  result->return_data[4]=0;
  printf("Product Revision: %s\n",result->return_data);
  free(result);

  init_param(&param,"1013",0,8);
  result=SendRecHack(&param,0,8);
  result->return_data[8]=0;
  printf("Production Date: %s\n",result->return_data);
  free(result);

  init_param(&param,"1014",0,8);
  result=SendRecHack(&param,0,8);
  result->return_data[8]=0;
  printf("Part Number: %s\n",result->return_data);
  free(result);

  init_param(&param,"1015",0,12);
  result=SendRecHack(&param,0,12);
  result->return_data[12]=0;
  printf("Serial Number: %s\n",result->return_data);
  free(result);

  init_param(&param,"1016",0,4);
  result=SendRecHack(&param,0,4);
  result->return_data[4]=0;
  printf("Firmware Release: %s\n",result->return_data);
  free(result);

  init_param(&param,"1017",0,8);
  result=SendRecHack(&param,0,8);
  result->return_data[8]=0;
  printf("Firmware Date: %s\n",result->return_data);
  free(result);

  return 0;
}

static int S_slotinfo(void) {
  NSM_Result_T *result;
  NSM_Param_T param;

  if (!device)
    Usage();

  /* Okay, let's see what I can get from slotinfo: */
  init_param(&param,"1020",0,6);
  result=SendRecHack(&param,0,6);
  result->return_data[6]=0;
  printf("Layout: %s\n",result->return_data);
  free(result);
  
  return 0;
}

static int S_tongue_in(void) {
  return 0;
}

/* okay, stick our tongue out. We need a slot ID to grab a caddy from. */
static int S_tongue_out(void) {
  int slotnum=arg1;
  Inquiry_T *inquiry_info;  /* needed by MoveMedium etc... */
  RequestSense_T RequestSense;

  /* see if we have element status: */
  if (ElementStatus==NULL) {
    inquiry_info=RequestInquiry(MediumChangerFD,&RequestSense);
    if (!inquiry_info) {
      PrintRequestSense(&RequestSense);                   
      FatalError("INQUIRY Command Failed\n"); 
    }
    ElementStatus = ReadElementStatus(MediumChangerFD,&RequestSense,inquiry_info,&SCSI_Flags);
    if (!ElementStatus) {
      PrintRequestSense(&RequestSense);                   
      FatalError("READ ELEMENT STATUS Command Failed\n"); 
    }
  }
  
  /* Okay, we have element status, so now let's assume that */
  return 0;
}

/* See parse_args for the scoop. parse_args does all. */
int main(int argc, char **argv) {
  argv0=argv[0];
  parse_args(argc,argv);

  if (device) 
    SCSI_CloseDevice(device,MediumChangerFD);

  exit(0);
}
