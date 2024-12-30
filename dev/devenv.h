//
//  devenv.h
//  xESP
//
//  Created by ocfu on 12.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef devenv_h
#define devenv_h

#include <stdio.h>
#include <iostream>
#include <fstream>


// dummies
class Stream {
public:
   int available() {return 0;}
   char read() {return '\000';}
   void println(){}
   void println(const char* sz){}
   void print(const char c){}
   void print(const char* sz){}
   void printf(const char* sz...){}
   void vprintf(const char*, va_list){}
   void flush(){}
};

class HardwareSerial : public Stream {};
class SoftwareSerial : public Stream {};
class WiFiClient : public Stream {public: bool connected() {return false;}};
class WiFiServer {public: WiFiClient available() {return WiFiClient();} };

#define File int
#define Dir int

struct FSInfo {
   size_t totalBytes;
   size_t usedBytes;
   size_t blockSize;
   size_t pageSize;
   size_t maxOpenFiles;
   size_t maxPathLength;
};



#endif /* devenv_h */
