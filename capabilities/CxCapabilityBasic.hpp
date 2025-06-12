/**
 * @file CxCapabilityBasic.hpp
 * @brief Defines the CxCapabilityBasic class for basic command capabilities in the ESP console.
 *
 * This file contains the following class:
 * - CxCapabilityBasic: Provides basic system commands such as reboot, info, uptime, and network information.
 *
 * @date Created by ocfu on 09.01.25.
 *  @copyright © 2025 ocfu
 *
 * Key Features:
 * 1. Classes and Enumerations:
 *    - CxCapabilityBasic: Provides basic system commands.
 *
 * 2. CxCapabilityBasic Class:
 *    - Manages basic system commands such as reboot, info, uptime, and network information.
 *    - Provides methods to enable/disable commands, set/get system information, and update system status.
 *    - Registers and unregisters commands with the CxESPConsole.
 *
 * Relationships:
 * - CxCapabilityBasic is a subclass of CxCapability and interacts with CxESPConsole to register/unregister commands.
 *
 * How They Work Together:
 * - CxCapabilityBasic represents basic system commands with specific properties and methods.
 * - Commands register themselves with the CxESPConsole upon creation and unregister upon destruction.
 * - The CxCapabilityBasic can initialize, end, and print system information.
 *
 * Suggested Improvements:
 * 1. Error Handling:
 *    - Add error handling for edge cases, such as invalid command inputs or failed command executions.
 *
 * 2. Code Refactoring:
 *    - Improve code readability and maintainability by refactoring complex methods and reducing code duplication.
 *
 * 3. Documentation:
 *    - Enhance documentation with more detailed explanations of methods and their parameters.
 *
 * 4. Testing:
 *    - Implement unit tests to ensure the reliability and correctness of the command management functionality.
 *
 * 5. Resource Management:
 *    - Monitor and optimize resource usage, such as memory and processing time, especially for embedded systems with limited resources.
 *
 * 6. Extensibility:
 *    - Provide a more flexible mechanism for adding new command types and capabilities without modifying the core classes.
 */

#ifndef CxCapabilityBasic_hpp
#define CxCapabilityBasic_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

/**
 * @class CxCapabilityBasic
 * @brief Provides basic system commands for the ESP console.
 * @details The `CxCapabilityBasic` class manages basic system commands such as reboot, info, uptime, and network information.
 * It includes methods for enabling/disabling commands, setting/getting system information, and updating system status.
 *
 */
class CxCapabilityBasic : public CxCapability {
   /// access to the instance of the master console
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();
   
public:
   /// Default constructor and default capabilities methods.
   explicit CxCapabilityBasic()
   : CxCapability("basic", getCmds()) {}
   static constexpr const char* getName() { return "basic"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "?", "reboot", "cls", "info", "uptime", "time", "date", "heap", "hostname", "ip", "ssid", "exit", "users", "usr", "cap", "net", "ps", "stack", "delay", "echo", "wlcm", "prompt", "loopdelay", "timer" };
      return commands;
   }
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityBasic>();
   }

   /// Destructor to end the capability
   ~CxCapabilityBasic() {
   }

   /// Setup method to initialize the capability
   void setup() override {
      CxCapability::setup();
      
      setIoStream(*__console.getStream());
      __bLocked = true;
      
      _CONSOLE_INFO(F("====  Cap: %s  ===="), getName());

   }
   
   /// Loop method, currently no recurring tasks to handle.
   void loop() override {
   }
   
   /// Execute a command
   uint8_t execute(const char *szCmd, uint8_t nClient) override {
            
      // validate the call
      if (!szCmd) return EXIT_FAILURE;
      
      // get the arguments into the token buffer
      CxStrToken tkArgs(szCmd, " ");
      
      // we have a command, find the action to take
      String cmd = TKTOCHAR(tkArgs, 0);
      
      // removes heading and trailing white spaces
      cmd.trim();
      
      uint32_t nExitValue = EXIT_FAILURE; // default exit value is 1 (error)
      
      if (cmd == "?") {
         nExitValue = printCommands();
      } else if (cmd == "cap") {
         if (tkArgs.count() > 1) {
            String strSubCmd = TKTOCHAR(tkArgs, 1);
            if (strSubCmd == "load" && tkArgs.count() > 2) {
               __console.createCapInstance(TKTOCHAR(tkArgs, 2), "");
               nExitValue = EXIT_SUCCESS;
            } else if (strSubCmd == "unload" && tkArgs.count() > 2) {
               __console.deleteCapInstance(TKTOCHAR(tkArgs, 2));
               nExitValue = EXIT_SUCCESS;
            } else if (strSubCmd == "list") {
               __console.listCap();
            }
         } else {
            if (__console.hasFS()) {
               __console.man("cap");
            } else {
#ifndef MINIMAL_HELP
               println(F("usage: cap <cmd> [<param> <...>]"));
               println(F("commands:"));
               println(F(" load <cap. name>"));
               println(F(" unload <cap. name>"));
               println(F(" list"));
#endif
            }
         }
         return nExitValue;  // MARK: why not fallding down as the other commands?
      } else if (cmd == "reboot") {
         String opt = TKTOCHAR(tkArgs, 1);
         
         // force reboot
         if (opt == "-f") {
            reboot();
         } else {
            // TODO: prompt user to be improved
            //         __console.__promptUserYN("Are you sure you want to reboot?", [this](bool confirmed) {
            //            if (confirmed) {
            //               __console.reboot();
            //            }
            //         });
            nExitValue = EXIT_SUCCESS;
         }
      } else if (cmd == "cls") {
         __console.cls();
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "prompt") {
         // prompt [-CL] [<prompt string>]
         // prompt [-OFF/ON]
         
         bool bClient = false;
         if (TKTOCHAR(tkArgs, 1)) {
            String strPrompt;
            String strOpt = TKTOCHAR(tkArgs, 1);
            uint8_t i = 1; // index for the prompt string

            if (strOpt == "-CL") {
               i++;
               bClient = true;
               strOpt = TKTOCHAR(tkArgs, i);
            }
            
            if (strOpt == "-OFF") {
               i++;
               if (bClient) {
                  __console.enableClientPrompt(false);
               } else {
                  __console.enablePrompt(false);
               }
            } else if (strOpt == "-ON") {
               i++;
               if (bClient) {
                  __console.enableClientPrompt(true);
               } else {
                  __console.enablePrompt(true);
               }
            }
            
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
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "wlcm") {
         __console.wlcm();
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "info") {
         if (tkArgs.count() > 1) {
            String strSubCmd = TKTOCHAR(tkArgs, 1);
            if (strSubCmd == "reason") {
               println(::getResetInfo());
               __console.setOutputVariable(::getResetInfo());
            } else if (strSubCmd == "last") {
               if (!isQuiet()) {  // FIXME: workaroung as the print function ignores @echo off
                  __console.setOutputVariable(__console.printStartTime(getIoStream()));
                  println();
               }
               __console.setOutputVariable(__console.getStartTime());
            } else if (strSubCmd == "up") {
               println((int32_t)__console.getUpTimeSeconds());
               __console.setOutputVariable((int32_t)__console.getUpTimeSeconds());
            }
         } else {
            printInfo();
            println();
         }
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "uptime") {
         __console.printUptimeExt(); // FIXME: @echo off does not work on this print
         println();
         __console.setOutputVariable(__console.getUpTimeISO());
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "ps") {
         __console.printPs();
         println();
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "loopdelay") {
         if (tkArgs.count() > 1) {
            __console.setLoopDelay(TKTOINT(tkArgs, 1, 0));
         } else {
            print(F("loopdelay = ")); println(__console.getLoopDelay());
            __console.setOutputVariable(__console.getLoopDelay());
         }
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "delay") {
         delay(TKTOINT(tkArgs, 1, 1));
      } else if (cmd == "time") {
         if(__console.getStream()) __console.setOutputVariable(__console.printTime(*__console.getStream(), true));
         println();
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "date") {
         if(__console.getStream()) __console.setOutputVariable(__console.printDate(*__console.getStream()));
         println();
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "heap") {
         printHeap();
         println();
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "frag") {
         printHeapFragmentation();
         println();
         __console.setOutputVariable((uint32_t)g_Heap.fragmentation());
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "stack" ) {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         strSubCmd.toLowerCase();

         if (strSubCmd == "on") {
            g_Stack.enableDebugPrint(true);
         } else if (strSubCmd == "off") {
            g_Stack.enableDebugPrint(false);
         } else if (strSubCmd == "low") {
            __console.setOutputVariable((uint32_t)g_Stack.getLow());
         } else if (strSubCmd == "high") {
            __console.setOutputVariable((uint32_t)g_Stack.getHigh());
         }
         else {
            if (!isQuiet()) {  // FIXME: workaround, as the print function ignores @echo off
               g_Stack.print(getIoStream());
            }
            __console.setOutputVariable((uint32_t)g_Stack.getSize());
         }
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "hostname") {
#ifndef ESP_CONSOLE_NOWIFI
         printHostName();
         println();
#endif
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "ip") {
#ifndef ESP_CONSOLE_NOWIFI
         printIp();
         println();
#endif
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "ssid") {
#ifndef ESP_CONSOLE_NOWIFI
         printSSID();
         println();
#endif
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "exit") {
#ifndef ESP_CONSOLE_NOWIFI
         _CONSOLE_INFO(F("exit wifi client"));
         //console._abortClient();
#else
         printf(F("exit has no function!"));
#endif
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "net") {
#ifndef ESP_CONSOLE_NOWIFI
         printNetworkInfo();
#endif
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "users") {
         printf(F("%d users\n"), __console.users());
         __console.setOutputVariable(__console.users());
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "usr") {
         // set user specific commands here. The first parameter is the command number, the second the flag
         // and the optional third how to set/clear. (0: clear flag, 1: set flag, default (-1): set the flag as value.)
         // usr <cmd> [<flag/value> [<0|1>]]
         int32_t nCmd = TKTOINT(tkArgs, 1, -1);
         uint32_t nValue = TKTOINT(tkArgs, 2, 0);
         int8_t set = TKTOINT(tkArgs, 3, -1);
         
         CxESPConsole& con = __console.getConsole(nClient);
         
         switch (nCmd) {
               // usr 0: be quite, switch all loggings off on the console. log to server/file remains
            case 0:
               con.setUsrLogLevel(LOGLEVEL_OFF);
               break;
               
               // usr 1: set the usr log level to show logs on the console
            case 1:
               if (nValue) {
                  con.setUsrLogLevel(nValue>LOGLEVEL_MAX ? LOGLEVEL_MAX : nValue);
               } else {
                  printf(F("usr log level: %d\n"), con.getUsrLogLevel());
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
               
            default:
               if (__console.hasFS()) {
                  __console.man("usr");
               } else {
                  println(F("usage: usr <cmd> [<flag/value> [<0|1>]]"));
#ifndef MINIMAL_HELP
                  println(F(" 0           be quiet, switch all log messages off on the console."));
                  println(F(" 1  <1..5>   set the log level to show log messages on the console."));
                  println(F(" 2  <flag>   set the extended debug flag(s) to the value."));
                  println(F(" 2  <flag> 0 clear an extended debug flag."));
                  println(F(" 2  <flag> 1 add an extended debug flag."));
#endif
               }
               break;
         }
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "echo") {
         String strValue;
         bool bSuppressNewLine = false;
         
         int i = 1;
         while (i < 7) { //tkArgs can max. hold 8 tokens, the first token in the command
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
               print(strValue.c_str());
               if (i < (tkArgs.count()-1)) print (" ");
            }            
            i++;
         }
         if (! bSuppressNewLine) println();
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "@echo") {
         if (strncmp(TKTOCHAR(tkArgs, 1), "off", 3) == 0) {
            __console.setEcho(false);
         } else if (strncmp(TKTOCHAR(tkArgs, 1), "on", 2) == 0) {
            __console.setEcho(true);
         }
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "timer") {
         // timer add <period>|<cron> <cmd> [<id> [<mode>]]
         // timer del [id]
         // cmd: command to execute
         // id: id of the timer. without id the timer runs one time, with id the timer repeats
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         
         if (strSubCmd == "add") {
            // add a timer
            if (tkArgs.count() > 3) {
               String strTime = TKTOCHAR(tkArgs, 2);
               bool bCron = (strTime.indexOf(' ') != -1);
               uint32_t nPeriod = __console.convertToMilliseconds(strTime.c_str());
               uint8_t nMode = 0; // once
               if (bCron || (nPeriod > 100 && nPeriod <= 7*24*3600*1000)) {
                  CxTimer* pTimer;
                  if (bCron) {
                     pTimer = new CxTimerCron(strTime.c_str());
                  } else {
                     pTimer = new CxTimer();
                  }
                  if (pTimer) {
                     if (TKTOCHAR(tkArgs, 4)) {  // id
                        pTimer->setId(TKTOCHAR(tkArgs, 4));
                        nMode = bCron ? 2 : 1; // id is set -> repeat timer
                     }
                     
                     if (TKTOCHAR(tkArgs, 5)) { // mode
                        String strMode = TKTOCHAR(tkArgs, 5);
                        if (strMode == "once") nMode = 0;
                        if (strMode == "repeat") nMode = 1;
                        if (strMode == "replace") {
                           CxTimer* p = __console.getTimer(TKTOCHAR(tkArgs, 4));
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
                        pTimer->start(nPeriod, [this, pTimer](const char* szCmd) {
                           __console.processCmd(szCmd);
                           // if the timer is set to once, remove it
                           if (pTimer->getMode() == 0) {
                              __console.info(F("timer %s expired, removing"), pTimer->getId());
                              __console.delTimer(pTimer->getId());
                           }
                        }, (nMode == 0));
                        nExitValue = 0;
                     } else {
                        __console.error(F("could not add timer %s! (existing or too many timers)"), pTimer->getId());
                        delete pTimer;
                     }
                  }
               } else {
                  __console.printf(F("invalid time for timer"), nPeriod);
               }
            } else {
               __console.man(cmd.c_str());
            }// count
         } else if (strSubCmd == "del") {
            __console.delTimer(TKTOCHAR(tkArgs, 2));
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "stop") {
            __console.stopTimer(TKTOCHAR(tkArgs, 2));
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "start") {
            __console.startTimer(TKTOCHAR(tkArgs, 2));
            nExitValue = EXIT_SUCCESS;
         }
         else if (strSubCmd == "list") {
            // list all timers
            __console.printTimers(getIoStream());
            nExitValue = EXIT_SUCCESS;
         } else {
            __console.man(cmd.c_str());
         }
      }
      else {
         return EXIT_NOT_HANDLED;
      }
      g_Stack.update();
      return nExitValue;
   }
      
   void reboot() {
      __console.warn(F("reboot..."));
#ifdef ARDUINO
      delay(1000); // let some time to handle last network messages
#ifndef ESP_CONSOLE_NOWIFI
      WiFi.disconnect();
#endif
      ESP.restart();
#endif
   }
   
#ifndef ESP_CONSOLE_NOWIFI
   void printHostName() {
      print(__console.getHostName());
      __console.setOutputVariable(__console.getHostName());
   }
   
   void printIp() {
#ifdef ARDUINO
      print(WiFi.localIP().toString().c_str());
      __console.setOutputVariable(WiFi.localIP().toString().c_str());
#endif
   }
   
   void printSSID() {
#ifdef ARDUINO
      if (WiFi.status() == WL_CONNECTED) {
         printf(F("%s (%d dBm)"), WiFi.SSID().c_str(), WiFi.RSSI());
         __console.setOutputVariable(WiFi.SSID().c_str());
      }
#endif
   }
   
   void printMode() {
#ifdef ARDUINO
      switch(WiFi.getMode()) {
         case WIFI_OFF:
            print(F("OFF"));;
            break;
         case WIFI_STA:
            print(F("Station (STA)"));
            break;
         case WIFI_AP:
            print(F("Access Point (AP)"));
            break;
         case WIFI_AP_STA:
            print(F("AP+STA"));
            break;
         default:
            print(F("unknown"));
            break;
      }
#endif
   }
#endif
   
   /**
    * @brief Prints system information.
    * @details Prints the hostname, IP address, SSID, uptime, and memory information.
    * The method also prints the system load, number of users, and last restart time.
    * @note This method is called by the `info` command and called at the start (as part of the welcome message) of the console.
    */
   void printInfo() {
      print(F(ESC_ATTR_BOLD "  Hostname: " ESC_ATTR_RESET));printHostName();printf(F(ESC_ATTR_BOLD " IP: " ESC_ATTR_RESET));printIp();printf(F(ESC_ATTR_BOLD " SSID: " ESC_ATTR_RESET));printSSID();println();
      print(F(ESC_ATTR_BOLD "    Uptime: " ESC_ATTR_RESET));__console.printUpTimeISO(getIoStream());printf(F(" - %d user(s)"), __console.users());    printf(F(ESC_ATTR_BOLD " Last Restart: " ESC_ATTR_RESET));__console.printStartTime(getIoStream());println();
      printHeap();println();
      print(F("    "));g_Stack.print(getIoStream());
   }
   
   void printHeap() {
      print(F(ESC_ATTR_BOLD " Heap Size: " ESC_ATTR_RESET));printHeapSize();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Used: " ESC_ATTR_RESET));printHeapUsed();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Free: " ESC_ATTR_RESET));printHeapAvailable();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Low: " ESC_ATTR_RESET));printHeapLow();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Fragm.: " ESC_ATTR_RESET));printHeapFragmentation();print(F(" % (peak: ")); printHeapFragmentationPeak();print(F(("%)")));
      __console.setOutputVariable((uint32_t)g_Heap.available());
   }
   
   void printHeapAvailable(bool fmt = false) {
      if (g_Heap.available() < 10000) print(F(ESC_TEXT_BRIGHT_YELLOW));
      if (g_Heap.available() < 3000) print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
      if (fmt) {
         printf(F("%7lu"), g_Heap.available());
      } else {
         printf(F("%lu"), g_Heap.available());
      }
      print(F(ESC_ATTR_RESET));
   }
   
   void printHeapLow(bool fmt = false) {
      if (g_Heap.available() < 10000) print(F(ESC_TEXT_BRIGHT_YELLOW));
      if (g_Heap.available() < 3000) print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
      if (fmt) {
         printf(F("%7lu"), g_Heap.low());
      } else {
         printf(F("%lu"), g_Heap.low());
      }
      print(F(ESC_ATTR_RESET));
   }
   

   void printHeapSize(bool fmt = false) {
      if (fmt) {
         printf(F("%7lu"), g_Heap.size());
      } else {
         printf(F("%lu"), g_Heap.size());
      }
   }
   
   void printHeapUsed(bool fmt = false) {
      if (fmt) {
         printf(F("%7lu"), g_Heap.used());
      } else {
         printf(F("%lu"), g_Heap.used());
      }
   }
   
   void printHeapFragmentation(bool fmt = false) {
      if (fmt) {
         printf(F("%7lu"), g_Heap.fragmentation());
      } else {
         printf(F("%lu"), g_Heap.fragmentation());
      }
   }
   
   void printHeapFragmentationPeak(bool fmt = false) {
      if (fmt) {
         printf(F("%7lu"), g_Heap.peak());
      } else {
         printf(F("%lu"), g_Heap.peak());
      }
   }

   void printNetworkInfo() {
#ifndef ESP_CONSOLE_NOWIFI
      print(F(ESC_ATTR_BOLD "Mode: " ESC_ATTR_RESET)); printMode();println();
      print(F(ESC_ATTR_BOLD "SSID: " ESC_ATTR_RESET)); printSSID(); printf(F(" (%s)"), __console.isConnected()? ESC_TEXT_BRIGHT_GREEN "connected" ESC_ATTR_RESET : ESC_TEXT_BRIGHT_RED "not connected" ESC_ATTR_RESET);println();
      print(F(ESC_ATTR_BOLD "Host: " ESC_ATTR_RESET)); printHostName(); println();
      print(F(ESC_ATTR_BOLD "IP:   " ESC_ATTR_RESET)); printIp(); println();
#ifdef ARDUINO
      printf(F(ESC_ATTR_BOLD "GW:   " ESC_ATTR_RESET "%s"), WiFi.gatewayIP().toString().c_str());println();
      printf(F(ESC_ATTR_BOLD "DNS:  " ESC_ATTR_RESET "%s" ESC_ATTR_BOLD " 2nd: " ESC_ATTR_RESET "%s"), WiFi.dnsIP().toString().c_str(), WiFi.dnsIP(1).toString().c_str());println();
      printf(F(ESC_ATTR_BOLD "NTP:  " ESC_ATTR_RESET "%s"), __console.getNtpServer());
      printf(F(ESC_ATTR_BOLD " TZ: " ESC_ATTR_RESET "%s"), __console.getTimeZone());println();
#endif
#endif
      __console.setOutputVariable(__console.isConnected() ? "online" : "offline");
   }

 
   static void loadCap() {
      CAPREG(CxCapabilityBasic);
      CAPLOAD(CxCapabilityBasic);
   };
};


#endif /* CxCapabilityBasic_hpp */
