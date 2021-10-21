#!/bin/bash

LOCAL_PATH="$(readlink -f $(dirname ${BASH_SOURCE[0]})/)"


readonly cross_compile=${LOCAL_PATH}/../../amlogic/linaro/gcc-linaro-7.3.1-2018.05-i686_aarch64-elf/bin/aarch64-elf-
#readonly cross_compile=${LOCAL_PATH}/../../amlogic/linaro/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/aarch64-none-elf-
readonly cross_compile_t32=${LOCAL_PATH}/../../amlogic/linaro/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-

function pre_build_uboot() {
	cd ${UBOOT_SRC_FOLDER}
	echo -n "Compile config: "
	echo "$1"
	make distclean # &> /dev/null
	make CROSS_COMPILE=$cross_compile CROSS_COMPILE_T32=$cross_compile_t32 $1'_config' # &> /dev/null
	if [ $? != 0 ]
	then
		echo "Pre-build failed! exit!"
		cd ${MAIN_FOLDER}
		exit -1
	fi
	cd ${MAIN_FOLDER}
}

function build_uboot() {
	echo "Build uboot...Please Wait..."
	mkdir -p ${FIP_BUILD_FOLDER}
	cd ${UBOOT_SRC_FOLDER}
	# make T=1 to use latest git commit time as build timestamp.
	make BOARD_NAME=$2 T=1 CROSS_COMPILE=$cross_compile CROSS_COMPILE_T32=$cross_compile_t32 BL33_DEBUG=$1 -j # &> /dev/null
	ret=$?
	cd ${MAIN_FOLDER}
	if [ 0 -ne $ret ]; then
		echo "Error: U-boot build failed... abort"
		exit -1
	fi
	find ./ -name u-boot.bin
	echo "find u-boot.bin done"
}

function uboot_config_list() {
	folder_board="${UBOOT_SRC_FOLDER}/board/amlogic"
	echo "      ******Amlogic Configs******"
	for file in ${folder_board}/*; do
		temp_file=`basename $file`
		#echo "$temp_file"
		if [ -d ${folder_board}/${temp_file} ] && [ "$temp_file" != "defconfigs" ] && [ "$temp_file" != "configs" ];then
			echo "          ${temp_file}"
		fi
	done

	customer_folder="${UBOOT_SRC_FOLDER}/customer/board"
	if [ -e ${customer_folder} ]; then
		echo "      ******Customer Configs******"
		for file in ${customer_folder}/*; do
			temp_file=`basename $file`
			if [ -d ${customer_folder}/${temp_file} ] && [ "$temp_file" != "defconfigs" ] && [ "$temp_file" != "configs" ];then
				echo "          ${temp_file}"
			fi
		done
	fi
	echo "      ***************************"
}

function copy_bl33() {
	cp ${UBOOT_SRC_FOLDER}/u-boot.bin ${FIP_BUILD_FOLDER}bl33.bin -f
}
