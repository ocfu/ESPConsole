# This script is used to finalize the initialization and signal ready state
userscript = init.$HOSTNAME.bat

set tz CET-1CEST,M3.5.0,M10.5.0/3

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
rm .safemode  # system start was successful, no safemode needed at next boot

#
# Client console setup
#
cl:
log info "Client is ready!"
prompt -CL "$USER@$HOSTNAME:/> "
