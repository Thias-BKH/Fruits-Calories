#pragma once
#include <stdint.h>

// Faelles handlinger for slideren, Serial Monitor og appens navigation.
enum class Input : uint8_t { Tryk = 1, SwipeOp = 2, SwipeNed = 3, Tilbage = 4 };

// Gesture-ID holder alle scrolltrin fra den samme fingerbevaegelse samlet.
struct TouchEvent {
  Input handling;
  uint32_t gesture;
};
