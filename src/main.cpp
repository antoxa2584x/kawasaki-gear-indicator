#include <Arduino.h>
#include <SoftwareSerial.h>
#include "KWP2000.h"

#define K_OUT 1       // Tx on Arduino
#define K_IN 0        // Rx on Arduino
#define BIKE Serial   // Serial communication with bike
#define FETCH_RATE 10 // 10 times / s

#define DEBUG_IN 2  // SoftwareSerial Rx
#define DEBUG_OUT 3 // SoftwareSerial Tx

#define BOARD_LED 13

// KL5611-ASR: single digit 0.56" 7-segment display, COMMON CATHODE.
// Both COM pins (3 and 8) go to GND, every segment anode is driven HIGH
// through its own current limiting resistor to light up.
// For a common anode part (KL5611-BSR, 5161BS) swap SEG_ON/SEG_OFF and tie
// COM to +5V instead.
#define SEG_ON HIGH
#define SEG_OFF LOW

// Segment pins in order a, b, c, d, e, f, g.
// Display pinout: 1=E 2=D 3=COM 4=C 5=DP 6=B 7=A 8=COM 9=F 10=G
// DP is left unconnected.
const uint8_t SEGMENT_PINS[] = {4, 5, 6, 7, 8, 9, 10};
#define SEGMENT_COUNT (sizeof(SEGMENT_PINS) / sizeof(SEGMENT_PINS[0]))

// Segment bit masks, matching the order of SEGMENT_PINS.
#define SEG_A 0x01
#define SEG_B 0x02
#define SEG_C 0x04
#define SEG_D 0x08
#define SEG_E 0x10
#define SEG_F 0x20
#define SEG_G 0x40

#define GLYPH_BLANK 0x00
#define GLYPH_NEUTRAL (SEG_C | SEG_E | SEG_G) // lowercase 'n'

// The outer ring of segments in clockwise order, used by the animations.
const uint8_t RING_SEGMENTS[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
#define RING_COUNT (sizeof(RING_SEGMENTS) / sizeof(RING_SEGMENTS[0]))
#define WAITING_FRAME_MS 100 // spinner step while waiting for the ECU

// Gear 0 is neutral, 1 - 6 are the gears.
const uint8_t GEAR_GLYPHS[] = {
    GLYPH_NEUTRAL,                                       // N
    SEG_B | SEG_C,                                       // 1
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,               // 2
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,               // 3
    SEG_B | SEG_C | SEG_F | SEG_G,                       // 4
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,               // 5
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,       // 6
};
#define GEAR_COUNT (sizeof(GEAR_GLYPHS) / sizeof(GEAR_GLYPHS[0]))

SoftwareSerial debug(DEBUG_IN, DEBUG_OUT);

KWP2000 ECU(&BIKE, K_OUT);

int8_t initKline();
void displayBegin();
void displayGlyph(uint8_t glyph);
void setGearOnDisplay(uint8_t gear);
void displayAnimation();
void waitingAnimation();
void terminate();

void setup()
{
  pinMode(K_OUT, OUTPUT); // required?
  pinMode(K_IN, INPUT);   // required?
  pinMode(BOARD_LED, OUTPUT);
  digitalWrite(BOARD_LED, LOW);

  displayBegin();
  displayAnimation();

  // Test gears display on the 7-segment
  /*for (uint8_t gear = 0; gear < GEAR_COUNT; gear++)
  {
    setGearOnDisplay(gear);
    delay(1000);
  }*/

  ECU.enableDebug(&debug, DEBUG_LEVEL_DEFAULT, 9600);
  // if initialization is negative, terminate (hardware must be reset).
  if (initKline() < 0)
  {
    terminate();
  }
  // turn on onboard LED if init successful
  digitalWrite(BOARD_LED, HIGH);
}

void loop()
{
  int8_t status = ECU.requestGPS();
  debug.println(status);
  if (status)
  {
    uint8_t gear = ECU.getGPS();
    // set segments accordingly
    debug.println(gear);
    setGearOnDisplay(gear);
  }
  else
  {
    // might have to be replaced with keepalive function,
    // because when arduino is not powered via bike battery
    // and bike is turned off, this _ECU_status = true and remains true
    if (initKline() < 0)
    {
      terminate();
    }
  }
  delay(1000 / FETCH_RATE);
}

int8_t initKline()
{
  int8_t status = 0;
  // init needs to go through 5 phases, spin the display while we wait for them
  while (status == 0)
  {
    status = ECU.initKline();
    waitingAnimation();
  }
  return status;
}

void displayBegin()
{
  for (uint8_t i = 0; i < SEGMENT_COUNT; i++)
  {
    pinMode(SEGMENT_PINS[i], OUTPUT);
    digitalWrite(SEGMENT_PINS[i], SEG_OFF);
  }
}

void displayGlyph(uint8_t glyph)
{
  for (uint8_t i = 0; i < SEGMENT_COUNT; i++)
  {
    digitalWrite(SEGMENT_PINS[i], (glyph & (1 << i)) ? SEG_ON : SEG_OFF);
  }
}

void setGearOnDisplay(uint8_t gear)
{
  // an out of range reading blanks the display instead of showing a wrong gear
  displayGlyph(gear < GEAR_COUNT ? GEAR_GLYPHS[gear] : GLYPH_BLANK);
}

void displayAnimation()
{
  // chase the outer ring of segments once, then clear
  for (uint8_t i = 0; i < RING_COUNT; i++)
  {
    displayGlyph(RING_SEGMENTS[i]);
    delay(50);
  }
  displayGlyph(GLYPH_BLANK);
}

// Rotates a single segment around the outer ring to show we have no gear data
// yet. Non blocking: every call advances the spinner at most one frame, so it
// can be polled from a wait loop without stretching it out.
void waitingAnimation()
{
  static uint32_t last_frame = 0;
  static uint8_t frame = 0;

  uint32_t now = millis();
  if (now - last_frame < WAITING_FRAME_MS)
  {
    return;
  }
  last_frame = now;

  displayGlyph(RING_SEGMENTS[frame]);
  frame = (frame + 1) % RING_COUNT;
}

void terminate()
{
  displayGlyph(GLYPH_BLANK);
  Serial.end();
  debug.println("Hardware terminating");
  exit(0);
}
