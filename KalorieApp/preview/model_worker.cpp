// Laeser praecis et RGB888-billede fra stdin og skriver det faktiske modelresultat som JSON.
#include <iostream>
#include <iomanip>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "rgb_billede.h"

static_assert(EI_CLASSIFIER_PROJECT_DEPLOY_VERSION == 8 &&
              EI_CLASSIFIER_INPUT_WIDTH == 96 && EI_CLASSIFIER_INPUT_HEIGHT == 96,
              "Previewet kraever RGB-model 1.0.8 / deployment 8.");
static uint8_t pixels[RGBBillede::BYTES];
static RGBBillede image;
static int signalData(size_t offset, size_t length, float* out) {
  return image.signal(offset, length, out);
}

int main() {
  // En separat indlaesningsbuffer undgaar overlappende memcpy i RGBBillede.
  uint8_t input[RGBBillede::BYTES];
  std::cin.read(reinterpret_cast<char*>(input), sizeof(input));
  if (std::cin.gcount() != sizeof(input) || std::cin.peek() != std::char_traits<char>::eof()) {
    std::cerr << "Forventede 96x96 RGB888-pixels.";
    return 2;
  }
  image.start(pixels, sizeof(pixels));
  image.tilfoej(input, sizeof(input));
  image.afslut();
  signal_t signal;
  signal.total_length = RGBBillede::PIXELS;
  signal.get_data = &signalData;
  ei_impulse_result_t result = {};
  const EI_IMPULSE_ERROR error = run_classifier(&signal, &result, false);
  if (error != EI_IMPULSE_OK) {
    std::cerr << "Edge Impulse-fejl: " << int(error);
    return 3;
  }
  size_t best = 0;
  for (size_t i = 1; i < EI_CLASSIFIER_LABEL_COUNT; ++i)
    if (result.classification[i].value > result.classification[best].value) best = i;
  std::cout << "{\"label\":" << std::quoted(result.classification[best].label)
            << ",\"confidence\":" << result.classification[best].value
            << ",\"inferenceMs\":" << result.timing.classification
            << ",\"predictions\":[";
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
    if (i) std::cout << ',';
    std::cout << "{\"label\":" << std::quoted(result.classification[i].label)
              << ",\"confidence\":" << result.classification[i].value << '}';
  }
  std::cout << "]}\n";
}
