# ESP Console for Arduino Projects (NONOS)
===========================================

This library offers a command console for the ESP8266 (with ESP32 support in the future) accessible via the serial port (Serial) and a WiFiClient. It provides users with an interactive console for entering commands. Designed to facilitate debugging during the development of Arduino ESP projects, the library also enables remote administration through a Linux shell-like command interface.

The console operates non-blockingly and requires frequent calls within the main loop() function without significant delays to function properly. It includes basic commands for viewing configuration, status, and processing information. Additionally, for projects utilizing a file system (LittleFS), the library provides a set of commands for managing the file system and files and the wifi settings.

The library also enables and manage basic services, which are quite often needed in many ESP projects, such as logging, OTA and an access point mode with a captive portal for the initial connection to a WiFi network.

The library is designed to be lightweight and easy to use, with a simple API for adding custom commands and features. It is compatible with the Arduino IDE and CLI, as well as other development environments. Additional capablities are provided by header-only files, which can be included in the user's sketch. This minimizes the sketch size size by avoiding including unnecessary code and libraries.

The user program can register and load additional command sets (capabilities), which can be added to the console. The library includes a set of basic commands, which can be extended by the user with additional commands.

# Contents
- [Installing](#installing)
- [Dependencies](#dependencies)
- [Documentation](#documentation)
- [Examples](#examples)
- [Issues and support](#issues-and-support)
- [Todo](#todo)  
- [License and credits](#license-and-credits)   

# Installing
Download a .zip or .tar.gz release from github. Determine the location of your sketchbook by selecting "preferences" on the Arduino menu. Create a "libraries" folder in your sketchbook and unzip the release file in that location.

# Dependencies
Depending on the used ESPConsole class, tools, capabilities and compiler options, the following libraries are used:

## Libraries
ESPConsole              0.0.1   
ESP8266WiFi             1.0     
ArduinoJson             6.19.4  
EEPROM                  1.0     
ArduinoOTA              1.0     
ESP8266WebServer        1.0     
DNSServer               1.1.1   
LittleFS                0.1.0   
PubSubClient            2.8     
Wire                    1.0     
SPI                     1.0     
Adafruit Unified Sensor 1.1.14  
Adafruit BME280 Library 2.2.4   
Adafruit BusIO          1.16.1  
TM1637TinyDisplay       1.6.0   


## Hardware
- ESP8266
- ESP32 (to come)

## Environment

Tested on:

### Arduino
- Arduino 1.8.19 (https://github.com/arduino/Arduino)
- Arduino CLI 0.29.0 (https://github.com/arduino/arduino-cli)

### Platforms
- esp8266:esp8266 3.1.2


# Documentation

## Console prompt

```
ESP console  - test 1 v1.0 - 15.03.2025 23:06:06 (CET)

  Hostname: espd65e02 IP: 192.168.178.146 SSID: OCap (-79 dBm)
    Uptime: 0T:00:00:12 - 1 user(s) Last Restart: 01.01.1970 00:59:59
 Heap Size: 51000 bytes Used: 22368 bytes Free: 28632 bytes Low: 28632 bytes Fragm.: 4 % (peak: 6%)
 Stack: 592 bytes Room: 52004 bytes High: 768 bytes Low: 1364 bytes



Enter ? to get help. Have a nice day :-)
esp@serial:/> 

```
## Terminal
The output of the console is formated and controlled by using ESC sequences. Your used terminal should support this (e.g. PuTTY).

## Capabilities
The following capabilities are available:

| Capability | Description |
|------------|-------------|
| Basic      | Basic system commands such as reboot, info, uptime, and network information. |
| Ext        | Additional commands, including WiFi management, OTA updates, GPIO control, and sensor management. |
| FS         | Provides various file system operations such as mounting, unmounting, formatting, and performing file operations like ls, cat, cp, rm, and touch. It also includes methods for handling environment variables and logging. |
| I2C        | Manages I2C capabilities, including initialization, scanning for devices, and executing commands. |
| MQTT       | It includes methods for setting up, managing, and executing MQTT-related commands. |
| MqttHa     | Provides MQTT Home Assistant capabilities for an ESP-based project. It includes methods for setting up, managing, and executing MQTT Home Assistant-related commands. |
| Seg        | Provides 7-segment display commands for managing displays. |


## Command Sets

| Command Set  | Commands        |
|--------------|-----------------|
| basic        | ?, cap, cls, date, exit, heap, hostname, info, ip, net, ps, reboot, ssid, stack, time, uptime, users, usr |
| ext          | eeprom, esp, flash, gpio, hw, led, ping, sensor, set, sw, wifi |
| fs           | cat, cp, df, du, format, fs, load, log, ls, mount, rm, save, size, touch, umount |
| i2c          | i2c |
| mqtt         | mqtt |
| mqttha       | ha |
| seg          | seg |


## Tools
The following tools are available:
| Tool | Description |
|------|-------------|
| SensorManager | Manages sensors and provides sensor data. |
| StackTracker  | Tracks the stack usage. |
| HeapTracker   | Tracks the heap usage. |
| GpioTracker   | Tracks the GPIO usage. |



More and description to come.

## Compiler options

By default the library includes WiFi and OTA.

| Macro | Description |
|-------|-------------|
|`ESP_CONSOLE_NOWIFI`| exclude Wi-Fi- and OTA-specific code |
|`ARDUINO_CLI_VER=<MMmmPP>`| (optional) Unfortunately Arduino CLI does not provide any (reliable) compiler macro indicating the used version. This marcro defines it manually, only for the benefit of the command `sw`, which shows relevant version info.

The library is developed using XCode and Arduino CLI. For testing purpose, there are code segments, only for the use in this test environment and Arduino specific code is framed by ```#ifdef ARDUINO ... #endif```. 
 
## Example Arduino CLI compiler call
```bash
arduino-cli compile -b esp8266:esp8266:nodemcuv2:eesz=4M1M,led=2 --clean --output-dir build/esp8266.esp8266.nodemcuv2  --export-binaries --build-property build.extra_flags="-DESP_CONSOLE_NOWIFI -DARDUINO_CLI_VER=2900 "
```


# Examples
[ESPConsole_min](https://github.com/ocfu/ESPConsole/tree/main/examples/ESPConsole_min)
Bare minimum Arduino example. The ESPConsole library provides a simple command-line terminal interface over the serial port with a minimal set of commands. To reduce the flash memory usage, you can define the compiler macro `ESP_CONSOLE_NOWIFI`. This will exclude Wi-Fi-specific code during compilation, saving space in the flash.


```cpp
tbd
```

[ESPConsole_min_wifi](https://github.com/ocfu/ESPConsole/tree/main/examples/ESPConsole_min_wifi)
This example offers the same command set as the previous one, but with added remote access capabilities. You can connect a terminal client (e.g., PuTTY) via the defined port 23 to access the same simple command-line terminal interface over the network.

```cpp
#include "CxESPConsole.hpp"
#include "../capabilities/CxCapabilityBasic.hpp"

WiFiServer server(8266);

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

void setup() {
   g_Stack.begin();

   Serial.begin(115200);
   Serial.println();
   
   WiFi.begin(STASSID, STAPSK);
   
   while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
   }
   Serial.println("\nWiFi connected.");
   
   ESPConsole.setAppNameVer("Min WiFi", "v1.0");
   ESPConsole.setTimeZone("CET-1CEST,M3.5.0,M10.5.0/3");

   CAPREG(CxCapabilityBasic);
   CAPLOAD(CxCapabilityBasic);
   
   ESPConsole.begin(server);
}

void loop() {
   ESPConsole.loop();
}```

More examples here [examples](https://github.com/ocfu/ESPConsole/tree/main/examples) 

# 📋 To-Do List

## 🛠️ Current Tasks

### High Priority
- [ ] Implement functionality for Ext features
   - Commands
      - **reset**: factory reset (erase files and all configuration)
- [ ] Testing

### Medium Priority
- [ ] Improve functionality for core features
   - [ ] User prompt (Yes/No and further)
- [ ] Improve functionality for FS features
   - Commands:
      - **cat**: introduce ">" and ">>" to write inputs from console users
- [ ] Refactor code
- [ ] Monitor and minimize of resources (code in flash, heap usage)
- [ ] Test in other environments and up-to-date versions
- [ ] Improve documentation
- [ ] Add error handling for edge cases in input processing

---

## 🚀 More to come
- [ ] Logging
   - to file(s)
   - to syslog   
- [ ] Improve Process Manager
   - New console commands: start, kill
   - User processes (in the ino sketh the user can register loops to be managed as a process with start parameter like delayed start, priority, standby, triggered ...)
- [ ] TLS support


---

## 📝 Notes
- For feature ideas or additional tasks, please open a new issue or submit a pull request.
- Keep tasks clear and concise to ensure smooth collaboration.

---

## 📂 Resources
- [Contributing Guidelines](https://github.com/ocfu/ESPConsole/blob/main/CONTRIBUTING.md)


# License and credits

[MIT License](https://github.com/ocfu/ESPConsole/blob/main/LICENSE)


## Credits

- ChatGPS: initial creation of classes and library documentation support
- CoPilot: coding and documentation support




