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
// 
// File player sketch for the Teensy 4.1 eurorack DSP module
// loads .raw files from SD card and plays them via trigger or MIDI - samples must be mono, 16 bit pcm, 44100hz 
// uses the excellent TeensyVariablePlayback library by Nic Newdigate https://github.com/newdigate
// the flashloader library loads samples into PSRAM which is much faster than playing from SD card
// Teensy 4.1 supports up to 16mb of PSRAM - approx 160 seconds of mono sample playback @ 44.1khz
// implements sample repitching via menus or MIDI notes, transpose, sample looping. ADSR envelopes and sample scrubbing (sample start end end points)
// code is kind of hacky but it works!

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <SH1106.h>
#include <SD.h>
#include "MIDI.h"
#include "io.h"
#include "ClickEncoder.h"
#include <Audio.h>
#include <SD.h>
#include <TeensyVariablePlayback.h>
#include "flashloader.h"

// Teensy audio stuff
// samples -> envelopes -> mixer -> output
AudioPlayArrayResmp      audiosample0;     
AudioPlayArrayResmp      audiosample1; 
AudioPlayArrayResmp      audiosample2;     
AudioPlayArrayResmp      audiosample3;     
AudioEffectEnvelope     envelope0;  
AudioEffectEnvelope     envelope1;  
AudioEffectEnvelope     envelope2;  
AudioEffectEnvelope     envelope3;    
AudioMixer4              mixer;   // sums the wav files
/*
AudioConnection          patchCord1(audiosample0, 0, mixer,0);
AudioConnection          patchCord2(audiosample1, 0, mixer,1);
AudioConnection          patchCord3(audiosample2, 0, mixer,2);
AudioConnection          patchCord4(audiosample3, 0, mixer,3);
*/
AudioConnection          patchCord0(audiosample0, 0, envelope0,0);
AudioConnection          patchCord1(envelope0, 0, mixer,0);

AudioConnection          patchCord10(audiosample1, 0, envelope1,0);
AudioConnection          patchCord11(envelope1, 0, mixer,1);
AudioConnection          patchCord20(audiosample2, 0, envelope2,0);
AudioConnection          patchCord21(envelope2, 0, mixer,2);
AudioConnection          patchCord30(audiosample3, 0, envelope3,0);
AudioConnection          patchCord31(envelope3, 0, mixer,3);

AudioOutputI2S           i2s2;           
AudioConnection          patchCord5(mixer, 0, i2s2, 0);
AudioConnection          patchCord6(mixer, 0, i2s2, 1);
AudioControlSGTL5000     audioShield;

#define NUMSAMPLES 4

const int CV0 = A0;
const int CV1 = A1;
const int CV2 = A2;
const int CV3 = A3;

struct controlvoltage {
  int16_t offset;   // to null out offset on this channel
  float value;   // current reading range -1.0 to +1.0
} cv[4] = {0,0,0,0,0.0,0.0,0.0,0.0};


newdigate::audiosample *sample0;
newdigate::audiosample *sample1;
newdigate::audiosample *sample2;
newdigate::audiosample *sample3;

// file browser stuff
const int linePerPage = 6;
#define PIXELSPERLINE 10
#define MAXFILES 200  // will blow up if more than this many files in a directory
File root;
char workingdir[30];  // storage for current directory on SD
int fileCount, firstOption=0, selectedLine;
String fileNames[MAXFILES] = {};
bool isdir[MAXFILES];    // flags directories

// loop mode definitions - these numbers much match the menu indexes (note: playback library has its own looping definitions) 
#define LOOPMODE_NONE 0   // do not loop sample
#define LOOPMODE_TRIGGERED 1 // loop continously after trigger, trigger restarts loop, AR envelope. MIDI note loops while key is down
#define LOOPMODE_GATED 2   // loop continously while gate is high - ADSR envelope

// sound structure holds parameters for each of the samples
struct snd {
char filename[30];    // filenames of the sounds we are using
int16_t speed;  // playback speed - we convert this to float since menus can't do floats directly
int16_t transpose;
int16_t mixlevel; 
int16_t loopmode;
int16_t loopstart;
int16_t loopend;
int16_t delay;  // envelope parameters
int16_t attack;
int16_t hold;
int16_t decay;
int16_t sustain;
int16_t release;
int16_t releasenoteon; // not sure what this is used for 
int16_t midichannel;
int16_t midinote;   // midi note we are playing
int16_t note;       // note pitch of the sample

} sound[NUMSAMPLES] = {
  "sample0.raw",  // filename
  1000,          // playback speed -this gets converted to 1.000
  0,            // transpose in semitones
  1000,         // mix level - gets translated to 0-1.0
  LOOPMODE_NONE,        // loop mode 
  0,              // loop start 0-9999, relative to actual buffer size
  9999,           // loop end 0-9999, ""
  0,                  // delay in ms 
  5,                  // attack ""
  0,              // hold time in ms
  5,            // decay time in ms
  1000,         // suatain level * 1000
  5,            // release time in ms
  5,            // releaseNoteOn in ms
  1,            // MIDI channel
  60,         // MIDI note we are playing
  60,       // note pitch of the sample
  
  "sample1.raw",
  1000,
  0,
  1000,
  LOOPMODE_NONE,
  0,              // loop start 0-9999, relative to actual buffer size
  9999,           // loop end 0-9999, ""
  0,                  // delay in ms 
  5,                  // attack ""
  0,              // hold time in ms
  5,            // decay time in ms
  1000,         // suatain level * 1000
  5,            // release time in ms
  5,            // releaseNoteOn in ms
  2,            // MIDI channel
  60,         // MIDI note we are playing
  60,       // note pitch of the sample
    
  "sample2.raw",
  1000,
  0,
  1000,
  LOOPMODE_NONE,
  0,              // loop start 0-9999, relative to actual buffer size
  9999,           // loop end 0-9999, ""
  0,                  // delay in ms 
  5,                  // attack ""
  0,              // hold time in ms
  5,            // decay time in ms
  1000,         // suatain level * 1000
  5,            // release time in ms
  5,            // releaseNoteOn in ms
  3,            // MIDI channel
  60,         // MIDI note we are playing
  60,       // note pitch of the sample
    
  "sample3.raw",
  1000,
  0,
  1000,
  LOOPMODE_NONE,
  0,              // loop start 0-9999,relative to actual buffer size
  9999,           // loop end 0-9999, ""
  0,                  // delay in ms 
  5,                  // attack ""
  0,              // hold time in ms
  5,            // decay time in ms
  1000,         // suatain level * 1000
  5,            // release time in ms
  5,            // releaseNoteOn in ms
  4,            // MIDI channel
  60,         // MIDI note we are playing
  60,       // note pitch of the sample
} ;



// menu handler functions
// file select functions 
void selectfile0(int s) {
  selectfile(0);      // call the file picker
  stopplayers();    // stop playing so we don't hear garbage
  loadsamples();    // reload samples
  restoremenus();    // redraw the main menus
}

void selectfile1(int s) {
  selectfile(1);      // call the file picker
  stopplayers();    // stop playing so we don't hear garbage
  loadsamples();    // reload samples
  restoremenus();    // redraw the main menus
}

void selectfile2(int s) {
  selectfile(2);      // call the file picker
  stopplayers();    // stop playing so we don't hear garbage
  loadsamples();    // reload samples
  restoremenus();    // redraw the main menus
}

void selectfile3(int s) {
  selectfile(3);      // call the file picker
  stopplayers();    // stop playing so we don't hear garbage
  loadsamples();    // reload samples
  restoremenus();    // redraw the main menus
}

// update all sample mixer levels 
void setmixerlevels(void) {
  for (int i=0; i< 4; ++i) {
   mixer.gain(i,(float)sound[i].mixlevel/1000);
  }
}

// set all samples loop mode
void setloopmodes(void) {
  if (sound[0].loopmode!=LOOPMODE_NONE) audiosample0.setLoopType(loop_type::looptype_repeat);
  else audiosample0.setLoopType(loop_type::looptype_none); 
  
  if (sound[1].loopmode!=LOOPMODE_NONE) audiosample1.setLoopType(loop_type::looptype_repeat);
  else audiosample1.setLoopType(loop_type::looptype_none); 
  
  if (sound[2].loopmode!=LOOPMODE_NONE) audiosample2.setLoopType(loop_type::looptype_repeat);
  else audiosample2.setLoopType(loop_type::looptype_none); 
  
  if (sound[3].loopmode!=LOOPMODE_NONE) audiosample3.setLoopType(loop_type::looptype_repeat);
  else audiosample3.setLoopType(loop_type::looptype_none);   
}

// set all sample envelopes
// only need to do this when they are changed in menus
void setenvelopes(void) {
  envelope0.delay((float)sound[0].delay);
  envelope0.attack((float)sound[0].attack);
  envelope0.hold((float)sound[0].hold);
  envelope0.decay((float)sound[0].decay);
  envelope0.sustain((float)sound[0].sustain/1000);
  envelope0.release((float)sound[0].release); 
  envelope0.releaseNoteOn((float)sound[0].releasenoteon);  

  envelope1.delay((float)sound[1].delay);
  envelope1.attack((float)sound[1].attack);
  envelope1.hold((float)sound[1].hold);
  envelope1.decay((float)sound[1].decay);
  envelope1.sustain((float)sound[1].sustain/1000);
  envelope1.release((float)sound[1].release); 
  envelope1.releaseNoteOn((float)sound[1].releasenoteon); 

  envelope2.delay((float)sound[2].delay);
  envelope2.attack((float)sound[2].attack);
  envelope2.hold((float)sound[2].hold);
  envelope2.decay((float)sound[2].decay);
  envelope2.sustain((float)sound[2].sustain/1000);
  envelope2.release((float)sound[2].release); 
  envelope2.releaseNoteOn((float)sound[2].releasenoteon);  

  envelope3.delay((float)sound[3].delay);
  envelope3.attack((float)sound[3].attack);
  envelope3.hold((float)sound[3].hold);
  envelope3.decay((float)sound[3].decay);
  envelope3.sustain((float)sound[3].sustain/1000);
  envelope3.release((float)sound[3].release); 
  envelope3.releaseNoteOn((float)sound[3].releasenoteon);  
  
}
// create OLED device
Adafruit_SH1106 display(OLED_DC, OLED_RESET, OLED_CS);

// encoders - from left to right on panel. 4 is the menu encoder
ClickEncoder P0Encoder(P0ENC_A,P0ENC_B,P0_SW,4); // divide by 4 works best with this encoder
ClickEncoder P1Encoder(P1ENC_A,P1ENC_B,P1_SW,4); // switch is hooked to RXD0 - have to set it up after calling serial.begin()
ClickEncoder P2Encoder(P2ENC_A,P2ENC_B,P2_SW,4); 
ClickEncoder P3Encoder(P3ENC_A,P3ENC_B,P3_SW,4); 
ClickEncoder P4Encoder(P4ENC_A,P4ENC_B,P4_SW,4); 

#include "menusystem.h"  // has to come after display and encoder objects creation

void restoremenus(void) {
  display.clearDisplay();
  drawtopmenu(topmenuindex);  // redraw the main menus
  drawsubmenus();   
}

// create serial MIDI device
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
//MIDI_CREATE_DEFAULT_INSTANCE();



// Create an IntervalTimer object to service the encoders
IntervalTimer msTimer;

// encoder timer interrupt handler at 1khz
// uses the msTimer timer
void msTimerhandler(){
  P0Encoder.service();
  P1Encoder.service();
  P2Encoder.service();
  P3Encoder.service();
  P4Encoder.service();
}

// MIDI handler functions

void handleNoteOn(uint8_t channel, uint8_t pitch, uint8_t velocity)
{
  for(int i=0; i<NUMSAMPLES; ++i) {
    if (sound[i].midichannel == channel) {
      if (i==0) {
        sound[0].midinote=(int16_t) pitch; // save the note - need this to set the pitch of the sample
        trigger0();
      }
      if (i==1) {
        sound[1].midinote=(int16_t) pitch; // save the note - need this to set the pitch of the sample
        trigger1();
      }
      if (i==2) {
        sound[2].midinote=(int16_t) pitch; // save the note - need this to set the pitch of the sample
        trigger2();
      }
      if (i==3) {
        sound[3].midinote=(int16_t) pitch; // save the note - need this to set the pitch of the sample
        trigger3();
      }
    }
  }
  setplaybackrates(); // update the pitch of all samples
}

void handleNoteOff(uint8_t channel, uint8_t pitch, uint8_t velocity)
{
  for(int i=0; i<NUMSAMPLES; ++i) {
    if ((uint8_t)sound[i].midichannel == channel) {
      if (i==0 && (sound[i].midinote == pitch)) envelope0.noteOff(); // make sure we are shutting off the last note played
      if (i==1 && (sound[i].midinote == pitch)) envelope1.noteOff(); // its a pain that you can't put audio objects in an array
      if (i==2 && (sound[i].midinote == pitch)) envelope2.noteOff();
      if (i==3 && (sound[i].midinote == pitch)) envelope3.noteOff();
    }
  }
}

// initialize everything

void setup() {
  
 Serial.begin(115200);
 
// while (!Serial) ; // wait for console
 
 Serial.println("Starting setup");   

    // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SH1106_SWITCHCAPVCC);
//  display.display();
//  delay(1000);
  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE,BLACK); // need to specify foreground and background so text cel gets completely rewritten- otherwise its OR drawing
  display.display();
   
//  Set up serial MIDI port
//  Serial1.begin(31250, SERIAL_8N1);
  MIDI.setHandleNoteOn(handleNoteOn);  
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  pinMode(GATE0,INPUT_PULLUP);   // pullups for transistor collectors
  pinMode(GATE1,INPUT_PULLUP);
  pinMode(GATE2,INPUT_PULLUP);
  pinMode(GATE3,INPUT_PULLUP);

  analogReference(0);
  pinMode(AIN0, INPUT_DISABLE);   // Analog CV inputs
  pinMode(AIN1, INPUT_DISABLE);   
  pinMode(AIN2, INPUT_DISABLE);   
  pinMode(AIN3, INPUT_DISABLE);   
    
  // timer for encoder sampling
  msTimer.begin(msTimerhandler, 1000);  // run every ms

// start up audio system
  AudioMemory(20);

  audioShield.enable();
  audioShield.volume(0.5);
      
 // some startup messages
  display.setCursor(2,4);
  display.println("    Sample Player"); 
  display.display();
  delay(2000);
  
  display.setCursor(0,25);
  if (!(SD.begin(BUILTIN_SDCARD))) {
        // stop here if no SD card, but print a message
     display.print("No SD card");
     display.display();
     while (1) {
     }
  }


  display.println("SD Card Ready");
  display.display();
  root = SD.open("/");
//   Serial.println("SD opened"); 
  setFileCounts(root);
 //  Serial.println("setfilecounts"); 
  String countLabel = "File Count: ";
  countLabel.concat(fileCount);
  display.println("");
  display.println(countLabel);
  display.display();
  delay(1000); 
  
  // load initial samples from SD
  loadsamples();
  setmixerlevels();
  setloopmodes();
  setenvelopes();
 
  audiosample0.enableInterpolation(true);
  audiosample1.enableInterpolation(true);
  audiosample2.enableInterpolation(true);
  audiosample3.enableInterpolation(true);
  
  display.clearDisplay();
  drawtopmenu(topmenuindex);
  drawsubmenus();  

}

long t = 0;
int type, note, velocity, channel, d1, d2;
int enc;
bool trig0,trig1,trig2,trig3; // trigger state 

// the superloop - handle MIDI, menus etc

void loop() {
  MIDI.read();   // check for MIDI input
  domenus();
  setplaybackrates();
  samplecvs();
  processgates();
}

// sample control voltages
void samplecvs(void) {
  cv[0].value=-(float)(analogRead(CV0) - 512 -cv[0].offset)/512; // input buffers are inverting
  cv[1].value=-(float)(analogRead(CV1) - 512 -cv[1].offset)/512; // convert to float range -1.0 to +1.0
  cv[2].value=-(float)(analogRead(CV2) - 512 -cv[2].offset)/512;
  cv[3].value=-(float)(analogRead(CV3) - 512 -cv[3].offset)/512; 
  Serial.println(cv[0].value);
}


// calculate sample position in buffer for looping 
// position values must be between 0 and 1.0
uint32_t sampleposition(newdigate::audiosample *sample,float position) {
  uint32_t pos=(uint32_t)((sample->samplesize/2) * position);
  return pos;
}

// set sample playback speeds
// there are a couple of different ways of setting the pitch of the samples so we do it out all here
void setplaybackrates(void) {
  double playbackrate;
  for (int i=0; i< NUMSAMPLES; ++i) {
    playbackrate=(double)sound[i].speed/1000+cv[i].value;  // start by adjusting the playback speed - sum of menu speed and CV 
    int16_t noteoffset = sound[i].midinote-sound[i].note+sound[i].transpose; // calculate pitch relative to the actual pitch of the sample
    playbackrate=playbackrate*powf(2.0, noteoffset / 12.0);
    switch (i) {
      case 0:
        audiosample0.setPlaybackRate(playbackrate);
        break;
      case 1:
        audiosample1.setPlaybackRate(playbackrate);
        break;      
      case 2:
        audiosample2.setPlaybackRate(playbackrate);
        break;      
      case 3:
        audiosample3.setPlaybackRate(playbackrate);
        break;
      default:
        break;
   }
 } 
}

// stop all sample players
void stopplayers(void) {
  audiosample0.stop();
  audiosample1.stop();
  audiosample2.stop();
  audiosample3.stop();
}

// trigger sample - used by triggers and MIDI noteon
// can't index the audio objects so this is the same code for each sample
void trigger0(void) {
  audiosample0.stop();  // retrigger when gate/trigger goes hi
  // loop start and end points - doesn't seem to be implemented in the library even tho the functions are there
  uint32_t start =sampleposition(sample0,(float)sound[0].loopstart/10000); // relative index of first sample in the loop
  uint32_t end =sampleposition(sample0,(float)sound[0].loopend/10000);  // relative index of last ""
  if (start > end) end = start; // can't do a backwards loop - not this way anyhow!
  uint32_t loopsize = end-start;
  if (loopsize <=0) loopsize=1; // crashes if samplesize is 0
  audiosample0.playRaw(sample0->sampledata+start, loopsize,1); // sample-> samplesize is in bytes
  //audiosample0.playRaw(sample0->sampledata, sample0->samplesize/2,1); // sample-> samplesize is in bytes
  envelope0.noteOn();  
}

void trigger1(void) {
  audiosample1.stop();  // retrigger when gate/trigger goes hi
  // loop start and end points - doesn't seem to be implemented in the library even tho the functions are there
  uint32_t start =sampleposition(sample1,(float)sound[1].loopstart/10000); // relative index of first sample in the loop
  uint32_t end =sampleposition(sample1,(float)sound[1].loopend/10000);  // relative index of last ""
  if (start > end) end = start; // can't do a backwards loop - not this way anyhow!
  uint32_t loopsize = end-start;
  if (loopsize <=0) loopsize=1; // crashes if samplesize is 0
  audiosample1.playRaw(sample1->sampledata+start, loopsize,1); // sample-> samplesize is in bytes
  envelope1.noteOn();  
}

void trigger2(void) {
  audiosample2.stop();  // retrigger when gate/trigger goes hi
  // loop start and end points - doesn't seem to be implemented in the library even tho the functions are there
  uint32_t start =sampleposition(sample2,(float)sound[2].loopstart/10000); // relative index of first sample in the loop
  uint32_t end =sampleposition(sample2,(float)sound[2].loopend/10000);  // relative index of last ""
  if (start > end) end = start; // can't do a backwards loop - not this way anyhow!
  uint32_t loopsize = end-start;
  if (loopsize <=0) loopsize=1; // crashes if samplesize is 0
  audiosample2.playRaw(sample2->sampledata+start, loopsize,1); // sample-> samplesize is in bytes
  envelope2.noteOn();  
}

void trigger3(void) {
  audiosample3.stop();  // retrigger when gate/trigger goes hi
  // loop start and end points - doesn't seem to be implemented in the library even tho the functions are there
  uint32_t start =sampleposition(sample3,(float)sound[3].loopstart/10000); // relative index of first sample in the loop
  uint32_t end =sampleposition(sample3,(float)sound[3].loopend/10000);  // relative index of last ""
  if (start > end) end = start; // can't do a backwards loop - not this way anyhow!
  uint32_t loopsize = end-start;
  if (loopsize <=0) loopsize=1; // crashes if samplesize is 0
  audiosample3.playRaw(sample3->sampledata+start, loopsize,1); // sample-> samplesize is in bytes
  envelope3.noteOn();  
}


// trigger audio samples from gate inputs
void processgates(void) {
  if (!digitalRead(GATE0)) { // gates are active low
    if (!trig0) {  // gate has just hone high
      trigger0();
      trig0=true;
    }
  }
  else {
    trig0=false;
    if (sound[0].loopmode==LOOPMODE_GATED) envelope0.noteOff(); // in hardware trigger mode we just let the loop keep going
  }


  if (!digitalRead(GATE1)) { // gates are active low
    if (!trig1) {
      trigger1();
      trig1=true;
    }
  }
  else {
    trig1=false;
    if (sound[1].loopmode==LOOPMODE_GATED) envelope1.noteOff(); // in trigger mode we just let the loop keep going
  }

  if (!digitalRead(GATE2)) { // gates are active low
    if (!trig2) {
      trigger2();
      trig2=true;
    }
  }
  else {
    trig2=false;
    if (sound[2].loopmode==LOOPMODE_GATED) envelope2.noteOff(); // in trigger mode we just let the loop keep going
  }

  if (!digitalRead(GATE3)) { // gates are active low
    if (!trig3) {
      trigger3();
      trig3=true;
    }
  }
  else {
    trig3=false;
    if (sound[3].loopmode==LOOPMODE_GATED) envelope3.noteOff(); // in trigger mode we just let the loop keep going
  }
}


// loadsamples - reload sample files
// reload all samples from beginning of PSRAM so memory isn't wasted. no memory manager!

void loadsamples(void) {
  display.clearDisplay();   // show a message here because it takes a second or 2 to load samples
  display.setCursor(2,30);
  display.println("loading samples...");
  display.display();
  newdigate::flashloader loader;      // new instance restarts loading at the start of external PSRAM
  sample0 = loader.loadSample(sound[0].filename);
  sample1 = loader.loadSample(sound[1].filename);
  sample2 = loader.loadSample(sound[2].filename);
  sample3 = loader.loadSample(sound[3].filename);
}

// file browser - select a file from SD and store its filename in the sound array

void selectfile(int s) {

  refreshMenu();
  moveSelector(0); // draws the selector
  P3Encoder.getButton();  // this eats any previous button presses 
  display.setCursor(0, selectedLine*PIXELSPERLINE);
  display.print(">" + cleanFileName(fileNames[firstOption + selectedLine]));
  
 while (true) {
  enc=P3Encoder.getValue(); // compiler bug - can't do this inside the if statement
  if (enc) {
    moveSelector(enc);
  }

  if (P3Encoder.getButton() == ClickEncoder::Clicked) {
    if (isdir[firstOption + selectedLine]) {  // open a subdirectory - this code only supports 1 level of nesting
      if (fileNames[firstOption + selectedLine] != "/") { // root dir is / or nothing, subdirectory is /subdir - SD library naming is always relative to root
        //workingdir[0]='/';            
        fileNames[firstOption + selectedLine].toCharArray(workingdir,13);
        root = SD.open(workingdir);
      }
      else {
       // strcpy(workingdir,"/");  // we can't go up one so just go back to root directory
        root = SD.open("/");
      }
      //   Serial.println(workingdir); 
      setFileCounts(root);
      //Serial.printf("filecount %d/n", fileCount); 
      firstOption=selectedLine=0;  // reset the menu positions
      refreshMenu();
      moveSelector(0); // draws the selector
    }
    else { // we are selecting a filename
      char temp[20];
      fileNames[firstOption + selectedLine].toCharArray(temp,13);
      strcpy(sound[s].filename,workingdir);
      strcat(sound[s].filename,"/");
      strcat(sound[s].filename,temp);
      Serial.println(sound[s].filename);
      return;
    }
   
  }

  if (P4Encoder.getButton() == ClickEncoder::Clicked) return; // abort file selection with main encoder
/*
    if (P3Encoder.getButton() == ClickEncoder::Pressed) { // long press also gets us back to root
        strcpy(workingdir,"/");  // we can't go up one so just go back to root directory
        root = SD.open("/");
              setFileCounts(root);
      //Serial.printf("filecount %d/n", fileCount); 
      firstOption=selectedLine=0;  // reset the menu positions
      refreshMenu();
      moveSelector(0); // draws the selector
    } */
 } 
}


// RH I snagged this code from https://github.com/matt-kendrick/arduino-file-browser. 
// it was pretty buggy and by the time I added directory navigation it got quite messy
// should probably have written it from scratch
// note that SD lib only supports 8.3 filenames and this browser only navigates root and 1 nested directory
// this is adequate for organizing samples and I only have 21 chars on the screen to display the full filename anyway

String cleanFileName(String fileName) {
  fileName.substring(0,15);
  fileName.replace(".TXT","");
  return fileName;
}

void moveSelector(int count) {
 
  display.setCursor(0,selectedLine*PIXELSPERLINE);
  // redraw the old selection without the selector
  if (isdir[firstOption + selectedLine]) display.println(" /" + fileNames[firstOption + selectedLine]);
  else display.println("  " + fileNames[firstOption + selectedLine]);

  if ((selectedLine + firstOption + count ) < fileCount) selectedLine +=count; // make sure we don't go off the end of the file list

  if(selectedLine >= linePerPage) {
    selectedLine = 0;
    if(firstOption + linePerPage >= fileCount) {
      firstOption = 0;
    } else {
      firstOption += linePerPage;
    }
    refreshMenu();
  }

  if(selectedLine < 0 ) {
    selectedLine = 0; 
    if(firstOption - linePerPage < 0) {
      firstOption = 0;
    } else {
      firstOption -= linePerPage;
    }
    refreshMenu();
  }
 // show the selection
  display.setCursor(0,selectedLine*PIXELSPERLINE);
  if (isdir[firstOption + selectedLine]) display.print(">/" +  fileNames[firstOption + selectedLine]);
  else display.print("> " + fileNames[firstOption + selectedLine]);
  display.display();
}

void refreshMenu() {
  int count = 0;
  
  display.clearDisplay();
  
  for(int i = firstOption; i < firstOption + linePerPage; i++) {
    display.setCursor(0,count*PIXELSPERLINE);
    if (i < fileCount) {
      if (isdir[i]) display.println(" /" + fileNames[i]);
      else display.println("  " + fileNames[i]);
    }
    count++;
  }
  display.display();
}

void setFileCounts(File dir) {
  fileCount = 0;
  
  while(true) {
    File entry = dir.openNextFile();
    if(!entry) {
      dir.rewindDirectory();
      fileNames[fileCount] = "/"; // add name of root directory so we can navigate back
      isdir[fileCount]=true;
      fileCount++;
      break;
    } else {
      if(!entry.isDirectory()) {
        fileNames[fileCount] = entry.name();
        isdir[fileCount]=false;  // tag it as a file ie not a directory
        fileCount++;     
      }
      else {
        fileNames[fileCount] = entry.name();
        isdir[fileCount]=true; // tag it as a directory
        fileCount++;
      }
    }
  }
}




