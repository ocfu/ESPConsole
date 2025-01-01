# ESP Console for Arduino Projects
===========================================

This library offers a command console for the ESP8266 (with ESP32 support coming soon) accessible via the serial port (Serial) and a WiFiClient. It provides users with an interactive console for entering commands. Designed to facilitate debugging during the development of Arduino ESP projects, the library also enables remote administration through a Linux shell-like command interface.

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

# Examples
[ESPConsole_min](https://github.com/ocfu/ESPConsole/tree/main/examples/ESPConsole_min)
Bare minimum Arduino example. The ESPConsole library provides a simple command-line terminal interface over the serial port with a minimal set of commands. To reduce the flash memory usage, you can define the compiler macro ESP_CONSOLE_NOWIFI. This will exclude Wi-Fi-specific code during compilation, saving space in the flash.


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


# 📋 To-Do List

## 🛠️ Current Tasks

### High Priority
- [ ] Implement functionality for Ext features
   - Commands
      - **flash**: show the flash table
      - **esp**: show esp status 
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
- [API Documentation](https://github.com/username/repository/docs/api.md)
- [Contributing Guidelines](https://github.com/username/repository/CONTRIBUTING.md)


# License and credits

MIT License

Copyright (c) 2024 ocfu

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Credits

- ChatGPS: initial creation of classes and library documentation




