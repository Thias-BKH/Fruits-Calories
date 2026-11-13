# Edge Impulse model + SDK (unpacked from the Arduino export zip) as an ESP-IDF component.
# Mirrors the ESP32 branch of edge-impulse-sdk/cmake/zephyr/CMakeLists.txt.
set(EI_SRC "${CMAKE_CURRENT_LIST_DIR}/src")
set(EI_ESP "${EI_SRC}/edge-impulse-sdk/porting/espressif")

file(GLOB_RECURSE EI_SOURCES "${EI_SRC}/*.cpp" "${EI_SRC}/*.cc" "${EI_SRC}/*.c")
# CMSIS is ARM-only; the arduino/particle porting layers are for other boards.
list(FILTER EI_SOURCES EXCLUDE REGEX "/CMSIS/|/porting/(arduino|particle)/")
# Hand-written ESP32-S3 assembly kernels for ESP-NN (neural net) and esp-dsp (FFT).
file(GLOB_RECURSE EI_ASM "${EI_ESP}/ESP-NN/*.S" "${EI_ESP}/esp-dsp/modules/fft/*.S")

idf_component_register(SRCS ${EI_SOURCES} ${EI_ASM}
                       INCLUDE_DIRS "src" "src/edge-impulse-sdk" "src/edge-impulse-sdk/porting"
                                    "${EI_ESP}/ESP-NN/include" "${EI_ESP}/ESP-NN/src/common"
                                    "${EI_ESP}/esp-dsp/modules/common/include" "${EI_ESP}/esp-dsp/modules/fft/include"
                       PRIV_REQUIRES esp_timer)

# The .S kernels never see sdkconfig.h, so the SDK can't detect the S3 there by itself.
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN=1 EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN_S3=1)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error -Wno-unused-variable -Wno-maybe-uninitialized -Wno-missing-field-initializers -Wno-deprecated-declarations)
# The SDK headers trip these warnings in any file that includes them; don't fail the app build on them.
target_compile_options(${COMPONENT_LIB} INTERFACE -Wno-error=maybe-uninitialized -Wno-error=uninitialized)
