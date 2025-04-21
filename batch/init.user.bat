# This script is a template for user specific configurations.
#
#
fs:

# set system variables.
set model xyz
set feature i2c
set hw ESP8266EX/80MHz/1M/64
set name "Test ESP"
set TZ CET-1CEST,M3.5.0,M10.5.0/3

led invert 1
led off

log server mac 8880

#gpio add 16 button btn 1 "relay rly toggle"    # add a button, gpio 16 (no interupt)
#gpio fn 16 Button # friendly name

# relay on gpio 0
#gpio add 0 relay rly 0 "gpio let led1 = rly; rc let 0 = rly" # rc: follow relay on ch 0
#gpio fn 0 Relay


#
# I2C
#
i2c:
#i2c setpins 12 14
#i2c init
#i2c scan
#i2c list

#sensor name 0 temperature
#sensor name 1 humidity
#sensor name 2 pressure

#
# MQTT
#
mqtt:
#mqtt server ocdk 8883
#mqtt qos 0
#mqtt root test
#mqtt name $name
#mqtt will 1
#mqtt heartbeat 1000

#
# Home Assistant
#
ha:
#ha sensor add temperature
#ha sensor add humidity
#ha sensor add pressure
#ha button add btn
#ha switch add rly

#
# Segment Display
#
seg:
#seg setpins 5 4
#seg init
#seg enable 1
#seg br 10
#seg screen add time time
#seg screen add temp sensor temperature
#seg screen add hum sensor humidity
#seg screen add pres sensor pressure
#seg slideshow add 0
#seg slideshow add 1
#seg slideshow add 2
#seg slideshow add 3
#seg slideshow on

#
# RC switch
#
rc:
#rc setpins -1 -1 # rx, tx
#rc ch 0 4198421 4198420 0   # channel, on-code, off-code, toggle
#rc ch 1 4210709 4210708 0
#rc fn S20 (Sonoff + RC) # friendly name
#rc enable 1
#rc init
#rc on

#
# Final init.
#
final:
wifi connect

#
# Wifi is up and connected
#
wifi-up:
log on
log info "wifi up"
set NTP fritz.box
#mqtt connect
#ha enable 1

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
log info "wifi online"

#
# wifi is offline
#
wifi-offline:
echo "wifi offline"
log off
