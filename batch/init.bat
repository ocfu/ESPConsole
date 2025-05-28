# Initialisation at start

#
# Filesystem capability and environment settings
#
fs:
touch .safemode   # system will boot into safe mode if this file exists on next boot
loopdelay 1       # sets the delay at the end of each loop in usec
set userscript init.$(HOSTNAME).bat
test -f $(userscript) || cp $(userscript).bak $(userscript)  # if no user script exists, first fall back is backup file
test -f $(userscript) || cp init.user.bat $(userscript)      # second fall back is default init file

# I2C capability
i2c:
break on $(SAFEMODE)

# MQTT capability
mqtt:
break on $(SAFEMODE)

# Home Assistant capability
ha:
break on $(SAFEMODE)

# Segment Display (seg) capability
seg:
break on $(SAFEMODE)

# RC
rc:
break on $(SAFEMODE)

# final initialisations
final:
timer add 15s "wifi connect;prompt" tiRecon
timer add 1m "wifi check -q" tiWifi
wifi connect
stack off
usr 0
break on $(SAFEMODE)

# wifi is up and connected
wifi-up:
timer stop tiRecon
break on $(SAFEMODE)

# wifi is down
wifi-down:
timer start tiRecon
break on $(SAFEMODE)

# wifi is online
wifi-online:
timer stop tiRecon
break on $(SAFEMODE)

# wifi is offline
wifi-offline:
timer start tiRecon
break on $(SAFEMODE)

# Access Point up
ap-up:
timer stop tiWifi
timer stop tiRecon
break on $(SAFEMODE)

# Access Point down
ap-down:
timer start tiWifi
timer start tiRecon
break on $(SAFEMODE)

#
# Safemode
#
sm:


# more commands for all labels
all:
exec $(userscript) $(LABEL)
