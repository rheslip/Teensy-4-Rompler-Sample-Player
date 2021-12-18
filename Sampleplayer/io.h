
// Copyright 2021 Rich Heslip
//
// Author: Rich Heslip 
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// I/O pin definitions for Teensy 4.1 eurorack DSP module

#ifndef IO_H_
#define IO_H_

// SPI pins - we use HW SPI for display
#define MISO 12
#define MOSI 11
#define SCLK 13

// OLED display pins
#define	OLED_CS	9
#define	OLED_RESET	33
#define	OLED_DC	34


// audio shield SD card chip select pin
#define SD_CS 10



#define P0ENC_A 5
#define P0ENC_B 22
#define P0_SW   39 
#define P1ENC_A 4
#define P1ENC_B 2
#define P1_SW   3 
#define P2ENC_A 25
#define P2ENC_B 24
#define P2_SW   26
#define P3ENC_A 31
#define P3ENC_B 29
#define P3_SW   32
#define P4ENC_A 28  // menu encoder
#define P4ENC_B 27
#define P4_SW   30

// analog inputs for voltage control - input opamp scaler inverts the jack input polarity
// ie -5v ~ 0, +5V ~ 1024
#define AIN0 	14
#define AIN1 	15
#define AIN2 	16
#define AIN3 	17

// Gate/trigger inputs (inverted logic)
#define GATE0	36
#define GATE1	37
#define GATE2	40
#define GATE3	41

// MIDI serial port pins
#define MIDIRX 0
#define MIDITX 1



#endif // IO_H_
