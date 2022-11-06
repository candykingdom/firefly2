
#include <chrono>
#include <cmath>
#include <iostream>

#include "FakeRadio.hpp"
#include "SimulatorLedManager.hpp"

int main() {
  constexpr uint16_t kNumLeds = 20;
  StripDescription strip(kNumLeds, {});
  const DeviceDescription device(5000, {&strip});

  FakeRadio radio;
  NetworkManager *network_manager = new NetworkManager(&radio);
  RadioStateMachine state_machine(network_manager);
  SimulatorLedManager<kNumLeds> led_manager(&device, &state_machine);

  const std::chrono::steady_clock::time_point start_time =
      std::chrono::steady_clock::now();

  while (led_manager.Run()) {
    led_manager.RunEffect();
    std::chrono::steady_clock::time_point end_time =
        std::chrono::steady_clock::now();
    setMillis(std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                    start_time)
                  .count());
  }

  return 0;
}
