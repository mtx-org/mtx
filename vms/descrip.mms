!
! MMS System build for MTX and LDRSET utility
!
!Global build flag macros
!
CDEBUG = /DEB/NOOP
MDEBUG = /DEB

CFLAGS = /DECC$(CDEBUG)
MFLAGS = $(MDEBUG)

.IFDEF __AXP__
.SUFFIXES .ALPHA_OBJ
MFLAGS = /MIGRATE$(MFLAGS)/NOOP
DBG = .ALPHA_DBG
EXE = .ALPHA_EXE
OBJ = .ALPHA_OBJ
OPT = .ALPHA_OPT
SYSEXE=/SYSEXE

.ELSE
DBG = .DBG
EXE = .EXE
OPT = .OPT
OBJ = .OBJ
SYSEXE=

.ENDIF

PURGEOBJ = if f$search("$(MMS$TARGET_NAME)$(OBJ);-1").nes."" then purge/log $(MMS$TARGET_NAME)$(OBJ)

!
!Bend the default build rules for C, MACRO, and MESSAGE
!
.C$(OBJ) :
	$(CC) $(CFLAGS) $(MMS$SOURCE)$(CDEBUG)/OBJECT=$(MMS$TARGET_NAME)$(OBJ)
	$(PURGEOBJ)
.MAR$(OBJ) :
	$(MACRO) $(MFLAGS) $(MMS$SOURCE)$(MDEBUG)/OBJECT=$(MMS$TARGET_NAME)$(OBJ)
	$(PURGEOBJ)
.CLD$(OBJ) :
	SET COMMAND/OBJECT=$(MMS$TARGET_NAME)$(OBJ)  $(MMS$SOURCE)
	$(PURGEOBJ)
.MSG$(OBJ) :
	MESSAGE $(MMS$SOURCE)/OBJECT=$(MMS$TARGET_NAME)$(OBJ)
	$(PURGEOBJ)


DEFAULT		:	ERROR,-
			MTX,-
			LDRSET
	@ !

ERROR		:
	@ if f$parse("[.VMS]A.A").eqs."" then write sys$output "?Error: Use $ MMS/DESCRIP=[.VMS] from the mtx directory"

MTX		:	mtx$(EXE)
	@ !

mtx$(EXE)	:	mtx$(OBJ)
	$ link/notrace mtx$(OBJ)/exe=mtx$(EXE)

mtx$(OBJ)	:	mtx.c,[.vms]scsi.c,[.vms]defs.h

LDRSET		:	ldrset$(EXE),ldrset.cld
	@ !

ldrset.cld	:	[.vms]ldrset.cld
	$ copy [.vms]ldrset.cld []/log

ldrset$(EXE)	:	[.vms]ldrset$(OBJ),[.vms]ldrutil$(OBJ)
	$ link [.vms]ldrset$(OBJ),[.vms]ldrutil$(OBJ)/exe=ldrset$(EXE)$(SYSEXE)

[.vms]ldrset$(OBJ)	:	[.vms]ldrset.c

[.vms]ldrutil$(OBJ)	:	[.vms]ldrutil.mar

