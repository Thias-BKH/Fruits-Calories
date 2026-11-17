// Desktop-adapter til de platformfunktioner, Edge Impulse kraever.
// SDK/model fra zip-filen aendres ikke. Modeldiagnostik gaar til stderr, JSON til stdout.
#include <chrono>
#include <thread>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"

EI_IMPULSE_ERROR ei_run_impulse_check_canceled() { return EI_IMPULSE_OK; }
EI_IMPULSE_ERROR ei_sleep(int32_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  return EI_IMPULSE_OK;
}
uint64_t ei_read_timer_us() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
}
uint64_t ei_read_timer_ms() { return ei_read_timer_us() / 1000; }
void ei_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}
void ei_printf_float(float value) { fprintf(stderr, "%f", double(value)); }
void* ei_malloc(size_t size) { return malloc(size); }
void* ei_calloc(size_t count, size_t size) { return calloc(count, size); }
void ei_free(void* pointer) { free(pointer); }
extern "C" void DebugLog(const char* text) { fputs(text, stderr); }
