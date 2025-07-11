# This script is used to finalize the initialization and signal ready state

# set default user
set USER esp

ESC_BOLD=\033[1m
ESC_BRED=\033[91m
ESC_RESET=\033[0m
ESC_BLINK=\033[5m

cls
wlcm
info
fs
sw
echo
echo
echo "Enter $(ESC_BOLD)?$(ESC_RESET) to get help. Have a nice day :-)"

#
# Master console setup
#
ma:
prompt "$(USER)@serial:/> "
log info "System is ready!"
rm .safemode   # system start was successful, no safemode needed at next boot
test ! -f $(userscript).bak && cp $(userscript) $(userscript).bak

#
# Client console setup
#
cl:
log info "Client is ready!"
prompt -CL "$(USER)@$(HOSTNAME):/> "

#
# ESP in safemode
#
sm:
log warn "ESP in safemode"
exec $(userscript) sm
prompt "$(ESC_BOLD)$(USER)@serial-$(ESC_BRED)$(ESC_BLINK)SAFEMODE$(ESC_RESET):/> "
timer add 2s "led flash 100 100 2" tiSMLed repeat

sm-cl:
log warn "ESP in safemode"
prompt -CL "$(ESC_BOLD)$(USER)@$(HOSTNAME)-$(ESC_BRED)$(ESC_BLINK)SAFEMODE$(ESC_RESET):/> "
