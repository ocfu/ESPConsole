#include "commands.h"

// Note: The help_ functions provide usage information for each command.
// If ESP_CONSOLE_FS is defined, help is available via man pages (file: man.man); otherwise, help_ functions are compiled and used for command help output.

#include "tools/espmath.h"  // for ExprParser

// Command function to reboot the device
void cmd_reboot(CxStrToken &tkArgs) {
   // force reboot
   if (tkArgs.indexOf("-f") != -1) {
      reboot();
   } else {
      // Optionally handle non-force reboot here
   }
}
void help_reboot() {
   __console.println(
       F("reboot [-f] : Reboot the device. Use -f for "
         "force."));
}

// Command function cls (clear screen)
void cmd_cls(CxStrToken &tkArgs) {
   __console.cls();
}

// Command prompt
void cmd_prompt(CxStrToken &tkArgs) {
   // prompt [-CL] [<prompt string>]
   // prompt [-OFF/ON]

   bool bClient = false;
   if (tkArgs.count() > 1) {
      String strPrompt;

      if (tkArgs.indexOf("-CL") > -1) {
         bClient = true;
      }

      if (tkArgs.indexOf("-OFF") > -1) {
         if (bClient) {
            __console.enableClientPrompt(false);
         } else {
            __console.enablePrompt(false);
         }
      } else if (tkArgs.indexOf("-ON") > -1) {
         if (bClient) {
            __console.enableClientPrompt(true);
         } else {
            __console.enablePrompt(true);
         }
      }

      // take the last token as the prompt string
      uint8_t i = tkArgs.count() - 1;

      if (TKTOCHAR(tkArgs, i)) {
         strPrompt.reserve(50);
         strPrompt = FMT_PROMPT_START;
         strPrompt += TKTOCHAR(tkArgs, i);

         // Perform esc code substitution
         strPrompt.replace("\\033", ESC_CODE);
         strPrompt.replace("\\0x1b", ESC_CODE);
         strPrompt.replace("\\0x1B", ESC_CODE);
         strPrompt += FMT_PROMPT_END;

         if (bClient) {
            __console.setPromptClient(strPrompt.c_str());
         } else {
            __console.setPrompt(strPrompt.c_str());
         }
      }
   }
   __console.prompt(bClient);
}
void help_prompt() {
   __console.println(
       F("prompt [-CL] [<prompt string>] : Set the command) "
         "prompt. Use -CL for "
         "client prompt."));
   __console.println(
       F("prompt [-OFF/ON] : Enable or disable the command "
         "prompt."));
}

// Command wlcm
void cmd_wlcm(CxStrToken &tkArgs) {
   __console.wlcm();
}

// Command info
void cmd_info(CxStrToken &tkArgs) {
   // Print system information
   if (tkArgs.count() > 1) {
      if (tkArgs.indexOf("reason") == 1) {
         __console.println(::getResetInfo());
         __console.setOutputVariable(::getResetInfo());
      } else if (tkArgs.indexOf("last") == 1) {
         if (!isQuiet()) {  // FIXME: workaroung as the
                            // print function ignores @echo
                            // off
            __console.setOutputVariable(__console.printStartTime(getIoStream()));
            __console.println();
         }
         __console.setOutputVariable(__console.getStartTime());
      } else if (tkArgs.indexOf("up") == 1) {
         __console.println((int32_t)__console.getUpTimeSeconds());
         __console.setOutputVariable((int32_t)__console.getUpTimeSeconds());
      }
   } else {
      printInfo();
      __console.println();
   }
}
void help_info() {
   __console.println(
       F("info [reason|last|up] : Print system "
         "information."));
   __console.println(F("  reason : Print the reset reason."));
   __console.println(F("  last   : Print the last start time."));
   __console.println(F("  up     : Print the uptime in seconds."));
}

// Command uptime
void cmd_uptime(CxStrToken &tkArgs) {
   // Print system uptime
   __console.printUptimeExt();  // FIXME: @echo off does not
                                // work on this print
   __console.println();
   __console.setOutputVariable(__console.getUpTimeISO());
}

// Command set
void cmd_set(CxStrToken &tkArgs) {
   // set <name>[[/<prec>] [<value>] | [ = <expr>]
   // examples:
   // set foo/2 42.12
   // set foo = 42
   // set foo/1 = 42.5 * $x
   // set foo "hello world"
   // Use of the equal sign expects a numeric expression. Strings are set without the equal sign.

   // MARK: room for improvement of mem usage here using standard c-strings vs. readability of the code

   String strVar = TKTOCHAR(tkArgs, 1);
   uint8_t prec = 0;
   bool bIsExpr = false;

   String strValue;
   strValue.reserve(128);
   // 128 is an estimate. The worst case is a concat
   // of two strings, while both could be a variable
   // with a max lenght of ?

   bool bSuccess = false;

   // Check if the operation is an assignment
   if (tkArgs.indexOf("=") == 2) {
      strValue = TKTOCHARAFTER(tkArgs, 3);
      bIsExpr = true;
   } else {
      strValue = TKTOCHARAFTER(tkArgs, 2);
   }

   // Check for precision
   int32_t nPrecIndex = strVar.indexOf("/");
   if (nPrecIndex > 0) {
      prec = strVar.substring(nPrecIndex + 1).toInt();
      strVar = strVar.substring(0, nPrecIndex);
   }

   // Check for system variables
   if (strVar == "TZ") {  // Time Zone
      __console.setTimeZone(strValue.c_str());
      __console.addVariable(strVar.c_str(), strValue.c_str());
      bSuccess = true;
   } else if (strVar == "BUF") {  // Input Buffer Size for the console
      uint32_t nBufLen = (uint32_t)strValue.toInt();
      if (nBufLen >= 64) {
         __console.setCmdBufferLen(nBufLen);
         __console.addVariable(strVar.c_str(), String(__console.getCmdBufferLen()).c_str());
         bSuccess = true;
      }
   } else if (strVar.length() > 0) {
      // user defined variables
      bool bValid = true;

      // Evaluate the expression
      if (bIsExpr) {
         ExprParser parser;
         float fValue = 0.0F;

         fValue = parser.eval(strValue.c_str(), bValid);

         if (bValid) {
            strValue = String(fValue, prec);
         } else {
            strValue = "nan";
         }
      }

      strValue.trim();

      // Add or remove the variable to the variable list
      if (strValue.length() > 0) {
         __console.addVariable(strVar.c_str(), strValue.c_str());
      } else {
         __console.removeVariable(strVar.c_str());
      }
      // "?" is the exit variable and shall not be set by the user
      if (strVar != "?") {
         bSuccess = bValid;
      }
   } else {
      // print all variables
      __console.printVariables(getIoStream());
      bSuccess = true;
   }
   if (!bSuccess) __console.setExitValue(EXIT_FAILURE);
}

// Command ps
void cmd_ps(CxStrToken &tkArgs) {
   // Print process statistics
   __console.printPs();
   __console.println();
}

// Command loopdelay
void cmd_loopdelay(CxStrToken &tkArgs) {
   if (tkArgs.count() > 1) {
      __console.setLoopDelay(TKTOINT(tkArgs, 1, 0));
   } else {
      __console.print(F("loopdelay = "));
      __console.println(__console.getLoopDelay());
      __console.setOutputVariable(__console.getLoopDelay());
   }
}

// Command delay
void cmd_delay(CxStrToken &tkArgs) {
   delay(TKTOINT(tkArgs, 1, 1));
}

// Command time
void cmd_time(CxStrToken &tkArgs) {
   if (__console.getStream()) __console.setOutputVariable(__console.printTime(*__console.getStream(), true));
   __console.println();
}

// Command date
void cmd_date(CxStrToken &tkArgs) {
   if (__console.getStream()) __console.setOutputVariable(__console.printDate(*__console.getStream()));
   __console.println();
}

// Command heap
void cmd_heap(CxStrToken &tkArgs) {
   printHeap();
   __console.println();
}

// Command frag
void cmd_frag(CxStrToken &tkArgs) {
   printHeapFragmentation();
   __console.println();
   __console.setOutputVariable((uint32_t)g_Heap.fragmentation());
}

// Command stack
void cmd_stack(CxStrToken &tkArgs) {
   if (tkArgs.count() > 1) {
      if (tkArgs.indexOf("on") == 1) {
         g_Stack.enableDebugPrint(true);
         return;
      } else if (tkArgs.indexOf("off") == 1) {
         g_Stack.enableDebugPrint(false);
         return;
      } else if (tkArgs.indexOf("free") == 1) {
         __console.setOutputVariable((uint32_t)g_Stack.getFree());
         return;
      }
   } else {
      // No specific sub-command given, print stack info and set output variable
      if (!isQuiet()) {  // FIXME: workaround, as the print
                         // function ignores
                         // @echo off
         g_Stack.print(getIoStream());
      }
      __console.setOutputVariable((uint32_t)g_Stack.getFree());
   }
}

// Command users
void cmd_users(CxStrToken &tkArgs) {
   __console.printf(F("%d users\n"), __console.users());
   __console.setOutputVariable(__console.users());
}

// Command usr
void cmd_usr(CxStrToken &tkArgs) {
   // set user specific commands here. The first parameter
   // is the command number, the second the flag and the
   // optional third how to set / clear.
   //(0: clear flag, 1: set flag, default (-1): set the flag
   // as value.)
   // usr <cmd> [<flag/value> [<0|1>]]

   int32_t nCmd = TKTOINT(tkArgs, 1, -1);
   uint32_t nValue = TKTOINT(tkArgs, 2, 0);
   int8_t set = TKTOINT(tkArgs, 3, -1);

   int nClient = 0;  // FIXME: use the client id from the console

   CxESPConsole &con = __console.getConsole(nClient);

   switch (nCmd) {
         // usr 0: be quite, switch all loggings off on the
         // console. log to server/file remains
      case 0:
         con.setUsrLogLevel(LOGLEVEL_OFF);
         break;

         // usr 1: set the usr log level to show logs on the
         // console
      case 1:
         if (nValue) {
            con.setUsrLogLevel(nValue > LOGLEVEL_MAX ? LOGLEVEL_MAX : nValue);
         } else {
            __console.printf(F("usr log level: %d\n"), con.getUsrLogLevel());
         }
         break;

         // usr 2: set extended debug flag
      case 2:
         if (set < 0) {
            con.setDebugFlag(nValue);
         } else if (set == 0) {
            con.resetDebugFlag(nValue);
         } else {
            con.setDebugFlag(con.getDebugFlag() | nValue);
         }
         if (con.getDebugFlag()) {
            con.setUsrLogLevel(LOGLEVEL_DEBUG_EXT);
         }
         break;
   }
}
void help_usr() {
   __console.println(
       F("usr <cmd> [<flag/value> [<0|1>]] : User specific "
         "commands."));
   __console.println(F(" 0           Set log level to OFF (quiet mode)."));
   __console.println(
       F(" 1  <1..5>   Set the log level to show log "
         "messages on the console."));
   __console.println(
       F(" 2  <flag>   Set the extended debug flag(s) to "
         "the value."));
   __console.println(F(" 2  <flag> 0 clear an extended debug flag."));
   __console.println(F(" 2  <flag> 1 add an extended debug flag."));
}

// Command echo
void cmd_echo(CxStrToken &tkArgs) {
   String strValue;
   bool bSuppressNewLine = false;

   int i = 1;
   while (i < 7) {  // tkArgs can max. hold 8 tokens, the
                    // first token is the command
      strValue = TKTOCHAR(tkArgs, i);

      if (strValue == "-n") {
         bSuppressNewLine = true;
      } else {
         // Substitue with global variables
         __console.substituteVariables(strValue);

         // Stop if argument is empty
         if (strValue.length() == 0) {
            break;
         }

         // Perform esc code substitution
         strValue.replace("\\033", ESC_CODE);
         strValue.replace("\\0x1b", ESC_CODE);
         strValue.replace("\\0x1B", ESC_CODE);
         __console.print(strValue.c_str());
         if (i < (tkArgs.count() - 1)) __console.print(" ");
      }
      i++;
   }
   if (!bSuppressNewLine) __console.println();
}

// Command @echo
void cmd_echo_off(CxStrToken &tkArgs) {
   if (tkArgs.count() > 1) {
      if (strncmp(TKTOCHAR(tkArgs, 1), "off", 3) == 0) {
         __console.setEcho(false);
      } else if (strncmp(TKTOCHAR(tkArgs, 1), "on", 2) == 0) {
         __console.setEcho(true);
      }
   }
}

// Command timer
void cmd_timer(CxStrToken &tkArgs) {
   // timer add <period>[unit]|<cron> <cmd> [<id> [<mode>]]
   // timer del [id]
   // cmd: command to execute
   // id: id of the timer. without id the timer runs one
   // time, with id the timer repeats
   // cron example: "0 0 * * *"
   // time example: "1000", "1s", "1d"

   if (tkArgs.count() > 1) {
      if (tkArgs.indexOf("add") == 1) {
         // add a timer
         if (tkArgs.count() > 3) {
            String strTime = TKTOCHAR(tkArgs, 2);
            bool bCron = (strTime.indexOf(' ') != -1);
            uint32_t nPeriod = __console.convertToMilliseconds(strTime.c_str());
            uint8_t nMode = 0;  // once
            if (bCron || (nPeriod > 100 && nPeriod <= 7 * 24 * 3600 * 1000)) {
               CxTimer *pTimer;
               if (bCron) {
                  pTimer = new CxTimerCron(strTime.c_str());
               } else {
                  pTimer = new CxTimer();
               }
               if (pTimer) {
                  if (TKTOCHAR(tkArgs, 4)) {  // id
                     pTimer->setId(TKTOCHAR(tkArgs, 4));
                     nMode = bCron ? 2 : 1;  // id is set -> repeat timer
                  }

                  if (TKTOCHAR(tkArgs, 5)) {  // mode
                     if (tkArgs.indexOf("once") == 5) nMode = 0;
                     if (tkArgs.indexOf("repeat") == 5) nMode = 1;
                     if (tkArgs.indexOf("replace") == 5) {
                        CxTimer *p = __console.getTimer(TKTOCHAR(tkArgs, 4));
                        if (p) {
                           nMode = p->getMode();
                           __console.delTimer(TKTOCHAR(tkArgs, 4));
                        }
                     }
                  }

                  if (__console.addTimer(pTimer)) {
                     if (bCron) {
                        __console.info(F("add timer %s, at %s, cmd %s"), pTimer->getId(), strTime.c_str(), TKTOCHAR(tkArgs, 3));
                     } else {
                        __console.info(F("add timer %s, period %d ms, mode %d, cmd %s"), pTimer->getId(), nPeriod, nMode, TKTOCHAR(tkArgs, 3));
                     }
                     pTimer->setCmd(TKTOCHAR(tkArgs, 3));
                     pTimer->start(nPeriod, [pTimer](const char *szCmd) {
                         __console.processCmd(szCmd);
                         // if the timer is set to once,
                         // remove it
                         if (pTimer->getMode() == 0) {
                            __console.info(F("timer %s expired, "
                                             "removing"),
                                           pTimer->getId());
                            __console.delTimer(pTimer->getId());
                         } }, (nMode == 0));
                  } else {
                     __console.error(F("could not add timer %s! (existing or too many timers)"), pTimer->getId());
                     delete pTimer;
                     __console.setExitValue(EXIT_FAILURE);
                  }
               }
            } else {
               __console.error(F("invalid time for timer"));
               __console.setExitValue(EXIT_FAILURE);
            }
         } else {
            __console.error(F("not enough arguments for timer add!"));
            __console.setExitValue(EXIT_FAILURE);
         }  // count
      } else if (tkArgs.indexOf("del") == 1) {
         // Handle del sub-command
         __console.delTimer(TKTOCHAR(tkArgs, 2));
      } else if (tkArgs.indexOf("stop") == 1) {
         // Handle stop sub-command
         __console.stopTimer(TKTOCHAR(tkArgs, 2));
      } else if (tkArgs.indexOf("start") == 1) {
         // Handle start sub-command
         __console.startTimer(TKTOCHAR(tkArgs, 2));
      } else if (tkArgs.indexOf("list") == 1) {
         // Handle list sub-command
         __console.printTimers(getIoStream());
      }
   }
}

// Command hw
void cmd_hw(CxStrToken &tkArgs) {
   __console.printf(F(ESC_ATTR_BOLD "    Chip Type:" ESC_ATTR_RESET " %s " ESC_ATTR_BOLD "Chip-ID: " ESC_ATTR_RESET "0x%X\n"), getChipType(), getChipId());
#ifdef ARDUINO
   __console.printf(F(ESC_ATTR_BOLD "   Flash Size:" ESC_ATTR_RESET " %dk (real) %dk (ide)\n"), getFlashChipRealSize() / 1024, getFlashChipSize() / 1024);
   __console.printf(F(ESC_ATTR_BOLD "Chip-Frequenz:" ESC_ATTR_RESET " %dMHz\n"), ESP.getCpuFreqMHz());
#endif
   __console.setOutputVariable(getChipType());
}

// Command id
void cmd_id(CxStrToken &tkArgs) {
   // Print the device ID
   __console.printf(F("Device ID: 0x%X\n"), getChipId());
   __console.setOutputVariable(getChipId());
}

// Command sw
void cmd_sw(CxStrToken &tkArgs) {
   // Print the software version
#ifdef ARDUINO
   __console.printf(F(ESC_ATTR_BOLD "   Plattform:" ESC_ATTR_RESET " %s"), ARDUINO_BOARD);
   __console.printf(F(ESC_ATTR_BOLD " Core:" ESC_ATTR_RESET " %s\n"), ESP.getCoreVersion().c_str());
   __console.printf(F(ESC_ATTR_BOLD "    SDK:" ESC_ATTR_RESET " %s"), ESP.getSdkVersion());

#ifdef ARDUINO_CLI_VER
   int arduinoVersion = ARDUINO_CLI_VER;
   const char *ide = "(cli)";
#else
   int arduinoVersion = ARDUINO;
   const char *ide = "(ide)";
#endif
   int major = arduinoVersion / 10000;
   int minor = (arduinoVersion / 100) % 100;
   int patch = arduinoVersion % 100;
   __console.printf(F(ESC_ATTR_BOLD " Arduino:" ESC_ATTR_RESET " %d.%d.%d %s\n"), major, minor, patch, ide);
#endif
   __console.printf(F(ESC_ATTR_BOLD "    Firmware:" ESC_ATTR_RESET " %s" ESC_ATTR_BOLD " Ver.:" ESC_ATTR_RESET " %s"), __console.getAppName(), __console.getAppVer());
   if (g_szBuildId && g_szBuildId[0]) {
      __console.printf(F(ESC_ATTR_BOLD " (" ESC_ATTR_RESET "%s)"), g_szBuildId);
   }
#ifdef DEBUG
   __console.printf(F(ESC_ATTR_BOLD ESC_TEXT_RED " DBG" ESC_ATTR_RESET));
#endif
#ifdef ARDUINO
   __console.printf(F(ESC_ATTR_BOLD " Sketch size: " ESC_ATTR_RESET));
   if (ESP.getSketchSize() / 1024 < 465) {
      __console.printf(F("%d kBytes\n"), ESP.getSketchSize() / 1024);
   } else if (ESP.getSketchSize()) {
      __console.printf(F(ESC_TEXT_BRIGHT_YELLOW ESC_ATTR_BOLD "%d kBytes\n"), ESP.getSketchSize() / 1024);
   } else if (getFreeOTA() < ESP.getSketchSize()) {
      __console.printf(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BOLD "%d kBytes\n"), ESP.getSketchSize() / 1024);
   }
   __console.print(ESC_ATTR_RESET);
#else
   __console.println();
#endif
   __console.setOutputVariable(__console.getAppVer());
}

// Command app
void cmd_app(CxStrToken &tkArgs) {
   // Print the application name
   __console.printf(F("Application Name: %s\n"), __console.getAppName());
   __console.setOutputVariable(__console.getAppName());
}

// Command esp
void cmd_esp(CxStrToken &tkArgs) {
   // Print ESP specific information
#ifdef DEBUG
#ifdef ARDUINO
#ifdef ESP32
   // TODO: get real flash size for esp32
   uint32_t realSize = ESP.getFlashChipSize();
#else
   uint32_t realSize = ESP.getFlashChipRealSize();
#endif
   uint32_t ideSize = ESP.getFlashChipSize();
   FlashMode_t ideMode = ESP.getFlashChipMode();

   __console.printf(F("-CPU--------------------\n"));
#ifdef ESP32
   __console.printf(F("ESP:          %s\n"), "ESP32");
#else
   __console.printf(F("ESP:          %s\n"), getChipType());
#endif
   __console.printf(F("Freq:         %d MHz\n"), ESP.getCpuFreqMHz());
   __console.printf(F("ChipId:       %X\n"), getChipId());
#ifdef ESP_CONSOLE_WIFI
   __console.printf(F("MAC:          %s\n"), WiFi.macAddress().c_str());
#endif
   __console.printf(F("\n"));
#ifdef ESP32
   __console.printf(F("-FLASH------------------\n"));
#else
   if (is_8285()) {
      __console.printf(F("-FLASH-(embeded)--------\n"));
   } else {
      __console.printf(F("-FLASH------------------\n"));
   }
#endif
#ifdef ESP32
   __console.printf(F("Vendor:       unknown\n"));
#else
   __console.printf(F("Vendor:       0x%X\n"), ESP.getFlashChipVendorId());  // complete list in spi_vendors.h
#ifdef PUYA_SUPPORT
   if (ESP.getFlashChipVendorId() == SPI_FLASH_VENDOR_PUYA) __console.printf(F("Puya support: Yes\n"));
#else
   __console.printf(F("Puya support: No\n"));
   if (ESP.getFlashChipVendorId() == SPI_FLASH_VENDOR_PUYA) {
      __console.printf(F("WARNING: #### vendor is PUYA, FLASHFS will fail, if you don't define -DPUYA_SUPPORT (ref. esp8266/Arduino #6221)\n"));
   }
#endif
#endif
   __console.printf(F("Size (real):  %d kBytes\n"), realSize / 1024);
   __console.printf(F("Size (comp.): %d kBytes\n"), ideSize / 1024);
   if (realSize != ideSize) {
      __console.printf(F("### compiled size differs from real chip size\n"));
   }
   // __console.printf(F("CRC ok:       %d\n"),ESP.checkFlashCRC());
   __console.printf(F("Freq:         %d MHz\n"), ESP.getFlashChipSpeed() / 1000000);
   __console.printf(F("Mode (ide):   %s\n"), ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT"
                                                                     : ideMode == FM_DIO    ? "DIO"
                                                                     : ideMode == FM_DOUT   ? "DOUT"
                                                                                            : "UNKNOWN");
#ifdef ESP32
   __console.printf(F("Size Map:     unknown\n"));
#else
   __console.printf(F("Size Map:     %s\n"), getMapName());
#endif
   __console.printf(F("Size avail.:  %5d kBytes\n"), (ESP.getSketchSize() + ESP.getFreeSketchSpace()) / 1024);
   __console.printf(F("     sketch:  %5d kBytes\n"), (ESP.getSketchSize() / 1024));
   __console.printf(F("       free:  %5d kBytes\n"), (ESP.getFreeSketchSpace() / 1024));
#ifdef ESP32
   __console.printf(F("   OTA room:  ? Bytes\n"));
#else
   __console.printf(F("   OTA room:  %5d kBytes\n"), getFreeOTA() / 1024);
   if (getFreeOTA() < ESP.getSketchSize()) {
      __console.printf(F("*** Free room for OTA too low!\n"));
   } else if (getFreeOTA() < (ESP.getSketchSize() + 10000)) {
      __console.printf(F("vvv Free room for OTA is getting low!\n"));
   }
   __console.printf(F("FLASHFS size: %5d kBytes\n"), getFSSize() / 1024);
#endif
   __console.printf(F("\n"));
   __console.printf(F("-FIRMWARE---------------\n"));
#ifdef ESP32
   // TODO: implement esp core version for esp32
   __console.printf(F("ESP core:     unknown\n"));
#else
   __console.printf(F("ESP core:     %s\n"), ESP.getCoreVersion().c_str());
#endif
   __console.printf(F("ESP sdk:      %s\n"), ESP.getSdkVersion());
   __console.printf(F("Application:  %s (%s)\n"), __console.getAppName(), __console.getAppVer());
   __console.printf(F("\n"));
   __console.printf(F("-BOOT-------------------\n"));
   __console.printf(F("reset reason: %s\n"), getResetInfo());
   __console.print(F("time to boot: "));
   __console.printTimeToBoot(getIoStream());
   __console.println();
   __console.printf(F("free heap:    %5d Bytes\n"), ESP.getFreeHeap());
   __console.printf(F("\n"));
#ifdef ESP32
   // TODO: implement esp core version for esp32
#else
   __console.setOutputVariable(ESP.getCoreVersion().c_str());
#endif
#endif /* ARDUINO */
#endif /* DEBUG */
}

// Command flash
void cmd_flash(CxStrToken &tkArgs) {
   // Print flash information
#ifdef DEBUG
#ifdef ARDUINO
   __console.printf(F("-FLASHMAP---------------\n"));
#ifdef ESP32
   __console.printf(F("Size:         %d kBytes (0x%X)\n"), ESP.getFlashChipSize() / 1024, ESP.getFlashChipSize());
#else
   __console.printf(F("Size:         %d kBytes (0x%X)\n"), ESP.getFlashChipRealSize() / 1024, ESP.getFlashChipRealSize());
#endif
   __console.printf(F("\n"));
#ifdef ESP32
   __console.printf(F("ESP32 Partition table:\n\n"));
   __console.printf(F("| Type | Sub |  Offset  |   Size   |       Label      |\n"));
   __console.printf(F("| ---- | --- | -------- | -------- | ---------------- |\n"));
   esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
   if (pi != NULL) {
      do {
         const esp_partition_t *p = esp_partition_get(pi);
         __console.printf(F("|  %02x  | %02x  | 0x%06X | 0x%06X | %-16s |\n"),
                          p->type, p->subtype, p->address, p->size, p->label);
      } while (pi = (esp_partition_next(pi)));
   }
#else
   __console.printf(F("Sketch start: %X\n"), getSketchStart());
   __console.printf(F("Sketch end:   %X (%d kBytes)\n"), getSketchStart() + ESP.getSketchSize() - 0x1, ESP.getSketchSize() / 1024);
   __console.printf(F("OTA start:    %X (lowest possible addr.)\n"), getOTAStart());
   __console.printf(F("OTA end:      %X (%d kBytes available)\n"), getOTAEnd(), getFreeOTA() / 1024);
   if (getFlashFSStart() < getWIFIEnd()) {
      __console.printf(F("FLASHFS start: %X\n"), getFlashFSStart());
      __console.printf(F("FLASHFS end:   %X (%d kBytes)\n"), getFlashFSEnd() - 0x1, (getFlashFSEnd() - getFlashFSStart()) / 1024);
   }
   __console.printf(F("EPPROM start: %X\n"), getEPROMStart());
   __console.printf(F("EPPROM end:   %X (%d kBytes)\n"), getEPROMEEnd() - 0x1, (getEPROMEEnd() - getEPROMStart()) / 1024);
   __console.printf(F("RFCAL start:  %X\n"), getRFCALStart());
   __console.printf(F("RFCAL end:    %X (%d kBytes)\n"), getRFCALEnd() - 0x1, (getRFCALEnd() - getRFCALStart()) / 1024);
   __console.printf(F("WIFI start:   %X\n"), getWIFIStart());
   __console.printf(F("WIFI end:     %X (%d kBytes)\n"), getWIFIEnd() - 0x1, (getWIFIEnd() - getWIFIStart()) / 1024);
   if (getFlashFSStart() >= getWIFIEnd()) {
      __console.printf(F("FS start:     %X"), getFlashFSStart());
      __console.println();
      __console.printf(F("FS end:       %X (%d kBytes)"), getFlashFSEnd() - 0x1, (getFlashFSEnd() - getFlashFSStart()) / 1024);
   }

#endif
   __console.printf(F("\n"));
   __console.printf(F("------------------------\n"));
   __console.setOutputVariable(ESP.getFlashChipSize() / 1024);
#endif  // ARDUINO
#endif  /* DEBUG */
}

// Command eeprom
void cmd_eeprom(CxStrToken &tkArgs) {
   // Print EEPROM information
   if (TKTOCHAR(tkArgs, 1)) {
      ::printEEPROM(getIoStream(), TKTOINT(tkArgs, 1, 0), TKTOINT(tkArgs, 2, 128));
   }
}
void help_eeprom() {
   __console.println(F("eeprom [<start> [<len>]] : Print EEPROM content."));
   __console.println(F("  start : Start address in EEPROM (default: 0)."));
   __console.println(F("  len   : Length to print (default: 128)."));
}

// Command table in PROGMEM
const CommandEntry commands[] PROGMEM = {
    {"reboot", cmd_reboot, HELP_OR_NULLPTR(help_reboot)},
    {"cls", cmd_cls, nullptr},
    {"prompt", cmd_prompt, HELP_OR_NULLPTR(help_prompt)},
    {"wlcm", cmd_wlcm, nullptr},
    {"info", cmd_info, HELP_OR_NULLPTR(help_info)},
    {"uptime", cmd_uptime, nullptr},
    {"set", cmd_set, nullptr},
    {"ps", cmd_ps, nullptr},
    {"loopdelay", cmd_loopdelay, nullptr},
    {"delay", cmd_delay, nullptr},
    {"time", cmd_time, nullptr},
    {"date", cmd_date, nullptr},
    {"heap", cmd_heap, nullptr},
    {"frag", cmd_frag, nullptr},
    {"stack", cmd_stack, nullptr},
    {"users", cmd_users, nullptr},
    {"usr", cmd_usr, help_usr},
    {"echo", cmd_echo, nullptr},
    {"@echo", cmd_echo_off, nullptr},
    {"timer", cmd_timer, nullptr},
    {"hw", cmd_hw, nullptr},
    {"id", cmd_id, nullptr},
    {"sw", cmd_sw, nullptr},
    {"app", cmd_app, nullptr},
    {"esp", cmd_esp, nullptr},
    {"flash", cmd_flash, nullptr},
    {"eeprom", cmd_eeprom, HELP_OR_NULLPTR(help_eeprom)},
    // Add more commands here
};

const size_t NUM_COMMANDS = sizeof(commands) / sizeof(commands[0]);

void printCommands(const CommandEntry *cmds, const size_t numCmds, const char *title = nullptr) {
   if (title) {
      __console.printf(ESC_ATTR_BOLD "%s: " ESC_ATTR_RESET, title);
   }
   for (size_t i = 0; i < numCmds; ++i) {
      char cmdName[MAX_COMMAND_NAME_LENGTH];
#if defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
      if (!pgm_read_ptr(&cmds[i].name)) continue;  // Skip empty entries
      // Read command name from PROGMEM
      strncpy_P(cmdName, (PGM_P)pgm_read_ptr(&cmds[i].name), sizeof(cmdName) - 1);
      cmdName[sizeof(cmdName) - 1] = '\0';
#else
      if (!cmds[i].name) continue;  // Skip empty entries
      // Read command name from PROGMEM
      strncpy(cmdName, cmds[i].name, sizeof(cmdName) - 1);
      cmdName[sizeof(cmdName) - 1] = '\0';
#endif
      if (cmdName[0] != '\0' && cmdName[0] != '$') {  // Skip empty or special commands
         if (i > 0) {
            __console.print(F(","));
         }
         __console.printf(F(" %s"), cmdName);
      }
   }
   __console.println();
}

void printHelp(const char *cmd, const CommandEntry *cmds, const size_t numCmds) {
   // Print help for a specific command
   for (size_t i = 0; i < numCmds; ++i) {
      char cmdName[MAX_COMMAND_NAME_LENGTH];
#if defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
      if (!pgm_read_ptr(&cmds[i].name)) continue;  // Skip empty entries
      // Read command name from PROGMEM
      strncpy_P(cmdName, (PGM_P)pgm_read_ptr(&cmds[i].name), sizeof(cmdName) - 1);
      cmdName[sizeof(cmdName) - 1] = '\0';
#else
      if (!cmds[i].name) continue;  // Skip empty entries
      // Read command name from PROGMEM
      strncpy(cmdName, cmds[i].name, sizeof(cmdName) - 1);
      cmdName[sizeof(cmdName) - 1] = '\0';
#endif
      if (strcmp(cmd, cmdName) == 0) {
#if defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
         HelpFunc help = (HelpFunc)pgm_read_ptr(&cmds[i].help);
#else
         HelpFunc help = cmds[i].help;
#endif
         if (help) {
            help();
         } else {
#ifdef ESP_CONSOLE_FS
            __console.man(cmd);
#else
            __console.println(F("No help available."));
#endif
         }
         return;
      }  // Command found
   }  // Loop through commands
   __console.println(F("Unknown command."));
}  // printHelp

bool executeInTable(const char *cmd, CxStrToken &tkArgs, const CommandEntry *cmds, size_t numCmds) {
   for (size_t i = 0; i < numCmds; ++i) {
      char entryName[MAX_COMMAND_NAME_LENGTH];
#if defined(ARDUINO) || defined(ESP8266) || defined(ESP32)
      strcpy_P(entryName, (PGM_P)pgm_read_ptr(&cmds[i].name));
#else
      strcpy(entryName, cmds[i].name);
#endif
      if (strcmp(cmd, entryName) == 0) {
         if (tkArgs.indexOf("-h") != -1) {
            printHelp(entryName, cmds, numCmds);
            return true;
         }
#ifdef ARDUINO
         CommandFunc func = (CommandFunc)pgm_read_ptr(&cmds[i].func);
#else
         CommandFunc func = cmds[i].func;
#endif
         __console.setExitValue(EXIT_SUCCESS);  // set exit value to success by default
         func(tkArgs);                          // Call the command function, a failed exit value will be set by the command function
         return true;                           // Command found and executed
      }
   }
   return false;
}

bool execute(const char *szCmd, uint8_t nClient) {
   // validate the call
   if (!szCmd) return false;

   // get the arguments into the token buffer
   CxStrToken tkArgs(szCmd, " ");

   // we have a command, find the action to take
   const char *cmd = TKTOCHAR(tkArgs, 0);

   if (!cmd || cmd[0] == '\0') {
      return true;
   }

   if (tkArgs.indexOf("?") != -1 || tkArgs.indexOf("help") != -1) {
      __console.println(F(ESC_ATTR_BOLD "Available commands:" ESC_ATTR_RESET));
      printCommands(commands, NUM_COMMANDS, " Core");
#ifdef ESP_CONSOLE_WIFI
      printCommands(commandsWiFi, NUM_COMMANDS_WIFI, " WiFi");
#endif /* ESP_CONSOLE_WIFI */
#ifdef ESP_CONSOLE_EXT
      printCommands(commandsExt, NUM_COMMANDS_EXT, " Extended");
#endif /* ESP_CONSOLE_EXT */
#ifdef ESP_CONSOLE_FS
      printCommands(commandsFS, NUM_COMMANDS_FS, " Filesystem");
#endif /* ESP_CONSOLE_FS */
#ifdef ESP_CONSOLE_I2C
      printCommands(commandsI2C, NUM_COMMANDS_I2C, " I2C");
#endif /* ESP_CONSOLE_I2C */

      return true;
   }

   // Try core commands
   if (executeInTable(cmd, tkArgs, commands, NUM_COMMANDS)) {
      return true;
   }
#ifdef ESP_CONSOLE_WIFI
   // Try WiFi commands
   if (executeInTable(cmd, tkArgs, commandsWiFi, NUM_COMMANDS_WIFI)) {
      return true;
   }
#endif /* ESP_CONSOLE_WIFI */
#ifdef ESP_CONSOLE_EXT
   // Try extended commands
   if (executeInTable(cmd, tkArgs, commandsExt, NUM_COMMANDS_EXT)) {
      return true;
   }
#endif /* ESP_CONSOLE_EXT */
#ifdef ESP_CONSOLE_FS
   // Try filesystem commands
   if (executeInTable(cmd, tkArgs, commandsFS, NUM_COMMANDS_FS)) {
      return true;
   }
#endif /* ESP_CONSOLE_FS */
#ifdef ESP_CONSOLE_I2C
   // Try I2C commands
   if (executeInTable(cmd, tkArgs, commandsI2C, NUM_COMMANDS_I2C)) {
      return true;
   }
#endif /* ESP_CONSOLE_I2C */
   return false;
}

void printInfo() {
#ifdef ESP_CONSOLE_WIFI
   __console.print(F(ESC_ATTR_BOLD "  Hostname: " ESC_ATTR_RESET));
   printHostName();
   __console.printf(F(ESC_ATTR_BOLD " IP: " ESC_ATTR_RESET));
   printIp();
   __console.printf(F(ESC_ATTR_BOLD " SSID: " ESC_ATTR_RESET));
   printSSID();
   __console.println();
#endif  // ESP_CONSOLE_NOWIFI
   __console.print(F(ESC_ATTR_BOLD "    Uptime: " ESC_ATTR_RESET));
   __console.printUpTimeISO(getIoStream());
   __console.printf(F(" - %d user(s)"), __console.users());
   __console.printf(F(ESC_ATTR_BOLD " Last Restart: " ESC_ATTR_RESET));
   __console.printStartTime(getIoStream());
   __console.println();
   printHeap();
   __console.println();
   __console.print(F("    "));
   g_Stack.print(getIoStream());
}

void printHeap() {
   __console.print(F(ESC_ATTR_BOLD " Heap Size: " ESC_ATTR_RESET));
   printHeapSize();
   __console.print(F(" bytes"));
   __console.print(F(ESC_ATTR_BOLD " Used: " ESC_ATTR_RESET));
   printHeapUsed();
   __console.print(F(" bytes"));
   __console.print(F(ESC_ATTR_BOLD " Free: " ESC_ATTR_RESET));
   printHeapAvailable();
   __console.print(F(" bytes"));
   __console.print(F(ESC_ATTR_BOLD " Low: " ESC_ATTR_RESET));
   printHeapLow();
   __console.print(F(" bytes"));
   __console.print(F(ESC_ATTR_BOLD " Fragm.: " ESC_ATTR_RESET));
   printHeapFragmentation();
   __console.print(F(" % (peak: "));
   printHeapFragmentationPeak();
   __console.print(F(("%)")));
   __console.setOutputVariable((uint32_t)g_Heap.available());
}

void printHeapAvailable(bool fmt) {
   if (g_Heap.available() < 10000) __console.print(F(ESC_TEXT_BRIGHT_YELLOW));
   if (g_Heap.available() < 3000) __console.print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
   if (fmt) {
      __console.printf(F("%7lu"), g_Heap.available());
   } else {
      __console.printf(F("%lu"), g_Heap.available());
   }
   __console.print(F(ESC_ATTR_RESET));
}

void printHeapLow(bool fmt) {
   if (g_Heap.available() < 10000) __console.print(F(ESC_TEXT_BRIGHT_YELLOW));
   if (g_Heap.available() < 3000) __console.print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
   if (fmt) {
      __console.printf(F("%7lu"), g_Heap.low());
   } else {
      __console.printf(F("%lu"), g_Heap.low());
   }
   __console.print(F(ESC_ATTR_RESET));
}

void printHeapSize(bool fmt) {
   if (fmt) {
      __console.printf(F("%7lu"), g_Heap.size());
   } else {
      __console.printf(F("%lu"), g_Heap.size());
   }
}

void printHeapUsed(bool fmt) {
   if (fmt) {
      __console.printf(F("%7lu"), g_Heap.used());
   } else {
      __console.printf(F("%lu"), g_Heap.used());
   }
}

void printHeapFragmentation(bool fmt) {
   if (fmt) {
      __console.printf(F("%7lu"), g_Heap.fragmentation());
   } else {
      __console.printf(F("%lu"), g_Heap.fragmentation());
   }
}

void printHeapFragmentationPeak(bool fmt) {
   if (fmt) {
      __console.printf(F("%7lu"), g_Heap.peak());
   } else {
      __console.printf(F("%lu"), g_Heap.peak());
   }
}

Stream &getIoStream() {
   if (__console.getStream()) {
      return *__console.getStream();
   } else {
      return Serial;
   }
}

bool isQuiet() {
   return false;  // TODO: implement quiet mode
}

void reboot() {
   __console.warn(F("reboot..."));
#ifdef ARDUINO
   delay(1000);  // let some time to handle last network
                 // messages
#ifdef ESP_CONSOLE_WIFI
   WiFi.disconnect();
#endif
   ESP.restart();
#endif
}
