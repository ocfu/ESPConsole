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
rm .safemode   # system start was successful, no safemode needed at next boot
test ! -f $(userscript).bak && test -f $(userscript) && cp $(userscript) $(userscript).bak
syslog "$(APPNAME) $(APPVER) started" 

#
# Client console setup
#
cl:
prompt -CL "$(USER)@$(HOSTNAME):/> "
syslog "$(APPNAME) $(APPVER) client started" 

#
# ESP in safemode
#
sm:
syslog "ESP in safemode" -s 4 
exec $(userscript) sm
prompt "$(ESC_BOLD)$(USER)@serial-$(ESC_BRED)$(ESC_BLINK)SAFEMODE$(ESC_RESET):/> "
timer add 2s "led flash 100 100 2" tiSMLed repeat

sm-cl:
syslog "ESP in safemode" -s 4 
prompt -CL "$(ESC_BOLD)$(USER)@$(HOSTNAME)-$(ESC_BRED)$(ESC_BLINK)SAFEMODE$(ESC_RESET):/> "
