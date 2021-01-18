#!/bin/sh

#  config.sh
#  config for apollo library
#
#  Created by duan .
#

workDir=`pwd`


create_apohome() {
    cd $HOME
    if [ -f .bash_profile ]; then
	echo "export NODEFLOW_WORKING=\"$workDir\"" >> .bash_profile
	source $HOME/.bash_profile
    elif [ -f .profile ]; then
	echo "export NODEFLOW_WORKING=\"$workDir\"" >> .profile
	source $HOME/.profile
    else
	echo "could not create NODEFLOW_WORKING "
	echo "PLEASE set env $NODEFLOW_WORKING "
	cd $workDir
	exit 1
    fi
    cd $workDir
}

# create env

if [ "x$NODEFLOW_WORKING" == "x" ]; then
    create_apohome
fi

# create output dir
PLATFORM_BITS=`getconf LONG_BIT`

[ -d lib ] || mkdir lib
[ -d bin ] || mkdir bin
[ -d log ] || mkdir log

ARCH_MACHINE=`uname -m`
OS_kernel=`uname -s |  tr '[A-Z]' '[a-z]'`

LIBDIR="./lib/"$OS_kernel"_"$ARCH_MACHINE

[ -d  $LIBDIR ] || mkdir $LIBDIR

echo "config SUCCESS "


