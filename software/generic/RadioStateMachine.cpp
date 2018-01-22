#include "RadioStateMachine.hpp"

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
    data.packet = &packet;
  } else if (timerExpiresAt && millis() > timerExpiresAt) {
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
    switch (nextState) {
      case RadioState::Slave:
        beginSlave();
        break;

      case RadioState::Master:
        beginMaster();
        break;
    }
    // 0 is disabled. Clear the timer - the state will probably set this
    // anyway.
    setTimer(0);
  }

  state = nextState;
}

void RadioStateMachine::handleSlaveEvent(RadioEventData &data) {
  // If the timer fired, then we haven't received a packet in a while and should
  // become master
  if (data.timerExpired) {
    nextState = RadioState::Master;
  }
}

void RadioStateMachine::handleMasterEvent(RadioEventData &data) {
  if (data.timerExpired) {
    packet.type = HEARTBEAT;
    packet.dataLength = 0;
    networkManager->send(packet);
  }
}

void RadioStateMachine::beginSlave() { setTimer(kSlaveNoPacketTimeout); }

void RadioStateMachine::beginMaster() { setTimer(kMasterHeartbeatInterval); }

void RadioStateMachine::setTimer(uint16_t delay) {
  timerExpiresAt = millis() + delay;
}
