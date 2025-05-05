//
//  CxGpioTracker.hpp
//
//
//  Created by ocfu on 30.07.22.
//

#ifndef CxGpioTracker_hpp
#define CxGpioTracker_hpp

#include "CxESPConsole.hpp"

#include <map>

#ifdef ESP32
#define GPIO_MAX_PIN_NUMBER 39 // Maximum GPIO pin number for ESP32
#else
#define GPIO_MAX_PIN_NUMBER 17 // Maximum GPIO pin number for other platforms
#endif

#define GPIO_VIRTUAL_PIN_NUMBER_START 100 // 100 ... 254 are virtual pins
#define VIRTUAL_MODE 254
#define INVALID_MODE 255 // Represents an invalid pin mode
#define INVALID_PIN  255

/**
 * @brief Class to track and manage the state and configuration of GPIO pins.
 *
 * The `CxGPIOTracker` class implements a singleton pattern and provides functionalities
 * for tracking GPIO pin modes, states, PWM, and analog properties. It maintains
 * a map of pin-specific configurations and offers various methods to manipulate
 * and retrieve pin data.
 */
class CxGPIOTracker {
private:
   // Internal structure to store individual GPIO pin properties
   struct GPIOData {
      char szName[4];         // short name for the pin
      uint8_t mode;           // Pin mode (e.g., INPUT, OUTPUT, INPUT_PULLUP)
      bool state;             // Pin state (HIGH or LOW)
      bool pwmEnabled;        // Whether PWM is enabled
      bool isAnalog;          // Whether the pin supports analog functionality
      bool isInverted;        // Whether the pin logic is inverted
      uint16_t analogValue;   // Analog value for the pin (e.g., for PWM or ADC)
      
      // Default constructor initializes all fields to default values
      GPIOData(uint8_t mode = INPUT) : mode(mode), state(LOW), pwmEnabled(false), isAnalog(false), isInverted(false), analogValue(0) {szName[0] = '\0';}
   };

   std::map<uint8_t, GPIOData> _pinData; // Map to store GPIO data by pin number

   /**
    * @brief Ensures a pin has default values initialized in the tracker.
    * @param pin The GPIO pin number.
    */
   void setDefaultValues(uint8_t pin) {
      if (!hasPin(pin)) {
         _pinData[pin] = GPIOData(); // Use default constructor
      }
   }
   
   CxGPIOTracker() = default; // Private constructor to enforce singleton pattern
   ~CxGPIOTracker() = default;

public:
   /**
    * @brief Access the singleton instance of CxGPIOTracker.
    * @return Reference to the singleton instance.
    */
   static CxGPIOTracker& getInstance() {
      static CxGPIOTracker instance; // Constructed on first access
      return instance;
   }
   
   // Disable copying and assignment to enforce singleton pattern
   CxGPIOTracker(const CxGPIOTracker&) = delete;
   CxGPIOTracker& operator=(const CxGPIOTracker&) = delete;
      
   /**
    * @brief Remove a GPIO pin from tracking.
    * @param pin The GPIO pin number.
    */
   void removePin(uint8_t pin) {
      _pinData.erase(pin);
   }
   
   bool isValidPin(uint8_t pin) {
      return (pin <= GPIO_MAX_PIN_NUMBER || isVirtualPin(pin));
   }
   
   bool isVirtualPin(uint8_t pin) {
      return (pin >= GPIO_VIRTUAL_PIN_NUMBER_START && pin < INVALID_PIN);
   }
   
   void printInvalidReason(Stream& stream, uint8_t pin) {
      if (!isValidPin(pin)) stream.printf("invalid pin number! (0...%d)", GPIO_MAX_PIN_NUMBER);
   }

   /**
    * @brief Set the mode of a GPIO pin.
    * @param pin The GPIO pin number.
    * @param mode The desired mode (e.g., INPUT, OUTPUT).
    */
   void setMode(uint8_t pin, uint8_t mode = INVALID_MODE) {
      if (isVirtualPin(pin)) {
         mode = VIRTUAL_MODE; // Treat virtual pins as special case
      }
      setDefaultValues(pin); // Ensure the pin is initialized
      _pinData[pin].mode = mode;
   }
   
   /**
    * @brief Get the mode of a GPIO pin.
    * @param pin The GPIO pin number.
    * @return The pin mode, or INVALID_MODE if the pin is not tracked.
    */
   uint8_t getMode(uint8_t pin) const {
      auto it = _pinData.find(pin);
      return (it != _pinData.end()) ? it->second.mode : INVALID_MODE;
   }
   
   /**
    * @brief Gets the pin mode as a string.
    * @param pin The pin number.
    * @return String representation of the pin mode.
    */
   String getPinModeString(uint8_t pin) {
      return String(getPinModeSz(pin));
   }
   
   const char* getPinModeSz(uint8_t pin) {
      if (isAnalog(pin)) {
         return "ANALOG";
      } else {
         uint8_t mode = getMode(pin);
         switch (mode) {
            case INPUT:
               return "INPUT";
            case OUTPUT:
               return "OUTPUT";
            case INPUT_PULLUP:
               return "INPUT_PULLUP";
#ifdef INPUT_PULLDOWN
            case INPUT_PULLDOWN:
               return "INPUT_PULLDOWN";
#endif
            case OUTPUT_OPEN_DRAIN:
               return "OUTPUT_OPEN_DRAIN";
            case INVALID_MODE:
               return "UNSET";
            case VIRTUAL_MODE:
               return "VIRTUAL I/O";
            default:
               return "UNKNOWN";
         }
      }
   }

#ifndef MINIMAL_COMMAND_SET

   ///
   /// Check if the pin supports analog functionality.
   /// @param pin The pin number.
   ///
   bool isAnalogPin(uint8_t pin) {
#if defined(ESP8266)
      return (pin == A0); // Only A0 (GPIO17) is analog-capable on ESP8266
#elif defined(ESP32)
      // TODO: to be extended to various MCUs
      static const std::set<uint8_t> analogPins = {
         32, 33, 34, 35, 36, 39, // ADC1 channels
         25, 26 // DAC pins
      };
      return analogPins.find(pin) != analogPins.end();
#else
      return false; // Unknown MCU
#endif
   }
#endif /*MINIMAL_COMMAND_SET*/

   ///
   /// Check if the pin is in OUTPUT mode.
   /// @param pin The pin number.
   ///
   bool isOutput(uint8_t pin) {
      return (hasPin(pin) && (_pinData[pin].mode == OUTPUT || _pinData[pin].mode == OUTPUT_OPEN_DRAIN));
   }
   
   ///
   /// Check if the pin is in INPUT mode.
   /// @param pin The pin number.
   ///
   bool isInput(uint8_t pin) {
#ifdef INPUT_PULLDOWN
      return (hasPin(pin) && (_pinData[pin].mode == INPUT || _pinData[pin].mode == INPUT_PULLUP || _pinData[pin].mode == INPUT_PULLDOWN));
#else
      return (hasPin(pin) && (_pinData[pin].mode == INPUT || _pinData[pin].mode == INPUT_PULLUP));
#endif
   }
   
   ///
   /// Check if the pin logic is inverted.
   /// @param pin The pin number.
   ///
   bool isInverted(uint8_t pin) {
      return (hasPin(pin) && _pinData[pin].isInverted);
   }
   
   ///
   /// Sets pin logic to inverted or not inverted
   /// @param pin The pin number.
   /// @param set true: pin logic inverted, false: pin logic not inverted
   ///
   void setInverted(uint8_t pin, bool set) {
      setDefaultValues(pin);
      _pinData[pin].isInverted = set;
   }
   
   ///
   /// Set digital state (HIGH/LOW)
   /// @param pin The pin number.
   /// @param state The pin state (true: HIGH, false: LOW).
   ///
   void setDigitalState(uint8_t pin, bool state) {
      setDefaultValues(pin);
      _pinData[pin].state = state;
   }
   
   ///
   /// Get digital state (HIGH/LOW).
   /// @param pin The pin number.
   /// @return The stored state of the pin.
   ///
   bool getDigitalState(uint8_t pin) {
      return (hasPin(pin) && _pinData[pin].state);
   }
   
#ifndef MINIMAL_COMMAND_SET

   // Set PWM state (enabled/disabled)
   void setPWM(uint8_t pin, bool enabled) {
      setDefaultValues(pin);
      _pinData[pin].pwmEnabled = enabled;
   }
   
   // Get PWM state (enabled/disabled)
   bool isPWM(uint8_t pin) {
      return (hasPin(pin) && _pinData[pin].pwmEnabled);
   }

   // Set analog state (enabled/disabled)
   void setAnalog(uint8_t pin, bool enabled) {
      setDefaultValues(pin);
      _pinData[pin].isAnalog = enabled;
   }
   
   bool isAnalog(uint8_t pin) {
      return (hasPin(pin) && _pinData[pin].isAnalog);
   }
   
   // Set analog value
   void setAnalogValue(uint8_t pin, uint16_t value) {
      setDefaultValues(pin);
      _pinData[pin].analogValue = value;
      setAnalog(pin, true);
   }
   
   // Get analog value
   uint16_t getAnalogValue(uint8_t pin) {
      return hasPin(pin) ? _pinData[pin].analogValue : 0;
   }
#endif /*MINIMAL_COMMAND_SET*/

   const char* getName(uint8_t pin) {
      return hasPin(pin) ? _pinData[pin].szName : "";
   }
   
   void setName(uint8_t pin, const char* name) {
      setDefaultValues(pin);
      snprintf(_pinData[pin].szName, sizeof(_pinData[pin].szName), "%s", name);
   }

   // Check if a pin is being tracked
   bool hasPin(uint8_t pin) const {
      return _pinData.find(pin) != _pinData.end();
   }
   
   // Change the pin (transfer settings from oldPin to newPin)
   bool changePin(uint8_t oldPin, uint8_t newPin) {
      if (!hasPin(oldPin)) {
         return false; // oldPin is not being tracked
      }
      
      removePin(newPin);
      
      // Transfer pin modes
      _pinData[newPin] = _pinData[oldPin];
      
      removePin(oldPin);

      return true; // Successfully changed the pin
   }

   
   // Print the current state of a pin
   void printState(Stream& stream, uint8_t pin) {
      // TODO: make it json format
      stream.printf((ESC_ATTR_BOLD  "Pin %02d" ESC_ATTR_RESET), pin);
      stream.print(" - " ESC_ATTR_BOLD "Mode: " ESC_ATTR_RESET);
      stream.print(getPinModeString(pin).c_str());
      stream.print(", " ESC_ATTR_BOLD "State: " ESC_ATTR_RESET);
      if (isInverted(pin)) {
         stream.printf("!");
      }
      stream.printf("%s", getDigitalState(pin) ? "HIGH" : "LOW");
      stream.print(", " ESC_ATTR_BOLD "PWM: " ESC_ATTR_RESET);
#ifndef MINIMAL_COMMAND_SET
      stream.print(isPWM(pin) ? "Enabled" : "Disabled");
      stream.print(", " ESC_ATTR_BOLD "Analog Value: " ESC_ATTR_RESET);
      stream.printf("%d\n", getAnalogValue(pin));
#else
      stream.println();
#endif
   }
   
   std::vector<uint8_t> getPins() {
      std::vector<uint8_t> pins;
      for (const auto& entry : _pinData) {
         pins.push_back(entry.first);
      }
      return pins;
   }
};


/**
 * @class CxGPIO
 * @brief A wrapper class for individual GPIO pins, utilizing CxGPIOTracker for state management.
 *
 * The CxGPIO class provides high-level abstractions for managing individual GPIO pins,
 * including setting modes, states, PWM, and analog properties.
 */
class CxGPIO {
private:
   uint8_t _nPin;                 // The GPIO pin number
   uint8_t _nPwmChannel;         // PWM channel associated with the pin

protected:
   //CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();  /// Reference to the console instance
   CxGPIOTracker& __gpioTracker = CxGPIOTracker::getInstance(); // Reference to the GPIO tracker singleton

   typedef void (*isr_t)();
   uint8_t __isrMode = 0;
   uint8_t __isrId = -1;
   isr_t __isr = nullptr;

   void setISR(uint8_t id, isr_t p, uint8_t mode = RISING) { setIsrId(id); __isr=p; setIsrMode(mode);}
   void setIsrId(uint8_t set) {__isrId = set;}
   int getIsrId() {return __isrId;}
   
   void setIsrMode(uint8_t set) {__isrMode = set;}
   int getIsrMode() {return __isrMode;}


public:
   /**
    * @brief Constructor for CxGPIO.
    * @param pin The GPIO pin number.
    * @param mode The GPIO mode. Default is INPUT.
    * @param inverted  Whether the pin logic is inverted. Defalut is false.
    */
   CxGPIO(uint8_t pin, uint8_t mode = INVALID_MODE, bool inverted = false) : _nPin(pin), _nPwmChannel(0) {
      if (__gpioTracker.isAnalogPin(pin)) {
         __gpioTracker.setAnalog(pin, true);
      } else if (isValidPin(pin) && isValidMode(mode)) {
         setPin(pin);
         setPinMode(mode);
         setInverted(inverted);
      }
   }
   CxGPIO() : CxGPIO(-1) {}
   
   
   // special constructors (input, interrupt etc.)
   CxGPIO(uint8_t nPin, isr_t p, uint8_t mode = 0x1) {setPin(nPin);setPinMode(INPUT_PULLUP); setISR(0, p, mode);}
//   CxGPIO(int nPin, INPUT_PULLUP, int id = 0, isr_t p = nullptr, int mode = RISING) {setPinMode(nPin, eMode, eUse);setISR(id, p, mode);}

   
   /**
    * @brief Set the pin number for this instance.
    * @param pin The new GPIO pin number.
    * @return True if the pin is valid, false otherwise.
    */
   bool setPin(uint8_t pin) {
      if (isValidPin(pin)) {
         if (isSet(pin)) {
            __gpioTracker.changePin(_nPin, pin);
         }
         _nPin = pin;
      }
      return false;
   }
   
   bool isSet(uint8_t pin) {
      return __gpioTracker.hasPin(pin);
   }
   
   bool isSet() {return isSet(_nPin);}
   
   uint8_t getPin() {return _nPin;}
   
   bool isValidPin(uint8_t pin) {
      return __gpioTracker.isValidPin(pin);
   }
   
   bool isVirtualPin(uint8_t pin) {
      return __gpioTracker.isVirtualPin(pin);
   }

   void setGpioName(const char* name) {
      if (isValidPin(_nPin)) {
         __gpioTracker.setName(_nPin, name);
      }
   }
   
   const char* getGpioName() {
      return __gpioTracker.getName(_nPin);
   }
   
   bool isValid() {return isValidPin(_nPin);}
   bool isVirtual() {return isVirtualPin(_nPin);}
   
   void enableISR() {
      if (__isr != nullptr) {
         int nMode = __isrMode;
         
         switch (__isrMode) {
            case LOW:
               nMode = (isInverted()) ? HIGH : LOW;
               break;
#if 0  // not available for all boards! ref https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
            case HIGH:
               nMode = (isInverted()) ? LOW : HIGH;
               break;
#endif
            case CHANGE:
               break;
            case RISING:
               nMode = (isInverted()) ? FALLING : RISING;
               break;
            case FALLING:
               nMode = (isInverted()) ? RISING : FALLING;
               break;
         }
#ifdef ARDUINO
         //__CONSOLE_DEBUG(F("GPIO%02d: attchInterrupt to pin %d, mode=%d, pinMode=%s"), getPin(), getPin(), nMode, _gpioTracker.getPinModeString().c_str());
         attachInterrupt(digitalPinToInterrupt(getPin()), __isr, nMode);
#endif
         delay(10);
      }
   }
   void disableISR() {
      if (__isr != nullptr) {
#ifdef ARDUINO
         detachInterrupt(digitalPinToInterrupt(getPin()));
#endif
         delay(10);
      }
   }


   
   ///
   /// @brief Verifies a valid mode id.
   /// @param mode The mode id to be checked.
   ///
   static bool isValidMode(uint8_t mode) {
      switch (mode) {
         case INPUT:
         case OUTPUT:
         case INPUT_PULLUP:
#ifdef INPUT_PULLDOWN
         case INPUT_PULLDOWN:
#endif
         case OUTPUT_OPEN_DRAIN:
            return true;
         default:
            return false;
      }
   }
                              
   void remove() {
      __gpioTracker.removePin(_nPin);
   }
   
   void setPwmChannel(uint8_t set) {_nPwmChannel = set;}
   uint8_t getPwmChannel() {return _nPwmChannel;}
   
   // Set and get the inverted property
   void setInverted(bool set) {__gpioTracker.setInverted(_nPin, set); }
   bool isInverted() const { return __gpioTracker.isInverted(_nPin); }
   
   void setPinMode(uint8_t mode) {
      if (isValid()) {
         if (isVirtual()) {
            __gpioTracker.setMode(_nPin, VIRTUAL_MODE);
         } else {
            if (isValidMode(mode)) {
               pinMode(_nPin, mode);
               __gpioTracker.setMode(_nPin, mode);
            }
         }
      }
   }
   
   bool isPinModeSet() {
      return (getPinMode() != INVALID_MODE);
   }
   
   // Get the pin mode as a string
   String getPinModeString() {
      return __gpioTracker.getPinModeString(_nPin);
   }
   
   const char* getPinModeSz() {
      return __gpioTracker.getPinModeSz(_nPin);
   };
   
   // Get the pin mode as a raw value
   uint8_t getPinMode() {
      return __gpioTracker.getMode(_nPin);
   }
   
   bool getDigitalState() {
      bool state = __gpioTracker.getDigitalState(_nPin);
      return isInverted() ? !state : state;
   }
#ifndef MINIMAL_COMMAND_SET
   uint16_t getAnalogValue() {
      return __gpioTracker.getAnalogValue(_nPin);
   }
#endif
   
   // Write a value to the pin and sets the mode implicitly
   void writePin(uint8_t value) {
      if (isValid()) {
         if (value != LOW) value = HIGH;
         
         if (!isVirtual()) {
            // set the pin mode, if not done yet to the default
            if (!isPinModeSet() || !isOutput()) {
               setPinMode(OUTPUT);
            }
            digitalWrite(_nPin, isInverted() ? !value : value);
            
#ifndef ARDUINO
            std::cout << (isInverted() ? !value : value) << std::endl;
#endif
         }
         __gpioTracker.setDigitalState(_nPin, (value == HIGH) != isInverted());
#ifndef MINIMAL_COMMAND_SET
         __gpioTracker.setAnalog(_nPin, false);
#endif
      }
   }
   
   // Read the digital state of the pin and sets the mode implicitly
   bool readPin() {
      if (isValid()) {
         bool state = LOW;
         if (!isVirtual()) {
            // set the pin mode, if not done yet to the default input
            if (!isPinModeSet() || !isInput()) {
               setPinMode(INPUT);
            }
            state = digitalRead(_nPin);
            __gpioTracker.setDigitalState(_nPin, state);
         } else {
            state = __gpioTracker.getDigitalState(_nPin);
         }
         return isInverted() ? !state : state;
      } else {
         return false;
      }
   }
   
#ifndef MINIMAL_COMMAND_SET
   // Enable PWM with inverted logic consideration
   void enablePWM(uint32_t frequency, uint8_t dutyCycle) {
      if (isValid()) {
#if defined(ESP32)
         ledcAttachPin(_nPin, _nPwmChannel);
         ledcSetup(_nPwmChannel, frequency, 8);
         ledcWrite(_nPwmChannel, isInverted() ? 255 - dutyCycle : dutyCycle);
#elif defined(ESP8266)
         analogWriteFreq(frequency);
         analogWrite(_nPin, isInverted() ? map(255 - dutyCycle, 0, 255, 0, 1023) : map(dutyCycle, 0, 255, 0, 1023));
#endif
         __gpioTracker.setPWM(_nPin, true);
         __gpioTracker.setAnalog(_nPin, false);
      }
   }
   
   // Disable PWM
   void disablePWM() {
      if (isValid()) {
#if defined(ESP32)
         ledcDetachPin(_nPin);
#elif defined(ESP8266)
         digitalWrite(_nPin, LOW);
#endif
         __gpioTracker.setPWM(_nPin, false);
      }
   }

   // Read an analog value
   int16_t readAnalog() {
      if (isValid()) {
         if (! isAnalog()) return -1;
         int16_t value = analogRead(_nPin);
         __gpioTracker.setAnalog(_nPin, true);
         __gpioTracker.setAnalogValue(_nPin, value);
         return value;
      } else {
         return -1;
      }
   }
   
   // Write an analog value with inverted logic consideration
   void writeAnalog(uint16_t value) {
      if (isValid()) {
         if (!isAnalog()) return;
#if defined(ESP32)
         ledcAttachPin(_nPin, _nPwmChannel);
         ledcWrite(_nPwmChannel, isInverted() ? 255 - value : value);
#elif defined(ESP8266)
         analogWrite(_nPin, isInverted() ? 1023 - value : value);
#endif
         __gpioTracker.setAnalog(_nPin, true);
         __gpioTracker.setAnalogValue(_nPin, value);
      }
   }
#endif
   
   // Check if the pin is logically HIGH (considers inverted state)
   bool isHigh() {
      if (isInput()) {
         return readPin();
      } else {
         return getDigitalState();
      }
   }
   
   void setHigh() {
      if (isOutput()) {
         writePin(HIGH);
      }
   }
   
   // Check if the pin is logically LOW (considers inverted state)
   bool isLow() {
      return !isHigh();
   }
   
   void setLow() {
      if (isOutput()) {
         writePin(LOW);
      }
   }
   
   void set(int16_t state) {
      if (isAnalog()) {
         writeAnalog(state);
      } else {
         if (state) {
            setHigh();
         } else {
            setLow();
         }
      }
   }
   
   int16_t get() {
      if (isAnalog()) {
         return readAnalog();
      } else {
         return isHigh();
      }
   }
      
   // Toggle the pin state
   void toggle() {
      if (isOutput()) {
         bool currentState = getDigitalState();
         writePin(!currentState);
      }
   }
#ifndef MINIMAL_COMMAND_SET

   // Check if PWM is enabled
   bool isPWM() {
      return __gpioTracker.isPWM(_nPin);
   }
   
   bool isAnalog() {
      return __gpioTracker.isAnalogPin(_nPin);
   }
#endif
   bool isInput() {
      return (isVirtual() || __gpioTracker.isInput(_nPin));
   }
   
   bool isOutput() {
      return (isVirtual() || __gpioTracker.isOutput(_nPin));
   }
   
   // Print the state of the GPIO pin
   void printState(Stream& stream) {
      if (isSet()) {
         get();
         __gpioTracker.printState(stream, _nPin);
      }
   }
};
   
#endif /* CxGpioTracker_hpp */
