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
};

class RadioStateMachine {
 public:
  RadioStateMachine(NetworkManager *networkManager);

  RadioState GetCurrentState();

  /** Runs the state machine. Should be called frequently. */
  void Tick();

  // Tuning constants. Public for testing.
  /**
   * If a slave doesn't receive a heartbeat for this long, it'll become master.
   * TODO: add a random factor.
   */
  static const uint16_t kSlaveNoPacketTimeout = 5000;

  /**
   * When master, send a heartbeat this often.
   */
  static const uint16_t kMasterHeartbeatInterval = 1000;

 private:
  // Handler functions
  void handleSlaveEvent(RadioEventData &data);
  void handleMasterEvent(RadioEventData &data);

  // These are called when a state is entered.
  void beginSlave();
  void beginMaster();

  // Support functions
  /** Sets an event to fire delay milliseconds from now.  */
  void setTimer(uint16_t delay);

  /** Performs master election based on the received heartbeat. */
  void PerformMasterElection(RadioPacket *receivedPacket);

  NetworkManager *const networkManager;

  /** The current state. */
  RadioState state;

  /** If not equal to state, the next state. */
  RadioState nextState;

  /** The milliseconds when the timer expires and fires an event. */
  uint16_t timerExpiresAt = 0;

  RadioPacket packet;
};

#endif
