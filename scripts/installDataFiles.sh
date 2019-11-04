#!/bin/bash
#
# Note:
#  Do not use: ``sh ./installDataFiles.sh``. It will probably kill your dog, at
#  the very least it will complain and fail.
#
#  Script assumes a default steam installation (see $STEAM_DATA_PATH).

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

	if [ ${VERBOSE} = "true" ]; then
		printf '\n%s\n' "Found the following ${#STEAM_LIBRARY_PATHS[@]} steam library paths:"
		for ind in "${STEAM_LIBRARY_PATHS[@]}"; do
			printf ' %s\n' "$ind"
		done
	fi
}

# Get the steam manifest file for selected game.
#
# $1 Game steam ID.
#
# Sets the global variable: $GAME_MANIFEST
get_manifest_location ()
{
	unset GAME_MANIFEST

	parse_steam_libraryfolders_file
	# Assume only one instance of a game can be installed.
	for ind in "${STEAM_LIBRARY_PATHS[@]}"; do
		if [ -f "${ind}/appmanifest_${1}.acf" ]; then
			GAME_MANIFEST="${ind}/appmanifest_${1}.acf"

			if [ ${VERBOSE} = "true" ]; then
				printf '\n%s\n' "Manifest found at: $GAME_MANIFEST"
			fi
			return 0
		fi
	done

	if [ ${VERBOSE} = "true" ]; then
		printf '\n%s\n' "Manifest not found."
	fi
	return 1
}

# Main
# ====
parse_steam_libraryfolders_file
