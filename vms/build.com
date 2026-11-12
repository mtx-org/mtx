$!x='f$ver(0)
$ if f$parse("[.VMS]A.A").eqs.""
$ then 
$   write sys$output "?Error: Use $ @[.VMS]BUILD from the mtx directory"
$   exit 44
$ endif
$ alpha = f$getsyi("hw_model").ge.1024
$ vax = .not.alpha
$ exe = "EXE"
$ obj = "OBJ"
$ sysexe=""
$ migrate=""
$ if alpha then exe="ALPHA_EXE"
$ if alpha then obj="ALPHA_OBJ"
$ if alpha then sysexe="/SYSEXE"
$ if alpha then migrate="/MIGRATION/NOOPT"
$ set verify
$ if "''p1'".eqs."LINK" then goto do_link
$ CC /DECC/DEB/NOOP MTX.C/DEB/NOOP/OBJECT=MTX.'obj'
$ if f$search("MTX.''obj';-1").nes."" then -
     purge/log MTX.'obj'
$ CC /DECC/DEB/NOOP [.VMS]LDRSET.C/DEB/NOOP/OBJECT=[.VMS]LDRSET.'obj'
$ if f$search("[.VMS]LDRSET.''obj';-1").nes."" then -
     purge/log [.VMS]LDRSET.'obj'
$ MACRO'migrate' /DEB [.VMS]LDRUTIL.MAR -
       /OBJECT=[.VMS]LDRUTIL.'obj'
$ if f$search("[.VMS]LDRUTIL.''obj';-1").nes."" then -
     purge/log [.VMS]LDRUTIL.'obj'
$!
$ do_link:
$ link/notrace mtx.'obj'/exe=mtx.'exe'
$ link [.vms]ldrset.'obj',[.vms]ldrutil.'obj' -
     /exe=ldrset.'exe' 'sysexe'
$ exit
