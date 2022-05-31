#include <LedManager.hpp>

#include "../../../lib/effect/Effects.hpp"
#include "DeviceDescription.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

TEST(LedManager, hasNonRandomEffects) {
  DeviceDescription device = DeviceDescription(1, {});
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  FakeLedManager *manager = new FakeLedManager(&device, state_machine);
  manager->ClearEffects();
  manager->PublicAddEffect(new SimpleBlinkEffect(&device, 10), 4);
  manager->PublicAddEffect(new PoliceEffect(&device), 0);
  manager->PublicAddEffect(new FireEffect(&device), 2);

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
  EXPECT_EQ(effect1->GetRGB(0, 0, setEffect),
            alsoEffect1->GetRGB(0, 0, setEffect));
  EXPECT_NE(effect1->GetRGB(0, 0, setEffect), effect2->GetRGB(0, 0, setEffect));
  // SimpleBlinkEffect and PoliceEffect have the same color at t=0
  EXPECT_NE(effect1->GetRGB(0, 15, setEffect),
            effect3->GetRGB(0, 15, setEffect));
  EXPECT_NE(effect2->GetRGB(0, 0, setEffect), effect3->GetRGB(0, 0, setEffect));
}

TEST(LedManager, effectIndexIsInRange) {
  DeviceDescription device = DeviceDescription(1, {});
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  new FakeLedManager(&device, state_machine);
}
