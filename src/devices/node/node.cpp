// Arduino.h, automatically included by the IDE, defines min and max macros
// (which it shouldn't)
#undef max
#undef min
#include <FlashStorage_SAMD.h>
#include <wiring_private.h>  // pinPeripheral

#include <DeviceDescription.hpp>
#include <Devices.hpp>
#include <StripDescription.hpp>
#include <vector>

#include "../../arduino/FastLedManager.hpp"
#include "../../arduino/RadioHeadRadio.hpp"
#include "../../arduino/SerialRadio.hpp"
#include "../../generic/FireflyNetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"

const int kLedPin = 0;

// Serial bridge to an ESP32-C3 over a hardware UART. Opt-in via
// -DSERIAL_BRIDGE=1; only enable it when an ESP32 is wired to the UART pins
// (see the hazard note in SerialRadio.cpp).
#ifndef SERIAL_BRIDGE
#define SERIAL_BRIDGE 0
#endif

#if SERIAL_BRIDGE
// The rfboard variant deliberately leaves `Serial1` undefined: the header pins
// labeled 1 & 2 map to PA04/PA05, which are on SERCOM0 -- the SERCOM already
// dedicated to the RFM69 radio's SPI (see PERIPH_SPI in the variant). They
// cannot drive a UART while the radio is active. Instead we bring up our own
// UART on SERCOM1, which is free and reachable on header pins 9 (PA17) and 10
// (PA16). This defines the `Serial1` the variant declares `extern`.
constexpr uint32_t kSerialBaud = 115200;
constexpr uint8_t kSerialRxPin = 9;   // PA17 -> SERCOM1/PAD[1]
constexpr uint8_t kSerialTxPin = 10;  // PA16 -> SERCOM1/PAD[0]

Uart Serial1(&sercom1, kSerialRxPin, kSerialTxPin, SERCOM_RX_PAD_1,
             UART_TX_PAD_0);
void SERCOM1_Handler() { Serial1.IrqHandler(); }
#endif

constexpr DeviceMode kDeviceMode = DeviceMode::CURRENT_FROM_HEADER;

// Note: `RadioHeadRadio` needs to be a pointer - if it's an object, the node
// crashes upon receiving a packet.
RadioHeadRadio *radio = new RadioHeadRadio();
#if SERIAL_BRIDGE
SerialRadio *serial_radio = new SerialRadio(Serial1);
#endif
FireflyNetworkManager nm(radio);
RadioStateMachine state_machine(&nm);
FastLedManager *led_manager;

constexpr uint32_t kEepromStart = 0x00020000 - 0x2000;
FlashClass device_storage(/*flash_addr=*/(void *)kEepromStart,
                          /*size=*/DeviceDescription::kMaxSize);

void FeedWatchdog() { WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY; }

FastLedManager *ReadDeviceFromFlash() {
  DeviceDescription *device =
      (DeviceDescription *)malloc(DeviceDescription::kMaxSize);
  if (device == nullptr) {
    Serial.println("Failed to malloc DeviceDescription");
    return new FastLedManager(Devices::current, &state_machine);
  }
  device_storage.read((void *)device);
  if (device->check_value != DeviceDescription::kCheckValue) {
    Serial.print("Got invalid check value when reading DeviceDescription: ");
    Serial.println(device->check_value);
    return new FastLedManager(Devices::current, &state_machine);
  }
  return new FastLedManager(*device, &state_machine);
}

void WriteDeviceToFlash() {
  // Don't write if current device is equal
  bool same = true;
  uint8_t *flash = (uint8_t *)kEepromStart;
  uint8_t *device = (uint8_t *)(&Devices::current);
  for (uint32_t i = 0; i < sizeof(Devices::current); i++) {
    if (flash[i] != device[i]) {
      same = false;
      break;
    }
  }

  if (same) {
    Serial.println("Current device already written to flash");
    return;
  }

  device_storage.erase();
  device_storage.write((void *)&Devices::current);
  Serial.println("Wrote current device to flash");
}

void setup() {
  Serial.begin(115200);
  pinMode(kLedPin, OUTPUT);

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

#if SERIAL_BRIDGE
  Serial1.begin(kSerialBaud);
  // Route the chosen pins to their SERCOM1 peripheral function.
  pinPeripheral(kSerialRxPin, PIO_SERCOM);
  pinPeripheral(kSerialTxPin, PIO_SERCOM);
  if (!serial_radio->Begin()) {
    led_manager->FatalErrorAnimation();
  }
  nm.addRadio(serial_radio);
#endif

  // NOTE: can check if we watchdog rebooted by checking REG_PM_RCAUSE
  // See https://github.com/gjt211/SAMD21-Reset-Cause
  led_manager->PlayStartupAnimation();

  // Set up the watchdog timer: this will reset the processor if it hasn't
  // 'fed' the watchdog in ~100ms.
  // Inspired by
  // https://github.com/adafruit/Adafruit_SleepyDog/blob/master/utility/WatchdogSAMD.cpp
  // Disable watchdog and wait for it to take effect
  WDT->CTRL.reg = 0;
  while (WDT->STATUS.bit.SYNCBUSY) {
  }

  // Configure the watchdog clock
  // Note: feeding the watchdog is slightly expensive - it seems to take O(1
  // tick of the oscillator). So, don't use a prescaler. I tried using a /32
  // prescaler, and the animations were horribly slow.
  // Generic clock generator 2, no divisor
  GCLK->GENDIV.reg = GCLK_GENDIV_ID(2);
  // Enable clock generator 2 using low-power 32KHz oscillator.
  GCLK->GENCTRL.reg =
      GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSCULP32K;
  while (GCLK->STATUS.bit.SYNCBUSY) {
  }
  // WDT clock = clock gen 2
  GCLK->CLKCTRL.reg =
      GCLK_CLKCTRL_ID_WDT | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2;

  // Now, configure
  WDT->INTENCLR.bit.EW = 1;  // Disable early warning interrupt
  // Documentation for this field here:
  // http://asf.atmel.com/docs/latest/samd20/html/group__asfdoc__sam0__wdt__group.html
  WDT->CONFIG.bit.PER =
      9;  // WDT clock runs at ~32kHz (effective). A value of 9 means 4096
          // cycles, which means a period of ~128ms.
  WDT->CTRL.bit.WEN = 0;  // Disable window mode
  while (WDT->STATUS.bit.SYNCBUSY) {
  }

  FeedWatchdog();
  // Now, enable the watchdog
  WDT->CTRL.bit.ENABLE = 1;
  while (WDT->STATUS.bit.SYNCBUSY) {
  }
}

uint8_t watchdog_counter = 0;

void loop() {
  state_machine.Tick();
  led_manager->RunEffect();

  // Feeding the watchdog takes some time. Each run through this loop should
  // take less than 10ms, so it should be safe to feed the watchdog every 5
  // cycles = ~50ms.
  if (watchdog_counter > 5) {
    FeedWatchdog();
    watchdog_counter = 0;
  }
  watchdog_counter++;
}
