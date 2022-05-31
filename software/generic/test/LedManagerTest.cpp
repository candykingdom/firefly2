#include <LedManager.hpp>

#include "../../../lib/effect/Effects.hpp"
#include "DeviceDescription.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

TEST(LedManager, hasNonRandomEffects) {
  StripDescription strip = StripDescription(1, {});
  DeviceDescription device = DeviceDescription(2000, {&strip});
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  FakeLedManager *manager = new FakeLedManager(&device, state_machine);
  manager->ClearEffects();
  manager->PublicAddEffect(new SimpleBlinkEffect(10), 4);
  manager->PublicAddEffect(new PoliceEffect(), 0);
  manager->PublicAddEffect(new FireEffect(), 2);

  EXPECT_EQ(manager->GetNumEffects(), 6);
  EXPECT_EQ(manager->GetNumUniqueEffects(), 3);

  EXPECT_EQ(manager->UniqueEffectNumberToIndex(0), 0);
  EXPECT_EQ(manager->UniqueEffectNumberToIndex(1), 4);
  EXPECT_EQ(manager->UniqueEffectNumberToIndex(2), 6);

  RadioPacket *setEffect = new RadioPacket();
  setEffect->writeSetEffect(0, 0, 0);

  Effect *effect1 = manager->GetEffect(0);
  Effect *alsoEffect1 = manager->GetEffect(1);
  Effect *effect2 = manager->GetEffect(4);
  Effect *effect3 = manager->GetEffect(6);
  EXPECT_EQ(effect1->GetRGB(0, 0, &strip, setEffect),
            alsoEffect1->GetRGB(0, 0, &strip, setEffect));
  EXPECT_NE(effect1->GetRGB(0, 0, &strip, setEffect),
            effect2->GetRGB(0, 0, &strip, setEffect));
  // SimpleBlinkEffect and PoliceEffect have the same color at t=0
  EXPECT_NE(effect1->GetRGB(0, 15, &strip, setEffect),
            effect3->GetRGB(0, 15, &strip, setEffect));
  EXPECT_NE(effect2->GetRGB(0, 0, &strip, setEffect),
            effect3->GetRGB(0, 0, &strip, setEffect));
}

TEST(LedManager, effectIndexIsInRange) {
  StripDescription strip = StripDescription(1, {});
  DeviceDescription device = DeviceDescription(2000, {&strip});
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  new FakeLedManager(&device, state_machine);
}
