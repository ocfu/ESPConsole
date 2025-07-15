#ifdef ESP_CONSOLE_WIFI
#include "commands.h"

#ifdef ARDUINO
#ifdef ESP32
#include <WebServer.h>
WebServer webServer(80);
#else
#include <ESP8266WebServer.h>
ESP8266WebServer webServer(80);
#endif /* ESP32*/
#include <DNSServer.h>
DNSServer dnsServer;
const byte DNS_PORT = 53;
#endif /* ARDUINO */

// HTML page for the AP without CSS style to save some bin size
const char htmlPageTemplate[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WiFi Setup</title>
</head>
<body>
  <div class="container">
    <h1>WiFi Setup</h1>
    <form action="/connect" method="POST">
      <label for="ssid">WiFi Network:</label>
      <select id="ssid" name="ssid" required>
        {{options}}
      </select>
      <label for="password">Password:</label>
      <input type="password" id="password" name="password" required>
      <button type="submit">Connect</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

bool _bWifiConnected = false;

#include <tools/CxOta.hpp>
CxOta Ota1;

bool g_bOTAinProgress = false;

void setupOta() {
   /// setup OTA service
   _CONSOLE_INFO(F("start OTA service"));
   char szOtaPassword[25];
   ::readOtaPassword(szOtaPassword, sizeof(szOtaPassword));

   Ota1.onStart([]() {
      __console.info(F("OTA start..."));
      // Led1.blinkFlash();
      g_bOTAinProgress = true;
   });

   Ota1.onEnd([]() {
      __console.info(F("OTA end"));
      if (g_bOTAinProgress) {
         __console.processCmd("reboot -f");
      }
      g_bOTAinProgress = false;
   });

   Ota1.onProgress([](unsigned int progress, unsigned int total) {
      int8_t p = (int8_t)round(progress * 100 / total);
      // Led1.action();
      static int8_t last = 0;
      if ((p % 10) == 0 && p != last) {
         __console.info(F("OTA Progress %u"), p);
         last = p;
      }
   });

   Ota1.onError([](ota_error_t error) {
      String strErr;
#ifdef ARDUINO
      if (error == OTA_AUTH_ERROR) {
         strErr = F("authorisation failed");
      } else if (error == OTA_BEGIN_ERROR) {
         strErr = F("begin failed");
      } else if (error == OTA_CONNECT_ERROR) {
         strErr = F("connect failed");
      } else if (error == OTA_RECEIVE_ERROR) {
         strErr = F("receive failed");
      } else if (error == OTA_END_ERROR) {
         strErr = F("end failed");
      }
#endif
      __console.error(F("OTA error: %s [%d]"), strErr.c_str(), error);
   });

   Ota1.begin(__console.getHostName(), szOtaPassword);
}

void loopWifi() {
   Ota1.loop();
#ifdef ARDUINO
   dnsServer.processNextRequest();
   webServer.handleClient();
#endif
}

// Command ssid
bool cmd_ssid(CxStrToken &tkArgs) {
   printSSID();
   __console.println();
   return true;
}

// Command ntp
bool cmd_ntp(CxStrToken &tkArgs) {
   String strSubCmd = TKTOCHAR(tkArgs, 1);

   if (strSubCmd == "server" && tkArgs.count() > 2) {
      __console.addVariable("NTP", TKTOCHAR(tkArgs, 2));
   } else if (strSubCmd == "sync") {
   } else {
      __console.print(F(ESC_ATTR_BOLD "NTP Server: " ESC_ATTR_RESET));
      __console.print(__console.getNtpServer());
      if (__console.isSynced()) {
         __console.print(F(ESC_TEXT_GREEN " (synced)"));
      } else {
         __console.print(F(ESC_BG_BRIGHT_RED "(not snynced)"));
      }
      __console.println(ESC_ATTR_RESET);
      return true;
   }

   if (__console.setNtpServer(__console.getVariable("NTP"))) {
      __console.setTimeZone(__console.getVariable("TZ"));
      return true;
   }
   return false;
}

// Command hostname
bool cmd_hostname(CxStrToken &tkArgs) {
   printHostName();
   __console.println();
   return true;
}

// Command ip
bool cmd_ip(CxStrToken &tkArgs) {
   printIp();
   __console.println();
   return true;
}

// Command exit
bool cmd_exit(CxStrToken &tkArgs) {
   _CONSOLE_INFO(F("exit wifi client"));
   // console._abortClient();
   __console.printf(F("exit has no function!"));
   return true;
}

// Command net
bool cmd_net(CxStrToken &tkArgs) {
   printNetworkInfo();
   return true;
}

// Command wifi
bool cmd_wifi(CxStrToken &tkArgs) {
   String strCmd = TKTOCHAR(tkArgs, 1);
   const char *b = TKTOCHAR(tkArgs, 2);

   if (strCmd == "ssid") {
      if (b) {
         ::writeSSID(TKTOCHAR(tkArgs, 2));
      } else {
         char buf[20];
         ::readSSID(buf, sizeof(buf));
         __console.print(F(ESC_ATTR_BOLD "SSID: " ESC_ATTR_RESET));
         __console.print(buf);
         __console.println();
         __console.setOutputVariable(buf);
      }
   } else if (strCmd == "password") {
      if (b) {
         ::writePassword(TKTOCHAR(tkArgs, 2));
      } else {
         char buf[25];
         ::readPassword(buf, sizeof(buf));
         __console.print(F(ESC_ATTR_BOLD "Password: " ESC_ATTR_RESET));
         __console.print(buf);
         __console.println();
      }
   } else if (strCmd == "hostname") {
      if (b) {
         __console.setHostName(TKTOCHAR(tkArgs, 2));
         ::writeHostName(TKTOCHAR(tkArgs, 2));
      } else {
         char buf[80];
         ::readHostName(buf, sizeof(buf));
         __console.print(F(ESC_ATTR_BOLD "Hostname: " ESC_ATTR_RESET));
         __console.print(buf);
         __console.println();
         __console.setOutputVariable(buf);
      }
   } else if (strCmd == "connect") {
      startWiFi(TKTOCHAR(tkArgs, 2), TKTOCHAR(tkArgs, 3));
   } else if (strCmd == "disconnect") {
      stopWiFi();
   } else if (strCmd == "scan") {
      ::scanWiFi(getIoStream());
   } else if (strCmd == "otapw") {
      if (b) {
         ::writeOtaPassword(TKTOCHAR(tkArgs, 2));
      } else {
         char buf[25];
         ::readOtaPassword(buf, sizeof(buf));
         __console.print(F(ESC_ATTR_BOLD "Password: " ESC_ATTR_RESET));
         __console.print(buf);
         __console.println();
      }
   } else if (strCmd == "ap") {
      if (__console.isWiFiClient()) __console.println(F("switching to AP mode. Note: this disconnects this console!"));
      delay(500);
      _beginAP();
   } else if (strCmd == "check") {
      bool bStatus = checkWifi();
      if (!b) {
         __console.print(F("WiFi is "));
         if (bStatus) {
            __console.println(F("connected"));
         } else {
            __console.println(F("not connected"));
            return false;
         }
      }
   } else if (strCmd == "rssi") {
#ifdef ARDUINO
      __console.print(WiFi.RSSI());
      __console.println(F("dBm"));
      __console.setOutputVariable(WiFi.RSSI());
#endif
   }
   return true;
}
void help_wifi() {
   __console.println(F("wifi commands:"));
   __console.println(F("  ssid [<ssid>]"));
   __console.println(F("  password [<password>]"));
   __console.println(F("  hostname [<hostname>]"));
   __console.println(F("  connect [<ssid> <password>]"));
   __console.println(F("  disconnect"));
   __console.println(F("  status"));
   __console.println(F("  scan"));
   __console.println(F("  otapw [<password>]"));
   __console.println(F("  ap"));
   __console.println(F("  check [-q]"));
}

// Command ping
bool cmd_ping(CxStrToken &tkArgs) {
   if (tkArgs.count() > 2) {
      if (isHostAvailble(TKTOCHAR(tkArgs, 1), TKTOINT(tkArgs, 2, 0))) {
         __console.println(F("ok"));
         return true;
      } else {
         __console.println(F("host not available on this port!"));
      };
   }
   return false;
}
void help_ping() {
   __console.println(F("ping <host> [<port>]"));
}

// Command table in PROGMEM
const CommandEntry commandsWiFi[] PROGMEM = {
    {"ssid", cmd_ssid, nullptr},
    {"net", cmd_net, nullptr},
    {"ntp", cmd_ntp, nullptr},
    {"hostname", cmd_hostname, nullptr},
    {"ip", cmd_ip, nullptr},
    {"exit", cmd_exit, nullptr},
    {"wifi", cmd_wifi, help_wifi},
    {"ping", cmd_ping, help_ping},

    // Add more commands here
};

const size_t NUM_COMMANDS_WIFI = sizeof(commandsWiFi) / sizeof(commandsWiFi[0]);

void printHostName() {
   __console.print(__console.getHostName());
   __console.setOutputVariable(__console.getHostName());
}

void printIp() {
#ifdef ARDUINO
   __console.print(WiFi.localIP().toString().c_str());
   __console.setOutputVariable(WiFi.localIP().toString().c_str());
#endif
}

void printSSID() {
#ifdef ARDUINO
   if (WiFi.status() == WL_CONNECTED) {
      __console.printf(F("%s (%d dBm)"), WiFi.SSID().c_str(), WiFi.RSSI());
      __console.setOutputVariable(WiFi.SSID().c_str());
   }
#endif
}

void printNetworkInfo() {
   __console.print(F(ESC_ATTR_BOLD "Mode: " ESC_ATTR_RESET));
   printMode();
   __console.println();
   __console.print(F(ESC_ATTR_BOLD "SSID: " ESC_ATTR_RESET));
   printSSID();
   __console.printf(F(" (%s)"), __console.isConnected() ? ESC_TEXT_BRIGHT_GREEN "connected" ESC_ATTR_RESET : ESC_TEXT_BRIGHT_RED "not connected" ESC_ATTR_RESET);
   __console.println();
   __console.print(F(ESC_ATTR_BOLD "Host: " ESC_ATTR_RESET));
   printHostName();
   __console.println();
   __console.print(F(ESC_ATTR_BOLD "IP:   " ESC_ATTR_RESET));
   printIp();
   __console.println();
#ifdef ARDUINO
   __console.printf(F(ESC_ATTR_BOLD "GW:   " ESC_ATTR_RESET "%s"), WiFi.gatewayIP().toString().c_str());
   __console.println();
   __console.printf(F(ESC_ATTR_BOLD "DNS:  " ESC_ATTR_RESET "%s" ESC_ATTR_BOLD " 2nd: " ESC_ATTR_RESET "%s"), WiFi.dnsIP().toString().c_str(), WiFi.dnsIP(1).toString().c_str());
   __console.println();
   __console.printf(F(ESC_ATTR_BOLD "NTP:  " ESC_ATTR_RESET "%s"), __console.getNtpServer());
   __console.printf(F(ESC_ATTR_BOLD " TZ: " ESC_ATTR_RESET "%s"), __console.getTimeZone());
   __console.println();
#endif
   __console.setOutputVariable(__console.isConnected() ? "online" : "offline");
}

void printMode() {
#ifdef ARDUINO
   switch (WiFi.getMode()) {
      case WIFI_OFF:
         __console.print(F("OFF"));
         ;
         break;
      case WIFI_STA:
         __console.print(F("Station (STA)"));
         break;
      case WIFI_AP:
         __console.print(F("Access Point (AP)"));
         break;
      case WIFI_AP_STA:
         __console.print(F("AP+STA"));
         break;
      default:
         __console.print(F("unknown"));
         break;
   }
#endif
}

bool checkWifi() {
#ifdef ARDUINO
   bool bConnected = (WiFi.status() == WL_CONNECTED);

   if (_bWifiConnected != bConnected) {
      _bWifiConnected = bConnected;
      if (bConnected) {
         __console.executeBatch("init", "wifi-online");
      } else {
         __console.executeBatch("init", "wifi-offline");
      }
   }
   return bConnected;
#else
   return false;
#endif
}

bool isHostAvailble(const char *host, uint32_t port) {
#ifdef ARDUINO
   if (WiFi.status() == WL_CONNECTED && port && host) {  // Check WiFi connection status
      WiFiClient client;
      if (client.connect(host, port)) {
         client.stop();
         return true;
      }
   }
#endif
   return false;
}

void stopWiFi() {
   _CONSOLE_INFO(F("WiFi disconnect and switch off."));
   __console.println(F("WiFi disconnect and switch off."));
#ifdef ARDUINO
   WiFi.disconnect();
   WiFi.softAPdisconnect();
   WiFi.mode(WIFI_OFF);
   WiFi.forceSleepBegin();
#endif
   checkWifi();
   __console.executeBatch("init", "wifi-down");
}

void _stopAP() {
   // Led1.off();

#if defined(ESP32)
   webServer.stop();
#elif defined(ESP8266)
   webServer.close();
   dnsServer.stop();
#endif
   __console.setAPMode(false);
   __console.executeBatch("init", "ap-down");
}

void startWiFi(const char *ssid, const char *pw) {
   bool bUp = false;

   _stopAP();

   if (checkWifi()) {
      stopWiFi();
   }

   if (!bUp) {
      //
      // Set the ssid, password and hostname from the console settings or from the arguments.
      // If set by the arguments, it will replace settings stored in the eprom.
      //
      // All can be set in the console with the commands
      //   wifi ssid <ssid>
      //   wifi password <password>
      //   wifi hostname <hostname>
      // These settings will be stored in the EEPROM.
      //

      static char szSSID[20];
      static char szPassword[25];
      static char szHostname[80];

      if (ssid) ::writeSSID(ssid);
      ::readSSID(szSSID, sizeof(szSSID));

      if (pw) ::writePassword(pw);
      ::readPassword(szPassword, sizeof(szPassword));

      ::readHostName(szHostname, sizeof(szHostname));

#ifdef ARDUINO
      WiFi.persistent(false);  // Disable persistent WiFi settings, preventing flash wear and keep control of saved settings
      WiFi.mode(WIFI_STA);
      WiFi.begin(szSSID, szPassword);
      WiFi.setAutoReconnect(true);
      WiFi.hostname(szHostname);

      __console.printf(F(ESC_ATTR_BOLD "WiFi: connecting to %s" ESC_ATTR_RESET), szSSID);
      __console.print(F(ESC_ATTR_BLINK "..." ESC_ATTR_RESET));

      // Led1.blinkConnect();

      // try to connect to the network for max. 10 seconds
      CxTimer10s timerTO;  // set timeout

      while (WiFi.status() != WL_CONNECTED && !timerTO.isDue()) {
         // Led1.action();
         delay(1);
      }

      // stop blinking "..." and let the message on the screen
      __console.print(ESC_CLEAR_LINE "\r");
      __console.printf(F(ESC_ATTR_BOLD "WiFi: connecting to %s..." ESC_ATTR_RESET), szSSID);

      // Led1.off();

      if (WiFi.status() != WL_CONNECTED) {
         _bWifiConnected = true;
         __console.println(F(ESC_ATTR_BOLD ESC_TEXT_BRIGHT_RED "not connected!" ESC_ATTR_RESET));
         __console.error("WiFi not connected.");
         // Led1.blinkError();
      } else {
         __console.println(F(ESC_TEXT_BRIGHT_GREEN "connected!" ESC_ATTR_RESET));
         _CONSOLE_INFO("WiFi connected.");
         // Led1.flashOk();
         if (WiFi.getHostname() != szHostname) {
#ifdef ESP32
            __console.setHostName(WiFi.getHostname().c_str());
#else
            __console.setHostName(WiFi.hostname().c_str());
#endif
         }

         bUp = true;
      }

#endif /* Arduino */
   }

   if (bUp) {
      __console.executeBatch("init", "wifi-up");
      checkWifi();
   }
}

void _beginAP() {
   _CONSOLE_INFO(F("Starting Access Point..."));

   stopWiFi();

   // Led1.blinkWait();

#ifdef ARDUINO
   WiFi.forceSleepWake();  ///< wake up the wifi chip
   delay(100);
   WiFi.persistent(false);  ///< Disable persistent WiFi settings, preventing flash wear and keep control of saved settings
   WiFi.mode(WIFI_AP);      ///< Set WiFi mode to AP

   /// Start the Access Point with the given hostname and password
   if (WiFi.softAP(__console.getHostName(), "12345678")) {
      // Start DNS Server
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

      // Define routes
      webServer.on("/", _handleRoot);
      webServer.on("/connect", HTTP_POST, _handleConnect);
      webServer.onNotFound([]() {
         webServer.sendHeader("Location", "/", true);  // Redirect to root
         webServer.send(302, "text/plain", "Redirecting to Captive Portal");
      });

      // Start the web server
      webServer.begin();
      _CONSOLE_INFO(F("ESP started in AP mode"));
      __console.printf(F("ESP started in AP mode. SSID: %s, PW: %s, IP: %s\n"), __console.getHostName(), "12345678", WiFi.softAPIP().toString().c_str());

      __console.setAPMode(true);
      __console.executeBatch("init", "ap-up");

   } else {
      __console.error(F("Failed to start Access Point, going back to STA mode"));
      startWiFi();
   }
#endif
}

void _handleRoot() {
#ifdef ARDUINO
   String htmlPage;

#if defined(CxCapabilityFS_hpp) && !defined(ESP_CONSOLE_NOWIFI)
   File file = LittleFS.open("/ap.html", "r");
   if (!file) {
      webServer.send(404, "text/plain", "HTML file not found");
      return;
   }

   htmlPage = file.readString();
   file.close();
#else
   htmlPage = htmlPageTemplate;
#endif /* CxCapabilityFS_hpp */

   // Scan for available Wi-Fi networks
   int n = WiFi.scanNetworks();
   String options = "";

   if (n == 0) {
      options = "<option value=\"\">No networks found</option>";
   } else {
      for (int i = 0; i < n; ++i) {
         // Get network name (SSID) and signal strength
         String ssid = WiFi.SSID(i);
         int rssi = WiFi.RSSI(i);
         options += "<option value=\"" + ssid + "\">" + ssid + " (Signal: " + String(rssi) + " dBm)</option>";
      }
   }

   // Replace placeholder with actual network options
   htmlPage.replace("{{options}}", options);

   webServer.send(200, "text/html", htmlPage);
#endif /* ARDUINO */
}

/// Handle the connect request from the captive portal to connect to a WiFi network.
void _handleConnect() {
#ifdef ARDUINO

   if (webServer.hasArg("ssid") && webServer.hasArg("password")) {
      String ssid = webServer.arg("ssid");
      String password = webServer.arg("password");

      CxESPConsoleMaster &con = CxESPConsoleMaster::getInstance();

      webServer.send(200, "text/plain", "Attempting to connect to WiFi...");
      con.info(F("SSID: %s, Password: %s"), ssid.c_str(), password.c_str());

      // Attempt WiFi connection
      WiFi.begin(ssid.c_str(), password.c_str());

      // Wait a bit to connect
      CxTimer10s timerTO;  // set timeout

      while (WiFi.status() != WL_CONNECTED && !timerTO.isDue()) {
         delay(1);
      }

      if (WiFi.status() == WL_CONNECTED) {
         con.info("Connected successfully!");
         webServer.send(200, "text/plain", "Connected to WiFi!");

         // switch to STA mode, saves credentials and stop web and dns server.
         String cmd = "wifi connect ";
         cmd += ssid + " " + password;
         con.processCmd(cmd.c_str());
      } else {
         con.error("Connection failed.");
         webServer.send(200, "text/plain", "Failed to connect. Check credentials.");
      }
   } else {
      webServer.send(400, "text/plain", "Missing SSID or Password");
   }

#endif
}

#endif  // ESP_CONSOLE_NOWIFI