# Code Style Guide

This guide outlines the coding conventions for the project to ensure consistency and maintainability. The codebase targets the **Arduino IDE** and **CLI** for compatibility with both **ESP8266** and **ESP32** platforms. The code must adhere to **C++17** standards.

---

## 🎯 General Principles
- Prioritize readability and maintainability.
- Avoid public class members; use protected or private members accessed through methods.
- Always use English for naming conventions and comments.

---

## 🔧 Naming Conventions

### Classes
- Use **PascalCase** for class names.
- Example: `class MotorController`.

### Methods
- Use **camelCase** for method names.
- Example: `void initializeMotor();`

### Variables
- Prefer `int8_t` for `byte`, `int32_t` for `int` etc.
- Prefer `bool` for `boolean`
- Follow the prefixes below based on scope and type:

#### Private Members
- Start with `_` followed by a type prefix:
  - **`sz`**: C-style null-terminated strings.
  - **`str`**: `String`.
  - **`b`**: Boolean.
  - **`n`**: Integer types (`int8_t`, `uint32_t`, etc.).
  - **`i`**: Unsigned integer types as index (0: first item).  
  - **`f`**: Floating-point types (`float`, `double`).
  - **`p`**: Pointer.
  - **`a`**: Arrays.
  - **`vec`**: `std::vector`.
  - **`map`**: `std::map`.

Example: `_bInitialized`, `_nCounter`, `_pMotor`, `_szName`, `_aItems[i]`, `_fValue`, `_strUrl`

#### Protected Members
- Start with `__` followed by the same type prefix as private members.

Example: `__szName`, `__fSpeed`.

### Constants
- Use **UPPER_SNAKE_CASE** for constants, using same type prefix as for members.

- Example: `const int _nMAX_SPEED = 100;`

#### Global Variables
- Avoid global variables whenever possible. If necessary, declare them as static class members.
- If no appropriate class exists, use the prefix `g_` and follow the same naming conventions as for members.
- For globally defined objects (instances), use PascalCase.

Example:
```cpp
int g_nGlobalCounter;
bool g_bIsActive;
MotorController GlobalMotorController;
```
#### Static Class Members
Static class members should be used when shared state (member) or behavior (method) is required across all instances of a class.

Example:
```cpp
class Logger {
private:
   static int _nLogCount;

public:
   static void logMessage(const char* szMessage) {
      _nLogCount++;
      Serial.println(szMessage);
   }

   static int getLogCount() {
      return _nLogCount;
   }
};

// Define the static member outside the class
int Logger::_nLogCount = 0;

// Usage
Logger::logMessage("System started.");
Serial.println(Logger::getLogCount());
```


---

## 🌲 Code Structure

### Classes
- Class definitions should include:
  - A private section for member variables.
  - Protected methods for inherited functionality.
  - Public methods for interfacing.

Example:
```cpp
class Example {
private:
   int _nPrivateMember;

protected:
   void __protectedMethod();

public:
   void publicMethod();
};
```

### Method Implementation
- Open curly braces `{` at the end of the same line.
- Indent with **3 spaces**, not tabs.

Example:
```cpp
void Example::publicMethod() {
   if (_nPrivateMember > 0) {
      _nPrivateMember++;
   }
}
```

---

## 🚀 Platform-Specific Notes

### Arduino IDE Compatibility
- Use `#ifdef` or `#if defined` to differentiate between ESP8266 and ESP32 code where required.

Example:
```cpp
#ifdef ESP32
   // ESP32-specific code
#endif
```

- Use `#ifdef` or `#if defined` to differentiate between ARDUINO and non-ARDUINO code (e.g. for test and dev environments).

Example:
```cpp
#ifdef ARDUINO
   // ARDUINO-specific code
#else
   // when compiled in a test or dev environment
#endif
```

### Memory Usage
- Be mindful of limited memory on microcontrollers, especially the ESP8266.
- Avoid dynamic memory allocation where possible.

## 📝 Golden Rules for the Arduino String Class
The Arduino `String` class is convenient but can lead to memory fragmentation on devices with limited RAM. Follow these rules to use it safely:

1. **Minimize Usage**: Prefer null-terminated C-strings (`char[]`) for small, fixed-length strings.
2. **Avoid Frequent Concatenations**: Repeated concatenation creates temporary objects, increasing memory usage.
3. **Reserve Memory**: Use `reserve()` to allocate enough memory upfront, especially in loops.
   ```cpp
   String str;
   str.reserve(50);
   ```
4. **Release Memory**: Call `String::remove()` or `String::clear()` to free unused memory when a `String` object is no longer needed.
   ```cpp
   str.remove(0); // Clears content
   ```
5. **Monitor Heap Usage**: On ESP8266/ESP32, monitor free heap memory with `ESP.getFreeHeap()` to detect leakage and `ESP.getHeapFragmentation()` to detect fragmentation.

6. **Avoid Global Strings**: Declare `String` variables inside functions or classes to limit their lifetime and avoid memory leaks.

Example:
```cpp
void handleInput() {
   String input;
   input.reserve(100); // Allocate memory upfront
   while (Serial.available()) {
      char c = Serial.read();
      input += c;
   }
   Serial.println(input);
}
```


---

## ✅ Best Practices
- **Use descriptive names**: Variables and methods should clearly indicate their purpose.
- **Comment thoroughly**: Add meaningful comments explaining the "why" behind non-obvious logic.
- **Test rigorously**: Test on both ESP8266 and ESP32 devices.

---

## Example Code
```cpp
class MotorController {
private:
   bool _bIsRunning;
   int _nSpeed;

protected:
   void __setDefaults();

public:
   MotorController();
   void startMotor(int nSpeed);
   void stopMotor();
};

MotorController::MotorController() {
   __setDefaults();
}

void MotorController::__setDefaults() {
   _bIsRunning = false;
   _nSpeed = 0;
}

void MotorController::startMotor(int nSpeed) {
   _bIsRunning = true;
   _nSpeed = nSpeed;
}

void MotorController::stopMotor() {
   _bIsRunning = false;
   _nSpeed = 0;
}
```

This guide ensures the code remains clean, consistent, and portable between supported platforms.

