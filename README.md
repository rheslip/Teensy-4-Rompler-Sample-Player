# Teensy 4 Rompler/Sample Player
 Loads up to 4 .raw files into PSRAM and plays them via MIDI or CV and Gate inputs
 
 Code was written for my "TEENSY DSP" eurorack module which has a Teensy 4, audio shield, 4 CV inputs, 4 trigger inputs, a small OLED display, 5 parameter encoders, MIDI in and MIDI out.
 
 It uses the excellent variable playback library by Nic Newdigate https://github.com/newdigate. It also uses a slightly modified version of the Clickencoder library which is included.
 
 The menu system implements a simple FAT file browser and menus to load, configure and play back up to four samples at a time. Files must be 44100hz sample rate, mono, 16 bit signed PCM .raw file format. The SD card in the Teensy 4's card slot can have directories to help organize your samples but directories cannot be nested. .RAW files are loaded into PSRAM which must be added to the Teensy 4 board. I have two 8mb chips on mine.
 
 The menu system is set up for horizontal scrolling. The main encoder selects the top level menus and the four parameter encoders allow submenu values to be set. Press and hold one of the parameter encoders to scroll through the submenus. Turning encoder #3 enters the file browser and pressing the button selects and loads a sample file. Press the main encoder button to exit the file browser without loading a file.
 
 Main menus are for setting up each of the four samples and four ADSR envelopes for each sample. Submenus allow sample playback speed from -2x (reverse) to 2x, transpose by semitones, fine pitch adjustment, MIDI channel, MIDI note # of sample (e.g. if the sample is pitched at A4 set the note to 69), triggered onshot samples, triggered loops, gated loops and selection of loop start and end points.

Samples can be played via trigger/gate and CV or MIDI. CV sets sample playback speed but not at 1v/octave (this is planned). Its important to set the original pitch of the sample so MIDI notes will play back at the correct pitch. Samples are resampled to get different pitches - no time stretching or compression is done so pitching up results in shorter note durations.


Note that when you change samples it will reload all four sample selections into PSRAM. This is to avoid fragmenting PSRAM and having to do complex memory management.

The code works very well and its a ton of fun but it still has a few glitches. It crashed a couple of times I think when loading samples larger than PSRAM. I plan to add CV control of sample start and end points and 1v/octave.


 
