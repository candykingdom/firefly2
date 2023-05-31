#pragma once

#include "Types.hpp"

namespace stm32 {

void Set_nBOOT_SEL();
void Clear_nBOOT_SEL();

void JumpToBootloader();
void StartupBootloaderCheck();

};  // namespace stm32