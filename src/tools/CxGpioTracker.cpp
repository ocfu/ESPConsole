#include <tools/CxGpioTracker.hpp>

// isr handling
volatile uint32_t g_anEdgeCounter[3] = {0, 0, 0};
volatile uint32_t g_anLastInterruptTime[3] = {0, 0, 0};
volatile uint32_t g_anDebounceDelay[3] = {20000, 20000, 20000};  // debounce time in microseconds (20ms default)

void IRAM_ATTR handleInterrupt(uint8_t idx) {
   uint32_t now = (uint32_t)micros();
   if ((now - g_anLastInterruptTime[idx]) > g_anDebounceDelay[idx]) {
      g_anEdgeCounter[idx] = g_anEdgeCounter[idx] + 1;
      g_anLastInterruptTime[idx] = now;
   }
}

void IRAM_ATTR handleInterrupt0() { handleInterrupt(0); }
void IRAM_ATTR handleInterrupt1() { handleInterrupt(1); }
void IRAM_ATTR handleInterrupt2() { handleInterrupt(2); }
