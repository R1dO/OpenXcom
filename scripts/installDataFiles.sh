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

# Steam variables
# ---------------
STEAM_DATA_PATH="${DATA_HOME}/Steam"
STEAM_LIBRARY_PATHS="${STEAM_DATA_PATH}/steamapps"

# Functions
# =========

# Get the (user defined) library folders.
#
# Adds all folders to the global array: $STEAM_LIBRARY_PATHS
parse_steam_libraryfolders_file ()
{
	if [ ${#STEAM_LIBRARY_PATHS[@]} -gt 1 ]; then
		printf "Line $BASH_LINENO: '${FUNCNAME[0]}' only needs to run once.\n"
		return 1
	fi

	VDF_FILE="${STEAM_LIBRARY_PATHS}/libraryfolders.vdf"

	if [ -f "${VDF_FILE}" ]; then
		: # Continue
	else
		printf "Line $BASH_LINENO: Could not find '${VDF_FILE}'.\n"
		return 1
	fi

	# Only interested in the specific part containing the known locations:
	# ``	"1"		"/opt/valve/SteamLibrary"``
	#
	# Supports up to 9 extra locations (to keep the regexp simple).
	# Steam stores all relevant data under the sub-folder 'steamapps'.
	STEAM_LIBRARY_PATHS+=($(cat ${VDF_FILE} | awk -F '\t' '{if($2 ~ /^"[1-9]"$/) {gsub(/"/,"",$4) ; print $4 "/steamapps"}}'))

	# Reporting:
	printf '\n%s\n' "Found the following ${#STEAM_LIBRARY_PATHS[@]} steam library paths:"
	for ind in "${STEAM_LIBRARY_PATHS[@]}"; do
		printf '  %s\n' "$ind"
	done
}

# Main
# ====
parse_steam_libraryfolders_file
