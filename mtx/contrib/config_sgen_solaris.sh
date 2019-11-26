#!/bin/sh
# Copyright 2001 Enhanced Software Technologies Inc.
# All Rights Reserved
#
# This software is licensed under the terms of the Free Software Foundation's
# General Public License, version 2. See http://www.fsf.org for more
# inforation on the General Public License. It is released for public use in
# the hope that others will find it useful. Please contact eric@estinc.com
# if you have problems. Also check out our backup products at
# http://www.estinc.com (grin). 
#
# usage: config_sgen_solaris.sh check|[un]install
#
# This configures sgen under Solaris (we hope! :-). Note that this
# *CAN* do a reboot of the system. Do NOT call this function unless
# you are willing to let it do a reboot of the system! Also note that
# this *must* be run as user 'root', since it does highly grokety things.


mode="$1"
cvs upd
SGEN="/kernel/drv/sgen"
SGEN_CONF="/kernel/drv/sgen.conf"

do_check() {
    if test ! -f $SGEN_CONF; then
	# sgen.conf not installed...
	return 1
    fi

    changer_type_count=`grep "changer" $SGEN_CONF | grep -v "^#" | wc -l`
    target_count=`grep "target=" $SGEN_CONF | grep -v "^#" | wc -l`

    if test $changer_type_count = 0 -o $target_count = 0; then
	# sgen.conf not configured
	return 1
    fi

    # sgen.conf installed, and configured
    return 0
}

do_install() {

    # see if already installed
    do_check
    if test $? = 0; then
	echo "sgen already configured, skipping"
	return 0 # successfully installed (?)
    fi

    if test ! -f $SGEN; then
	echo "sgen driver not installed, aborting"
	return 1
    fi

    echo "configuring sgen driver..."
    
    echo 'device-type-config-list="changer"; # BRU-PRO' >>$SGEN_CONF
    target=0
    while test $target -le 15; do
	echo "name=\"sgen\" class=\"scsi\" target=$target lun=0; # BRU-PRO" >>$SGEN_CONF
	target=`expr $target + 1`
    done

    echo "Attempting to reload driver..."
    rem_drv sgen >/dev/null 2>&1
    add_drv sgen
    if test "$?" != "0"; then
	# failed
	touch /reconfigure
	echo "Driver was successfully configured, but could not be re-loaded."
	echo "The system must be rebooted for the driver changes to take effect."

	ans=""
	while test "$ans" = ""; do
	    printf "Do you want to reboot now (shutdown -g 1 -y -i 6)? [Y/n] "
	    read ans

	    if test "$ans" = "Y"; then
		ans="y"
	    fi

	    if test "$ans" = "N"; then
		ans="n"
	    fi

	    if test "$ans" != "y" -a "$ans" != "n"; then
		echo "Please enter 'y' or 'n'"
		ans=""
	    fi
	done

	if test "$ans" = "y"; then
	    shutdown -g 1 -y -i 6
	    # will be killed by reboot...
	    while true; do
		echo "Waiting for reboot..."
		sleep 300
	    done
	fi

	# not rebooted, exit with error
	return 2
    fi

    # successful
    return 0
}

do_uninstall() {
    do_check
    if test $? = 1; then
	echo "sgen not configured, skipping"
	return 0 # successfully uninstalled (?)
    fi

    printf "removing BRU-PRO configuration from $SGEN_CONF..."
    grep -v "# BRU-PRO" $SGEN_CONF > ${SGEN_CONF}.$$ || return 1
    cat ${SGEN_CONF}.$$ >${SGEN_CONF} || return 1
    rm -f ${SGEN_CONF}.$$ >/dev/null  || return 1
    printf "done\n"

    touch /reconfigure
    printf "Devices will be reconfigured at next reboot.\n"
    return 0
}

uname | grep SunOS >/dev/null 2>&1
if test $? != 0; then
    echo "$0: not on Solaris, ABORT!"
    exit 99
fi

case "$mode" in
    check)
	do_check
	;;
    install)
	do_install
	;;
    uninstall)
	do_uninstall
	;;
    *)
	echo "usage: $0 check|[un]install"
	exit 1
	;;
esac

exit $?