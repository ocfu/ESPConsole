# This script is used to set up user specific configurations.

#
# Called after the filesystem has been initialized.
#
fs:
echo -fs------------------------
stack
echo ---------------------------

led invert 1
led off

set ntp fritz.box
set tz CET-1CEST,M3.5.0,M10.5.0/3

log server mac 8880

gpio add 5 button btn 1 relay rly toggle    # add a button
gpio fn 5 Button # friendly name

gpio add 4 relay rly 0 gpio let led1 = rly
gpio fn 4 Relay

echo -fs------------------------
stack
echo ---------------------------

#
# I2C
#
i2c:

echo -i2c-----------------------
stack
echo ---------------------------

i2c setpins 12 14
i2c init
i2c scan
i2c list

sensor name 0 temperature
sensor name 1 humidity
sensor name 2 pressure

echo -i2c-----------------------
stack
echo ---------------------------

#
# MQTT
#
mqtt:

echo -mqtt------------------------
stack
echo ---------------------------

mqtt server ocdk 8884
mqtt qos 0
mqtt root test
mqtt name test
mqtt will 1
mqtt heartbeat 1000
mqtt connect

echo -mqtt----------------------
stack
echo ---------------------------


#
# Home Assistant
#
ha:
echo -ha------------------------
stack
echo ---------------------------

ha sensor add temperature
ha sensor add humidity
ha sensor add pressure
ha button add btn
ha switch add rly
ha enable 1

echo -ha------------------------
stack
echo ---------------------------


#
# Segment Display
#
seg:
echo -seg-----------------------
stack
echo ---------------------------

seg setpins 5 4
seg init
seg enable 1
seg br 10
seg screen add time time
seg screen add temp sensor temperature
seg screen add hum sensor humidity
seg screen add pres sensor pressure
seg slideshow add 0
seg slideshow add 1
seg slideshow add 2
seg slideshow add 3
seg slideshow on

echo -seg-----------------------
stack
echo ---------------------------


#
# Called after the system has been initialized.
#
rdy:
