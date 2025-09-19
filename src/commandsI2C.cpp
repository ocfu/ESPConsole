#ifdef ESP_CONSOLE_I2C

#include "commands.h"

#include <tools/CxI2C.hpp>

// I2C Manager
CxI2CManager _i2cManager;
tInitializerVector VI2CInitializers;

void setupI2C() {
   __console.executeBatch("init", "i2c");
}

void loopI2C() {
   _i2cManager.loop();
}

// Command i2c
void cmd_i2c(CxStrToken& tkArgs) {
   if (tkArgs.indexOf("enable") == 1) {
      _i2cManager.setEnabled((bool)TKTOINT(tkArgs, 2, 0));
      __console.setExitValue(_i2cManager.init());
   } else if (tkArgs.indexOf("list") == 1) {
      _i2cManager.printDevices();
      __console.setExitValue(EXIT_SUCCESS);
   } else if (tkArgs.indexOf("scan") == 1) {
      __console.setExitValue(_i2cManager.scan());
   } else if (tkArgs.indexOf("setpins") == 1 && (tkArgs.count() >= 4)) {
      __console.setExitValue(_i2cManager.setPins(TKTOINT(tkArgs, 2, -1), TKTOINT(tkArgs, 3, -1), TKTOINT(tkArgs, 4, -1)));
   } else if (tkArgs.indexOf("init") == 1) {
      __console.setExitValue(_i2cManager.init());
   } else {
      __console.man("i2c");
   }
}

// Command table in progmem
const CommandEntry commandsI2C[] PROGMEM = {
    {"i2c", cmd_i2c, nullptr},
};

const size_t NUM_COMMANDS_I2C = sizeof(commandsI2C) / sizeof(CommandEntry);

#endif  // ESP_CONSOLE_I2C