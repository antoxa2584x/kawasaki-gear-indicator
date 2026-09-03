# Kawasaki 7-Segment Gear Indicator
 <a href="https://www.youtube.com/shorts/qYxOT7fzgA0"><img align="right" src="https://i.ytimg.com/vi/qYxOT7fzgA0/oar2.jpg?sqp=-oaymwEaCJUDENAFSFXyq4qpAwwIARUAAIhCcAHAAQY=&rs=AOn4CLBmHTwcPXswSw9OFq_qdH_oEBWi4Q" width="215" height="324"/></a>
A custom made gear indicator with a 5161BS 7-segment display and some hardware to read the Gear Position Sensor (GPS) from the Kawasaki Diagnostic System (KDS).


## KWP2000 Library
Most of the work is done by Aster94 through his [KWP2000](https://github.com/aster94/Keyword-Protocol-2000) library, providing an interface to communicate with Suzuki and Kawasaki bikes. Check out that GitHub page for more info about the possible hardware communication possibilities.
I modified the library to limit it for communication with Kawasaki ECUs. Next to this, I added a *SoftwareSerial* debugging to allow debugging on devices that do not provide two Hardware Serial interfaces, e.g. the Atmega328P on the Arduino Uno/Nano.

## Hardware used
- **Arduino Nano**;
- **L9637D**: *ISO 9141 IC that converts the two line UART (Tx/Rx) of the Arduino to the single bi-directional K-line, and vice-versa. The original build used an MC33660, either works;*
- **KL5611-ASR display**: *single digit 0.56" 7-segment display, common cathode, powered from 5V. Shows `1`-`6` for the gears and a lowercase `n` for neutral;*
- **FWY-C-4F-B Connector**: *Male KDS connector, I found a cheap one on [AliExpress](https://nl.aliexpress.com/item/1005002438381284.html?spm=a2g0o.order_list.order_list_main.5.186f79d26YG6rV&gatewayAdapt=glo2nld).*

### Diagram
![SDF](https://github.com/BramNH/kawasaki-gear-indicator/blob/main/img/schematic.jpg?raw=true)

> Note: the diagram still shows the older W2812B LED strip wiring, the 7-segment wiring is described below.

### KL5611-ASR wiring
The KL5611-ASR is **common cathode**: both COM pins (3 and 8) go to GND and each segment anode is driven HIGH by the Arduino to light up. Every segment needs its own current limiting resistor (470 Ohm - 1k) in series, which keeps the total current through the shared COM pin low.

| Segment | Display pin | Arduino Nano pin |
| ------- | ----------- | ---------------- |
| A       | 7           | D4               |
| B       | 6           | D5               |
| C       | 4           | D6               |
| D       | 2           | D7               |
| E       | 1           | D8               |
| F       | 9           | D9               |
| G       | 10          | D10              |
| COM     | 3 and 8     | GND              |
| DP      | 5           | not connected    |

Some notes:
- For a common anode variant (KL5611-BSR, 5161BS) tie both COM pins to +5V and swap `SEG_ON`/`SEG_OFF` in `src/main.cpp`.
- The K-line uses D0/D1, the same pins as the USB serial. Disconnect the L9637D Tx line before uploading new firmware.
- The display can be powered from the 5V output of the Arduino Nano. A digit lights at most 6 segments at once, so with 470 Ohm resistors that is roughly 6 x 7mA through the COM pin, well inside the Atmega328P limits. It is still better to power the Arduino Nano from an external power source (12v bike battery).
