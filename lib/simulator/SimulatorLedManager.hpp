#pragma once

#include <fast-led-simulator.h>

#include <Effect.hpp>
#include <Types.hpp>

#include "../led_manager/LedManager.hpp"

template <uint16_t NumLeds>
class SimulatorLedManager : private FastLEDSimulator<NumLeds>,
                            public LedManager {
 public:
  SimulatorLedManager(const DeviceDescription *device,
                      RadioStateMachine *radio_state);
  ~SimulatorLedManager();

  using FastLEDSimulator<NumLeds>::Run;

  void SetLedPositions() override;
  SDL_Point GetInitialSize() override;
  SDL_Point GetInitialPosition() override;

  void SetGlobalColor(const CRGB &rgb) override;
  void SetLed(uint8_t led_index, const CRGB &rgb) override;

 protected:
  void WriteOutLeds() override {}

 private:
  static constexpr int kLedFrameSize = 20;
  static constexpr int kLedSize = 16;

  using FastLEDSimulator<NumLeds>::led_layout_;
};

template <uint16_t NumLeds>
SimulatorLedManager<NumLeds>::SimulatorLedManager(
    const DeviceDescription *device, RadioStateMachine *radio_state)
    : LedManager(device, radio_state) {
  FastLEDSimulator<NumLeds>::Init();
}

template <uint16_t NumLeds>
SimulatorLedManager<NumLeds>::~SimulatorLedManager() {
  FastLEDSimulator<NumLeds>::Close();
}

template <uint16_t NumLeds>
void SimulatorLedManager<NumLeds>::SetLedPositions() {
  for (uint16_t i = 0; i < NumLeds; i++) {
    led_layout_[i].location = {(i + 1) * kLedFrameSize, kLedFrameSize / 2};
    led_layout_[i].frame_size = kLedFrameSize;
    led_layout_[i].led_size = kLedSize;
  }
}

template <uint16_t NumLeds>
SDL_Point SimulatorLedManager<NumLeds>::GetInitialSize() {
  return {kLedFrameSize * (NumLeds + 2), kLedFrameSize * 2};
}

template <uint16_t NumLeds>
SDL_Point SimulatorLedManager<NumLeds>::GetInitialPosition() {
  return {0, 0};
}

template <uint16_t NumLeds>
void SimulatorLedManager<NumLeds>::SetGlobalColor(const CRGB &rgb) {
  for (uint16_t i = 0; i < NumLeds; i++) {
    FastLEDSimulator<NumLeds>::leds[i] = rgb;
  }
}

template <uint16_t NumLeds>
void SimulatorLedManager<NumLeds>::SetLed(uint8_t led_index, const CRGB &rgb) {
  FastLEDSimulator<NumLeds>::leds[led_index] = rgb;
}
