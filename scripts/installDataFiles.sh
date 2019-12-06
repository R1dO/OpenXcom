#!/bin/bash
#
# Note:
#  Do not use: ``sh ./installDataFiles.sh``. It will probably kill your dog, at
#  the very least it will complain and fail.
#
#  Script assumes a default steam installation (see $STEAM_DATA_PATH).

# Guardian
# ========
# This script is not meant to run with superuser credentials. Steam will refuse
# to work in that scenario.
if [ "$(id -u)" == "0" ]; then
	printf '%s\n' "Steam does not allow running as root, this script wont work ... exiting."
	exit 1
fi


# Global variables
# ================
DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"
# Clutter the CLI with noise?
VERBOSE=true

# Steam
# -----
STEAM_DATA_PATH="${DATA_HOME}/Steam"
STEAM_LIBRARY_PATHS="" #Set by 'parse_steam_libraryfolders_file()'.
STEAM_LIBRARY_PATHS_DEFAULT="${STEAM_DATA_PATH}/steamapps"
STEAM_ID_UFO="7760"
STEAM_ID_TFTD="7650"
STEAM_UFO_DATA_PATH="XCOM"
STEAM_TFTD_DATA_PATH="TFD"

# Game dependent variables
# ------------------------
GAME_MANIFEST=""
GAME_INSTALL_STATE=""
GAME_DATA_PATH=""

# Functions
# =========

# Get the (user defined) library folders.
#
# Sets the global array: $STEAM_LIBRARY_PATHS
parse_steam_libraryfolders_file ()
{
	# Reset array to default value (in case we ran before).
	unset STEAM_LIBRARY_PATHS
	STEAM_LIBRARY_PATHS=${STEAM_LIBRARY_PATHS_DEFAULT}

	# User defined folders are stored in this file.
	vdf_file="${STEAM_LIBRARY_PATHS}/libraryfolders.vdf"

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
#   For simplicity we will assume that anything bigger constitutes to a download
#   in progress, either due to new installation or validation.
#   For an overview of known states see:
#    https://github.com/lutris/lutris/blob/master/docs/steam.rst
get_game_install_status ()
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
# Updates the global variable: $GAME_DATA_PATH
get_game_data_path ()
{
	unset GAME_DATA_PATH
	installPath=""

	get_game_manifest ${1}
	if [ $? -eq 0 ]; then
	{
		installPath=$(cat "${GAME_MANIFEST}" | awk -F '\t' '{if($2 ~ /^"installdir"$/) {gsub(/"/,"",$4) ; print $4}}')

		# Installed game is stored under the library's subdirectory "common".
		GAME_DATA_PATH="$(dirname ${GAME_MANIFEST})/common/${installPath}"
		case ${1} in
			${STEAM_ID_UFO})
				GAME_DATA_PATH="${GAME_DATA_PATH}/${STEAM_UFO_DATA_PATH}"
				;;
			${STEAM_ID_TFTD})
				GAME_DATA_PATH="${GAME_DATA_PATH}/${STEAM_TFTD_DATA_PATH}"
				;;
			*)
				printf '\n%s\n' "Unknown game"
				return 1
				;;
		esac

		if [ "${VERBOSE}" = "true" ]; then
			printf '%s\n' "Game data files: $GAME_DATA_PATH"
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
#   Steam updates the manifest upon state changes and after some unknown (to me)
#   time delta. Hence expect this function to return a lower value.
print_download_progress ()
{
	downloadSize=""
	downloadedBytes=""

	get_game_manifest ${1}
	if [ $? -eq 0 ]; then
	{
		downloadSize=$(cat "${GAME_MANIFEST}"| awk -F '\t' '{if($2 ~ /^"BytesToDownload"$/) {gsub(/"/,"",$4) ; print $4}}')
		downloadedBytes=$(cat "${GAME_MANIFEST}" | awk -F '\t' '{if($2 ~ /^"BytesDownloaded"$/) {gsub(/"/,"",$4) ; print $4}}')
		if [ -z "$downloadSize" ] || [ "$downloadSize" -eq "0" ]; then
		{
			printf '%s\n' "Cannot determine download status."
			# Well unless '$GAME_DATA_PATH=4' (100% downloaded), but then this function is not supposed to run.
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
	}
	else
	{
		printf '%s\n' "Starting steam in the background."
		# If not started in a separate process it blocks rest of script until steam
		# is terminated by the user.
		# Steam is kinda verbose when starting so redirect all output to /dev/null.
		steam >/dev/null 2>&1 &

		# Artificial wait (to prevent double start-up from install/validate).
		read -s -p "Once the GUI becomes visible press [enter] to continue."
		printf '\n'
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

		timer=0
		interval=1 # Seconds
		# Query state, allow for an (arbitrary) 1 minute time-out.
		while [ $timer -lt 60 ]; do
		{
			printf '%s\t' "$(date +%T)"
			get_game_install_status ${1}

			case ${GAME_INSTALL_STATE} in
			4)
				printf '%s\n' "All files validated."
				return 0
				;;
			1062)
				# Not all files are in pristine condition.
				printf '%s' "Validation download: "
				print_download_progress ${1}
				;;
			*)
				printf '%s' "Validating ... "
				timer=$(($timer + $interval))
				;;
			esac
			sleep $interval
		}
		done
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
	printf '\n'

	# Not starting in the background in order to block script till steam decides
	# it is time to start downloading (thereby creating a necessary file).
	# Redirecting output saves 3 lines of CLI noise.
	steam steam://install/${1} >/dev/null 2>&1

	# User might have decided to define another library.
	parse_steam_libraryfolders_file

	timer=0
	interval=1 # Seconds
	# Timer should only update when download has not started yet.
	while [ $timer -lt 60 ]; do
	{
		printf '%s\t' "$(date +%T)"
		get_game_manifest ${1}
		if [ $? -eq 0 ]; then
		{
			get_game_install_status ${1}

			case ${GAME_INSTALL_STATE} in
			2)
				printf '%s\n' "Download will start shortly."
				;;
			4)
				printf '%s\n' "Download finished."
				return 0
				;;
			1026)
				printf '%s' "Downloading; "
				print_download_progress ${1}
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
	"Timeout while trying to install the game."\
	"Possible causes:"\
	" * You haven't purchased the game yet."\
	" * You forgot to force the steam compatibility tool for this game."\
	"   In this case you should have gotten a message dialog stating:"\
	"    \"The game is not available on your current platform\""
	return 1
}

# Main
# ====
# Start steam as early as possible so it has time to fully initialize.
start_steam


# TEMPORAL reporting
# ==================
printf "\nUFO:\n====\n"
download_game  "$STEAM_ID_UFO"
echo $?

printf "\nTFTD:\n=====\n"
validate_game "$STEAM_ID_TFTD"
echo $?

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

# Known status indicators
# -----------------------
# Numbers encountered when testing the script
# * 2:    Update required, e.g. about to start download
# * 4:    Game is fully installed
# * 1062: Need to download some files due to validation. Does not matter if proton is enabled/disabled.
# * 1026: Downloading files, game not installed before.
