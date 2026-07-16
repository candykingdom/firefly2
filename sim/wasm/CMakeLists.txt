cmake_minimum_required(VERSION 3.21)

project(firefly-simulator-renderer LANGUAGES CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR
          "Configure this target with Emscripten SDK 6.0.3 (emcmake cmake ...)")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(FIREFLY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(FIREFLY_SIM_OUTPUT_DIR "${FIREFLY_ROOT}/sim/generated" CACHE PATH
    "Directory for the generated simulator renderer")

FetchContent_Declare(
  fake-fast-led
  GIT_REPOSITORY https://github.com/ademuri/FakeFastLED.git
  GIT_TAG f00dd2dd4efc34e90c16dd6a1a8eada0922d56ca
)
FetchContent_MakeAvailable(fake-fast-led)

# Keep this list explicit and ordered. It is both the Wasm compilation input
# and part of the committed artifact's reproducibility contract.
set(FIREFLY_RENDERER_SOURCES
  ${FIREFLY_ROOT}/lib/color/ColorPalette.cpp
  ${FIREFLY_ROOT}/lib/device/DeviceDescription.cpp
  ${FIREFLY_ROOT}/lib/device/StripDescription.cpp
  ${FIREFLY_ROOT}/lib/device/DeviceCatalog.cpp
  ${FIREFLY_ROOT}/lib/effect/Effect.cpp
  ${FIREFLY_ROOT}/lib/effect/EffectRegistry.cpp
  ${FIREFLY_ROOT}/lib/effect/ColorCycleEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/ContrastBumpsEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/ControlEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/DarkEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/DisplayColorPaletteEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/FireEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/FireflyEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/LightningEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/PoliceEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/PrideEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/RainbowBumpsEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/RainbowEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/RorschachEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/SimpleBlinkEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/SparkEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/StopLightEffect.cpp
  ${FIREFLY_ROOT}/lib/effect/SwingingLights.cpp
  ${FIREFLY_ROOT}/lib/fake-radio/FakeRadio.cpp
  ${FIREFLY_ROOT}/lib/led_manager/LedManager.cpp
  ${FIREFLY_ROOT}/lib/math/Math.cpp
  ${FIREFLY_ROOT}/lib/radio/Radio.cpp
  ${FIREFLY_ROOT}/lib/types/Types.cpp
  ${FIREFLY_ROOT}/src/generic/NetworkManager.cpp
  ${FIREFLY_ROOT}/src/generic/RadioStateMachine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/SimulatorRenderer.cpp
  ${CMAKE_CURRENT_LIST_DIR}/SimulatorBridge.cpp
)

add_executable(firefly-renderer ${FIREFLY_RENDERER_SOURCES})

target_include_directories(firefly-renderer PRIVATE
  ${FIREFLY_ROOT}/lib/color
  ${FIREFLY_ROOT}/lib/debug
  ${FIREFLY_ROOT}/lib/device
  ${FIREFLY_ROOT}/lib/effect
  ${FIREFLY_ROOT}/lib/fake-radio
  ${FIREFLY_ROOT}/lib/led_manager
  ${FIREFLY_ROOT}/lib/math
  ${FIREFLY_ROOT}/lib/radio
  ${FIREFLY_ROOT}/lib/types
  ${FIREFLY_ROOT}/src/generic
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(firefly-renderer PRIVATE fake-fast-led)

target_compile_options(firefly-renderer PRIVATE
  -O3
  -Wall
  -Wextra
  -Wno-mismatched-tags
  -Wno-unused-template
  -fno-exceptions
  -fno-rtti
  "-ffile-prefix-map=${FIREFLY_ROOT}=."
)

set(FIREFLY_EXPORTED_FUNCTIONS
    "['_ff_sim_abi_version','_ff_sim_effect_count','_ff_sim_effect_name','_ff_sim_effect_weight','_ff_sim_random_effect_count','_ff_sim_palette_count','_ff_sim_palette_name','_ff_sim_palette_color_count','_ff_sim_palette_color_hsv','_ff_sim_device_count','_ff_sim_device_name','_ff_sim_device_milliamps','_ff_sim_device_strip_count','_ff_sim_strip_led_count','_ff_sim_strip_flags','_ff_sim_renderer_create','_ff_sim_renderer_destroy','_ff_sim_render_device','_ff_sim_render_strip','_ff_sim_render_size','_ff_sim_render_data']")

target_link_options(firefly-renderer PRIVATE
  -O3
  --no-entry
  -sWASM=1
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sEXPORT_NAME=createFireflyRenderer
  -sENVIRONMENT=web,node
  -sFILESYSTEM=0
  -sALLOW_MEMORY_GROWTH=1
  -sASSERTIONS=0
  "SHELL:-sEXPORTED_FUNCTIONS=${FIREFLY_EXPORTED_FUNCTIONS}"
  "SHELL:-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','HEAPU8']"
  "SHELL:-sINCOMING_MODULE_JS_API=['locateFile']"
)

set_target_properties(firefly-renderer PROPERTIES
  OUTPUT_NAME firefly-renderer
  SUFFIX .js
  RUNTIME_OUTPUT_DIRECTORY "${FIREFLY_SIM_OUTPUT_DIR}"
)
