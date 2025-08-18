/*
 * SonyBMDMonitorTally.ino
 *
 * This sketch converts simple contact‑closure tally outputs from a Sony
 * camera control unit (CCU) into an embedded monitor tally for Blackmagic
 * Design viewfinders.  The Blackmagic 3G‑SDI Shield for Arduino allows
 * an Arduino to embed tally control packets into the VANC region of an SDI
 * video feed.  A monitor such as the URSA Studio Viewfinder reads only
 * the first byte of this packet – bits 0 and 1 – to determine whether
 * program (red) or preview (green) tally should be illuminated【463815681556571†L2163-L2180】.
 * Cameras themselves ignore this monitor byte and instead listen for
 * per‑camera bits later in the packet.  Therefore, you can drive a
 * monitor directly with no knowledge of camera IDs.
 *
 * Hardware assumptions:
 *  - A Blackmagic 3G‑SDI Shield is mounted on top of an Arduino UNO or
 *    Nano.  The shield must be powered from 12 V; it will feed 5 V to
 *    the Arduino.
 *  - Two tally lines from the Sony CCU are connected through
 *    opto‑isolators to digital pins defined below.  The opto‑isolators
 *    ensure galvanic isolation.  When the CCU closes the tally contact
 *    the corresponding Arduino input is pulled **LOW**.
 *  - The SDI input of the shield carries the return feed from your
 *    switcher; the SDI output goes directly to the viewfinder.
 *
 * Dependencies:
 *  The project depends on the BMDSDIControl library which contains the
 *  `BMD_SDITallyControl_I2C` class.  You can obtain this library from
 *  <https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering> and
 *  install it into your Arduino libraries folder.
 */

#include <BMDSDIControl.h>

// -----------------------------------------------------------------------------
// Pin assignments
// Adjust these constants to match your wiring.  Using INPUT_PULLUP
// configuration means that the input reads HIGH when idle and LOW when the
// CCU closes the contact (via the opto‑isolator).  Feel free to swap pins
// around if your board requires it.
const uint8_t PROGRAM_PIN = 2;  // Digital input for program tally (red)
const uint8_t PREVIEW_PIN = 3;  // Digital input for preview tally (green)

// -----------------------------------------------------------------------------
// SDI shield configuration
// The shield communicates over I2C.  The default address is 0x6E.  If you
// changed the shield’s address by soldering jumpers, update the value here.
BMD_SDITallyControl_I2C sdiTallyControl(0x6E);

// Remember the last tally state to avoid writing the same value repeatedly.
bool previousProgram = false;
bool previousPreview = false;

/**
 * Prepare the Arduino and the SDI shield.
 */
void setup() {
  // Start the serial port for debugging.  Use 115200 baud so messages
  // print quickly and clearly.  You can monitor the output with the
  // Arduino Serial Monitor.
  Serial.begin(115200);
  Serial.println(F("SonyBMDMonitorTally starting…"));

  // Configure the input pins.  INPUT_PULLUP activates an internal pull‑up
  // resistor so the input floats HIGH until pulled LOW via the opto‑isolator.
  pinMode(PROGRAM_PIN, INPUT_PULLUP);
  pinMode(PREVIEW_PIN, INPUT_PULLUP);

  // Initialise the SDI shield.  This call sets up the I2C bus and
  // prepares the shield for tally control.  If the shield is not
  // connected, subsequent calls will silently fail.
  sdiTallyControl.begin();

  // Enable tally override.  Without this call the shield will simply pass
  // through incoming tally data from the SDI input.  Setting override
  // instructs the shield to replace the incoming tally data with our
  // generated packet【16298302544168†L43-L53】.
  sdiTallyControl.setOverride(true);

  // Write an initial tally state (all off) so that the monitor starts in
  // a known state.  We do this explicitly rather than relying on the
  // default to ensure there is no residual tally from previous uses.
  updateMonitorTally(false, false);
}

/**
 * Main loop.  Poll the CCU tally inputs and update the SDI tally packet
 * whenever a change occurs.  A simple polling loop is sufficient because
 * tally changes occur infrequently and do not require precise timing.
 */
void loop() {
  // Read the current state of the inputs.  Since we use INPUT_PULLUP,
  // the input is LOW when the tally is **active** (contact closed) and
  // HIGH when it is **inactive** (contact open).
  bool programActive = (digitalRead(PROGRAM_PIN) == LOW);
  bool previewActive = (digitalRead(PREVIEW_PIN) == LOW);

  // Only update the monitor tally if there has been a change.
  if (programActive != previousProgram || previewActive != previousPreview) {
    updateMonitorTally(programActive, previewActive);
    previousProgram = programActive;
    previousPreview = previewActive;
  }

  // A small delay prevents the loop from running unnecessarily fast.  If
  // your CCU outputs bounce or if you wish to reduce CPU load, increase
  // this value.  10 ms strikes a good balance between responsiveness and
  // resource usage.
  delay(10);
}

/**
 * Send a monitor tally packet to the SDI shield.  The Blackmagic
 * Embedded Tally Control Protocol defines the first byte of the packet as
 * the monitor status: bit 0 = program, bit 1 = preview, bits 2–3 = reserved
 * and bits 4–7 = protocol version【463815681556571†L2163-L2184】.  The shield
 * will embed this byte into the SDI stream and update the viewfinder.
 *
 * @param programOn  true to turn on the red (program) tally
 * @param previewOn  true to turn on the green (preview) tally
 */
void updateMonitorTally(bool programOn, bool previewOn) {
  // Compose the packet.  We allocate a one‑element array for clarity
  // although a single byte variable would suffice.  Bits are set
  // individually to avoid side effects.
  uint8_t packet[1];
  packet[0] = 0; // start with all bits zero (protocol version 0)

  if (programOn) {
    packet[0] |= (1 << 0);
  }
  if (previewOn) {
    packet[0] |= (1 << 1);
  }

  // Write the packet to the shield.  The `write()` call blocks until the
  // shield is ready to accept new tally data.  Because we only ever
  // overwrite the entire packet, there is no need to set a length
  // separately.  Should you wish to address camera IDs later, you can
  // expand this array and set additional bits accordingly.
  sdiTallyControl.write(packet, sizeof(packet));

  // Optional debug output to the serial console.
  Serial.print(F("Monitor tally updated: program="));
  Serial.print(programOn ? F("ON") : F("off"));
  Serial.print(F(", preview="));
  Serial.println(previewOn ? F("ON") : F("off"));
}