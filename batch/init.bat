# Initialisation at start

userscript = init.$HOSTNAME.bat

#
# Filesystem capability and environment settings
#
fs:
touch .safemode   # system will boot into safe mode if this file exists on next boot
loopdelay 1       # sets the delay at the end of each loop in usec
exec $userscript fs

# I2C capability
i2c:
exec $userscript i2c

# MQTT capability
mqtt:
exec $userscript mqtt

# Home Assistant capability
ha:
exec $userscript ha

# Segment Display (seg) capability
seg:
exec $userscript seg

# RC
rc:
exec $userscript rc

# copy default init
cp:
cp init.user.bat $userscript

