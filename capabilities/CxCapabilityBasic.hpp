//
//  CxCapabilityBasic.hpp
//  ESP
//
//  Created by ocfu on 09.01.25.
//  Copyright © 2025 ocfu. All rights reserved.
//

#ifndef CxCapabilityBasic_hpp
#define CxCapabilityBasic_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

class CxCapabilityBasic : public CxCapability {
   CxESPConsoleMaster& console = CxESPConsoleMaster::getInstance();
   
public:
   explicit CxCapabilityBasic()
   : CxCapability("basic", getCmds()) {}
   static constexpr const char* getName() { return "basic"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "?", "reboot", "cls", "info", "uptime", "time", "date", "heap", "hostname", "ip", "ssid", "exit", "users", "usr", "cap", "net", "ps" };
      return commands;
   }
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityBasic>();
   }
   
   void setup() override {
      CxCapability::setup();
      
      setIoStream(*console.getStream());
      __bLocked = true;
      
      console.info(F("====  Cap: %s  ===="), getName());

   }
   
   void loop() override {
      startMeasure();
      CxCapability::loop();
      stopMeasure();
   }
   
   bool execute(const char *szCmd, const char *args) override {
      
      bool bQuiet = false;
      
      // validate the call
      if (!szCmd) return false;
      
      // get the arguments into the token buffer
      CxStrToken tkArgs(args, " ");
      
      // we have a command, find the action to take
      String cmd = szCmd;
      
      // removes heading and trailing white spaces
      cmd.trim();
      
      if (cmd == "?") {
         printCommands();
      } else if (cmd == "cap") {
         if (tkArgs.count() > 0) {
            String strSubCmd = TKTOCHAR(tkArgs, 0);
            if (strSubCmd == "load" && tkArgs.count() > 1) {
               console.createCapInstance(TKTOCHAR(tkArgs, 1), "");
            } else if (strSubCmd == "unload" && tkArgs.count() > 1) {
               console.deleteCapInstance(TKTOCHAR(tkArgs, 1));
            } else if (strSubCmd == "list") {
               console.listCap();
            }
         } else {
            println(F("usage: cap <cmd> [<param> <...>]"));
            println(F("commands:"));
            println(F(" load <cap. name>"));
            println(F(" unload <cap. name>"));
            println(F(" list"));
         }
         return true;
      } else if (cmd == "reboot") {
         String opt = TKTOCHAR(tkArgs, 0);
         
         // force reboot
         if (opt == "-f" || bQuiet) {
            reboot();
         } else {
            // TODO: prompt user to be improved
            //         console.__promptUserYN("Are you sure you want to reboot?", [this](bool confirmed) {
            //            if (confirmed) {
            //               console.reboot();
            //            }
            //         });
         }
      } else if (cmd == "cls") {
         console.cls();
      } else if (cmd == "info") {
         printInfo();
         println();
      } else if (cmd == "uptime") {
         console.printUptimeExt();
         println();
      } else if (cmd == "ps") {
         console.printPs();
         println();
      } else if (cmd == "time") {
         if(console.getStream()) console.printTime(*console.getStream());
         println();
      } else if (cmd == "date") {
         if(console.getStream()) console.printDate(*console.getStream());
         println();
      } else if (cmd == "heap") {
         printHeap();
         println();
      } else if (cmd == "hostname") {
#ifndef ESP_CONSOLE_NOWIFI
         printHostName();
         println();
#endif
      } else if (cmd == "ip") {
#ifndef ESP_CONSOLE_NOWIFI
         printIp();
         println();
#endif
      } else if (cmd == "ssid") {
#ifndef ESP_CONSOLE_NOWIFI
         printSSID();
         println();
#endif
      } else if (cmd == "exit") {
#ifndef ESP_CONSOLE_NOWIFI
         console.info(F("exit wifi client"));
         //console._abortClient();
#else
         printf(F("exit has no function!"));
#endif
      } else if (cmd == "net") {
#ifndef ESP_CONSOLE_NOWIFI
         printNetworkInfo();
#endif
      } else if (cmd == "users") {
         printf(F("%d users\n"), console.users());
      } else if (cmd == "usr") {
         // set user specific commands here. The first parameter is the command number, the second the flag
         // and the optional third how to set/clear. (0: clear flag, 1: set flag, default (-1): set the flag as value.)
         // usr <cmd> [<flag/value> [<0|1>]]
         int32_t nCmd = TKTOINT(tkArgs, 1, -1);
         uint32_t nValue = TKTOINT(tkArgs, 2, 0);
         int8_t set = TKTOINT(tkArgs, 3, -1);
         
         switch (nCmd) {
               // usr 0: be quite, switch all loggings off on the console. log to server/file remains
            case 0:
               console.setUsrLogLevel(LOGLEVEL_OFF);
               break;
               
               // usr 1: set the usr log level to show logs on the console
            case 1:
               if (nValue) {
                  console.setUsrLogLevel(nValue>LOGLEVEL_MAX ? LOGLEVEL_MAX : nValue);
               } else {
                  printf(F("usr log level: %d\n"), console.getUsrLogLevel());
               }
               break;
               
               // usr 2: set extended debug flag
            case 2:
               if (set < 0) {
                  console.setDebugFlag(nValue);
               } else if (set == 0) {
                  console.resetDebugFlag(nValue);
               } else {
                  console.setDebugFlag(console.getDebugFlag() | nValue);
               }
               if (console.getDebugFlag()) {
                  console.setLogLevel(LOGLEVEL_DEBUG_EXT);
               }
               break;
               
            default:
               println(F("usage: usr <cmd> [<flag/value> [<0|1>]]"));
               println(F(" 0           be quiet, switch all log messages off on the console."));
               println(F(" 1  <1..5>   set the log level to show log messages on the console."));
               println(F(" 2  <flag>   set the extended debug flag(s) to the value."));
               println(F(" 2  <flag> 0 clear an extended debug flag."));
               println(F(" 2  <flag> 1 add an extended debug flag."));
               break;
         }
      } else {
         return false;
      }
      return true;
   }
      
   void reboot() {
      console.warn(F("reboot..."));
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
      print(console.getHostName());
   }
   
   void printIp() {
#ifdef ARDUINO
      print(WiFi.localIP().toString().c_str());
#endif
   }
   
   void printSSID() {
#ifdef ARDUINO
      if (WiFi.status() == WL_CONNECTED) {
         printf(F("%s (%d dBm)"), WiFi.SSID().c_str(), WiFi.RSSI());
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
   
   void printInfo() {
      print(F(ESC_ATTR_BOLD "  Hostname: " ESC_ATTR_RESET));printHostName();printf(F(ESC_ATTR_BOLD " IP: " ESC_ATTR_RESET));printIp();printf(F(ESC_ATTR_BOLD " SSID: " ESC_ATTR_RESET));printSSID();println();
      print(F(ESC_ATTR_BOLD "    Uptime: " ESC_ATTR_RESET));console.printUpTimeISO(getIoStream());printf(F(" - %d user(s)"), console.users());    printf(F(ESC_ATTR_BOLD " Last Restart: " ESC_ATTR_RESET));console.printStartTime(getIoStream());println();
      printHeap();println();
   }
   
   void printHeap() {
      print(F(ESC_ATTR_BOLD " Heap Size: " ESC_ATTR_RESET));printHeapSize();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Used: " ESC_ATTR_RESET));printHeapUsed();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Free: " ESC_ATTR_RESET));printHeapAvailable();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Low: " ESC_ATTR_RESET));printHeapLow();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Fragm.: " ESC_ATTR_RESET));printHeapFragmentation();print(F(" % (peak: ")); printHeapFragmentationPeak();print(F(("%)")));
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
         printf(F("%s%7lu%s"), g_Heap.size());
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
      print(F(ESC_ATTR_BOLD "SSID: " ESC_ATTR_RESET)); printSSID(); printf(F(" (%s)"), console.isConnected()? ESC_TEXT_BRIGHT_GREEN "connected" ESC_ATTR_RESET : ESC_TEXT_BRIGHT_RED "not connected" ESC_ATTR_RESET);println();
      print(F(ESC_ATTR_BOLD "Host: " ESC_ATTR_RESET)); printHostName(); println();
      print(F(ESC_ATTR_BOLD "IP:   " ESC_ATTR_RESET)); printIp(); println();
#ifdef ARDUINO
      printf(F(ESC_ATTR_BOLD "GW:   " ESC_ATTR_RESET "%s"), WiFi.gatewayIP().toString().c_str());println();
      printf(F(ESC_ATTR_BOLD "DNS:  " ESC_ATTR_RESET "%s" ESC_ATTR_BOLD " 2nd: " ESC_ATTR_RESET "%s"), WiFi.dnsIP().toString().c_str(), WiFi.dnsIP(1).toString().c_str());println();
      printf(F(ESC_ATTR_BOLD "NTP:  " ESC_ATTR_RESET "%s"), console.getNtpServer());
      printf(F(ESC_ATTR_BOLD " TZ: " ESC_ATTR_RESET "%s"), console.getTimeZone());println();
#endif
#endif
   }


   
};


#endif /* CxCapabilityBasic_hpp */
