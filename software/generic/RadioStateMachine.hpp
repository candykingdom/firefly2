#ifndef __RADIO_STATE_MACHINE_H__
#define __RADIO_STATE_MACHINE_H__

#include "NetworkManager.hpp"
#include "Types.hpp"

/**
 * The data passed to a state when an event occurs.
 */
struct RadioEventData {
  /**
   * The packet that was received, or null if no packet was received.
   */
  RadioPacket *packet;

  /**
   * Whether a timer expired.
   */
  bool timerExpired;
};

/**
 * The possible states for this state machine.
 */
enum class RadioState {
  Slave,
  Master,
  // TODO: add Negotiate
};

class RadioStateMachine {
 public:
  RadioStateMachine(NetworkManager *networkManager);

  RadioState GetCurrentState();

  /**
   * Runs the state machine. Should be called frequently.
   */
  void Tick();

 private:
  // Handler functions
  void handleSlaveEvent(RadioEventData &data);
  void handleMasterEvent(RadioEventData &data);

  // Support functions
  /**
   * Sets an event to fire delay milliseconds from now.
   */
  void setTimer(uint16_t delay);

  NetworkManager *const networkManager;
  RadioState state;
  uint16_t timerExpiresAt = 0;
};

#endif
