#include <Arduino.h>
#include <FastLED.h>
#include <FlashStorage_STM32.h>
#include <arduino-timer.h>
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

constexpr DeviceMode kDeviceMode = DeviceMode::CURRENT_FROM_HEADER;

constexpr int kLedPin = PB0;
constexpr int kNeopixelPin = PA12;
constexpr int kBatteryPin = PA0;
constexpr int kButton1 = PB1;
constexpr int kButton2 = PB2;

RadioHeadRadio *radio = new RadioHeadRadio();
NetworkManager nm(radio);
RadioStateMachine state_machine(&nm);
FastLedManager *led_manager;

// Heavy filtering
static constexpr uint8_t kBatteryFilterAlpha = 1;
static constexpr uint8_t kBatteryMedianFilterSize = 5;
MedianFilter<uint16_t, uint16_t, kBatteryMedianFilterSize>
    battery_median_filter{filter_functions::ForAnalogRead<kBatteryPin>()};
ExponentialMovingAverageFilter<uint16_t> battery_average_filter{
    []() { return battery_median_filter.GetFilteredValue(); },
    kBatteryFilterAlpha};
bool battery_filters_initialized = false;

// On fancy-node v1.1, the filter cap on the battery voltage divider has a time
// constant of ~0.5s, so when the device is cold-booted, it takes a couple of
// seconds before the read battery level stabilizes. This timer ignores low
// battery detection until that happens.
CountDownTimer startup_battery_timer{3000};

// References for battery level at startup. We can only really measure the
// battery state-of-charge when the battery is unloaded, and has been unloaded
// for at least a second or so. At startup, we check the approximate charge
// level. We display this charge level on the onboard LED, and also put the
// device into low power mode if the battery is effectively empty.
static constexpr float kBatteryEmpty = 3.7;
static constexpr float kBatteryFull = 4.2;

// Cutoff under load. Since the LEDs may draw significant current (2-3A), the
// under-load voltage could be significantly lower than the battery-empty
// open-circuit voltage. 3V is a conservative cutoff voltage.
static constexpr float kBatteryLowCutoff = 3.0;

// Once we detect low battery, the battery must hit this voltage before turning
// back on. This is about 50% charged (open voltage).
static constexpr float kBatteryResume = 3.85;

// voltage = battery * (62 + 180) / 180 * 3.3 / 1024
constexpr float kBatteryDividerHigh = 62;
constexpr float kBatteryDividerLow = 180;
constexpr uint16_t BatteryVoltageToRawReading(float voltage) {
  const float divided_voltage =
      voltage /
      ((kBatteryDividerHigh + kBatteryDividerLow) / kBatteryDividerLow);
  return divided_voltage / 3.3 * 1024.0;
}

uint16_t battery_at_boot = 0;
bool battery_low = false;

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
    Serial2.print("Got invalid check value when reading DeviceDescription: ");
    Serial2.println(device->check_value);
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

void LowBatteryMode() {
  FastLED.clearData();
  led_manager->SetOnboardLed(CRGB(4, 0, 0));
  FastLED.leds()[1] = CRGB(4, 0, 0);
  FastLED.show();
  radio->sleep();
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

  startup_battery_timer.Reset();

  pinMode(kBatteryPin, INPUT);
  pinMode(kButton1, INPUT_PULLUP);
  pinMode(kButton2, INPUT_PULLUP);

  // Note: buttons are inverted (low is pressed)
  bool button_pressed = !(digitalRead(kButton1) && digitalRead(kButton2));

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

  analogReadResolution(10);

  bool button_still_pressed = !(digitalRead(kButton1) && digitalRead(kButton2));
  if (button_pressed && button_still_pressed) {
    led_manager->SetOnboardLed(CHSV(0, 0, 32));
    FastLED.show();
    // Give time for the battery input filter capacitor to charge.
    // TODO: remove this for fancy-node v1.2
    delay(2000);
    battery_at_boot = analogRead(kBatteryPin);
  }

  led_manager->PlayStartupAnimation();
  digitalWrite(kLedPin, LOW);
}

void loop() {
  digitalWrite(kLedPin, ((millis() / 200) % 5) == 0);

  if (startup_battery_timer.Expired()) {
    if (!battery_filters_initialized) {
      for (uint8_t i = 0; i < kBatteryMedianFilterSize; i++) {
        battery_median_filter.Run();
      }
      battery_average_filter.Initialize(analogRead(kBatteryPin));
      battery_average_filter.Run();

      battery_median_filter.SetMinRunInterval(100);
      battery_average_filter.SetMinRunInterval(100);
      battery_filters_initialized = true;
    }
    battery_median_filter.Run();
    battery_average_filter.Run();

    if (!battery_low && battery_average_filter.GetFilteredValue() <
                            BatteryVoltageToRawReading(kBatteryLowCutoff)) {
      battery_low = true;
      LowBatteryMode();
    }

    if (battery_low) {
      if (battery_average_filter.GetFilteredValue() >
          BatteryVoltageToRawReading(kBatteryResume)) {
        battery_low = false;
      } else {
        delay(100);
        return;
      }
    }
  }

  // Display battery status on onboard LED for the first 20 seconds
  if (battery_at_boot != 0 && millis() < 20 * 1000) {
    uint16_t battery_val =
        battery_at_boot - BatteryVoltageToRawReading(kBatteryEmpty);
    constexpr uint16_t battery_range =
        BatteryVoltageToRawReading(kBatteryFull) -
        BatteryVoltageToRawReading(kBatteryEmpty);
    static_assert(battery_range != 0);
    if (battery_val > battery_range) {
      battery_val = 0;
    }

    // Red is hue 0
    uint8_t hue = (battery_val * HUE_GREEN) / battery_range;
    led_manager->SetOnboardLed(CHSV(hue, 255, 255));
  } else {
    led_manager->SetOnboardLed(CRGB::Black);
  }

  state_machine.Tick();
  led_manager->RunEffect();
  delay(5);
}
