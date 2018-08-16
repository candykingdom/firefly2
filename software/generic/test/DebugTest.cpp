#include "gtest/gtest.h"

#include "../Debug.hpp"

TEST(Debug, shouldNotBeEnabled) {
#if DEBUG
  FAIL() << "You accidentally left debug mode on in Debug.hpp. Turn it off";
#endif
}
