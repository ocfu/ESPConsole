# Initialisation at start


#
# Filesystem capability and environment settings
#
fs:
touch .safemode   # system will boot into safe mode if this file exists on next boot
loopdelay 1       # sets the delay at the end of each loop in usec
set userscript init.$HOSTNAME.bat
exec $userscript fs

# I2C capability
i2c:
break on $SAFEMODE
exec $userscript i2c

# MQTT capability
mqtt:
break on $SAFEMODE
exec $userscript mqtt

# Home Assistant capability
ha:
break on $SAFEMODE
exec $userscript ha

# Segment Display (seg) capability
seg:
break on $SAFEMODE
exec $userscript seg

# RC
rc:
break on $SAFEMODE
exec $userscript rc

# final initialisations
final:
break on $SAFEMODE
exec $userscript final

# copy default init
cp:
cp init.user.bat $userscript

# wifi is up and connected
wifi-up:
break on $SAFEMODE
exec $userscript wifi-up

# wifi is down
wifi-down:
break on $SAFEMODE
exec $userscript wifi-down

# wifi is online
wifi-online:
break on $SAFEMODE
exec $userscript wifi-online

# wifi is offline
wifi-offline:
break on $SAFEMODE
exec $userscript wifi-offline


