#!/bin/bash

function build_bl2() {
	echo -n "Build bl2...Please wait..."
	local target="$1/build/$3/release/bl2.bin"
	# $1: src_folder, $2: bin_folder, $3: soc
	cd $1
	export CROSS_COMPILE=${AARCH64_TOOL_CHAIN}
	/bin/bash mk $3
	if [ $? != 0 ]; then
		cd ${MAIN_FOLDER}
		echo "Error: Build bl2 failed... abort"
		exit -1
	fi
	cd ${MAIN_FOLDER}
	cp ${target} $2 -f
	echo "done"
	return
}