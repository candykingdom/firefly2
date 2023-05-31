#include "stm32-lib.h"

#include <STM32RTC.h>

namespace stm32 {

// Arbitrary (valid) date
static constexpr uint32_t kEpochTimer = 1451606400;
static constexpr uint32_t kEpochReset = 1441606400;

void Set_nBOOT_SEL() {
  if (!(FLASH->OPTR & FLASH_OPTR_nBOOT_SEL)) {
    // unlock flash/option
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
    FLASH->OPTKEYR = 0x08192A3B;
    FLASH->OPTKEYR = 0x4C5D6E7F;

    while (FLASH->SR & FLASH_SR_BSY1)
      ;

    // Set nBOOT_SEL bit
    FLASH->OPTR |= FLASH_OPTR_nBOOT_SEL;

    // write
    FLASH->CR |= FLASH_CR_OPTSTRT;
    while (FLASH->SR & FLASH_SR_BSY1)
      ;
  }
}

void Clear_nBOOT_SEL() {
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

// See
// https://community.st.com/s/question/0D53W00000BLufdSAD/how-do-i-jump-from-application-code-to-bootloader-without-toggling-the-boot0-pin-on-stm32h745
// and
// https://community.st.com/s/article/STM32H7-bootloader-jump-from-application
void JumpToBootloader() {
  void (*SysMemBootJump)(void);

  /* Set the address of the entry point to bootloader */
  volatile uint32_t BootAddr = 0x1FFF0000;

  /* Disable all interrupts */
  __disable_irq();

  /* Disable Systick timer */
  SysTick->CTRL = 0;

  /* Set the clock to the default state */
  HAL_RCC_DeInit();

  /* Clear Interrupt Enable Register & Interrupt Pending Register */
  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICPR[0] = 0xFFFFFFFF;

  /* Re-enable all interrupts */
  __enable_irq();

  /* Set up the jump to booloader address + 4 */
  SysMemBootJump = (void (*)(void))(*((uint32_t *)((BootAddr + 4))));

  /* Set the main stack pointer to the bootloader stack */
  __set_MSP(*(uint32_t *)BootAddr);

  /* Call the function to jump to bootloader location */
  SysMemBootJump();

  /* Jump is done successfully */
  while (1) {
    /* Code should never reach this loop */
  }
}

void StartupBootloaderCheck() {
  /*
   * When the reset pin is pulled high the following operations take place:
   * 0. The system is started normally and the timer is set to `kEpochTimer`
   * 1. Within the first second (before the timer ticks) the reset pin is pulled
   *    high
   * 2. The CPU is reset but not the timer
   * 3. `setup()` is re-entered and this time the timer is set to `kEpochTimer`
   * 4. The timer is set to `kEpochReset`
   * 5. The CPU is software reset to ensure all registers are zeroed out
   * 6. `setup()` is re-entered and this time the timer is set to `kEpochReset`
   * 7. The timer is set to `kEpochTimer` (in case the reset pin is pulled high
   *    in the next second and we need to do the full restart again)
   * 8. The bootloader is entered and never returns
   */
  STM32RTC &rtc = STM32RTC::getInstance();
  rtc.begin(/*resetTime=*/false);
  if (rtc.getEpoch() == kEpochTimer) {
    rtc.setEpoch(kEpochReset);
    // This call never returns
    NVIC_SystemReset();
  } else if (rtc.getEpoch() == kEpochReset) {
    rtc.setEpoch(kEpochTimer);
    // This call never returns
    JumpToBootloader();
  }
  rtc.setEpoch(kEpochTimer);

  // Note: must have a short delay here, in the (typical) case that the
  // application code starts using the serial port. The bootloader double-taps
  // the reset line - we can't start writing to serial after the first reset, or
  // the bootloader won't be able to use the serial port.
  delay(20);
}

};  // namespace stm32