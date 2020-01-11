#!/bin/bash
#
# Install UFO and TFTD data files (and patches) to the default location.
#
# Note:
#  Do not use: 'sh ./installDataFiles.sh'.
#  It will probably kill your dog, most likely it will complain and fail.
#
#  Script assumes a default steam installation (see $STEAM_DATA_PATH).

# Global variables
# ================
DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"

# OpenXcom
# --------
OXC_DATA_ROOT="${DATA_HOME}/openxcom"
OXC_UFO_PATCH_URL="https://openxcom.org/download/extras/universal-patch-ufo.zip"
OXC_TFTD_PATCH_URL="https://openxcom.org/download/extras/universal-patch-tftd.zip"
OXC_GAME_DATA_PATH=""   # Set by 'steam_get_game_data_paths()'.
GAME_SUBDIRS=""         # Set by 'steam_get_game_data_paths()'.
GAME_PATCH_URL=""       # Set by 'steam_get_game_data_paths()'.

# Steam
# -----
STEAM_DATA_PATH="${DATA_HOME}/Steam"
STEAM_LIBRARY_PATHS_DEFAULT="${STEAM_DATA_PATH}/steamapps"
STEAM_ID_UFO="7760"
STEAM_ID_TFTD="7650"
STEAM_ID_APOC="7660"
STEAM_LIBRARY_PATHS=""  # Set by 'steam_parse_libraryfolders_file()'.
STEAM_GAME_DATA_PATH="" # Set by 'steam_get_game_data_paths()'.
GAME_MANIFEST=""        # Set by 'steam_get_game_manifest()'.
GAME_INSTALL_STATE=""   # Set by 'steam_get_game_install_state()'.


# Functions
# =========

# Display help text.
show_help ()
{
	printf '%s\n' ""\
	"Install openxcom required data files."\
	"Will apply the universal data patches (when possible)."\
	""\
	"Usage: installDataFiles"\
	"or: installDataFiles SOURCE"\
	"or: installDataFiles SOURCE DESTINATION"\
	""\
	"SOURCE can be:"\
	"* steam"\
	"  Use 'steam' installations of UFO and TFTD present on the system."\
	"  The *only* option that can install both games at once."\
	"* /path/to/original/data"\
	"  Use the known data folders below this path."\
	"  Based on folder contents the game (UFO/TFTD) is guessed."\
	""\
	"DESTINATION will override the default openxcom data path."\
	"* Data files are put in the appropriate game specific sub-folder."\
	"* When using this argument SOURCE is required!"\
	""\
	"When no arguments are given this script defaults to:"\
	"  installDataFiles steam ${OXC_DATA_ROOT}"
}

# Parse script arguments
#
# $@ All script parameters.
parse_script_arguments ()
{
	local specified_path
	while :; do
		case ${1} in
			-h|-\?|--help)
				show_help
				exit 0
				;;
			-?*)
				printf '%s\n' "ERROR: Unknown option: ${1}"
				printf '%s\n' "Try '${0} --help' for more information."
				exit 1
				;;
			steam|STEAM)
				printf '%s\n' "Will query steam libraries for both UFO and TFTD."
				SOURCE_OVERRIDE="steam"
				;;
			/*) # Absolute path (bash expands '~' before passing it on).
				specified_path=${1}
				;;
			./*|../*) # Relative path w.r.t. working folder. Target must exist.
				pushd "${1}" > /dev/null || exit 1 # Safe for paths with spaces.
				specified_path=$(pwd -P)
				popd > /dev/null
				;;
			*) # End of (known) arguments reached.
				break
		esac

		if [ -v specified_path ]; then
			if [ -v SOURCE_OVERRIDE ]; then
				DESTINATION_OVERRIDE="${specified_path}"
				break # Only 1 destination makes sense.
			else
				printf '%s\n' "Requested source override: ${specified_path}"
				SOURCE_OVERRIDE="${specified_path}"
			fi
		fi

		unset specified_path
		shift
	done
}

# Set installation root folder.
#
# Sets the global variable $DESTINATION
set_destination ()
{
	if [ -v DESTINATION_OVERRIDE ]; then
		DESTINATION=${DESTINATION_OVERRIDE}
	else
		DESTINATION=${OXC_DATA_ROOT}
	fi

	printf '%s\n' "Will install data files under:"
	printf '%s\n' " $DESTINATION"
}

# Get the (user defined) library folders.
#
# Sets the global array: $STEAM_LIBRARY_PATHS
steam_parse_libraryfolders_file ()
{
	# Reset array to default value (in case we ran before).
	unset STEAM_LIBRARY_PATHS
	STEAM_LIBRARY_PATHS=${STEAM_LIBRARY_PATHS_DEFAULT}

	# User defined folders are stored in this file.
	local vdf_file="${STEAM_LIBRARY_PATHS}/libraryfolders.vdf"

	if [ -f ${vdf_file} ]; then
		: # Continue
	else
		#printf "Line $BASH_LINENO: Could not find '${vdf_file}'.\n"
		return 1
	fi

	# Only interested in the part with the known locations, for example:
	# ``	"1"		"/opt/valve/SteamLibrary"``
	#
	# Supports up to 9 extra locations (to keep the regexp simple).
	# Steam stores all relevant data under the sub-folder 'steamapps'.
	STEAM_LIBRARY_PATHS+=($(cat ${vdf_file} | awk -F '\t' '{if($2 ~ /^"[1-9]"$/) {gsub(/"/,"",$4) ; print $4 "/steamapps"}}'))

	#printf '\n%s\n' "Found the following ${#STEAM_LIBRARY_PATHS[@]} steam library path(s):"
	#for index in "${STEAM_LIBRARY_PATHS[@]}"; do
		#printf ' %s\n' "$index"
	#done
	#printf '\n'
	return 0
}

# Get the steam manifest file for selected game.
#
# $1 Game steam ID.
#
# Sets the global variable: $GAME_MANIFEST
steam_get_game_manifest ()
{
	unset GAME_MANIFEST

	# Assume only one instance of a game can be installed.
	for ind in "${STEAM_LIBRARY_PATHS[@]}"; do
	{
		if [ -f "${ind}/appmanifest_${1}.acf" ]; then
		{
			GAME_MANIFEST="${ind}/appmanifest_${1}.acf"

			#printf '%s\n' "Game manifest: $GAME_MANIFEST"
			return 0
		}
		fi
	}
	done

	#printf '\n%s\n' "${FUNCNAME[0]}(): Manifest not found."
	return 1
}

# Return the installation state of the game
#
# $1 Game steam ID.
#
# Updates the global variable: $GAME_INSTALL_STATE
#
# Note:
#   If it reports "4" the game is fully downloaded.
#   Anything else will be considered an error.
steam_get_game_install_state ()
{
	unset GAME_INSTALL_STATE

	if [ -v GAME_MANIFEST ]; then
	{
		GAME_INSTALL_STATE=$(cat "${GAME_MANIFEST}" | awk -F '\t' '{if($2 ~ /^"StateFlags"$/) {gsub(/"/,"",$4) ; print $4}}')

		#printf '%s\n' "Install state: $GAME_INSTALL_STATE"
		return 0
	}
	else
		return 1
	fi
}

# Return the location of the data files.
#
# $1 Game steam ID.
#
# Updates the global variable: $STEAM_GAME_DATA_PATH
steam_get_game_data_path ()
{
	unset STEAM_GAME_DATA_PATH

	steam_get_game_manifest ${1}
	if [ $? -ne 0 ]; then
	{
		case ${1} in
			${STEAM_ID_UFO})
				printf '%s\n' "Could not find steam installation of UFO"
				;;
			${STEAM_ID_TFTD})
				printf '%s\n' "Could not find steam installation of TFTD"
				;;
			*)
				printf '%s\n' "Could not find steam installation of unsupported game"
		esac
		return 1
	}
	fi

	steam_get_game_install_state ${1}
	if [ -v GAME_INSTALL_STATE ] && [ $GAME_INSTALL_STATE -eq 4 ]; then
	{
		local installPath
		installPath=$(cat "${GAME_MANIFEST}" | awk -F '\t' '{if($2 ~ /^"installdir"$/) {gsub(/"/,"",$4) ; print $4}}')

		# Installed game is stored under the library's subdirectory "common".
		STEAM_GAME_DATA_PATH="$(dirname ${GAME_MANIFEST})/common/${installPath}"
		case ${1} in
			${STEAM_ID_UFO})
				STEAM_GAME_DATA_PATH="${STEAM_GAME_DATA_PATH}/XCOM"
				;;
			${STEAM_ID_TFTD})
				STEAM_GAME_DATA_PATH="${STEAM_GAME_DATA_PATH}/TFD"
				;;
			*)
				printf '\n%s\n' "Unknown game ID ${1}"
				return 1
		esac
		return 0
	}
	else
	{
		case ${1} in
			${STEAM_ID_UFO})
				printf '%s\n' "UFO has not been fully downloaded (or validated)"
				;;
			${STEAM_ID_TFTD})
				printf '%s\n' "TFTD has not been fully downloaded (or validated)"
				;;
			*)
				printf '\n%s\n' "Unknown game ID ${1} has not been fully downloaded (or validated)"
		esac
		return 1
	}
	fi
}

# Let user decide yes/no on a question (default to yes).
#
# Return 0 if user presses [Enter] or anything beginning with 'y' or 'Y'.
# Return 1 if user chooses otherwise.
ask_for_user_confirmation ()
{
	local userinput

	read -p " [Yes]/no: " userinput
	userinput="${userinput:-yes}" # [Enter] = yes

	case ${userinput:0:1} in
		y|Y)
			return 0
			;;
		*)
			return 1
			;;
	esac
}

# Copy data files from steam to OXC installation.
#
# Depends on global variables
#
# Will create a backup of existing 'UFO' and 'TFTD' folders.
copy_data_files ()
{
	if [ -d ${OXC_GAME_DATA_PATH} ]; then
		printf '\n%s\n' "Looks like there is already a data folder ... will create a backup."
		mv ${OXC_GAME_DATA_PATH} "${OXC_GAME_DATA_PATH}.backup.$(date +%F_%T)"
	fi

	printf '\n%s\n' "Copying data files ..."
	mkdir ${OXC_GAME_DATA_PATH}
	for ind in "${GAME_SUBDIRS[@]}"; do
		cp -r "${STEAM_GAME_DATA_PATH}/${ind}" ${OXC_GAME_DATA_PATH}
	done
}

# Install OXC data patches.
#
# Depends on global variables
#
# Will download (to '/tmp/') and apply the data patches.
apply_OXC_patches ()
{
	printf '%s\n' "Downloading patch ..."
	curl -q -L ${GAME_PATCH_URL} > "/tmp/$(basename ${GAME_PATCH_URL})"

	if [ $? -eq 0 ]; then
		printf '\n%s\n' "Applying the patch"
		unzip -o "/tmp/$(basename ${GAME_PATCH_URL})" -d "${OXC_GAME_DATA_PATH}"
	else
		printf '\n%s\n' "Something went wrong downloading the patch."
		return 1
	fi
}

# Install the data files from the steam version
#
# $1 Game steam ID.
#
# This copies the steam data files to the expected OXC folder.
# It will also apply the data patches from the openxcom.com site.
install_data_files ()
{
	case ${1} in
		${STEAM_ID_UFO})
			printf "\nUFO:\n===="
			printf '\n%s' "Install datafiles for 'X-COM: UFO Defense'?"
			ask_for_user_confirmation
			if [ $? -eq 1 ]; then
				return 1
			fi
			;;
		${STEAM_ID_TFTD})
			printf "\nTFTD:\n===="
			printf '\n%s' "Install datafiles for 'X-COM: Terror from the Deep'?"
			ask_for_user_confirmation
			if [ $? -eq 1 ]; then
				return 1
			fi
			;;
		${STEAM_ID_APOC})
			printf "\nAPOC:\n===="
			printf '\n%s\n' "This script needs more love to support 'X-COM: Apocalypse'."
			return 1
			;;
		*)
			printf '\n%s\n' "Unknown game."
			return 1
			;;
	esac

	steam_parse_libraryfolders_file
	steam_get_game_install_state ${1}

	case ${GAME_INSTALL_STATE} in
		"")
			;;
		"4")
			;;
		*)
			printf '\n%s\n' "Got an unexpected state ... install manually and try this script again."
			return 1
			;;
	esac

	# Only continue if call inside previous case block was successful.
	if [ $? -eq 0 ]; then
		steam_get_game_data_path ${1}
		#copy_data_files
	else
		printf '\n%s\n' "Something went wrong installing/validating the steam version."
		return 1
	fi

	#apply_OXC_patches
}

# Main
# ====
parse_script_arguments "$@"
set_sources
set_destination

echo "====="
echo "SOURCE = $SOURCE_OVERRIDE"
#install_data_files "$STEAM_ID_UFO"
#install_data_files "$STEAM_ID_TFTD"
#install_data_files "$STEAM_ID_APOC"

# Do not terminate script without confirmation.
printf '\n'
read -n 1 -s -p "Finished ... press any key to terminate script"
printf '\n'

# Extra Info
# ==========
# Steam library
# -------------
# Default:
#  ~/.local/share/Steam/steamapps/
# Additional library folders defined in:
#  ~/.local/share/Steam/steamapps/libraryfolders.vdf
# Games are stored under a library subfolder ``common``.