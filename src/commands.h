#pragma once

#include <ESPConsole.h>

#include <tools/CxStrToken.hpp>

#define MAX_COMMAND_NAME_LENGTH 32  // Maximum length of command names

// Command function type
typedef bool (*CommandFunc)(CxStrToken &);

// Help function type (no args, just prints help)
typedef void (*HelpFunc)();

// Command structure
struct CommandEntry {
   const char *name = nullptr;  // Command name in PROGMEM
   CommandFunc func = nullptr;  // Function to call
   HelpFunc help = nullptr;     // Help function (optional)
};

// Main command execution function
bool execute(const char *szCmd, uint8_t nClient = 0);

// Info/utility functions
void printInfo();
void printHeap();
void printHeapAvailable(bool fmt = false);
void printHeapLow(bool fmt = false);
void printHeapSize(bool fmt = false);
void printHeapUsed(bool fmt = false);
void printHeapFragmentation(bool fmt = false);
void printHeapFragmentationPeak(bool fmt = false);
void printNetworkInfo();

void printCommands(const CommandEntry *cmds, const size_t numCmds = 0);
void printHelp(const char *cmd, const CommandEntry *cmds, const size_t numCmds = 0);

Stream &getIoStream();
bool isQuiet();
void reboot();

#ifdef ESP_CONSOLE_WIFI
extern const CommandEntry commandsWiFi[] PROGMEM;
extern const size_t NUM_COMMANDS_WIFI;

void printHostName();
void printIp();
void printSSID();
void printMode();

bool isHostAvailble(const char *host, uint32_t port);

void setupOta();
void loopWifi();
bool checkWifi();
void stopWiFi();
void startWiFi(const char *ssid = nullptr, const char *pw = nullptr);
void _beginAP();
void _stopAP();

void _handleRoot();
void _handleConnect();

#endif
#ifdef ESP_CONSOLE_EXT
extern const CommandEntry commandsExt[] PROGMEM;
extern const size_t NUM_COMMANDS_EXT;  
void setupExt();
void loopExt();
#endif 
