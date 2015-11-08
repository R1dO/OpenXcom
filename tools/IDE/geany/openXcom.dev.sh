#!/bin/sh

dev_config_file=~/.config/openxcom/options.dev.cfg
play_config_file=~/.config/openxcom/options.play.cfg
config_file=~/.config/openxcom/options.cfg

# Check if there are multiple config versions
if [ ! -f $dev_config_file ]; then
    cp $config_file $dev_config_file
fi
if [ ! -f $play_config_file ]; then
    cp $config_file $play_config_file
fi

# Use dev version of config
ln -sf $dev_config_file $config_file

# Start program.
# Assumes geany basepath equals repository root, and the working dir of the
# program is set to "%p".
if [ -f bin/openxcom ]; then
    cd bin
    ./openxcom
else
    cd build/bin
    ./openxcom  # Out of source build
fi

echo "openXcom exit status = $?"

# Return to play version of config
ln -sf $play_config_file $config_file
