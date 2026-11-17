#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Telefonen sender 96x96 RGB888 (R,G,B pr. pixel), uafhaengigt af CPU-endianness.
// Under scanning genbruges skaermbufferen. Der tegnes ikke, mens den indeholder RGB.
class RGBBillede {
 public:
  static constexpr size_t PIXELS = 96 * 96;
  static constexpr size_t BYTES = PIXELS * 3;

  bool start(uint8_t* buffer, size_t kapacitet) {
    if (startet) { gyldig = false; return false; }  // Kun en fil pr. request.
    data = buffer; antal = 0; startet = true; faerdig = false;
    gyldig = buffer && kapacitet >= BYTES;
    return gyldig;
  }
  bool tilfoej(const uint8_t* bytes, size_t length) {
    if (!startet || faerdig || !gyldig || !bytes || length > BYTES - antal) {
      gyldig = false;
      return false;
    }
    memcpy(data + antal, bytes, length);
    antal += length;
    return true;
  }
  void afslut() { faerdig = true; }
  void nulstil() { data = nullptr; antal = 0; startet = false; faerdig = false; gyldig = false; }
  bool klar() const { return startet && faerdig && gyldig && antal == BYTES; }
  int signal(size_t offset, size_t length, float* out) const {
    if (!klar() || !out || offset > PIXELS || length > PIXELS - offset) return -1;
    for (size_t i = 0; i < length; ++i) {
      const uint8_t* rgb = data + (offset + i) * 3;
      out[i] = float((uint32_t(rgb[0]) << 16) | (uint32_t(rgb[1]) << 8) | rgb[2]);
    }
    return 0;
  }
 private:
  uint8_t* data = nullptr;
  size_t antal = 0;
  bool startet = false, faerdig = false, gyldig = false;
};
