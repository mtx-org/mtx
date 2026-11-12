/* LDRSET - Set the state of the LDR flag in UCB$L_DEVCHAR2 for a
**     SCSI magtape.  REQUIRES CMKRNL privilege.
**
**  Copyright 1999 by TECSys Development, Inc. http://www.tditx.com
**
**  This program is free software; you may redistribute and/or modify it under
**  the terms of the GNU General Public License Version 2 as published by the
**  Free Software Foundation.
**
**  This program is distributed in the hope that it will be useful, but
**  WITHOUT ANY WARRANTY, without even the implied warranty of MERCHANTABILITY
**  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
**  for complete details.
**
** Description:
**   This is a small KERNEL MODE utility program to go set or reset
**   the DEV$M_LDR flag in UCB$L_DEVCHAR2 for a specified device. While
**   a certain amount of checking is performed, and while the author
**   believes that this utility is safe, ONLY YOU can make that
**   determination with respect to your environment. There are NO
**   GUARANTEES WHATSOEVER that use of this program will not CRASH
**   YOUR SYSTEM or worse. TDI disclaims any responsibility for any
**   losses or damages from the use of this program.
**
**   With all that said... this utility can be used [example: system
**   startup] to set the LDR flag for the autoloader tape device.
**   With the loader flag properly set, you can re-enable the LDR
**   check section in MTX and be fairly well certain that the MTX
**   utility will not be used against a drive that is not a SCSI
**   MAGTAPE with a LOADER attached.
**
**
** LDRSET.CLD:
**	define verb LDRSET
**		image		sys$disk:[]ldrset.exe
**		parameter	p1,		label=device,
**						prompt="Device",
**						value(required,type=$FILE)
**		qualifier	SET, nonnegatable
**		qualifier	RESET, nonnegatable
**		disallow	any2 (SET, RESET)
**		disallow	NOT SET AND NOT RESET
*/
#ifdef __DECC
#pragma module LDRSET "V01-00"
#else
#module LDRSET "V01-00"
#endif

#include <ssdef.h>
#include <dcdef.h>
#include <devdef.h>
#include <dvidef.h>
#include <descrip.h>
#include <stdio.h>
#include <string.h>
#include <lib$routines.h>
#include <starlet.h>

#ifndef DESCR_CNT
#define DESCR_CNT 16
/* MUST BE of the form 2^N!, big enough for max concurrent usage */
#endif

static struct dsc$descriptor_s *
descr(
  char *str)
{
static struct dsc$descriptor_s d_descrtbl[DESCR_CNT];  /* MAX usage! */
static unsigned short int descridx=0;
  struct dsc$descriptor_s *d_ret = &d_descrtbl[descridx];

  descridx = (descridx+1)&(DESCR_CNT-1);

  d_ret->dsc$w_length = strlen((const char *)str);
  d_ret->dsc$a_pointer = (char *)str;

  d_ret->dsc$b_class =
  d_ret->dsc$b_dtype = 0;
  return(d_ret);
}

extern unsigned long int finducb();
extern unsigned long int _setldr();
extern unsigned long int _clrldr();

unsigned long int
set_ldrstate(
  int devch,
  int setstate)
{
  unsigned long int ret;
  struct ucbdef *ucb;

  if (~(ret=finducb(devch,&ucb))&1)
    return(ret);

  if (setstate)
    return(_setldr(ucb));
  else
    return(_clrldr(ucb));
}

extern unsigned long int CLI$PRESENT();
extern unsigned long int CLI$GET_VALUE();

static unsigned long int
cld_special(
  char *kwd_name)
{
  lib$establish(lib$sig_to_ret);
  return(CLI$PRESENT(descr(kwd_name)));
}

int
main(){
  unsigned long int ret;
  unsigned long int ismnt = 0;
  unsigned long int dvcls = 0;
  unsigned long int dchr2 = 0;
  struct itmlst_3 {
    unsigned short int ilen;
    unsigned short int code;
    unsigned long int *returnP;
    unsigned long int ignored;
  } dvi_itmlst[] = {
    {4, DVI$_MNT, 0/*&ismnt*/, 0},
    {4, DVI$_DEVCLASS, 0/*&dvcls*/, 0},
    {4, DVI$_DEVCHAR2, 0/*&dchr2*/, 0},
    {0,0,0,0} };
  unsigned long int iosb[2];
  struct dsc$descriptor_s *dp_tmp;
  struct dsc$descriptor_d dy_devn = { 0,DSC$K_DTYPE_T,DSC$K_CLASS_D,0 };
  unsigned long int devch=0;

  dvi_itmlst[0].returnP = &ismnt;
  dvi_itmlst[1].returnP = &dvcls;
  dvi_itmlst[2].returnP = &dchr2;

  if (~(ret=CLI$PRESENT(dp_tmp=descr("DEVICE")))&1) {
    printf("?Error obtaining CLD DEVICE parameter\n");
    return(ret); }
  if (~(ret=CLI$GET_VALUE(dp_tmp,&dy_devn,0))&1) {
    printf("?Error obtaining CLD DEVICE value\n");
    return(ret); }

  if (~(ret=sys$alloc(&dy_devn,0,0,0,0))&1) {
    printf("?Error allocating specified device\n");
    return(ret); }

  if (~(ret=sys$assign(&dy_devn,&devch,0,0))&1) {
    printf("?Error assigning a channel to specified device\n");
    return(ret); }

  if (~(ret=sys$getdviw(0,devch,0,&dvi_itmlst,&iosb,0,0,0))&1) {
    printf("?Error obtaining device information(1)\n");
    return(ret); }
  if (~(ret=iosb[0])&1) {
    printf("?Error obtaining device information(2)\n");
    return(ret); }

  if (dvcls != DC$_TAPE) {
    printf("?Device is not a tape drive\n");
    return(SS$_IVDEVNAM); }

  if (~dchr2 & DEV$M_SCSI) {
    printf("?Device is not a SCSI device\n");
    return(SS$_IVDEVNAM); }

  if (ismnt) {
    printf("?Device is mounted\n");
    return(SS$_DEVMOUNT); }

  if (cld_special("SET")&1)
    return(set_ldrstate(devch,1));

  if (cld_special("RESET")&1)
    return(set_ldrstate(devch,0));

  /* Either SET or RESET above must be present to win */
  printf("?CLD structural error - see source\n");
  return(SS$_BADPARAM);
}
