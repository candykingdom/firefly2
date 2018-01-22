#include "RadioStateMachine.hpp"

RadioStateMachine::RadioStateMachine(NetworkManager *networkManager)
    : networkManager(networkManager) {
  state = RadioState::Slave;
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
}

void RadioStateMachine::handleSlaveEvent(RadioEventData &data) {
  // TODO
}

void RadioStateMachine::handleMasterEvent(RadioEventData &data) {
  // TODO
}

void RadioStateMachine::setTimer(uint16_t delay) {
  timerExpiresAt = millis() + delay;
}
