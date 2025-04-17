# This script is used to finalize the initialization and signal ready state
userscript = init.$HOSTNAME.bat

set TZ CET-1CEST,M3.5.0,M10.5.0/3

set USER esp

cls
wlcm
info
fs
sw
echo
echo
echo "Enter ? to get help. Have a nice day :-)"

#
# Master console setup
#
ma:
prompt "$USER@serial:/> "
log info "System is ready!"
exec $userscript rdy
set userscript # removes temp variable
rm .safemode   # system start was successful, no safemode needed at next boot

#
# Client console setup
#
cl:
set userscript # removes temp variable
log info "Client is ready!"
prompt -CL "$USER@$HOSTNAME:/> "
