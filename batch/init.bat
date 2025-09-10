# Initialisation at start

#############################
# This script is used to initialize the system and set up the environment
# 
start:
echo Initializing system...
touch .safemode   # system will boot into safe mode if this file exists on next boot
loopdelay 1       # sets the delay at the end of each loop in usec
set userscript init.$(HOSTNAME).bat
test -f $(userscript) || test -f $(userscript).bak && cp $(userscript).bak $(userscript)  # if no user script exists, first fall back is backup file
test -f $(userscript) || test -f init.user.bat     && cp init.user.bat $(userscript)      # second fall back is default init file

# final initialisations
final:
timer add 15s "wifi connect;prompt" tiRecon
timer add 1m "wifi check -q" tiWifi
timer add 1m "syslog Metrics -M" tMetrics

wifi connect
stack off
usr 0

# Safemode
sm:
echo SAFEMODE ON

#############################
# Specific initializations
#

# I2C capability
i2c:

# MQTT capability
mqtt:

# Home Assistant capability
ha:

# Segment Display (seg) capability
seg:

# RC
rc:

#############################
# Runtime settings
#

# wifi is up and connected
wifi-up:
timer stop tiRecon

# wifi is down
wifi-down:
timer start tiRecon

# wifi is online
wifi-online:
timer stop tiRecon
ntp sync
syslog "$(HOSTNAME) online" 


# wifi is offline
wifi-offline:
timer start tiRecon

# Access Point up
ap-up:
timer stop tiWifi
timer stop tiRecon

# Access Point down
ap-down:
timer start tiWifi
timer start tiRecon

#############################
# more commands for all labels
all:
test 1 -eq $(SAFEMODE) && break
test -f $(userscript) && exec $(userscript) $(LABEL) # calls the user script if it exists
