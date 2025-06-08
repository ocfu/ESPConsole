#
# init.user.bat [<label> [<arguments>]]
#
# Template for user-specific configurations. This file is copied by default
# if no init.<hostname>.bat exists. For custom settings, use init.<hostname>.bat.
#
# Arguments:
#   <label>     Specifies the starting point for batch execution. Execution begins
#               after the specified label (<label>:), ending at the next label or EOF.
#               Note: Commands before the first label always execute, regardless of the label.
#               The reserved label 'all:' should be placed at the end of the file.
#   <arguments> Additional arguments for command execution, accessible via:
#               $<ARG>    ARG: 0..n (n <= 8)
#               $@        All arguments ($0..$n)
#               $*        All arguments except the caller ($1..$n) (not POSIX compliant)
#               $#        Number of arguments
#

###############################
# Local Variables
# These variables are set each time the batch file runs and are deleted after execution.
# Syntax: <variable name> = <value or string>
# Note: Spaces are required before and after the equal sign!
#

# MQTT configuration
name = "Application Name"
root = esp/myapp
port = 8883

###############################
# General Setup at Start
#
# Batch execution begins with the 'fs' label, which is called during initialization.
#
fs:

#
# Environment Variables
#
#   TZ   - Timezone
#   url  - Device URL, used for Home Assistant (HA) setup
#
set TZ CET-1CEST,M3.5.0,M10.5.0/3
set url http://$(HOSTNAME)

#
# Logging Configuration
#
log server mac 8880
log level 1

#
# Built-in LED Setup
#
# The 'led' command controls the built-in LED by default.
#
led invert 1
led off

#
# GPIO Setup
#
# Syntax: (see 'man gpio')
#   gpio set <pin> <mode>
#   gpio add <pin> <type> <name> <inverted> [<cmd> [<param>]]
#

#
# Timer Setup
#
# Syntax: <period/cron> <command> <id> <mode> (see 'man timer')
#
timer add "0 0 * * *" "exec §(userscript) crMidnight" crMidnight  # Cron timer triggers 'crMidnight' event at midnight

#
# Variable-based Sensor Setup
#
# Variable-based sensors represent global variables (not physical sensors).
# Useful for displaying values or exposing them as sensor entities in HA.
# Syntax: sensor add <name> <unit> <variable> [<friendly name>] (see 'man sensor')
#

#
# Events
#
# User-defined labels (<label>:) that can be triggered with:
#   exec §(userscript) <label> [<arguments>]
# Events can be defined anywhere in this file.
#
crMidnight:
#timer add 1m "reboot -f"   # Reboot 1 minute after midnight (prevents multiple reboots at midnight)

################################
# I2C Configuration
#
i2c:
i2c setpins 4 5
i2c init
#
# If CxSensorBme.hpp is included, detected BME sensors are auto-added with generic names.
# Rename them here for use in HA or elsewhere.
sensor name 0 temperature
sensor name 1 humidity
sensor name 2 pressure

################################
# MQTT Configuration
#
mqtt:
mqtt server ocdk $(port)
mqtt qos 0
mqtt root $(root)
mqtt name $(name)
mqtt will 1
mqtt heartbeat 1000

################################
# Home Assistant (HA) Integration
#
# Syntax: See 'man ha'
#
ha:
ha text add txtinput "Eingabe" 0 64
ha sensor add temperature
ha sensor add humidity
ha sensor add pressure

ha select add selsegopt "Display" 1 # as config
ha select addopt selsegopt "Time"
ha select addopt selsegopt "Temperature"
ha select addopt selsegopt "Humidity"
ha select addopt selsegopt "Pressure"
ha select addopt selsegopt "Loop"

# HA Event Handlers
#
haenable:
test $1 -eq 0 && break
ha state txtinput ""

txtinput:
test ! $# -gt 0 && break
test $1 = V && test -n $2 && seg msg $($2) && break # Show variable value on segment display
$*  # Execute input as command
ha state txtinput ""

selsegopt:
seg slideshow off
set screen = $1 - 1
test $(screen) -lt 4 && seg show $(screen) && break
test $(screen) -eq 4 && seg slideshow on && break

################################
# Segment Display Configuration
#
# Syntax: See 'man seg'
#
seg:
seg setpins 14 12
seg init
seg enable 1
seg br 10
seg screen add time time                      # screen 0
seg screen add temp sensor temperature
seg screen add hum sensor humidity
seg screen add pres sensor pressure

seg slideshow add 0
seg slideshow add 1

seg slideshow on

################################
# Final Initialization Phase
#
# Note: Not called if system starts in safemode.
#
final:

################################
# WiFi Connected
#
wifi-up:
log on
set NTP fritz.box
mqtt connect
ha enable 1

################################
# WiFi Shutdown
#
wifi-down:
echo "wifi down"
#mqtt stop

################################
# WiFi Online
#
wifi-online:
echo "wifi online"

################################
# WiFi Offline
#
wifi-offline:
echo "wifi offline"
log off

################################
# Access Point Status
#
ap-up:

ap-down:
