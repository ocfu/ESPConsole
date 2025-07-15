#include "commands.h"

#ifdef ESP_CONSOLE_EXT

#ifdef ARDUINO
#include <FS.h>
#ifdef ESP32
#include "LITTLEFS.h"
struct FSInfo {
   size_t totalBytes;
   size_t usedBytes;
   size_t blockSize;
   size_t pageSize;
   size_t maxOpenFiles;
   size_t maxPathLength;
};
#define Dir File
#define LittleFS LITTLEFS
#else
#include <LittleFS.h>
#endif /* ESP32*/
#endif /* ARDUINO */

bool _bBreakBatch = false;
uint8_t _nBatchDepth = 0;

void setupFS() {
   mount();
   ls(true, true);

   if (fileExists(".safemode")) {
      __console.warn(F("Start in SAFEMODE"));
      __console.setSafeMode(true);
   }

   // implement specific fs functions
   //__console.setFuncPrintLog2Server([this](const char *sz) { this->_print2logServer(sz); });
   //__console.setFuncExecuteBatch([this](const char *sz, const char *label) { this->executeBatch(sz, label); });
   //__console.setFuncMan([this](const char *sz, const char *param) { this->man(sz, param); });

   //CxPersistentImpl::getInstance().setImplementation(ESPConsole);

   __console.executeBatch("init", "fs");
}

void loopFS() {
}

// Command du
bool cmd_du(CxStrToken& tkArgs) {
   bool bReturn = true;
   if (TKTOCHAR(tkArgs, 1)) {
      bReturn = (printDu(TKTOCHAR(tkArgs, 1)) == EXIT_SUCCESS);
      __console.println();
   } else {
      __console.println(".");
   }
   return bReturn;
}

// Command df
bool cmd_df(CxStrToken& tkArgs) {
   bool bReturn = true;
   bReturn = (printDf(TKTOCHAR(tkArgs, 1)) == EXIT_SUCCESS);
   __console.println(F(" bytes"));
   return bReturn;
}

// Command size
bool cmd_size(CxStrToken& tkArgs) {
   bool bReturn = true;
   bReturn = (printSize() == EXIT_SUCCESS);
   __console.println(F(" bytes"));
   return bReturn;
}

// Command ls
bool cmd_ls(CxStrToken& tkArgs) {
   String strOpt = TKTOCHAR(tkArgs, 1);
   return (ls(strOpt == "-a" || strOpt == "-la", strOpt == "-l" || strOpt == "-la") == EXIT_SUCCESS);
}

// Command la
bool cmd_la(CxStrToken& tkArgs) {
   return (ls(true, true) == EXIT_SUCCESS);
}

// Command cat
bool cmd_cat(CxStrToken& tkArgs) {
   return (cat(TKTOCHAR(tkArgs, 1)) == EXIT_SUCCESS);
}

// Command cp
bool cmd_cp(CxStrToken& tkArgs) {
   return (cp(TKTOCHAR(tkArgs, 1), TKTOCHAR(tkArgs, 1)) == EXIT_SUCCESS);
}

// Command rm
bool cmd_rm(CxStrToken& tkArgs) {
   return (rm(TKTOCHAR(tkArgs, 1)) == EXIT_SUCCESS);
}

// Command mv
bool cmd_mv(CxStrToken& tkArgs) {
   return (mv(TKTOCHAR(tkArgs, 1), TKTOCHAR(tkArgs, 1)) == EXIT_SUCCESS);
}

// Command touch
bool cmd_touch(CxStrToken& tkArgs) {
   return (touch(TKTOCHAR(tkArgs, 1)) == EXIT_SUCCESS);
}

// Command mount
bool cmd_mount(CxStrToken& tkArgs) {
   return (mount() == EXIT_SUCCESS);
}

// Comand umount
bool cmd_umount(CxStrToken& tkArgs) {
   return (umount() == EXIT_SUCCESS);
}

// Command format
bool cmd_format(CxStrToken& tkArgs) {
   return (format() == EXIT_SUCCESS);
}

// Command hasfs
bool cmd_hasfs(CxStrToken& tkArgs) {
   bool bHasFS = hasFS();
   __console.setOutputVariable(bHasFS ? "true" : "false");
   return bHasFS;
}

// Command fs
bool cmd_fs(CxStrToken& tkArgs) {
   bool bReturn = true;
   bReturn = (printFsInfo() == EXIT_SUCCESS);
   __console.println();
   return bReturn;
}

// Command $UPLOAD$
bool cmd_upload(CxStrToken& tkArgs) {
   return (_handleFile() == EXIT_SUCCESS);
}

// Command $DOWNLOAD$
bool cmd_download(CxStrToken& tkArgs) {
   return (_handleFile() == EXIT_SUCCESS);
}

// Command exec
bool cmd_exec(CxStrToken& tkArgs) {
   return (executeBatch(TKTOCHAR(tkArgs, 1), TKTOCHAR(tkArgs, 2), TKTOCHAR(tkArgs, 3)) == EXIT_SUCCESS);
}

// Command break
bool cmd_break(CxStrToken& tkArgs) {
   String strCond = TKTOCHAR(tkArgs, 1);
   strCond.toLowerCase();

   uint8_t nValue = TKTOINT(tkArgs, 2, 0);

   if (strCond == "on" && nValue) {
      _bBreakBatch = true;
   } else if (strCond.length() == 0) {  // simple break
      _bBreakBatch = true;
   } else {
      _bBreakBatch = false;
   }
   return true;
}

// Command man
bool cmd_man(CxStrToken& tkArgs) {
   return (man(TKTOCHAR(tkArgs, 1), TKTOCHARAFTER(tkArgs, 2)) == EXIT_SUCCESS);
}

// Command table in PROGMEM
const CommandEntry commandsFS[] PROGMEM = {
   {"du", cmd_du, nullptr},
   {"df", cmd_df, nullptr},
   {"size", cmd_size, nullptr},
   {"ls", cmd_ls, nullptr},
   {"la", cmd_la, nullptr},
   {"cat", cmd_cat, nullptr},
   {"cp", cmd_cp, nullptr},
   {"rm", cmd_rm, nullptr},
   {"mv", cmd_mv, nullptr},
   {"touch", cmd_touch, nullptr},
   {"mount", cmd_mount, nullptr},
   {"umount", cmd_umount, nullptr},
   {"format", cmd_format, nullptr},
   {"hasfs", cmd_hasfs, nullptr},
   {"fs", cmd_fs, nullptr},
   {"upload", cmd_upload, nullptr},
   {"download", cmd_download, nullptr},
   {"exec", cmd_exec, nullptr},
   {"break", cmd_break, nullptr},
   {"man", cmd_man, nullptr},
   
   // Add more filesystem commands here
};

const size_t NUM_COMMANDS_FS = sizeof(commandsFS) / sizeof(CommandEntry);

bool hasFS() {
   bool bResult = false;
#ifdef ARDUINO
   FSInfo fsinfo;
#ifdef ESP32
   fsinfo.totalBytes = LittleFS.totalBytes();
   bResult = (fsinfo.totalBytes > 0);
#else
   bResult = LittleFS.info(fsinfo);
#endif
#endif
   return bResult;
}

uint32_t getDf() {
   if (hasFS()) {
      FSInfo fsinfo;
      _getFSInfo(fsinfo);
      return (uint32_t)(fsinfo.totalBytes - fsinfo.usedBytes);
   } else {
      return 0;
   }
}

uint8_t printFsInfo() {
   if (hasFS()) {
      __console.printf(F(ESC_ATTR_BOLD "Filesystem: " ESC_ATTR_RESET "Little FS"));
      __console.printf(F(ESC_ATTR_BOLD " Size: " ESC_ATTR_RESET));
      printSize();
      __console.printf(F(" bytes"));
      __console.printf(F(ESC_ATTR_BOLD " Used: " ESC_ATTR_RESET));
      printDu();
      __console.printf(F(" bytes"));
      __console.printf(F(ESC_ATTR_BOLD " Free: " ESC_ATTR_RESET));
      printDf();
      __console.printf(F(" bytes"));
      __console.setOutputVariable("Little FS");
      return EXIT_SUCCESS;
   } else {
      __console.printf(F(ESC_ATTR_BOLD "Filesystem: " ESC_ATTR_RESET "not mounted"));
   }
   return EXIT_FAILURE;
}

uint8_t printDu(const char* szFn) {
   if (hasFS()) {
      if (szFn) {
#ifdef ARDUINO
         if (LittleFS.exists(szFn)) {
            File file = LittleFS.open(szFn, "r");
            if (file) {
               __console.printf(F("%d %s"), file.size(), file.name());
               __console.setOutputVariable((uint32_t)file.size());
               file.close();
               return EXIT_SUCCESS;
            }
         } else {
            _printNoSuchFileOrDir("du", szFn);
         }
#endif
      } else {
         FSInfo fsinfo;
         _getFSInfo(fsinfo);
         __console.printf(F("%ld"), fsinfo.usedBytes);
         __console.setOutputVariable((uint32_t)fsinfo.usedBytes);
         return EXIT_SUCCESS;
      }
   } else {
      _printNoFS();
   }
   return EXIT_FAILURE;
}

uint8_t printSize(bool fmt) {
   if (hasFS()) {
      FSInfo fsinfo;
      _getFSInfo(fsinfo);
      if (fmt) {
         __console.printf(F("%07ld"), fsinfo.totalBytes);
      } else {
         __console.printf(F("%ld"), fsinfo.totalBytes);
      }
      __console.setOutputVariable((uint32_t)fsinfo.totalBytes);
      return EXIT_SUCCESS;
   } else {
      _printNoFS();
   }
   return EXIT_FAILURE;
}

uint8_t printDf(bool fmt) {
   if (hasFS()) {
      if (fmt) {
         __console.printf(F("%7ld"), getDf());
      } else {
         __console.printf(F("%ld"), getDf());
      }
      __console.setOutputVariable(getDf());
      return EXIT_SUCCESS;
   } else {
      _printNoFS();
   }
   return EXIT_FAILURE;
}

uint8_t ls(bool bAll, bool bLong) {
   if (hasFS()) {
      uint32_t totalBytes;
      uint32_t usedBytes;

      FSInfo fsinfo;
      _getFSInfo(fsinfo);

      totalBytes = (uint32_t)fsinfo.totalBytes;
      usedBytes = (uint32_t)fsinfo.usedBytes;

#ifdef ARDUINO
      uint32_t total = 0;
#ifdef ESP32
      // TODO: recursively https://unsinnsbasis.de/littlefs-esp32/
      File root = LittleFS.open("/");
      if (root) {
         File file = root.openNextFile();
         while (file) {
            if (file.isDirectory()) {
               __console.printf(F("DIR     %s/\n"), file.name());
            } else {
               const char* fn = file.name();

               // skip hidden files
               if (!bAll && fn && fn[0] == '.') continue;

               if (bAll) {
                  __console.printf(F("%7d "), file.size());
                  __console.printFileDateTime(getIoStream(), file.getCreationTime(), file.getLastWrite());
               }
               __console.printf(F(" %s\n"), file.name());
            }
            file = root.openNextFile();
         }
      }
#else   // no ESP32
      Dir dir = LittleFS.openDir("");
      while (dir.next()) {
         File file = dir.openFile("r");
         const char* fn = file.name();

         // skip hidden files
         if (!bAll && fn && fn[0] == '.') continue;

         // print file size and date/time
         if (bLong) {
            __console.printf(F("%7d "), file.size());
            __console.printFileDateTime(getIoStream(), file.getCreationTime(), file.getLastWrite());
         }
         __console.printf(F(" %s\n"), file.name());
         total += file.size();
         file.close();
      }
#endif  // end ESP32
      if (bLong) {
         __console.printf(F("%7d (%d bytes free)\n"), total, totalBytes - usedBytes);
      }
      return 0;
#endif  // end ARDUINO
   } else {
      _printNoFS();
   }
   return 1;
}

uint8_t cat(const char* szFn) {
   if (!szFn) {
      __console.println(F("usage: cat <file>"));
      return EXIT_FAILURE;
   }
   if (hasFS()) {
#ifdef ARDUINO
      // Open file for reading
      File file = LittleFS.open(szFn, "r");
      if (file) {
         while (file.available()) {
            __console.print((char)file.read());
         }
         __console.println();
      } else {
         _printNoSuchFileOrDir("cat", szFn);
         return EXIT_FAILURE;
      }
#else
      std::ifstream file;
      file.open(szFn);
      if (file.is_open()) {
         char c = 0;
         while (file.get(c)) {
            __console.printf(c);
         }
         __console.println();
      } else {
         return EXIT_FAILURE;
      }
#endif
      file.close();
      return EXIT_SUCCESS;
   } else {
      _printNoFS();
   }
   return EXIT_FAILURE;
}

uint8_t rm(const char* szFn) {
   if (!szFn) {
      __console.println(F("usage: rm <file>"));
      return EXIT_FAILURE;
   }
   if (hasFS()) {
#ifdef ARDUINO
      if (!LittleFS.remove(szFn)) {
         _printNoSuchFileOrDir("rm", szFn);
      } else {
         return EXIT_SUCCESS;
      }
#else
#endif
   } else {
      _printNoFS();
   }
   return EXIT_FAILURE;
}

uint8_t cp(const char* szSrc, const char* szDst) {
   if (!szSrc || !szDst) {
      __console.println(F("usage: cp <src_file> <tgt_file>"));
      return EXIT_FAILURE;
   }
   if (hasFS()) {
#ifdef ARDUINO
      if (LittleFS.exists(szSrc)) {
         static char buf[64];

         // FIXME: cp need y/n query if dst exist, unless -f is given as parameter
         if (LittleFS.exists(szDst)) LittleFS.remove(szDst);

         File fileSrc = LittleFS.open(szSrc, "r");

         if (fileSrc) {
            File fileDst = LittleFS.open(szDst, "w");
            if (fileDst) {
               while (fileSrc.available() > 0) {
                  uint8_t n = fileSrc.readBytes(buf, sizeof(buf));
                  fileDst.write((uint8_t*)buf, n);
               }
               fileDst.close();
            }
            fileSrc.close();
            return EXIT_SUCCESS;
         }
      } else {
         _printNoSuchFileOrDir("cp", szSrc);
      }
#else
#endif
   } else {
      _printNoFS();
   }
   return EXIT_FAILURE;
}

uint8_t mv(const char* szSrc, const char* szDst) {
   if (!szSrc || !szDst) {
      __console.println(F("usage: mv <src_file> <tgt_file>"));
      return EXIT_FAILURE;
   }
   if (hasFS()) {
#ifdef ARDUINO
      if (LittleFS.exists(szSrc)) {
         // FIXME: cp need y/n query if dst exist, unless -f is given as parameter
         if (LittleFS.exists(szDst)) LittleFS.remove(szDst);
         if (!LittleFS.rename(szSrc, szDst)) {
            __console.println(F("Failed to rename file"));
         } else {
            return EXIT_SUCCESS;
         }
      } else {
         _printNoSuchFileOrDir("mv", szSrc);
      }
#else
#endif
   } else {
      _printNoFS();
   }
   return EXIT_FAILURE;
}

uint8_t touch(const char* szFn) {
   if (!szFn) {
      __console.println(F("usage: touch <file>"));
      return EXIT_FAILURE;
   }
   if (hasFS()) {
#ifdef ARDUINO
      const char* mode = "a";
      if (!LittleFS.exists(szFn)) {
         mode = "w";
      }
      File file = LittleFS.open(szFn, mode);
      if (file) {
         file.close();
         return EXIT_SUCCESS;
      }
#else
#endif
   } else {
      _printNoFS();
   }
   return EXIT_FAILURE;
}

uint8_t mount() {
   if (!hasFS()) {
#ifdef ARDUINO
      if (!LittleFS.begin()) {
         __console.error("LittleFS mount failed");
         return EXIT_FAILURE;
      } else {
         return EXIT_SUCCESS;
      }
#else
#endif
   } else {
      // __console.println(F("LittleFS already mounted!"));
      return EXIT_SUCCESS;
   }
   return EXIT_FAILURE;
}

uint8_t umount() {
   if (hasFS()) {
#ifdef ARDUINO
      LittleFS.end();
#else
#endif
      return EXIT_SUCCESS;
   }
   return EXIT_FAILURE;
}

uint8_t format() {
   if (hasFS()) {
      __console.println(F("LittleFS still mounted! -> 'umount' first"));
      return EXIT_FAILURE;
   } else {
//         __console.promptUserYN("Are you sure you want to format?", [](bool confirmed) {
//            if (confirmed) {
#ifdef ARDUINO
      LittleFS.format();
      return EXIT_SUCCESS;
#endif
      //            }
      //         });
      // #else
      // #endif
   }
   return EXIT_FAILURE;
}

bool fileExists(const char* szFn) {
#ifdef ARDUINO
   return LittleFS.exists(szFn);
#else
   return false;
#endif
}

void _getFSInfo(FSInfo& fsinfo) {
#ifdef ARDUINO
#ifdef ESP32
   fsinfo.totalBytes = LITTLEFS.totalBytes();
   fsinfo.usedBytes = LITTLEFS.usedBytes();
#else
   LittleFS.info(fsinfo);
#endif
#endif
}

uint8_t _handleFile() {
#ifdef ARDUINO
   static char buffer[64];
   String header = "";
   String filename = "";
   size_t expectedSize = 0;
   size_t receivedSize = 0;
   File file;

   WiFiClient* client = static_cast<WiFiClient*>(&getIoStream());  // tricky

   // read header
   while (client->connected() && header.indexOf("\n") == -1) {
      if (client->available()) {
         char c = client->read();
         header += c;
      }
   }

   //__CONSOLE_DEBUG(F("receive header: %s"), header.c_str());

   // analyse header
   if (header.startsWith("GET ")) {
      // Handle GET <filename>
      String filename = header.substring(4);
      filename.trim();
      return _sendFile(client, filename.c_str());
   } else if (header.startsWith("FILE:")) {
      // receive file
      int fileStart = header.indexOf("FILE:") + 5;
      int sizeStart = header.indexOf("SIZE:") + 5;
      int sizeEnd = header.indexOf("\n");

      filename = header.substring(fileStart, header.indexOf(" ", fileStart));
      expectedSize = header.substring(sizeStart, sizeEnd).toInt();

      if (expectedSize > getDf() * 0.9) {
         __console.error(F("not enough space available for the file!"));
         return EXIT_FAILURE;
      }

      _CONSOLE_INFO(F("receive file: %s (size: %d Bytes)"), filename.c_str(), expectedSize);

      // file open
      file = LittleFS.open(filename, "w");
      if (!file) {
         __console.error(F("error: create file %s"), filename.c_str());
         return EXIT_FAILURE;
      }
   } else {
      __console.error(F("error: invalid header received during file transfer"));
      return EXIT_FAILURE;
   }

   // receive file data
   CxTimer timerTO(5000);  // set timeout

   while (client->connected() && receivedSize < expectedSize) {
      size_t bytesToRead = client->available();
      if (bytesToRead > 0) {
         size_t bytesRead = client->readBytes(buffer, MIN(bytesToRead, sizeof(buffer)));
         file.write((uint8_t*)buffer, bytesRead);
         receivedSize += bytesRead;
         // printProgress(receivedSize, expectedSize, filename.c_str(), "bytes");
         __console.printProgressBar((uint32_t)receivedSize, (uint32_t)expectedSize, filename.c_str());
         timerTO.restart();          // reset timeout
      } else if (timerTO.isDue()) {  // timeout
         __console.error(F("timeout receiving a file"));
         break;
      }
      delay(1);
   }
   file.close();

   // Empfang überprüfen
   if (receivedSize == expectedSize) {
      _CONSOLE_INFO(F("file transfer finished."));
      return EXIT_SUCCESS;
   } else {
      __console.error(F("received size of data (%d bytes) not same as expected file size (%d bytes)!"), receivedSize, expectedSize);
   }
#endif
   return EXIT_FAILURE;
}

uint8_t _sendFile(WiFiClient* client, const char* filename) {
#ifdef ARDUINO
   // without the log capability, it would print to __ioStream, which is the client stream for the file transfer!
   //_CONSOLE_DEBUG(F("download file: %s"), filename);

   File file = LittleFS.open(filename, "r");
   if (!file) {
      client->println("ERROR: File not found");
      // console.warn("File not found: %s", filename);
      return EXIT_FAILURE;
   }

   size_t fileSize = file.size();
   client->printf("SIZE: %d\n", fileSize);
   //_CONSOLE_INFO("Sending file: %s (%d bytes)\n", filename, fileSize);

   static char buffer[64];
   size_t bytesRead;

   g_Stack.update();

   // Send file data
   while ((bytesRead = file.readBytes(buffer, sizeof(buffer))) > 0) {
      client->write((uint8_t*)buffer, bytesRead);
   }

   file.close();
   //_CONSOLE_INFO("File transfer complete.");
#endif
   return EXIT_SUCCESS;
}

void _printNoFS() {
   __console.println(F("file system not mounted!"));
}

void _printNoSuchFileOrDir(const char* szCmd, const char* szFn) {
   if (szCmd && szFn) __console.printf(F("%s: %s: No such file or directory\n"), szCmd, szFn);
   if (szCmd && !szFn) __console.printf(F("%s: null : No such file or directory\n"), szCmd);
};


uint8_t executeBatch(const char* path, const char* label, const char* arg) {
   if (!path) return EXIT_FAILURE;

   g_Stack.DEBUGPrint(getIoStream(), 0, label);

   String strBatchFile;

   std::map<String, String> mapTempVariables;

   mapTempVariables[F("0")] = label ? label : "?";

   if (label) {
      mapTempVariables[F("LABEL")] = label;
   }
   if (arg) __console.setArgVariables(mapTempVariables, arg);

   strBatchFile = "";

   strBatchFile.reserve((uint32_t)strlen(path) + 5);  // +4 for ".bat" and +1 for null terminator
   strBatchFile = path;

   // veryfy if the file name ends with .bat and if it exists
   if (strBatchFile.length() > 4 && (strBatchFile.endsWith(".bat") || strBatchFile.endsWith(".man"))) {
      // file name is ok
   } else if (strBatchFile.length() > 0) {
      // add extension, presume it is a batch file
      strBatchFile += ".bat";
   } else {
      __console.error(F("Invalid batch/man file name '%s'. Must end with .bat or .man"), path);
      return EXIT_FAILURE;
   }

   if (label == nullptr) {
      label = "default";
   };

   _CONSOLE_INFO(F("Execute batch file: %s %s"), strBatchFile.c_str(), label);
   if (arg) _CONSOLE_INFO(F("Arguments: %s"), arg);

   uint8_t nExitValue = EXIT_FAILURE;

#ifdef ARDUINO
   if (!LittleFS.exists(strBatchFile.c_str())) {
      __console.error(F("Batch file '%s' not found"), strBatchFile.c_str());
      return EXIT_FAILURE;
   }

   File file = LittleFS.open(strBatchFile.c_str(), "r");
   if (!file) {
      __console.error(F("Failed to open batch file '%s"), strBatchFile.c_str());
      return EXIT_FAILURE;
   }

   bool processCommands = true;  // Start processing commands immediately
   _bBreakBatch = false;
   _nBatchDepth++;  // executeBatch will be called recursively, note the depth

   const size_t LINE_BUFFER_SIZE = 256;

   char* buffer = new char[LINE_BUFFER_SIZE];

   if (buffer) {
      g_Stack.DEBUGPrint(getIoStream(), 0, "buffer");

      while (file.available()) {
         size_t len = file.readBytesUntil('\n', buffer, LINE_BUFFER_SIZE - 1);
         buffer[len] = '\0';  // Null-terminate the string
         trim(buffer);        // Remove any leading/trailing whitespace

         // If the buffer filled up and no newline was found, discard the rest of the line
         if (len == LINE_BUFFER_SIZE - 1 && buffer[len - 1] != '\n') {
            char c;
            while (file.available() && (c = file.read()) != '\n') {
               // Discard characters
            }
         }

         if (strlen(buffer) == 0 || buffer[0] == '#') {
            // Ignore empty lines and comments
            continue;
         }

         // Remove inline comments starting with #
         char* commentStart = strchr(buffer, '#');
         if (commentStart && *(commentStart - 1) != '$' && (len > 2 && *(commentStart - 2) != '$' && *(commentStart - 1) != '(')) {  // $# and $(#) are not comments
            *commentStart = '\0';                                                                                                    // Truncate the line at the # character
            trim(buffer);                                                                                                            // Remove any trailing whitespace after truncation
         }

         if (strlen(buffer) == 0) {
            // If the line becomes empty after removing the comment, skip it
            continue;
         }

         // Check if the line is a variable definition
         char* equalsSign = strchr(buffer, '=');
         if (equalsSign) {
            static String varName;
            static String varValue;

            // Ensure the equal sign is in the first word
            varName = String(buffer).substring(0, equalsSign - buffer);
            varName.trim();

            // Ensure the equal sign is part of the first word (no spaces in the variable name), otherwise treat it as a command
            if (!varName.isEmpty() && varName.indexOf(' ') == -1) {
               varValue = String(buffer).substring(equalsSign - buffer + 1);
               varValue.trim();

               // Substitue value with local variables first
               __console.substituteVariables(varValue, mapTempVariables, false);

               // Substitue value with global variables
               __console.substituteVariables(varValue);

               mapTempVariables[varName] = varValue;  // Store the variable
               continue;
            }
            g_Stack.DEBUGPrint(getIoStream(), 0, "Variables");
         }

         // Handle variables in the batch file
         static String command;
         uint32_t extra_size = 50;  // FIXME: max. length of a variable length (to be determined actually)

         command.reserve(strlen(buffer) + extra_size);  // Reserve enough space for the command and potential longer label
         command = buffer;

         // Substitue command with local variables first
         __console.substituteVariables(command, mapTempVariables, false);

         // Substitue command with global variables
         //__console.substituteVariables(command);

         if (command.endsWith(":")) {
            // Check for labels
            processCommands = ((command == String(label) + ":") || command == "all:");
            continue;
         }

         if (processCommands) {
            _CONSOLE_DEBUG(F("Batch command: %s"), command.c_str());

            if (command.startsWith("exec")) {
               __console.substituteVariables(command);  // needed ?
               CxStrToken tkExecCmd(command.c_str(), " ");
               _CONSOLE_DEBUG(F("exec command found: %s"), command.c_str());
               // recursively call executeBatch and not go deeper by calling processCmd, this shall safe stack usage
               nExitValue = executeBatch(TKTOCHAR(tkExecCmd, 1), TKTOCHAR(tkExecCmd, 2), TKTOCHAR(tkExecCmd, 3));
            } else {
               g_Stack.DEBUGPrint(getIoStream(), +1, "processCmd-A");
               nExitValue = __console.processCmd(*__console.getStream(), command.c_str(), 0);  // MARK: getStream needed here?
               g_Stack.DEBUGPrint(getIoStream(), -1, "processCmd-B");
            }

            if (_bBreakBatch) break;
         }
      }  // while (file.available())

      _bBreakBatch = false;  // limits the break for the current batch, not for the upper one in nested calls

      delete[] buffer;
   }

   file.close();
#endif
   mapTempVariables.clear();

   g_Stack.DEBUGPrint(getIoStream(), 0, "end");

   // by default, switch echo on again, after processing a batch file at lowest recursive depth
   if (_nBatchDepth <= 1) __console.setEcho(true);

   if (_nBatchDepth > 0) _nBatchDepth--;

   return nExitValue;
}

uint8_t man(const char* szCap, const char* szParam) {
   return executeBatch("man.man", szCap, szParam);
}

void trim(char* str) {
   char* end;

   // Trim leading space
   while (isspace((unsigned char)*str)) str++;

   if (*str == 0) return;

   // Trim trailing space
   end = str + strlen(str) - 1;
   while (end > str && isspace((unsigned char)*end)) end--;

   // Write new null terminator
   *(end + 1) = 0;
}

bool test(std::vector<const char*>& vExpression) {
   // implement condition evaluation utility
   // test <expression> <cmd>
   // expressions:
   // -e <file>     True if file exists
   // -z <string>   True if the length of string is zero
   // -n <string>   True if the length of string is nonzero
   // s1 = s2       True if s1 == s2
   // s1 != s2      True if s1 != s2
   // n1 -eq n2     True if n1 == n2
   // n1 -ne n2     True if n1 != n2
   // n1 -lt n2     True if n1 < n2
   // n1 -le n2     True if n1 <= n2
   // n1 -gt n2     True if n1 > n2
   // n1 -ge n2     True if n1 >= n2
   // ! <expression> True if expression is false

   if (vExpression.empty()) return false;

   if (strcmp(vExpression[0], "!") == 0 && vExpression.size() > 1) {
      std::vector<const char*> subExpression(vExpression.begin() + 1, vExpression.end());
      return !test(subExpression);
   } else if ((strcmp(vExpression[0], "-e") == 0 || strcmp(vExpression[0], "-f") == 0) && vExpression.size() == 2) {
      // check if file exists
      return fileExists(vExpression[1]);
   } else if (strcmp(vExpression[0], "-z") == 0 && vExpression.size() == 2) {
      // check if string is empty
      return strlen(vExpression[1]) == 0;
   } else if (strcmp(vExpression[0], "-n") == 0 && vExpression.size() == 2) {
      // check if string is not empty
      return strlen(vExpression[1]) > 0;
   } else if (vExpression.size() == 3) {
      char* end1;
      char* end2;
      float n1 = std::strtof(vExpression[0], &end1);
      while (isspace(*end1)) end1++;
      float n2 = std::strtof(vExpression[2], &end2);
      while (isspace(*end2)) end2++;
      if (vExpression[0] != end1 && *end1 == '\0' && vExpression[2] != end2 && *end2 == '\0') {
         // check numeric comparison
         if (strcmp(vExpression[1], "-eq") == 0) {
            return n1 == n2;
         } else if (strcmp(vExpression[1], "-ne") == 0) {
            return n1 != n2;
         } else if (strcmp(vExpression[1], "-lt") == 0) {
            return n1 < n2;
         } else if (strcmp(vExpression[1], "-le") == 0) {
            return n1 <= n2;
         } else if (strcmp(vExpression[1], "-gt") == 0) {
            return n1 > n2;
         } else if (strcmp(vExpression[1], "-ge") == 0) {
            return n1 >= n2;
         }
      } else {
         // check string comparison
         if (strcmp(vExpression[1], "=") == 0) {
            return strcmp(vExpression[0], vExpression[2]) == 0;
         } else if (strcmp(vExpression[1], "!=") == 0) {
            return strcmp(vExpression[0], vExpression[2]) != 0;
         }
      }
   }

   return false;
}

#endif