# This script is used to finalize the initialization and signal ready state
echo Finalizing initialization...

cls
wlcm
fs
sw
echo
echo
echo Enter ? to get help. Have a nice day :-)
prompt
log info System is ready!

exec user.bat rdy

rm .safemode  # system start was successful, no safemode needed at next boot


