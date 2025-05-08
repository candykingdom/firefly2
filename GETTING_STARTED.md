# Firefly2 Getting Started Guide

This document provides an overview of the Firefly2 system for new engineers joining the project.

## What is Firefly2?

Firefly2 is an LED control system designed for Burning Man art installations. It enables synchronized light effects across multiple devices (bikes, costumes, art pieces) using wireless communication. The system consists of hardware designs and firmware that work together to create coordinated light shows.

## System Architecture

### Hardware Components

The system uses several hardware form factors:

1. **Node/RFBoard** - Basic wireless LED controller with SAMD21 microcontroller
2. **Fancy Node** - Advanced controller using STM32G030C8T with more memory and capabilities
3. **Controller** - Master control unit with buttons/UI using STM32G070CBT
4. **Rings** - Circular LED controllers (40mm/50mm versions) optimized for wearables
5. **Puck** - Another circular form factor with different mounting options
6. **DMX** - Interface for DMX lighting using ESP32

Each device typically contains:
- Microcontroller (SAMD21, STM32G0, or ESP32)
- RF radio module (RFM69HW operating at 915MHz)
- Connectors for WS2812B (NeoPixel) LED strips
- Power management circuitry (voltage regulators, protection)
- Optional components (buttons, sensors, battery monitoring)

### Software Architecture

The codebase is structured around these key components:

#### 1. Wireless Mesh Network

The foundation of the system is a **master/slave architecture** with mesh networking:

- `Radio` (abstract interface) → `RadioHeadRadio` (concrete implementation using RadioHead library)
- `NetworkManager` handles packet routing with a simple rebroadcast mechanism
  - Uses a circular buffer to track recently seen packet IDs to avoid loops
  - Routes packets between devices to ensure network-wide coverage
- `RadioStateMachine` orchestrates state transitions and timing:
  - Maintains timers for heartbeats, effect changes, and rebroadcasts
  - Implements master election protocol
  - Synchronizes network time across all devices

**Packet Types:**
- `HEARTBEAT` - Contains network time, sent by master every ~1 second
- `CLAIM_MASTER` - Used during master negotiation
- `SET_EFFECT` - Contains effect index, palette index, and delay parameters
- `SET_CONTROL` - Direct RGB control for manual color setting

All devices start as slaves. If no heartbeat is received within ~5-7 seconds (with random jitter), a device becomes master. When multiple masters detect each other, an election occurs based on packet ID, with higher IDs winning.

#### 2. LED Management

- `LedManager` (abstract interface) → `FastLedManager` (implementation for real hardware)
- Controls LED strips with these responsibilities:
  - Manages a collection of effect instances
  - Applies currently selected effect to LEDs
  - Enforces power constraints based on device configuration
  - Handles LED addressing for special configurations (circular, mirrored)
  - Maintains effect history to control randomization

The LED manager translates abstract effect calculations into concrete LED control signals, handling hardware-specific details like:
- Maximum brightness limits based on power budget
- LED strip addressing patterns
- Color correction and gamma correction

#### 3. Effects System

- `Effect` (base class) with specific implementations:
  - `RainbowEffect` - Smooth color transitions across LED strip
  - `FireEffect` - Fire simulation using Perlin noise
  - `FireflyEffect` - Random twinkling lights
  - `LightningEffect` - Bright flashes with decay
  - `ColorCycleEffect` - Cycles through palette colors
  - `RainbowBumpsEffect` - Rainbow with intensity variations
  - And many more...

Each effect implements these key methods:
- `Init()` - Sets up initial state
- `Calculate()` - Computes LED colors based on current time
- `GetName()` - Returns human-readable name

Effects can be:
- Synchronized across devices using network time
- Selected randomly or manually (with configurable occurrence probabilities)
- Parameterized with different color palettes
- Tagged as "non-random" for special/harsh effects that shouldn't appear in rotation

#### 4. Device Configuration

- `DeviceDescription` defines:
  - Power constraints in milliamps
  - Collection of LED strips with their configurations
  - Methods to calculate total LED count
- `StripDescription` configures individual strips:
  - LED count and type
  - Special flags (Bright, Dim, Circular, Mirrored, etc.)
  - Data pin assignment

Configurations can be stored in flash memory with three modes:
- `CURRENT_FROM_HEADER` - Use the current device from Devices.hpp
- `READ_FROM_FLASH` - Read from flash, falling back to header if invalid
- `WRITE_TO_FLASH` - Write current config to flash, then use it

## Main Code Paths

### Device Startup Flow

1. **Hardware Initialization**
   ```cpp
   // Initialize radio
   RadioHeadRadio radio;
   NetworkManager network_manager(&radio);
   
   // Set up device configuration
   DeviceDescription device = GetCurrentDevice(); // From Devices.hpp or flash
   
   // Create LED Manager with appropriate effects
   FastLedManager led_manager(device, &radio_state_machine);
   
   // Initialize the radio state machine
   RadioStateMachine radio_state_machine(&network_manager);
   ```

2. **Main Loop**
   ```cpp
   void loop() {
     // Run the state machine
     radio_state_machine.Tick();
     
     // Run the current effect
     led_manager.RunEffect();
     
     // Optional: power management, button handling, etc.
   }
   ```

### Master Device Flow

1. Send heartbeats at regular intervals (`kMasterHeartbeatInterval = 1000ms`)
   ```cpp
   // In RadioStateMachine.cpp
   void RadioStateMachine::SendHeartbeat() {
     packet_.writeHeartbeat(GetNetworkMillis());
     network_manager_->send(packet_);
   }
   ```

2. Change effects periodically (`kChangeEffectInterval = 60000ms`)
   ```cpp
   // In RadioStateMachine.cpp (simplified)
   void handleMasterEvent(RadioEventData &data) {
     if (data.timer_expired == TimerChangeEffect) {
       // Select new effect and palette
       uint8_t new_effect_index = random(num_effects_);
       uint8_t new_palette_index = random(num_palettes_);
       
       // Create and broadcast SET_EFFECT packet
       packet_.writeSetEffect(new_effect_index, 0, new_palette_index);
       network_manager_->send(packet_);
       
       // Save as current effect
       set_effect_packet_ = packet_;
     }
   }
   ```

3. Broadcast current effect regularly (`kBroadcastEffectInterval = 2000ms`)
4. Handle incoming `SET_EFFECT` commands (from remotes or other controllers)
5. Participate in master election if a competing master is detected

### Slave Device Flow

1. Listen for heartbeats from master
   ```cpp
   // In RadioStateMachine.cpp (simplified)
   void handleSlaveEvent(RadioEventData &data) {
     if (data.packet && data.packet->type == HEARTBEAT) {
       // Update network time offset
       int32_t received_time = data.packet->readTimeFromHeartbeat();
       millis_offset_ = received_time - millis();
       
       // Reset slave timeout timer
       SetTimer(TimerHeartbeat, kSlaveNoPacketTimeout + random(kSlaveNoPacketRandom));
     }
   }
   ```

2. Update network time based on heartbeats
3. Apply effects as directed by master
4. Rebroadcast packets to extend network range
5. Become master if heartbeat timer expires

### Effect Rendering Flow

1. Get current effect from the network state
   ```cpp
   // In LedManager.cpp (simplified)
   void LedManager::RunEffect() {
     // Get the current effect based on radio state
     uint8_t effect_index = radio_state->GetEffectIndex();
     Effect* effect = GetEffect(effect_index);
     
     // Calculate LED values based on network time
     effect->Calculate(radio_state->GetNetworkMillis());
     
     // Write the LEDs
     WriteOutLeds();
   }
   ```

2. Calculate LED values based on network time and effect parameters
3. Apply power limitations based on device description
4. Send data to LEDs through the LED manager

## Development Workflow

### Setup Environment

1. Install PlatformIO:
   ```bash
   pip install platformio
   ```

2. Install build tools:
   ```bash
   sudo apt install cmake clang-format  # For Linux
   # For macOS: brew install cmake llvm
   ```

3. Install ST-Link tools (for STM32 programming):
   ```bash
   # Linux
   sudo apt install stlink-tools
   
   # macOS
   brew install stlink
   ```

4. Install stm32loader for USB programming:
   ```bash
   pip install 'stm32loader @ git+https://github.com/ademuri/stm32loader@4ef98d0'
   ```

### Building and Flashing

For embedded targets:
```bash
# List available environments
pio project config

# Build specific firmware
pio run -e fancy-node

# Flash using ST-Link
pio run -e fancy-node -t upload

# Flash via USB bootloader
pio run -e fancy-node-usb -t upload --upload-port /dev/ttyUSB0
```

For testing:
```bash
# Build and run all tests
mkdir -p build && cd build
cmake ..
make
make test

# Build and run individual test suite
make && ./smalltests --gtest_filter=NetworkManagerTest*
```

### Common Development Tasks

1. **Modifying an effect**:
   - Edit the effect file in `lib/effect/`
   - Update calculation logic
   ```cpp
   // Example: changing parameters in FireEffect.cpp
   void FireEffect::Calculate(uint32_t time_ms) {
     // Adjust parameters for more intense fire
     cooling = 80;  // Increased from 55
     sparking = 180;  // Increased from 120
     
     // Continue with effect calculation...
   }
   ```
   - Build and flash to test

2. **Adding a new effect**:
   - Create new effect class inheriting from `Effect`
   ```cpp
   // NewEffect.hpp
   class NewEffect : public Effect {
    public:
     NewEffect(uint8_t num_leds, const DeviceDescription& device);
     virtual ~NewEffect();
     
     virtual void Calculate(uint32_t current_time);
     virtual const char* GetName() const { return "New Effect"; }
   
    private:
     // Effect-specific state variables
   };
   ```
   - Implement calculation logic in the CPP file
   - Register in LedManager.cpp using `AddEffect(new NewEffect(...), proportion)`
   - Add to Effects.hpp
   ```cpp
   #include "NewEffect.hpp"
   ```

3. **Changing device configuration**:
   - Modify `Devices.hpp` to add/update device definitions
   ```cpp
   // Example: Adding a new device configuration
   const DeviceDescription my_custom_device(
     1000,  // 1000mA power budget
     {
       StripDescription(60, StripType::NEOPIXEL, true, false, false),
       StripDescription(30, StripType::NEOPIXEL, false, true, false)
     }
   );
   
   // Update current device
   #define current my_custom_device
   ```

4. **Debugging**:
   - Monitor serial output: 
   ```bash
   pio device monitor --port /dev/ttyUSB0 --baud 115200
   ```
   - Add debug print statements with `Debug::print()`
   - Use GDB for advanced debugging (requires debugging hardware)
   ```bash
   pio debug -e fancy-node
   ```

### Advanced Topics

1. **Device-specific customization**:
   - Each device type has its own implementation in `src/devices/`
   - Common functions are shared through `src/generic/`
   - Create a custom device by adding a new directory in `src/devices/`

2. **Power management**:
   - Power limitations are enforced at runtime based on `milliamps_supported`
   - Each LED can draw up to 60mA at full brightness
   - The system scales brightness to stay within the power budget

3. **Radio optimization**:
   - The mesh network adds overhead
   - For large installations, strategically place devices to create an effective mesh
   - The packet cache size (5) limits redundant transmissions

## Common Issues and Solutions

1. **Radio communication problems**:
   - Check proper SPI pin configuration (MISO, MOSI, SCK, SS)
   - Verify radio frequency settings match across devices (915MHz in US)
   - Check antenna connection and placement
   - Use debug print to track packet reception: 
   ```cpp
   Debug::print("Received packet ID: %d, Type: %d\n", packet.packet_id, packet.type);
   ```

2. **LED power issues**:
   - Verify device's `milliamps_supported` configuration matches power supply
   - Measure actual current draw at full brightness
   - Consider using `StripDescription` flags to limit specific strips:
   ```cpp
   // For lower brightness strip
   StripDescription(60, StripType::NEOPIXEL, false, true, false) // Dim flag enabled
   ```

3. **Effect synchronization**:
   - Check network time by adding debug print:
   ```cpp
   Debug::print("Network time: %lu, Local: %lu, Offset: %ld\n", 
                radio_state.GetNetworkMillis(), millis(), radio_state.GetTimeDifference());
   ```
   - Ensure effect calculations use network time, not local time
   - Look for phase or timing inconsistencies in effect animation

4. **Flash storage issues**:
   - Check `FLASH_BASE_ADDRESS` definition in platformio.ini
   - Test different device modes in sequence:
   ```cpp
   // First run with WRITE_TO_FLASH
   DeviceMode mode = DeviceMode::WRITE_TO_FLASH;
   
   // Then switch to READ_FROM_FLASH
   DeviceMode mode = DeviceMode::READ_FROM_FLASH;
   ```
   - If flash becomes corrupted, reset to header default with `CURRENT_FROM_HEADER`

5. **Unexpected effects or behaviors**:
   - The master randomly changes effects on a 60-second timer
   - Remotes can force specific effects with delay parameter
   - Check if the device is master or slave: `radio_state.GetCurrentState()`

## Key Files to Understand

- `lib/radio/Radio.hpp` - Core radio interface
- `src/generic/NetworkManager.hpp/.cpp` - Network protocol handling
- `src/generic/RadioStateMachine.hpp/.cpp` - State management
- `lib/led_manager/LedManager.hpp` - LED control abstraction
- `lib/device/DeviceDescription.hpp` - Device configuration
- `lib/device/Devices.hpp` - Device catalog and definitions
- `lib/effect/Effect.hpp` - Base class for effects
- `src/devices/fancy-node/fancy-node.cc` - Example device implementation

## Further Resources

- PCB designs are in `hardware/` directory
- Check `test/` directory for examples of component usage
- For flashing via bootloader, see `bootloader/` directory
- Assembly instructions and BOM in `assembly/`
