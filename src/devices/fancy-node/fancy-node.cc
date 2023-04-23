#include <Arduino.h>
#include <FastLED.h>
#include <FlashStorage_STM32.h>
#include <exponential-moving-average-filter.h>
#include <median-filter.h>

#include <DeviceDescription.hpp>
#include <Devices.hpp>
#include <StripDescription.hpp>
#include <vector>

#include "../../arduino/FastLedManager.hpp"
#include "../../arduino/RadioHeadRadio.hpp"
#include "../../generic/NetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"

constexpr DeviceMode kDeviceMode = DeviceMode::READ_FROM_FLASH;

const int kLedPin = PB0;
const int kNeopixelPin = PA12;
const int kBatteryPin = PA0;

const DeviceDescription test_device =
    Devices::SimpleRfBoardDescription(1, {Bright});

RadioHeadRadio *radio = new RadioHeadRadio();
NetworkManager nm(radio);
RadioStateMachine state_machine(&nm);
FastLedManager *led_manager;

// val = (previous * 3 + current) / 4
static constexpr uint8_t kBatteryFilterAlpha = 96;
MedianFilter<uint16_t, uint16_t, 5> battery_median_filter{
    filter_functions::ForAnalogRead<kBatteryPin>()};
ExponentialMovingAverageFilter<uint16_t> battery_average_filter{
    []() { return battery_median_filter.GetFilteredValue(); },
    kBatteryFilterAlpha};

static constexpr float kBatteryDead = 3.72;
static constexpr float kBatteryFull = 4.2;

// voltage = battery * 180 / (62 + 180) * 3.3 / 1024
constexpr uint16_t kBatteryDividerHigh = 62;
constexpr uint16_t kBatteryDividerLow = 180;
constexpr uint16_t BatteryVoltageToRawReading(float voltage) {
  return voltage / (kBatteryDividerLow * 3.3 /
                    (kBatteryDividerHigh + kBatteryDividerLow) / 1024);
}

FastLedManager *ReadDeviceFromFlash() {
  DeviceDescription *device =
      (DeviceDescription *)malloc(DeviceDescription::kMaxSize);
  if (device == nullptr) {
    Serial2.println("Failed to malloc DeviceDescription");
    return new FastLedManager(Devices::current, &state_machine);
  }

  uint8_t *device_buffer = (uint8_t *)device;
  for (uint32_t i = 0; i < DeviceDescription::kMaxSize; i++) {
    device_buffer[i] = EEPROM.read(i);
  }

  if (device->check_value != DeviceDescription::kCheckValue) {
    Serial.print("Got invalid check value when reading DeviceDescription: ");
    Serial.println(device->check_value);
    return new FastLedManager(Devices::current, &state_machine);
  }
  return new FastLedManager(*device, &state_machine);
}

void WriteDeviceToFlash() {
  bool same = true;
  uint8_t *device = (uint8_t *)(&Devices::current);
  for (uint32_t i = 0; i < sizeof(Devices::current); i++) {
    if (EEPROM.read(i) != device[i]) {
      same = false;
      break;
    }
  }

  if (same) {
    Serial.println("Current device already written to flash");
    return;
  }

  for (uint32_t i = 0;
       i < sizeof(Devices::current) && i < DeviceDescription::kMaxSize; i++) {
    EEPROM.write(i, device[i]);
  }
  EEPROM.commit();
}

void setup() {
  // check if nBOOT_SEL bit is set
  if (FLASH->OPTR & FLASH_OPTR_nBOOT_SEL) {
    // unlock flash/option
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
    FLASH->OPTKEYR = 0x08192A3B;
    FLASH->OPTKEYR = 0x4C5D6E7F;

    while (FLASH->SR & FLASH_SR_BSY1)
      ;

    // clear nBOOT_SEL bit
    FLASH->OPTR &= ~FLASH_OPTR_nBOOT_SEL;

    // write
    FLASH->CR |= FLASH_CR_OPTSTRT;
    while (FLASH->SR & FLASH_SR_BSY1)
      ;
  }

  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, HIGH);

  Serial2.begin(115200);
  Serial2.println("Booting...");

  analogReadResolution(10);
  battery_median_filter.SetMinRunInterval(10);
  battery_average_filter.SetMinRunInterval(10);

  SPI.setMISO(PA6);
  SPI.setMOSI(PA7);
  SPI.setSCLK(PA5);
  SPI.setSSEL(PA4);

  DeviceDescription const *device;
  switch (kDeviceMode) {
    case DeviceMode::READ_FROM_FLASH:
      led_manager = ReadDeviceFromFlash();
      break;

    case DeviceMode::WRITE_TO_FLASH:
      WriteDeviceToFlash();
      led_manager = ReadDeviceFromFlash();
      break;

    default:
    case DeviceMode::CURRENT_FROM_HEADER:
      led_manager = new FastLedManager(Devices::current, &state_machine);
      break;
  }

  if (!radio->Begin()) {
    led_manager->FatalErrorAnimation();
  }

  led_manager->PlayStartupAnimation();
  digitalWrite(kLedPin, LOW);
}

void loop() {
  battery_median_filter.Run();
  battery_average_filter.Run();
  if (battery_average_filter.GetFilteredValue() <
      BatteryVoltageToRawReading(kBatteryDead)) {
    // TODO: display a battery-low pattern?
    FastLED.clearData();
    led_manager->SetOnboardLed(CRGB(8, 0, 0));
    FastLED.show();
  } else {
    // Display battery status on onboard LED for the first 20 seconds
    if (millis() < 20 * 1000) {
      uint16_t battery_val = battery_average_filter.GetFilteredValue() -
                             BatteryVoltageToRawReading(kBatteryDead);
      uint16_t battery_range = BatteryVoltageToRawReading(kBatteryFull) -
                               BatteryVoltageToRawReading(kBatteryDead);
      // Red is hue 0
      uint8_t hue = (battery_val * HUE_GREEN) / battery_range;
      led_manager->SetOnboardLed(CHSV(hue, 255, 255));
    } else {
      led_manager->SetOnboardLed(CRGB::Black);
    }

    state_machine.Tick();
    led_manager->RunEffect();
  }
  delay(5);
}
