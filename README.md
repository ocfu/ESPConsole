# ESP Console for Arduino Projects
===========================================

This library offers a command console for the ESP8266 (with ESP32 support in the future) accessible via the serial port (Serial) and a WiFiClient. It provides users with an interactive console for entering commands. Designed to facilitate debugging during the development of Arduino ESP projects, the library also enables remote administration through a Linux shell-like command interface.

The console operates non-blockingly and requires frequent calls within the main loop() function without significant delays to function properly. It includes basic commands for viewing configuration, status, and processing information. Additionally, for projects utilizing a file system (LittleFS), the library provides a set of commands for managing the file system and files.

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

## Libraries
- ESP8266WiFi  1.0   (optional)
- LittleFS     0.1.0 (optional)

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
ESP console Ext+FS - Test App 1.0 - 01.01.2025 (CET) 13:24:45

  Hostname: ESP-A75F12 IP: 192.168.1.12 SSID: XYZ (-45 dBm)
    Uptime: 0T:00:00:04 - 1 user(s) Last Restart: 01.01.2025 13:24:41
 Heap Size: 49576 bytes Used: 2216 bytes Free: 47360 bytes
      Chip: ESP8266EX/80MHz/4M Sw: core 3.1.2 sdk 2.2.2-dev(38a443e)
Filesystem: Little FS Size: 1024000 bytes Used: 16384 bytes Free: 1007616 bytes

Enter ? to get help. Have a nice day :-)
esp@serial:/> _
```
## Terminal
The output of the console is formated and controlled by using ESC sequences. Your used terminal should support this (e.g. PuTTY).

## Command Sets

| Command Set  | Commands        |
|--------------|-----------------|
| Bare minimum | ?, reboot, cls, info, uptime, time, exit, date, users, heap, hostname, ip, ssid, hw, sw |
| Extended     | flash, esp   | 
| Filesystem   | du, df, size, ls, cat, cp, rm, touch, mount, umount, format |

More and description to come.

## Compiler options

| Macro | Description |
|-------|-------------|
|`ESP_CONSOLE_NOWIFI`| exclude Wi-Fi-specific code |
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
#include "CxESPConsole.hpp"

CxESPConsole ESPConsole(Serial, "Test App", "1.0");

void setup() {
   Serial.begin(115200);
   ESPConsole.begin();
}

void loop() {
   ESPConsole.loop();
}
```

[ESPConsole_min_wifi](https://github.com/ocfu/ESPConsole/tree/main/examples/ESPConsole_min_wifi)
This example offers the same command set as the previous one, but with added remote access capabilities. You can connect a terminal client (e.g., PuTTY) via the defined port 23 to access the same simple command-line terminal interface over the network.

```cpp
#include "CxESPConsole.hpp"

CxESPConsole ESPConsole(Serial, "Test App", "1.0");

WiFiServer server(23);

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

void setup() {
   Serial.begin(115200);
   WiFi.begin(STASSID, STAPSK);
    
    while (WiFi.status() != WL_CONNECTED) {
       delay(500);
       Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    server.begin();
    
    ESPConsole.begin(server);
}

void loop() {
   ESPConsole.loop();
}
```

More examples here [examples](https://github.com/ocfu/ESPConsole/tree/main/examples) 

# 📋 To-Do List

## 🛠️ Current Tasks

### High Priority
- [ ] Implement functionality for Ext features
   - Commands
      - **flash**: show the flash table
      - **net**: show the network setup
      - **ntp**: show the ntp configuration
      - **set**: set configuration items
         - ssid, ssidpw, ntpserver
      - **reset**: factory reset (erase files and all configuration)
   - Implement functionality to add additional command sets.
      - **mqtt**: mqtt configuration 
      - **ha**: home assistant configuration
- [ ] Testing

### Medium Priority
- [ ] Improve functionality for core features
   - [ ] User prompt (Yes/No and further)
   - Commands
      - **set**: set configuration items
         - tz, hostname
- [ ] Improve functionality for Ext features
   - Commands:
      - **uptime**: verfy load and loop time measurements
- [ ] Improve functionality for FS features
   - Commands:
      - **cat**: introduce ">" and ">>" to write inputs from console users
- [ ] Implement further Ext features
   - Commands:
      - **eprom**
- [ ] Refactor code
- [ ] Monitor and minimize of resources (code in flash, heap usage)
- [ ] Test in other environments and up-to-date versions
- [ ] Improve documentation
- [ ] Add error handling for edge cases in input processing

---

## 🚀 Upcoming Features
- [ ] Configuration file on FS
- [ ] Logging
   - to file(s)
   - to a log server
   - ...   
- [ ] Process Manager
   - Managing user processes (control and measure times and resources etc.). 
   - New console commands: start, kill, top, ps
   - Console processes (e.g. start heap to continuesly showing heap information until user quits with ctr-c)
   - User processes (in the ino sketh the user can register loops to be managed as a process with start parameter like delayed start, priority, standby, triggered ...)
- [ ] SSH / secured login


---

## ✅ Completed Tasks
- [x] Set up project repository
- [x] Define project goals and milestones

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




