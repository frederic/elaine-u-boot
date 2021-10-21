
declare UBOOT_SRC_FOLDER="./"
declare MAIN_FOLDER=""
export UBOOT_SRC_FOLDER MAIN_FOLDER

# build environment
BUILD_FOLDER="./build/"
FIP_FOLDER="./fip/"
MANIFEST="../.repo/manifest.xml"
FIP_BUILD_FOLDER="./fip/build/"

# include uboot pre-build defines
SOURCE_FILE=("./.config")
CONFIG_FILE=("./build/include/autoconf.mk")

# variables
declare BOARD_DIR=""
declare CUR_SOC=""

# current branch/path/rev/name/remote in xml
declare -a GIT_INFO=("branch", "path", "rev", "name", "remote")

function export_variables() {
	export BUILD_FOLDER
	export FIP_FOLDER
	export MANIFEST
	export CUR_SOC
	export BOARD_DIR
}
