#include <DeviceDescription.hpp>
#include <LedManager.hpp>

#include "../../../lib/effect/Effect.hpp"
#include "../../../lib/effect/Effects.hpp"
#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "gtest/gtest.h"

TEST(LedManager, hasNonRandomEffects) {
  StripDescription strip = StripDescription(1, {});
  DeviceDescription device = DeviceDescription(2000, {&strip});
  EXPECT_EQ(device.GetLedCount(), 1);
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

  delete setEffect;
  delete manager;
  delete state_machine;
}

TEST(LedManager, effectIndexIsInRange) {
  StripDescription strip = StripDescription(1, {});
  DeviceDescription device = DeviceDescription(2000, {&strip});
  EXPECT_EQ(device.GetLedCount(), 1);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  FakeLedManager *led_manager = new FakeLedManager(&device, state_machine);
  delete led_manager;
  delete state_machine;
}

class TestEffect : public Effect {
 public:
  std::vector<uint8_t> called_indicies;

  TestEffect() : called_indicies() {}

  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription *strip, RadioPacket *setEffectPacket) {
    UNUSED(time_ms);
    UNUSED(strip);
    UNUSED(setEffectPacket);
    called_indicies.push_back(led_index);
    return CRGB(0, 0, 0);
  }
};

TEST(LedManager, callStripInReverse) {
  StripDescription strip = StripDescription(5, {Reversed});
  DeviceDescription device = DeviceDescription(2000, {&strip});
  EXPECT_EQ(device.GetLedCount(), 5);
  FakeRadio radio;
  NetworkManager *networkManager = new NetworkManager(&radio);
  RadioStateMachine *state_machine = new RadioStateMachine(networkManager);
  FakeLedManager *manager = new FakeLedManager(&device, state_machine);

  manager->ClearEffects();
  TestEffect *test_effect = new TestEffect();
  manager->PublicAddEffect(test_effect, 1);

  RadioPacket *setEffect = new RadioPacket();
  setEffect->writeSetEffect(0, 0, 0);
  state_machine->SetEffect(setEffect);

  manager->RunEffect();

  ASSERT_EQ(test_effect->called_indicies.size(), 5);

  uint8_t expected_index = 4;
  for (auto actual : test_effect->called_indicies) {
    ASSERT_EQ(actual, expected_index--)
        << "Reverse strip should be called in reverse order!";
  }

  delete setEffect;
  delete manager;
  delete state_machine;
}