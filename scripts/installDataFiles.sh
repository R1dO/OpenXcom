#!/bin/bash
#
# Install UFO and TFTD data files (and patches) to the default location.
#
# Note:
#  Do not use: ``sh ./installDataFiles.sh``.
#  It will probably kill your dog, most likely it will complain and fail.
#
#  Script assumes a default steam installation (see $STEAM_DATA_PATH).

# Guardian
# ========
# This script is not meant to run with superuser credentials.
if [ "$(id -u)" == "0" ]; then
	printf '%s\n' "Steam does not allow running as root, this script wont work ... exiting."
	exit 1
fi


# Global variables
# ================
DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"
# Clutter the CLI with noise?
#VERBOSE=true

# OpenXcom
# --------
OXC_DATA_ROOT="${DATA_HOME}/openxcom"
OXC_UFO_PATCH_URL="https://openxcom.org/download/extras/universal-patch-ufo.zip"
OXC_TFTD_PATCH_URL="https://openxcom.org/download/extras/universal-patch-tftd.zip"
OXC_GAME_DATA_PATH=""   # Set by 'get_game_data_paths()'.
GAME_SUBDIRS=""         # Set by 'get_game_data_paths()'.
GAME_PATCH_URL=""       # Set by 'get_game_data_paths()'.

# Steam
# -----
STEAM_DATA_PATH="${DATA_HOME}/Steam"
STEAM_LIBRARY_PATHS_DEFAULT="${STEAM_DATA_PATH}/steamapps"
STEAM_ID_UFO="7760"
STEAM_ID_TFTD="7650"
STEAM_ID_APOC="7660"    # Used to test download/pause behavior for 'big' games.
STEAM_LIBRARY_PATHS=""  # Set by 'parse_steam_libraryfolders_file()'.
STEAM_GAME_DATA_PATH="" # Set by 'get_game_data_paths()'.
GAME_MANIFEST=""        # Set by 'get_game_manifest()'.
GAME_INSTALL_STATE=""   # Set by 'get_game_install_state()'.


# Functions
# =========

# Display help text.
show_help ()
{
	printf '%s\n' ""\
	"Install the game data files into the default location."\
	"Will apply the universal data patches (when possible)."\
	""\
	"Usage: installDataFiles [SOURCE]"\
	"or: installDataFiles SOURCE [DESTINATION]"\
	""\
	"SOURCE can be:"\
	"* steam"\
	"  Use the 'steam' installations of UFO and TFTD."\
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

# Error message on input
input_error ()
{
	printf '%s\n' "Unknown option."\
	"Try 'installDataFiles --help' for more information."
	exit 1
}

# Get the (user defined) library folders.
#
# Sets the global array: $STEAM_LIBRARY_PATHS
parse_steam_libraryfolders_file ()
{
	# Reset array to default value (in case we ran before).
	unset STEAM_LIBRARY_PATHS
	STEAM_LIBRARY_PATHS=${STEAM_LIBRARY_PATHS_DEFAULT}

	# User defined folders are stored in this file.
	local vdf_file="${STEAM_LIBRARY_PATHS}/libraryfolders.vdf"

	if [ -f ${vdf_file} ]; then
		: # Continue
	else
		printf "Line $BASH_LINENO: Could not find '${vdf_file}'.\n"
		return 1
	fi

	# Only interested in the part with the known locations, for example:
	# ``	"1"		"/opt/valve/SteamLibrary"``
	#
	# Supports up to 9 extra locations (to keep the regexp simple).
	# Steam stores all relevant data under the sub-folder 'steamapps'.
	STEAM_LIBRARY_PATHS+=($(cat ${vdf_file} | awk -F '\t' '{if($2 ~ /^"[1-9]"$/) {gsub(/"/,"",$4) ; print $4 "/steamapps"}}'))

	if [ "${VERBOSE}" = "true" ]; then
	{
		printf '\n%s\n' "Found the following ${#STEAM_LIBRARY_PATHS[@]} steam library paths:"
		for ind in "${STEAM_LIBRARY_PATHS[@]}"; do
			printf ' %s\n' "$ind"
		done
		printf '\n'
	}
	fi
}

# Get the steam manifest file for selected game.
#
# $1 Game steam ID.
#
# Sets the global variable: $GAME_MANIFEST
get_game_manifest ()
{
	unset GAME_MANIFEST

	# Assume only one instance of a game can be installed.
	for ind in "${STEAM_LIBRARY_PATHS[@]}"; do
	{
		if [ -f "${ind}/appmanifest_${1}.acf" ]; then
		{
			GAME_MANIFEST="${ind}/appmanifest_${1}.acf"

			if [ "${VERBOSE}" = "true" ]; then
				printf '%s\n' "Game manifest: $GAME_MANIFEST"
			fi
			return 0
		}
		fi
	}
	done

	if [ "${VERBOSE}" = "true" ]; then
		printf '\n%s\n' "${FUNCNAME[0]}(): Manifest not found."
	fi
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
get_game_install_state ()
{
	unset GAME_INSTALL_STATE

	get_game_manifest ${1}
	if [ $? -eq 0 ]; then
	{
		GAME_INSTALL_STATE=$(cat "${GAME_MANIFEST}" | awk -F '\t' '{if($2 ~ /^"StateFlags"$/) {gsub(/"/,"",$4) ; print $4}}')

		if [ "${VERBOSE}" = "true" ]; then
			printf '%s\n' "Install state: $GAME_INSTALL_STATE"
		fi
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
# Updates the global variable: $STEAM_GAME_DATA_PATH and $OXC_GAME_DATA_PATH
get_game_data_paths ()
{
	unset STEAM_GAME_DATA_PATH
	unset OXC_GAME_DATA_PATH
	unset GAME_SUBDIRS
	local installPath

	get_game_manifest ${1}
	if [ $? -eq 0 ]; then
	{
		installPath=$(cat "${GAME_MANIFEST}" | awk -F '\t' '{if($2 ~ /^"installdir"$/) {gsub(/"/,"",$4) ; print $4}}')

		# Installed game is stored under the library's subdirectory "common".
		STEAM_GAME_DATA_PATH="$(dirname ${GAME_MANIFEST})/common/${installPath}"
		case ${1} in
			${STEAM_ID_UFO})
				STEAM_GAME_DATA_PATH="${STEAM_GAME_DATA_PATH}/XCOM"
				OXC_GAME_DATA_PATH="${OXC_DATA_ROOT}/UFO"
				GAME_SUBDIRS=(
					"GEODATA"
					"GEOGRAPH"
					"MAPS"
					"ROUTES"
					"SOUND"
					"TERRAIN"
					"UFOGRAPH"
					"UFOINTRO" # Optional
					"UNITS"
				)
				GAME_PATCH_URL=${OXC_UFO_PATCH_URL}
				;;
			${STEAM_ID_TFTD})
				STEAM_GAME_DATA_PATH="${STEAM_GAME_DATA_PATH}/TFD"
				OXC_GAME_DATA_PATH="${OXC_DATA_ROOT}/TFTD"
				GAME_SUBDIRS=(
					"ANIMS"    # Optional
					"FLOP_INT" # Optional
					"GEODATA"
					"GEOGRAPH"
					"MAPS"
					"ROUTES"
					"SOUND"
					"TERRAIN"
					"UFOGRAPH"
					"UNITS"
				)
				GAME_PATCH_URL=${OXC_TFTD_PATCH_URL}
				;;
			*)
				printf '\n%s\n' "Unknown game"
				return 1
				;;
		esac

		if [ "${VERBOSE}" = "true" ]; then
			printf '%s\n' "Steam game data files: $STEAM_GAME_DATA_PATH"
			printf '%s\n' "OXC game data files: $OXC_GAME_DATA_PATH"
		fi
		return 0
	}
	else
		return 1
	fi
}

# Report the progress of the current download.
#
# $1 Game steam ID.
#
# Prints the current download percentage.
#
# Note:
#   Steam only updates the manifest upon state changes (this includes pausing a
#   download), hence this function is of limited use.
print_download_progress ()
{
	local downloadSize
	local downloadedBytes

	get_game_manifest ${1}
	if [ $? -eq 0 ]; then
	{
		downloadSize=$(cat "${GAME_MANIFEST}"| awk -F '\t' '{if($2 ~ /^"BytesToDownload"$/) {gsub(/"/,"",$4) ; print $4}}')
		downloadedBytes=$(cat "${GAME_MANIFEST}" | awk -F '\t' '{if($2 ~ /^"BytesDownloaded"$/) {gsub(/"/,"",$4) ; print $4}}')
		if [ -z "$downloadSize" ] || [ "$downloadSize" -eq "0" ]; then
		{
			printf '%s\n' "Cannot determine download status."
			# Well unless '$GAME_INSTALL_STATE=4' (100% downloaded), but then this function is not supposed to run.
		}
		elif [ "$downloadSize" -gt "0" ]; then
		{
			# If we have a '$downloadSize' we should also have '$downloadedBytes'.
			printf '%s\n' "$((100*$downloadedBytes/$downloadSize))%"
		}
		fi
	}
	fi
}

# If steam is not already running start it.
#
# Note:
#   I cannot reliable determine when steam has started completely, make sure
#   there is some sort of delay before attempting further actions.
# Note:
#   Unreliable if user quits steam just moments before a call to this function
#   (due to leftover steam process till it is fully terminated).
start_steam ()
{
	pgrep -x steam >/dev/null
	if [ $? -eq 0 ]; then
	{
		printf '%s\n' "Detected a running steam instance, script will continue."
		return 0
	}
	fi

	if [ "$(command -v steam)" ]; then
	{
		printf '%s\n' "Starting steam in the background."
		# Steam is kinda verbose when starting so redirect all output to /dev/null.
		# Start in a separate process to prevent blocking rest of script until steam
		# is terminated by the user.
		steam >/dev/null 2>&1 &

		# Artificial wait (to prevent double start-up from install/validate).
		read -s -p "Once the GUI becomes visible press [enter] to continue."
		printf '\n'
	}
	else
	{
		printf '%s\n' "Steam runtime not found ... exiting"
		exit 1
	}
	fi
}

# Validate game files
#
# $1 Game steam ID.
#
# This function ensures that the data files have not been altered.
# Validation does NOT touch existing saves (thank you steam)!
#
# Note:
#   Cannot be used in the following cases:
#   * Checking if user owns a game.
#     If not owned the validation dialog stalls instead of going to the shop.
#   * Installing the game
#     If no installation is present on the system the validation dialog stalls.
# Note:
#   Will undo any user alterations to the steam managed data files.
validate_game ()
{
	# Assume user changed settings just before entering this part of the code.
	parse_steam_libraryfolders_file

	get_game_manifest ${1}
	if [ $? -eq 0 ]; then
	{
		# Not starting in the background in order to block script till steam decides
		# it is time to start validating.
		# Redirecting output saves 3 lines of CLI noise.
		steam steam://validate/${1} >/dev/null 2>&1
		printf '\n%s\n' "Validating files."

		# Steam validation process is not instant, give it some extra time.
		sleep 2 # Seconds (system dependent)
		# Apparently starting a validation does not always update the manifest :-(.
		#read -s -p "Once validation dialog states 100% complete press [enter] to continue."
		#printf '\n'

		local timer=0
		local interval=1 # Seconds
		local timeout=60 # seconds
		# Query state, allow for an (arbitrary) 1 minute time-out.
		while [ $timer -lt $timeout ]; do
		{
			printf '%s\t' "$(date +%T)"
			get_game_install_state ${1}

			case ${GAME_INSTALL_STATE} in
			4)
				printf '%s\n' "All files validated."
				return 0
				;;
			38)
				printf '%s\n' "Need to download files due to validation."
				timer=0;
				;;
			1062)
				# Not all files are in pristine condition.
				printf '%s\n' "Validation download in progress... "
				;;
			1574)
				printf '%s' "Validation download paused at: "
				print_download_progress ${1}
				timer=$(($timer + $interval))
				;;
			*)
				printf '%s' "Validating ... "
				timer=$(($timer + $interval))
				;;
			esac
			sleep $interval
		}
		done

		printf '%s\n' "Timeout (${timeout} seconds) reached while trying to validate the game."
		return 1
	}
	else
	{
		printf '\n%s\n' "${FUNCNAME[0]}(): Game not installed"
		return 1
	}
	fi
}

# Download game files
#
# $1 Game steam ID.
#
# This function installs the steam version of the game.
#
# Note:
#   As a byproduct some proton version will be installed on the system.
#   This is required by steam to allow downloading the game files.
download_game ()
{
	printf '%s\n' ""\
	"About to install the game..."\
	"Make sure proton is enabled for this game:"\
	" * Steam Library -> select game -> select 'Properties...'"\
	"   Under Manage, or via RMB on title."\
	" * Enable the option:  'Force the use of a specific Steam Play compatibility tool'"\
	"   Any version (including the default 'Steam Linux Runtime') will do."
	read -s -p "Once proton is enabled press [enter] to continue."
	printf '\n\n'

	# Not starting in the background in order to block script till steam decides
	# it is time to start downloading (thereby creating a necessary file).
	# Redirecting output saves 3 lines of CLI noise.
	steam steam://install/${1} >/dev/null 2>&1

	# User might have decided to define another library.
	parse_steam_libraryfolders_file

	local timer=0
	local interval=1 # Seconds
	local timeout=60 # seconds
	while [ $timer -lt $timeout ]; do
	{
		printf '%s\t' "$(date +%T)"
		get_game_manifest ${1}
		if [ $? -eq 0 ]; then
		{
			get_game_install_state ${1}

			case ${GAME_INSTALL_STATE} in
			2)
				printf '%s\n' "Download will start shortly."
				timer=0;
				;;
			4)
				printf '%s\n' "Download finished."
				return 0
				;;
			1026)
				printf '%s\n' "Downloading..."
				;;
			1538)
				printf '%s' "Download paused at: "
				print_download_progress ${1}
				timer=$(($timer + $interval))
				;;
			*)
				printf '%s\n' "Error, unknown status: ${GAME_INSTALL_STATE}"
				return 1
				;;
			esac
		}
		else
		{
			printf '%s\n' "Waiting for download to start"
			timer=$(($timer + $interval))
		}
		fi
		sleep $interval
	}
	done

	printf '%s\n' ""\
	"Timeout (${timeout} seconds) reached while trying to install the game."\
	"Possible causes:"\
	" * Game has not been purchased yet."\
	" * Steam compatibility tool is not activated for this game."\
	"   In this case a message dialog should have popped up stating:"\
	"    \"The game is not available on your current platform\""\
	" * The download was paused for too long."\
	" * Too much time has passed while clicking through steam's confirmation dialogs."
	return 1
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

	parse_steam_libraryfolders_file
	get_game_install_state ${1}

	case ${GAME_INSTALL_STATE} in
		"")
			download_game ${1}
			;;
		"4")
			validate_game ${1}
			;;
		*)
			printf '\n%s\n' "Got an unexpected state ... install manually and try this script again."
			return 1
			;;
	esac

	# Only continue if call inside previous case block was successful.
	if [ $? -eq 0 ]; then
		get_game_data_paths ${1}
		copy_data_files
	else
		printf '\n%s\n' "Something went wrong installing/validating the steam version."
		return 1
	fi

	apply_OXC_patches
}

# Main
# ====
show_help
input_error

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

# Steam browser protocol
# ----------------------
# This is what we use to control steam
# see: https://developer.valvesoftware.com/wiki/Steam_browser_protocol
# Note:
#  Could not get the ExitSteam functionality to work, that is why it's not implemented

# Steam status indicators
# -----------------------
# Numbers encountered when testing the script
# * 2:    Update required, e.g. about to start download
# * 4:    Game is fully installed
# * 38:   Validation result.
#         Weird thing, number equals: FullyInstalled + UpdateRequired + FilesMissing
# * 1062: Downloading files due to validation.
#         Does not matter if proton is enabled/disabled.
# * 1026: Downloading files, game not installed before.
# * 1538: Install, download paused
# * 1574: Validation. download paused
#
# For an overview of known states see:
#  https://github.com/lutris/lutris/blob/master/docs/steam.rst