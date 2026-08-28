#ifndef CxCapabilitySerial_hpp
#define CxCapabilitySerial_hpp

#include "../capabilities/CxCapabilityFS.hpp"
#include "../tools/CxGpioTracker.hpp"
#include "../tools/CxTimer.hpp"
#include "../tools/esptools.h"
#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#ifdef ARDUINO
#include <SoftwareSerial.h>
#else
#ifndef SWSERIAL_7N1
#define SWSERIAL_7N1 0x02
#endif
#ifndef SWSERIAL_8N1
#define SWSERIAL_8N1 0x06
#endif
#ifndef SWSERIAL_7E1
#define SWSERIAL_7E1 0x22
#endif
#ifndef SWSERIAL_8E1
#define SWSERIAL_8E1 0x26
#endif
#include <iostream>
#endif

// typedef wrapper for SoftwareSerial to allow for testing on non-Arduino platforms
#ifdef ARDUINO
typedef EspSoftwareSerial::Config tEspSoftwareSerialConfig;
#else
typedef int tEspSoftwareSerialConfig;
#endif


class CxCapabilitySerial : public CxCapability {
   bool _bEnabled = false;
   
   CxTimer1s  _timerUpdate;

   SoftwareSerial *m_pSerial = NULL;
   
   CxGPIO m_gpioRx;
   CxGPIO m_gpioTx;

   bool _bTestMode = false;
   
   String _strFriendlyName;

   uint16_t _nBaudRate = 115200;
   tEspSoftwareSerialConfig _nProtocol = SWSERIAL_8N1;

   String _strSyncStart = "";
   String _strSyncEnd = "";

   CxGPIODeviceManagerManager& _gpioDeviceManager = CxGPIODeviceManagerManager::getInstance();

  protected:
   /**
    * @var __console
    * @brief Reference to the console instance
    */
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();

  public:
   explicit CxCapabilitySerial() : CxCapability("serial", getCmds()) {}
   static constexpr const char* getName() { return "serial"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "serial" };
      return commands;
   }
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilitySerial>();
   }
   ~CxCapabilitySerial() {
      end();
      _bEnabled = false;
   }
   void end() {
      if (m_pSerial) {
#ifdef ARDUINO
         m_pSerial->end();
#endif         
         delete m_pSerial;
         m_pSerial = nullptr;
      }
   }

   void setup() override {
      CxCapability::setup();
      setIoStream(*__console.getStream());

      _CONSOLE_INFO(F("==== Cap: %s ===="), getName());

      __console.executeBatch("init", getName());
   }

   static CxCapabilitySerial* getInstance() {
      return static_cast<CxCapabilitySerial*>(ESPConsole.getCapInstance("serial"));
   }

   char     _rxBuffer[256];
   uint16_t _indexBuffer = 0;
   
   uint8_t _nStateSync = 0; // 0: waiting for sync start string, 1: sync start string received, processing data

   void logRXBuffer() {
      // log 5 bytes of the buffer in hex format for debugging
      // Each byte is represented as one hex digit, followed by a space, and then the ASCII representation (if printable) or a dot (.) if not printable.
      char hexBuf[(5 * 5) + 1] = {0}; // 5 bytes * (2 hex digits + 1 space + 1 ASCII char + 1 space) + null terminator
      for (int i = 0; i < ((_indexBuffer > 5) ? 5 : _indexBuffer) && i < sizeof(_rxBuffer); ++i) {
         snprintf(hexBuf + i * 5, 6, "%02X:%c ", (unsigned char)_rxBuffer[i], isprint(_rxBuffer[i]) ? _rxBuffer[i] : '.');
      }
      _CONSOLE_DEBUG(F("RXBUF: %s ..."), hexBuf);
      __console.addVariable("RXBUF", hexBuf);
   }

   void loop() override {
      if (_bTestMode) {
         test();
      } else if (_bEnabled) {
         while (m_pSerial && m_pSerial->available() > 0) {            
            char c = (char) m_pSerial->read();

            size_t syncLen = _strSyncStart.length();
            size_t endLen = _strSyncEnd.length();

            if ( syncLen == 0) {
               _nStateSync = 1;  // no sync start string, go to processing data directly
            }

            _rxBuffer[_indexBuffer++] = c;

            switch (_nStateSync) {
               case 0: // waiting for sync start string
                  if (_indexBuffer >= syncLen &&
                      memcmp(_rxBuffer + _indexBuffer - syncLen,
                             _strSyncStart.c_str(),
                             syncLen) == 0) {
                     _CONSOLE_DEBUG(F("RXSTATE: SYN1"));
                     __console.addVariable("RXSTATE", "SYN1");
                     logRXBuffer();
                     _nStateSync = 1;
                     _indexBuffer = 0;
                  } else if (_indexBuffer >= sizeof(_rxBuffer)) {
                     // Buffer is full but START not yet found.
                     // To avoid missing a START string that spans the buffer boundary,
                     // keep only the last (startLen - 1) bytes. These bytes could be
                     // the beginning of START, so we shift them to the front of the
                     // buffer and continue appending new data after them.
                     _CONSOLE_DEBUG(F("RXSTATE: ErrSYN1."));
                     __console.addVariable("RXSTATE", "ErrSYN1");
                     logRXBuffer();
                    if (syncLen > 1) {
                        memmove(_rxBuffer,
                                _rxBuffer + _indexBuffer - (syncLen - 1),
                                syncLen - 1);
                        _indexBuffer = syncLen - 1;
                     } else {
                        _indexBuffer = 0;
                     }
                   }
                  break;
               case 1:  // processing data
                  // If an end string is defined and the last endLen bytes match it,
                  // the payload is complete and should be processed.
                  if (endLen > 0 &&
                      _indexBuffer >= endLen &&
                      memcmp(_rxBuffer + _indexBuffer - endLen, _strSyncEnd.c_str(), endLen) == 0) {
                     // End found → cut off the end string by inserting '\0'
                     size_t payloadLength = _indexBuffer - endLen;
                     _rxBuffer[payloadLength] = '\0';  // null-terminate the payload

                     _CONSOLE_DEBUG(F("RXSTATE: RX1"));
                     __console.addVariable("RXSTATE", "RX1");
                     logRXBuffer();

                     processData(_rxBuffer);

                     // Reset to searching for start
                     _nStateSync = 0;
                     _indexBuffer = 0;

                  } else if (_indexBuffer >= sizeof(_rxBuffer)) {
                     // Buffer is full.
                     // If no end string is defined (endLen == 0), there is no delimiter
                     // to wait for, so we must process (or discard) the accumulated data now.
                     // If an end string is defined but we didn't find it before the buffer
                     // filled up, the data is invalid and should be discarded.
                     if (endLen == 0) {
                        _CONSOLE_DEBUG(F("RXSTATE: RX2"));
                        __console.addVariable("RXSTATE", "RX2");
                        _rxBuffer[_indexBuffer] = '\0';  // ensure null termination
                        processData(_rxBuffer);          // optional: process full buffer
                     } else {
                        _CONSOLE_DEBUG(F("RXSTATE: ErrSYN2."));
                        __console.addVariable("RXSTATE", "ErrSYN2");
                        logRXBuffer();
                     }

                     // Reset state and buffer for the next frame
                     _nStateSync = 0;
                     _indexBuffer = 0;
                  }
                  break;
               default:
                  _nStateSync = 0;  // reset state on unexpected value
                  break;
            }
         }
      }
   }

   void processData(const char* data) {
      // Process the received data as needed
      __console.setOutputVariable(data);
      __console.executeBatch("init", "rx");
   }

   uint8_t execute(const char* szCmd, uint8_t nClient) override {
      // validate the call
      if (!szCmd) return EXIT_FAILURE;

      // get the command and argumetns into the token buffer
      CxStrToken tkCmd(szCmd, " ");

      // we hae a command, find the action to take
      String strCmd = TKTOCHAR(tkCmd, 0);

      // removes heading and trailing white spaces
      strCmd.trim();

      uint8_t nExitValue = EXIT_FAILURE;

      if (strCmd == "?") {
         nExitValue = printCommands();
      } else if (strCmd == "serial") {
         String strSubCmd = TKTOCHAR(tkCmd, 1);
         strSubCmd.toLowerCase();
         nExitValue = EXIT_SUCCESS;
         if (strSubCmd == "enable") {
            _bEnabled = (bool) TKTOINT(tkCmd, 2, 0);
            if (_bEnabled) { 
               nExitValue = init(); 
            } else {
               end();
            }
         } else if (strSubCmd == "setpins" && tkCmd.count() >= 4) {
            nExitValue = setPins(TKTOINT(tkCmd, 2, -1), TKTOINT(tkCmd, 3, -1));
         } else if (strSubCmd == "baud" && tkCmd.count() >= 3) {
            int baudRate = TKTOINT(tkCmd, 2, -1);
            String strProtocol = TKTOCHAR(tkCmd, 3);
            strProtocol.toLowerCase();
            if (strProtocol == "7n1") {
               _nProtocol = SWSERIAL_7N1;
            } else if (strProtocol == "8n1") {
               _nProtocol = SWSERIAL_8N1;
            } else if (strProtocol == "7e1") {
               _nProtocol = SWSERIAL_7E1;
            } else if (strProtocol == "8e1") {
               _nProtocol = SWSERIAL_8E1;
            } else {
               _nProtocol = SWSERIAL_8N1; // default
            }
            if (baudRate > 0) {  
                  _nBaudRate = baudRate;
            } else {
               //_CONSOLE_ERROR(F("Invalid baud rate specified."));
               nExitValue = EXIT_FAILURE;
            }
         } else if (strSubCmd == "fn" && tkCmd.count() >= 3) {
            _strFriendlyName = TKTOCHAR(tkCmd, 2);
         } else if (strSubCmd == "init") {
            nExitValue = init();
         } else if (strSubCmd == "test") {
            _bTestMode = (bool) TKTOINT(tkCmd, 2, 0);
            __console.addVariable("SERTESTMODE", _bTestMode ? "1" : "0");
         } else if (strSubCmd == "sync" && tkCmd.count() >= 3) {
            convertHexSequences(TKTOCHAR(tkCmd, 2), _strSyncStart);
            if (tkCmd.count() >= 4) {
               convertHexSequences(TKTOCHAR(tkCmd, 3), _strSyncEnd);
            } else {
               _strSyncEnd = "";
            }
         } else if (strSubCmd == "send" && tkCmd.count() >= 3) {
            String strData = TKTOCHAR(tkCmd, 2);
            if (m_pSerial) {
               m_pSerial->println(strData);
            }
         } else {
            nExitValue = EXIT_NOT_HANDLED;
         }
      } else {
         //_CONSOLE_ERROR(F("Unknown command: %s"), strCmd.c_str());
         return EXIT_NOT_HANDLED;
      }
      g_Stack.update();
      return nExitValue;
   }

   uint8_t init() {
      if (_bEnabled) {
         end();
      }

      // at least one pin must bet set, otherwise disable the service
      if (hasValidPins()) {
         // check, if gpio is used by the StatusLed

         _CONSOLE_INFO(F("Serial: start ..."));
         if (m_pSerial) {
#ifdef ARDUINO            
            m_pSerial->end();
#endif
            delete m_pSerial;
            m_pSerial = nullptr;
         }

#ifdef ARDUINO

         //Serial.begin(_nBaudRate, SERIAL_8N1);
         //Serial.pins(m_gpioRx.getPin(), m_gpioTx.getPin());

         m_pSerial = new SoftwareSerial(m_gpioRx.getPin(), m_gpioTx.getPin());
         if (m_pSerial) {
            m_pSerial->begin(_nBaudRate, _nProtocol);
         }
         
#endif
         return EXIT_SUCCESS;
      } else {
         __console.error(F("Serial: ### cannot initialize serial, invalid RX or TX pin specified."));
      }
      return EXIT_FAILURE;
   }

   bool init (int nPinRx, int nPinTx) {
      setPins(nPinRx, nPinTx);
      return init();
   }

   void setEnabled(bool set) { _bEnabled = set; }
   bool isEnabled() { return _bEnabled; }

   bool hasValidPins() { return ((m_gpioRx.isValid() || m_gpioTx.isValid()) && (m_gpioRx.getPin() != m_gpioTx.getPin())); }

   uint8_t setPins(int nPinRx, int nPinTx) {   
      m_gpioRx.setPin(nPinRx);
      m_gpioRx.setPinMode(INPUT);
      m_gpioRx.setGpioName("rx");
      m_gpioTx.setPin(nPinTx);
      m_gpioTx.setPinMode(OUTPUT);
      m_gpioTx.setGpioName("tx");
      return (hasValidPins()) ? EXIT_SUCCESS : EXIT_FAILURE;
   }

   CxGPIO& getGPIOTx() { return m_gpioTx; }
   CxGPIO& getGPIORx() { return m_gpioRx; }

   void test() {
      // send a test message to the console every 10 seconds
      if (_timerUpdate.isDue()) {
         if (_bEnabled) {
            String strTest = __console.getVariable("tstTelegram");
            __console.setOutputVariable(strTest.c_str());
            __console.executeBatch("init", "rx");
         }
      }
   }
};


#endif // CxCapabilitySerial_hpp