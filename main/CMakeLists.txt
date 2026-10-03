idf_component_register(
    SRCS
        "main.cpp"
        "core0.cpp"
        "core1.cpp"
        "Adafruit_NeoPixel/Adafruit_NeoPixel.cpp"
        "Adafruit_NeoPixel/esp.c"

    INCLUDE_DIRS
        "."
        "Adafruit_NeoPixel"

    REQUIRES
        arduino-esp32
        esp32-camera
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=missing-field-initializers
)      