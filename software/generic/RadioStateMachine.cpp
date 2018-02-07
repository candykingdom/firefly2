#include "RadioStateMachine.hpp"
#include <cstdio>
#include "Debug.hpp"

//#define DEBUG

RadioStateMachine::RadioStateMachine(NetworkManager *networkManager)
    : networkManager(networkManager) {
  state = RadioState::Slave;
  nextState = RadioState::Slave;
  beginSlave();
}

RadioState RadioStateMachine::GetCurrentState() { return state; }

void RadioStateMachine::Tick() {
  RadioEventData data;
  data.packet = nullptr;
  data.timerExpired = false;

  RadioPacket packet;

  // Note: only allow one event type per iteration. This makes the handler
  // functions simpler (and less error prone).
  if (networkManager->receive(packet)) {
    debug_printf("Received packet\n");
    data.packet = &packet;
  } else if (timerExpiresAt && millis() > timerExpiresAt) {
    debug_printf("Timer expired\n");
    data.timerExpired = true;
  }

  if (data.packet != nullptr || data.timerExpired) {
    switch (state) {
      case RadioState::Slave:
        handleSlaveEvent(data);
        break;

      case RadioState::Master:
        handleMasterEvent(data);
        break;
    }
  }

  if (nextState != state) {
    // 0 is disabled. Clear the timer - the state will probably set this
    // anyway.
    setTimer(0);

    switch (nextState) {
      case RadioState::Slave:
        beginSlave();
        break;

      case RadioState::Master:
        beginMaster();
        break;
    }
  }

  state = nextState;
}

void RadioStateMachine::handleSlaveEvent(RadioEventData &data) {
  // If the timer fired, then we haven't received a packet in a while and should
  // become master
  if (data.packet != nullptr) {
    switch (data.packet->type) {
      case HEARTBEAT:
      case CLAIM_MASTER:
        setTimer(kSlaveNoPacketTimeout + rand() % kSlaveNoPacketRandom);
        break;
    }
  } else if (data.timerExpired) {
    nextState = RadioState::Master;
  }
}

void RadioStateMachine::PerformMasterElection(RadioPacket *receivedPacket) {
  debug_printf("Performing master election\n");

  // Master election: generate a random number. If our number is greater
  // than the packet's ID, become master. Otherwise, become slave.
  const uint16_t ourId = random(1, 0xFFFF);
  if (ourId > receivedPacket->packetId) {
    packet.type = CLAIM_MASTER;
    packet.dataLength = 0;
    networkManager->send(packet);
  } else {
    nextState = RadioState::Slave;
  }
}

void RadioStateMachine::handleMasterEvent(RadioEventData &data) {
  if (data.packet != nullptr) {
    debug_printf("Received type %d\n", data.packet->type);
    switch (data.packet->type) {
      case HEARTBEAT:
        PerformMasterElection(data.packet);
        break;

      case CLAIM_MASTER:
        nextState = RadioState::Slave;
        break;
    }
  } else if (data.timerExpired) {
    SendHeartbeat();
  }
}

void RadioStateMachine::beginSlave() {
  setTimer(kSlaveNoPacketTimeout + rand() % kSlaveNoPacketRandom);
}

void RadioStateMachine::beginMaster() {
  SendHeartbeat();
  setTimer(kMasterHeartbeatInterval);
}

void RadioStateMachine::setTimer(uint16_t delay) {
  if (delay == 0) {
    timerExpiresAt = 0;
  } else {
    timerExpiresAt = millis() + delay;
  }
}

void RadioStateMachine::SendHeartbeat() {
  packet.type = HEARTBEAT;
  packet.dataLength = 0;
  networkManager->send(packet);
}
