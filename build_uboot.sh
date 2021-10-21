#!/bin/bash
set -x
exec_name=$0

set -o errtrace
trap 'echo Fatal error: script ${exec_name} aborting at line $LINENO, command \"$BASH_COMMAND\" returned $?; exit 1' ERR

cpu_num=$(grep -c processor /proc/cpuinfo)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo DIR:$DIR

function usage(){
  echo "Usage: ${exec_name} <board> [workspace path]"
  echo "supported boards: estelle-p1, estelle-p2"
  echo "                  newman-p2, newman-p2_1, newman-b1, newman-b3, newman-b4"
  echo "                  legion-p1"
  echo "                  puddy-p0"
  echo "                  elaine-p1, elaine-p2, elaine-b1, elaine-b3, elaine-b4"
}

readonly fct_options="--ddrenable --bftenable"
readonly fsi_folder="bootloader"
readonly fct_folder="factory/bootloader"
readonly fsi="$fsi_folder:"
readonly fct="$fct_folder:$fct_options"

function building_uboot(){
  soc_family_name=$1
  local_name=$2
  rev=$3
  board_name=$4
  cfg_suffix=$6

  config=${local_name}_${rev}${cfg_suffix}

  for cfg in "$fsi" "$fct"; do
    local folder="${cfg%:*}"
    local options="${cfg#*:}"
    local print_flag="${folder%/*}"

    echo "building u-boot for ${board} in ${folder}"

    if [ ${print_flag} == "factory" ]; then
	    print_flag=debug
    else
	    print_flag=$5
    fi
    echo "print_flag: $print_flag"

    ./mk ${config} --board_name $board_name --bl2 fip/${soc_family_name}/bl2.bin --bl30 fip/${soc_family_name}/bl30.bin --bl31 fip/${soc_family_name}/bl31.img --bl32 fip/${soc_family_name}/bl32.img $print_flag $options

    # make T=1 to use latest git commit time as build timestamp.

    echo "mk done\n"

    local product=`echo ${board} | cut -d "-" -f1`
    local bootloader_path=${workspace_path}/vendor/amlogic/${product}/prebuilt/${folder}

    if [ ! -z $workspace_path ]; then
      mkdir -p ${bootloader_path}
      if [ "$product" == "estelle" ] || [ "$product" == "newman" ] || [ "$product" == "legion" ] || [ "$product" == "puddy" ] || [ "$product" == "elaine" ] ; then
        # Copy bl2 and bl3x images for bootloader signing under eureka source.
        cp fip/build/bl2_new.bin ${bootloader_path}/bl2_new.bin.${board}
        cp fip/build/bl30_new.bin ${bootloader_path}/bl30_new.bin.${board}
        cp fip/build/bl31.img ${bootloader_path}/bl31.img.${board}
        cp fip/build/bl32.img ${bootloader_path}/bl32.img.${board}
        cp fip/build/bl33.bin ${bootloader_path}/bl33.bin.${board}

        # Copy ddr bin for bootloader signing under eureka source.
        # TODO(ljchen): Remove hard code of ddr files under vendor/amlogic.
        cp fip/${soc_family_name}/ddr4_1d.fw ${bootloader_path}/
        cp fip/${soc_family_name}/ddr4_2d.fw ${bootloader_path}/
        cp fip/${soc_family_name}/ddr3_1d.fw ${bootloader_path}/
        cp fip/${soc_family_name}/piei.fw ${bootloader_path}/
        cp fip/${soc_family_name}/lpddr4_1d.fw ${bootloader_path}/
        cp fip/${soc_family_name}/lpddr4_2d.fw ${bootloader_path}/
        cp fip/${soc_family_name}/diag_lpddr4.fw ${bootloader_path}/
        cp fip/${soc_family_name}/aml_ddr.fw ${bootloader_path}/

      else
        cp fip/${soc_family_name}/u-boot.bin.usb.bl2 ${bootloader_path}/u-boot.bin.usb.bl2
        cp fip/${soc_family_name}/u-boot.bin.usb.tpl ${bootloader_path}/u-boot.bin.usb.tpl
        cp fip/${soc_family_name}/u-boot.bin ${bootloader_path}/u-boot.bin
      fi
    fi
  done
}

if (( $# < 1 ))
then
  usage
  exit 2
fi

pushd $DIR

readonly board=$1
readonly workspace_path=$2
readonly cross_compile=$DIR/../amlogic/linaro/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/aarch64-none-elf-
readonly cross_compile_t32=$DIR/../amlogic/linaro/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-
readonly vendor_amlogic=$DIR/../vendor/amlogic

dbg_flag="debug"
zircon_cfg=""

if [ "$3" = "release" -o "$4" = "release" ]; then
	dbg_flag="release"
elif [ "$4" = "zircon" -o "$5" = "zircon" ]; then
	zircon_cfg="_zircon"
fi


export ENABLE_UBOOT_UPDATE=1

case $board in
  estelle-p1)
    building_uboot g12a g12a_estelle p1 $board
    ;;
  estelle-p2)
    building_uboot g12a g12a_estelle p2 $board
    ;;
  newman-p2)
    building_uboot g12b g12b_newman px $board $dbg_flag $zircon_cfg
    ;;
  newman-p2_1)
    building_uboot g12b g12b_newman p2_1 $board $dbg_flag $zircon_cfg
    ;;
  newman-b1|newman-b3)
    building_uboot g12b g12b_newman bx $board $dbg_flag $zircon_cfg
    ;;
  newman-b4)
    export ENABLE_UBOOT_UPDATE=0
    building_uboot g12b g12b_newman bx $board $dbg_flag $zircon_cfg
    ;;
  legion-p1)
    building_uboot g12a gl2a_legion p1 $board
    ;;
  puddy-p0)
    building_uboot g12b g12b_puddy px $board $dbg_flag $zircon_cfg
    ;;
  elaine-p0)
    building_uboot sm1 sm1_elaine p0 $board $dbg_flag
    ;;
  elaine-p1)
    building_uboot sm1 sm1_elaine p1 $board $dbg_flag
    ;;
  elaine-p2)
    building_uboot sm1 sm1_elaine p2 $board $dbg_flag
    ;;
  elaine-b1)
    building_uboot sm1 sm1_elaine b1 $board $dbg_flag
    ;;
  elaine-b3)
    building_uboot sm1 sm1_elaine bx $board $dbg_flag
    ;;
  elaine-b4)
    export ENABLE_UBOOT_UPDATE=0
    building_uboot sm1 sm1_elaine bx $board $dbg_flag
    ;;
  *)
    echo "unknown board: $board"
    exit 1
esac
popd
