# This script is a template for user specific configurations.
# Filename syntax for the project: init.<HOSTNAME>.bat
#
#
# Called after the filesystem has been initialized.
#
fs:

#led invert 1
led off
set NTP fritz.box
set TZ CET-1CEST,M3.5.0,M10.5.0/3
log server mac 8880

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
#mqtt server ocdk 8884
#mqtt qos 0
#mqtt root test
#mqtt name test
#mqtt will 1
#mqtt heartbeat 1000
#mqtt connect

#
# Home Assistant
#
ha:
#ha sensor add temperature
#ha sensor add humidity
#ha sensor add pressure
#ha button add btn
#ha switch add rly
#ha enable 1

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
# Called after the system has been initialized.
#
rdy:
