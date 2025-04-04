#include "FakeNetwork.hpp"

#include <Debug.hpp>
#include <DeviceDescription.hpp>
#include <cstdio>

#include "NetworkManager.hpp"

// #define DEBUG

FakeNetwork::FakeNetwork() {
  setMillis(0);

  for (int i = 0; i < kNumNodes; i++) {
    advanceMillis(1);
    radios[i] = FakeRadio();
    networkManagers[i] = new NetworkManager(&radios[i]);
    stateMachines[i] = new RadioStateMachine(networkManagers[i]);
    ledManagers[i] = new FakeLedManager(device, stateMachines[i]);
    stateMachines[i]->Tick();
  }
}

FakeNetwork::~FakeNetwork() {
  for (NetworkManager* network_manager : networkManagers) {
    delete network_manager;
  }
  for (RadioStateMachine* state_machine : stateMachines) {
    delete state_machine;
  }
  for (FakeLedManager* led_manager : ledManagers) {
    delete led_manager;
  }
  if (packet != nullptr) {
    delete packet;
  }
  if (previous_packet != nullptr) {
    delete previous_packet;
  }
}

void FakeNetwork::Tick() {
  // Note: this means that a sender will receive its own packet. This is OK,
  // since senders should be ignoring their own packets anyway, because of the
  // repeater functionality.
  for (int i = 0; i < kNumNodes; i++) {
    if (packet_loss == 0 || rand() % packet_loss) {
      radios[i].setReceivedPacket(packet);
    } else {
      debug_printf("Randomly dropping packet\n");
      radios[i].setReceivedPacket(nullptr);
    }
  }
  if (previous_packet != nullptr) {
    delete previous_packet;
  }
  previous_packet = packet;
  packet = nullptr;

  debug_printf("Running FakeNetwork::Tick at %u millis\n", millis());

  for (int i = 0; i < kNumNodes; i++) {
    debug_printf("Node %d state before: %d\n", i,
                 stateMachines[i]->GetCurrentState());
    stateMachines[i]->Tick();
    debug_printf("Node %d state after: %d\n", i,
                 stateMachines[i]->GetCurrentState());

    if (packet == nullptr) {
      packet = radios[i].getSentPacket();
      if (packet != nullptr) {
        debug_printf("Node %d sending type %d\n", i, packet->type);
      }
    }

    ledManagers[i]->RunEffect();

    debug_printf("\n");
  }
}

void FakeNetwork::reinitNode(int index) {
  delete stateMachines[index];
  delete networkManagers[index];
  networkManagers[index] = new NetworkManager(&radios[index]);
  stateMachines[index] = new RadioStateMachine(networkManagers[index]);
  delete ledManagers[index];
  ledManagers[index] = new FakeLedManager(device, stateMachines[index]);
}

void FakeNetwork::setPacketLoss(int n) { packet_loss = n; }

void FakeNetwork::TransmitPacket(RadioPacket& packet) {
  for (int i = 0; i < kNumNodes; i++) {
    radios[i].setReceivedPacket(&packet);
  }
}
