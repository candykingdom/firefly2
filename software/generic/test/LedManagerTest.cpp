#include "../LedManager.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "FireEffect.hpp"
#include "PoliceEffect.hpp"
#include "SimpleBlinkEffect.hpp"
#include "gtest/gtest.h"

TEST(LedManager, hasNonRandomEffects) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);
  FakeLedManager *manager = new FakeLedManager(1, stateMachine);
  manager->ClearEffects();
  manager->PublicAddEffect(new SimpleBlinkEffect(1, DeviceType::Wearable, 10),
                           4);
  manager->PublicAddEffect(new PoliceEffect(1), 0);
  manager->PublicAddEffect(new FireEffect(1), 2);

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
  EXPECT_NE(effect1->GetRGB(0, 0, setEffect), effect3->GetRGB(0, 0, setEffect));
  EXPECT_NE(effect2->GetRGB(0, 0, setEffect), effect3->GetRGB(0, 0, setEffect));
}

TEST(LedManager, effectIndexIsInRange) {
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *stateMachine = new RadioStateMachine(networkManager);
  new FakeLedManager(1, stateMachine);
}
