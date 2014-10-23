// This library is derived from Adafruit's I2C LiquidCrystal library.
// Adafruit provide excellent products and support for makers and tinkerers.
// Please support them from buying products from their web site.
// https://github.com/adafruit/LiquidCrystal/blob/master/LiquidCrystal.cpp
//
// This library controls the LCD display, the LCD backlight and the buzzer.  These
// are connected to the microcontroller using a I2C GPIO IC (MCP23008).
//
// Written by Peter Easton
// Released under WTFPL license
//
// Change History:
// 14 August 2014        Initial Version

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "ControLeo2_LiquidCrystal.h"


// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1 
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).


ControLeo2_LiquidCrystal::ControLeo2_LiquidCrystal(void) {
    
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    
    // Save the pins used to drive the LCD
    _rs_pin = A0;
    _enable_pin = A1;
    _data_pins[0] = A2;
    _data_pins[1] = A3;
    _data_pins[2] = A4;
    _data_pins[3] = A5;
    
    // Set all the pins to be outputs
    pinMode(_rs_pin, OUTPUT);
    pinMode(_enable_pin, OUTPUT);
    for (int i=0; i<4; i++)
      pinMode(_data_pins[i], OUTPUT);
}


void ControLeo2_LiquidCrystal::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
    if (lines > 1)
        _displayfunction |= LCD_2LINE;
    _numlines = lines;
    _currline = 0;
    
    // For some 1 line displays you can select a 10 pixel high font
    if ((dotsize != 0) && (lines == 1)) {
        _displayfunction |= LCD_5x10DOTS;
    }

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // According to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
    delayMicroseconds(50000);
    
    // Now we pull both RS and R/W low to begin commands
    digitalWrite(_rs_pin, LOW);
    digitalWrite(_enable_pin, LOW);
    
    //Put the LCD into 4 bit mode
    // This is according to the hitachi HD44780 datasheet figure 24, pg 46
    // We start in 8bit mode, try to set 4 bit mode
    write4bits(0x03);
    delayMicroseconds(4500); // wait min 4.1ms
    
    // Second try
    write4bits(0x03);
    delayMicroseconds(4500); // wait min 4.1ms
    
    // Third go!
    write4bits(0x03);
    delayMicroseconds(150);
    
    // Finally, set to 8-bit interface
    write4bits(0x02);
    
    // Finally, set # lines, font size, etc.
    command(LCD_FUNCTIONSET | _displayfunction);
    
    // Turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();
    
    // Clear the display
    clear();
    
    // Initialize to default text direction (for romance languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // Set the entry mode
    command(LCD_ENTRYMODESET | _displaymode);
}


// Clear the LCD display
void ControLeo2_LiquidCrystal::clear()
{
    command(LCD_CLEARDISPLAY);    // Clear display, set cursor position to zero
    delayMicroseconds(2000);      // This command takes a long time!
}


// Set the cursor position to (0, 0)
void ControLeo2_LiquidCrystal::home()
{
    command(LCD_RETURNHOME);  // Set cursor position to zero
    delayMicroseconds(2000);  // This command takes a long time!
}


// Put the cursor in the specified position
void ControLeo2_LiquidCrystal::setCursor(uint8_t col, uint8_t row)
{
    int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > _numlines) {
        row = _numlines - 1;    // Count rows starting with 0
    }
    command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}


// Turn the display on/off (quickly)
void ControLeo2_LiquidCrystal::noDisplay() {
    _displaycontrol &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void ControLeo2_LiquidCrystal::display() {
    _displaycontrol |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}


// Turns the underline cursor on/off
void ControLeo2_LiquidCrystal::noCursor() {
    _displaycontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void ControLeo2_LiquidCrystal::cursor() {
    _displaycontrol |= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}


// Turn on and off the blinking cursor
void ControLeo2_LiquidCrystal::noBlink() {
    _displaycontrol &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void ControLeo2_LiquidCrystal::blink() {
    _displaycontrol |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}


// These commands scroll the display without changing the RAM
void ControLeo2_LiquidCrystal::scrollDisplayLeft(void) {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void ControLeo2_LiquidCrystal::scrollDisplayRight(void) {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}


// This is for text that flows Left to Right
void ControLeo2_LiquidCrystal::leftToRight(void) {
    _displaymode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}


// This is for text that flows Right to Left
void ControLeo2_LiquidCrystal::rightToLeft(void) {
    _displaymode &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}


// This will 'right justify' text from the cursor
void ControLeo2_LiquidCrystal::autoscroll(void) {
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}


// This will 'left justify' text from the cursor
void ControLeo2_LiquidCrystal::noAutoscroll(void) {
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}


// Allows us to fill the first 8 CGRAM locations
// with custom characters
void ControLeo2_LiquidCrystal::createChar(uint8_t location, uint8_t charmap[]) {
    location &= 0x7; // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    for (int i=0; i<8; i++)
        write(charmap[i]);
}


// Mid level commands, for sending data/cmds
inline void ControLeo2_LiquidCrystal::command(uint8_t value) {
    send(value, LOW);
}

inline size_t ControLeo2_LiquidCrystal::write(uint8_t value) {
    send(value, HIGH);
    return 1;
}


// Low level data pushing commands
// Write either command or data, with automatic 4/8-bit selection
void ControLeo2_LiquidCrystal::send(uint8_t value, uint8_t mode) {
    digitalWrite(_rs_pin, mode);
    
    write4bits(value>>4);
    write4bits(value);
}


void ControLeo2_LiquidCrystal::write4bits(uint8_t value) {
    for (int i = 0; i < 4; i++)
        digitalWrite(_data_pins[i], (value >> i) & 0x01);
    
    // Pulse enable
    digitalWrite(_enable_pin, LOW);
    delayMicroseconds(1);
    digitalWrite(_enable_pin, HIGH);
    delayMicroseconds(1);    // enable pulse must be >450ns
    digitalWrite(_enable_pin, LOW);
    delayMicroseconds(100);   // commands need > 37us to settle
}
