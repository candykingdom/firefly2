#include "stm32-lib.h"

namespace stm32 {

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

};  // namespace stm32