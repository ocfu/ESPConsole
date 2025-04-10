# Initialisation at start
# echo Initialising $LABEL

#
# Filesystem capability and environment settings
#
fs:
touch .safemode   # system will boot into safe mode if this file exists on next boot
loopdelay 1       # sets the delay at the end of each loop in usec
gpio load
exec user.bat fs

# I2C capability
i2c:
exec user.bat i2c

# MQTT capability
mqtt:
exec user.bat mqtt

# Home Assistant capability
ha:
exec user.bat ha

# Segment Display (seg) capability
seg:
exec user.bat seg

