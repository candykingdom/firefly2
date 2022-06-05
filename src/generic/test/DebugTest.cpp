#include "Debug.hpp"
#include "gtest/gtest.h"

TEST(Debug, shouldNotBeEnabled) {
#if DEBUG
  FAIL() << "You accidentally left debug mode on in Debug.hpp. Turn it off";
#endif
}
