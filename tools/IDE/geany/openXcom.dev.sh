#!/bin/sh

# Use dev version of config
ln -sf options.cfg.dev ~/.config/openxcom/options.cfg

# Start program.
# Assumes geany basepath equals repository root, and the working dir of the
# program is set to "%p/bin".
"./openxcom"

echo "openXcom exit status = $?"

# Return to play version of config
ln -sf options.cfg.play ~/.config/openxcom/options.cfg
