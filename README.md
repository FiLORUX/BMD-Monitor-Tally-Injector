# Sony CCU to Blackmagic Monitor Tally via Arduino

This repository contains a complete, self‑contained Arduino project that
converts simple contact–closure tally signals from a Sony camera control unit
(CCU) into SDI embedded tally for Blackmagic Design studio viewfinders.  The
code reads **program** and **preview** closures from the CCU, generates the
appropriate tally packet and injects it into the SDI return feed using the
Blackmagic **3G‑SDI Shield for Arduino**.

Unlike camera‑addressed tally, a viewfinder (or teleprompter monitor) does
not know its camera ID.  Instead it listens to the **monitor tally** field
embedded in the SDI stream.  The Blackmagic Embedded Tally Control
documentation explains that each tally packet starts with a byte where bits 0
and 1 represent the monitor's **program** and **preview** status and that
"slave devices pass through the tally packet on their output and update the
monitor tally status, so that monitor devices … may display tally status
without knowledge of the device id they are monitoring"【463815681556571†L2123-L2132】.
The sketch here writes that monitor byte directly using the library’s low
level `write()` function.

## Hardware requirements

The following inexpensive components are all that is required to build the
converter:

| Component | Purpose |
| --- | --- |
| **Arduino UNO or Nano** | Runs the sketch and hosts the SDI shield. |
| **Blackmagic 3G‑SDI Shield for Arduino** | Inserts the tally packet into the SDI return feed. |
| **Two opto‑isolators (e.g. PC817)** | Isolate the CCU’s open‑collector tally outputs from the Arduino. |
| **Resistors** | Pull‑ups (10 kΩ) on the Arduino side of the opto‑isolators. |
| **12 V power supply** | Powers the SDI shield; the shield regulates 5 V for the Arduino. |
| **SDI cables** | Feed SDI from the switcher/CCU into the shield and from the shield to the viewfinder. |

Any CCU that provides simple closing contacts for program and preview can
drive the inputs.  If your CCU only offers a single tally line (program
only), simply tie the preview input high (or leave it open) so that only
program red lights will trigger.

## Wiring

1. **Mount the shield** on top of the Arduino.  Connect a 12 V DC supply to
   the shield’s barrel jack.  The shield’s SDI **input** comes from your
   switcher’s return feed; the SDI **output** goes to the Blackmagic viewfinder.
2. **Isolate the CCU tally lines** using opto‑isolators:

   - Connect the CCU’s program tally output to the anode of the first
     opto‑isolator LED.  Connect the LED’s cathode to the CCU ground.
   - Repeat for the preview tally output on the second opto‑isolator.
   - On the transistor side of each opto‑isolator, connect the collector
     to an Arduino digital input (e.g. **D2** for program and **D3** for
     preview) and the emitter to Arduino ground.
   - Enable the Arduino’s internal pull‑up resistors in software.  When the CCU
     closes the tally contact the corresponding input reads **LOW**.

3. **SDI chain**:  Place the SDI shield as late as possible in the return
   chain so that no subsequent device strips the embedded tally data.  If
   multiple viewfinders need the same return feed, use an SDI distribution
   amplifier after the shield.

## Software

This project uses the Blackmagic Design `BMDSDIControl` library (available
from [Skaarhoj’s Open Engineering GitHub](https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering)).
You must install this library into your Arduino environment before
compiling.  On a recent version of the Arduino IDE you can do this by
placing the `BMDSDIControl` folder into your `~/Arduino/libraries` directory.

The sketch included here is called **`SonyBMDMonitorTally.ino`**.  It
performs the following tasks:

1. Configures two digital inputs with internal pull‑ups for program and
   preview.
2. Initialises the SDI shield and enables tally override.
3. Reads the inputs and constructs a one‑byte tally packet where bit 0 is
   program and bit 1 is preview【463815681556571†L2163-L2179】.
4. Writes this packet to the shield each time the state changes using
   `sdiTallyControl.write()`.  The shield then embeds the packet into the
   SDI stream.
5. Provides serial debugging output so that you can monitor the tally state
   changes when connected to a computer.

To compile the sketch:

1. Copy the contents of this repository into your Arduino **sketchbook**.
2. Ensure that the `BMDSDIControl` library is installed.
3. Open **`SonyBMDMonitorTally.ino`** in the Arduino IDE, select your board
   and port, then compile and upload.

## Extending the design

This code is deliberately simple.  There is no debouncing or timing logic
because most CCUs hold their tally outputs steady while the signal is valid.
If you encounter false triggering you can introduce a small delay or a
software debounce.  You can also repurpose unused inputs to control other
features, such as lighting a local LED or driving a second tally lamp.

## License

The contents of this repository are released under the MIT License.  See
`LICENSE` for details.