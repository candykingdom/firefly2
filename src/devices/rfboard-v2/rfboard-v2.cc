#include <Arduino.h>
#include <FastLED.h>
#include <FlashStorage_STM32.h>

#include <DeviceDescription.hpp>
#include <Devices.hpp>
#include <StripDescription.hpp>
#include <vector>

#include "../../arduino/FastLedManager.hpp"
#include "../../arduino/RadioHeadRadio.hpp"
#include "../../generic/NetworkManager.hpp"
#include "../../generic/RadioStateMachine.hpp"

// The onboard addressable LED, which then feeds the external LED connector
constexpr int kPinLedInternal = PA4; 
constexpr int kPinRedLed = PB1;
constexpr int kPinSw1 = PB6;
constexpr int kPinSw2 = PB7;

constexpr int kPin5vDivided = PA0;
constexpr int kPinCc1 = PA1;
constexpr int kPinCc2 = PB2;
constexpr int kPinVibrationSensor = PA8;

RadioHeadRadio *radio = new RadioHeadRadio(/*is_high_power*/true);
NetworkManager network_manager(radio);
RadioStateMachine state_machine(&network_manager);
FastLedManager *led_manager;

// the nBOOT_SEL bit needs to be cleared (0) so that the MCU uses the BOOT0 pin as BOOT0, and not a GPIO.
void MaybeClearBootSelectBit() {
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
}

void setup() {
  MaybeClearBootSelectBit();
  Serial.begin(115200);
  Serial.println("Booting...");

  pinMode(kPinLedInternal, OUTPUT);
  pinMode(kPinRedLed, OUTPUT);
  pinMode(kPinSw1, INPUT_PULLUP);
  pinMode(kPinSw2, INPUT_PULLUP);

  pinMode(kPin5vDivided, INPUT_ANALOG);
  pinMode(kPinCc1, INPUT_ANALOG);
  pinMode(kPinCc2, INPUT_ANALOG);
  pinMode(kPinVibrationSensor, INPUT);

  digitalWrite(kPinRedLed, true);
  delay(100);

  SPI.setMISO(PB4);
  SPI.setMOSI(PB5);
  SPI.setSCLK(PB3);
  // Note: DON'T set SSEL, since RadioHead manages it in software.

  // TODO: support reading the device description from flash
  led_manager = new FastLedManager(Devices::current, &state_machine);

  if (!radio->Begin()) {
    led_manager->FatalErrorAnimation();
  }

  // NOTE: can check if we watchdog rebooted by checking REG_PM_RCAUSE
  // See https://github.com/gjt211/SAMD21-Reset-Cause
  led_manager->PlayStartupAnimation();

  digitalWrite(kPinRedLed, false);

  // TODO: configure and enable the watchdog
}

void loop() {
  digitalWrite(kPinRedLed, (millis() / 500) % 2);
  state_machine.Tick();
  // led_manager->SetOnboardLed(CHSV(millis() / 10, 255, 255));
  led_manager->RunEffect();
  delay(1);
}
