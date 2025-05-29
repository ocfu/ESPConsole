# This script is a template for user specific configurations.
#

###############################
# local variables

# MQTT settings
name = "Project Name"
root = esp/test/sensta
port = 8883

###############################
# general and gpio setup is with fs
#
fs:

stack off # switch on to monitor stack usage

set TZ CET-1CEST,M3.5.0,M10.5.0/3
set url http://$(HOSTNAME)

log server mac 8880
log level 1
led invert 1
led off


# gpio setup
pinLed = 12
pinCnt = 14

#        <pin>     <type>  <id>
gpio add $(pinLed) led     ledbl
gpio add $(pinCnt) counter c0 1 "exec §(userscript) cnt §(ADD)" 1 #as INPUT_PULLUP

#additional settings
gpio isr $(pinCnt) 0 10000     # <pin> <id> <dbounce us>
gpio fn  $(pinLed) "Blue Led"  # <pin> <fiendly name>
gpio fn  $(pinCnt) "Counter""

# timer
#         <period/cron>   <command>                        <id>           <mode>
timer add "0 0 * * *"     "exec §(userscript) crMidnight"  crMidnight
timer add 2m              "exec §(userscript) tiNanoCheck" tiValidateData


# json processing of incoming data
#                <json path> <command>
processdata json "data.t"    "set t && test §(VALUE) -gt -40 && test §(VALUE) -lt 50   && set t/2 = §(VALUE)"
processdata json "data.p"    "set p && test §(VALUE) -gt 850 && test §(VALUE) -lt 1100 && set p/2 = §(VALUE)"
processdata json "data.h"    "set h && test §(VALUE) -ge 0   && test §(VALUE) -le 100  && set h/2 = §(VALUE) && exec §(userscript) data"

set jsonstate "data.state"    # state path (if part of the json) for validation.

# sensors
#          <name>      <type>      <unit> <variable> <friendly name>
sensor add cnt         volume      L      VT         Water
sensor add temperature temperature °C     t          Temperature
sensor add humidity    humidity    %      h          Humidity
sensor add pressure    pressure    hPa    p          Pressure

# events
#

# on new data
data:
timer start tiValidateData

# on validate data: invalidate old sensor data
tiValidateData:
set .t
set .p
set .h

# on counter:
cnt:
set VL/4 = $(VL) + $1 * $(LPP)   # fill the bucket


################################
# MQTT
#
mqtt:
mqtt server ocdk $(port)
mqtt qos 0
mqtt root $(root)
mqtt name $(name)
mqtt will 1
mqtt heartbeat 1000

#
# Home Assistant
#
ha:
ha sensor add cnt 10000
ha sensor add temperature 60000
ha sensor add humidity 60000
ha sensor add pressure 60000

ha text add txtinput "Eingabe" 0 64

# HA events
#
haenable:
test $1 -eq 0 && break
ha state txtinput ""

txtinput:
test ! $# -gt 0 && break
test $1 = V && test -n $2 && seg msg $($2) && break # show variable value on segment display
$*  # take input as command
ha state txtinput ""

#
# Segment Display
#
seg:
seg setpins 5 4
seg init
seg enable 1
seg br 10
seg screen add time time                      # screen 0
seg screen add temperature sensor temperature # screen 1
seg screen add watercnt    sensor watercnt    # screen 2
seg screen add pumppres    sensor pumppres    # screen 3

seg slideshow add 0
seg slideshow add 1

seg slideshow on

#
# Final init. Will not be called in safemode
#
final:

#
# Wifi is up and connected
#
wifi-up:
log on
set NTP fritz.box
mqtt connect
ha enable 1

#
# wifi is shut down
#
wifi-down:
echo "wifi down"
#mqtt stop

#
# wifi is online
#
wifi-online:
echo "wifi online"

#
# wifi is offline
#
wifi-offline:
echo "wifi offline"
log off

#
# Access Point is up
#
ap-up:

ap-down:

