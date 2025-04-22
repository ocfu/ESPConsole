# Initialisation at start


#
# Filesystem capability and environment settings
#
fs:
touch .safemode   # system will boot into safe mode if this file exists on next boot
loopdelay 1       # sets the delay at the end of each loop in usec
set userscript init.$HOSTNAME.bat

# I2C capability
i2c:
break on $SAFEMODE

# MQTT capability
mqtt:
break on $SAFEMODE

# Home Assistant capability
ha:
break on $SAFEMODE

# Segment Display (seg) capability
seg:
break on $SAFEMODE

# RC
rc:
break on $SAFEMODE

# final initialisations
final:
break on $SAFEMODE

# copy default init
cp:
cp init.user.bat $userscript

# wifi is up and connected
wifi-up:
break on $SAFEMODE

# wifi is down
wifi-down:
break on $SAFEMODE

# wifi is online
wifi-online:
break on $SAFEMODE

# wifi is offline
wifi-offline:
break on $SAFEMODE

# Access Point up
ap-up:
break on $SAFEMODE

# Access Point down
ap-down:
break on $SAFEMODE

# final calls for all labels
all:
exec $userscript $LABEL
