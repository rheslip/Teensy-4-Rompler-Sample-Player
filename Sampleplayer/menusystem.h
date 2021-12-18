
// Copyright 2020 Rich Heslip
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
// this is adapted from my XVA1 menus system. this one is bitmapped so its a mix of character and pixel addressing
// implements a two level horizontal scrolling menu system
// main encoder selects top level menu by scrolling left-right thru the top menus
// 4 parameter encoders modify values in the submenus
// press and hold parameter or main encoder to scroll submenus left-Right
// once you get the idea its very fast to navigate

#define DISPLAY_X 20  // 20 char display
#define DISPLAY_Y 8   // 8 lines
#define DISPLAY_CHAR_HEIGHT 8 // character height in pixels - for bitmap displays
#define DISPLAY_CHAR_WIDTH 6 // character width in pixels - for bitmap displays
#define DISPLAY_X_MENUPAD 2   // extra space between menu items
#define TOPMENU_Y 0   // line to display top menus
#define MSG_Y   30   // line to display messages
#define FILENAME_Y 15 // line to display filename
#define SUBMENU_FIELDS 4 // max number of subparameters on a line ie for 20 char display each field has 5 characters
#define SUBMENU_Y 46 // y pos to display submenus (parameter names)
#define SUBMENU_VALUE_Y 56 // y pos to display parameter values
uint8_t submenu_X[]= {0,32,64,96};  // location of the submenus by pixel

enum paramtype{TYPE_NONE,TYPE_INTEGER,TYPE_FLOAT, TYPE_TEXT}; // parameter display types

// submenus 
struct submenu {
  char *name; // display short name
  char *longname; // longer name displays on message line
  int16_t min;  // min value of parameter
  int16_t max;  // max value of parameter
  int16_t step; // step size. if 0, don't print ie spacer
  enum paramtype ptype; // how its displayed
  char ** ptext;   // points to array of text for text display
  int16_t *parameter; // value to modify
  void (*handler)(void);  // function to call on value change
};

// top menus
struct menu {
   char *name; // menu text
   struct submenu * submenus; // points to submenus for this menu
   int8_t submenuindex;   // stores the index of the submenu we are currently using
   int8_t numsubmenus; // number of submenus - not sure why this has to be int but it crashes otherwise. compiler bug?
};

// timer and flag for managing temporary messages
#define MESSAGE_TIMEOUT 1500
long messagetimer;
bool message_displayed;

int16_t enab1 =1;
int16_t enab2 =2;
int16_t level1,pan1;

int16_t nul;    // dummy parameter and function for testing
void dummy( void) {}

// ********** menu structs that build the menu system below *********

// text arrays used for submenu TYPE_TEXT fields
char * textoffon[] = {" OFF", "  ON"};
char * textloopmode[] = {" OFF", "TRIG","GATE"};
char * textwaves[] = {" SIN", " TRI"," SAW","RAMP"," SQR","PTRI","PSAW","PSQR"};

struct submenu sample0params[] = {
  // name,longname,min,max,step,type,*textfield,*parameter,*handler
  "TPOS","Transpose",-24,24,1,TYPE_INTEGER,0,&sound[0].transpose,0,
  "SPD","Playback Speed",-2000,2000,1,TYPE_FLOAT,0,&sound[0].speed,0,
  "MIX","Mixer Level",0,1000,10,TYPE_FLOAT,0,&sound[0].mixlevel,&setmixerlevels,
  "FILE","",0,0,0,TYPE_INTEGER,0,&nul,&selectfile0,
  "LOOP","Loop Mode",0,2,1,TYPE_TEXT,textloopmode,&sound[0].loopmode,&setloopmodes,
  "STRT","Start",0,9999,1,TYPE_INTEGER,0,&sound[0].loopstart,0,
  " END","End",0,9999,1,TYPE_INTEGER,0,&sound[0].loopend,0,
  "    ","",0,0,0,TYPE_INTEGER,0,&nul,0,
  "CHAN","MIDI Channel",1,16,1,TYPE_INTEGER,0,&sound[0].midichannel,0,
  "NOTE","MIDI Note of Sample",0,127,1,TYPE_INTEGER,0,&sound[0].note,0,
};

struct submenu sample1params[] = {
  // name,longname,min,max,step,type,*textfield,*parameter,*handler
  "TPOS","Transpose",-24,24,1,TYPE_INTEGER,0,&sound[1].transpose,&dummy,
  "SPD","Playback Speed",-2000,2000,1,TYPE_FLOAT,0,&sound[1].speed,&dummy,
  "MIX","Mixer Level",0,1000,10,TYPE_FLOAT,0,&sound[1].mixlevel,&setmixerlevels,
  "FILE","",0,0,0,TYPE_INTEGER,0,&nul,&selectfile1,
  "LOOP","Loop Mode",0,2,1,TYPE_TEXT,textloopmode,&sound[1].loopmode,&setloopmodes,
  "STRT","Start",0,9999,1,TYPE_INTEGER,0,&sound[1].loopstart,0,
  " END","End",0,9999,1,TYPE_INTEGER,0,&sound[1].loopend,0,
  "    ","",0,0,0,TYPE_INTEGER,0,&nul,0,
  "CHAN","MIDI Channel",1,16,1,TYPE_INTEGER,0,&sound[1].midichannel,0,
  "NOTE","MIDI Note of Sample",0,127,1,TYPE_INTEGER,0,&sound[1].note,0,

};

struct submenu sample2params[] = {
  // name,longname,min,max,step,type,*textfield,*parameter,*handler
  "TPOS","Transpose",-24,24,1,TYPE_INTEGER,0,&sound[2].transpose,&dummy,
  "SPD","Playback Speed",-2000,2000,1,TYPE_FLOAT,0,&sound[2].speed,&dummy,
  "MIX","Mixer Level",0,1000,10,TYPE_FLOAT,0,&sound[2].mixlevel,&setmixerlevels,
  "FILE","",0,0,0,TYPE_INTEGER,0,&nul,&selectfile2,
  "LOOP","Loop Mode",0,2,1,TYPE_TEXT,textloopmode,&sound[2].loopmode,&setloopmodes,
  "STRT","Start",0,9999,1,TYPE_INTEGER,0,&sound[2].loopstart,0,
  " END","End",0,9999,1,TYPE_INTEGER,0,&sound[2].loopend,0,
  "    ","",0,0,0,TYPE_INTEGER,0,&nul,0,
  "CHAN","MIDI Channel",1,16,1,TYPE_INTEGER,0,&sound[2].midichannel,0,
  "NOTE","MIDI Note of Sample",0,127,1,TYPE_INTEGER,0,&sound[2].note,0,
};

struct submenu sample3params[] = {
  // name,longname,min,max,step,type,*textfield,*parameter,*handler
  "TPOS","Transpose",-24,24,1,TYPE_INTEGER,0,&sound[3].transpose,&dummy,
  "SPD","Playback Speed",-2000,2000,1,TYPE_FLOAT,0,&sound[3].speed,&dummy,
  "MIX","Mixer Level",0,1000,10,TYPE_FLOAT,0,&sound[3].mixlevel,&setmixerlevels,
  "FILE","",0,0,0,TYPE_INTEGER,0,&nul,&selectfile3,
  "LOOP","Loop Mode",0,2,1,TYPE_TEXT,textloopmode,&sound[3].loopmode,&setloopmodes,
  "STRT","Start",0,9999,1,TYPE_INTEGER,0,&sound[3].loopstart,0,
  " END","End",0,9999,1,TYPE_INTEGER,0,&sound[3].loopend,0,
  "    ","",0,0,0,TYPE_INTEGER,0,&nul,0,
  "CHAN","MIDI Channel",1,16,1,TYPE_INTEGER,0,&sound[3].midichannel,0,
  "NOTE","MIDI Note of Sample",0,127,1,TYPE_INTEGER,0,&sound[3].note,0,
};


struct submenu env0params[] = {
  // name,longname,min,max,step,type,*textfield,*parameter,*handler
  "ATAK","Attack Time MS",0,9999,1,TYPE_INTEGER,0,&sound[0].attack,&setenvelopes,
  "DCAY","Decay Time MS",0,9999,1,TYPE_INTEGER,0,&sound[0].decay,&setenvelopes,
  "SUST","Sustain Level",0,1000,10,TYPE_FLOAT,0,&sound[0].sustain,&setenvelopes,
  "RELS","Release Time MS",0,9999,1,TYPE_INTEGER,0,&sound[0].release,&setenvelopes,
  "DELY","Delay Time MS",0,9999,1,TYPE_INTEGER,0,&sound[0].delay,&setenvelopes,
  "HOLD","HOLD Time MS",0,9999,1,TYPE_INTEGER,0,&sound[0].hold,&setenvelopes,
};

struct submenu env1params[] = {
  // name,longname,min,max,step,type,*textfield,*parameter,*handler
  "ATAK","Attack Time MS",0,9999,1,TYPE_INTEGER,0,&sound[1].attack,&setenvelopes,
  "DCAY","Decay Time MS",0,9999,1,TYPE_INTEGER,0,&sound[1].decay,&setenvelopes,
  "SUST","Sustain Level",0,1000,10,TYPE_FLOAT,0,&sound[1].sustain,&setenvelopes,
  "RELS","Release Time MS",0,9999,1,TYPE_INTEGER,0,&sound[1].release,&setenvelopes,
  "DELY","Delay Time MS",0,9999,1,TYPE_INTEGER,0,&sound[1].delay,&setenvelopes,
  "HOLD","HOLD Time MS",0,9999,1,TYPE_INTEGER,0,&sound[1].hold,&setenvelopes,
};

struct submenu env2params[] = {
  // name,longname,min,max,step,type,*textfield,*parameter,*handler
  "ATAK","Attack Time MS",0,9999,1,TYPE_INTEGER,0,&sound[2].attack,&setenvelopes,
  "DCAY","Decay Time MS",0,9999,1,TYPE_INTEGER,0,&sound[2].decay,&setenvelopes,
  "SUST","Sustain Level",0,1000,10,TYPE_FLOAT,0,&sound[2].sustain,&setenvelopes,
  "RELS","Release Time MS",0,9999,1,TYPE_INTEGER,0,&sound[2].release,&setenvelopes,
  "DELY","Delay Time MS",0,9999,1,TYPE_INTEGER,0,&sound[2].delay,&setenvelopes,
  "HOLD","HOLD Time MS",0,9999,1,TYPE_INTEGER,0,&sound[2].hold,&setenvelopes,
};

struct submenu env3params[] = {
  // name,longname,min,max,step,type,*textfield,*parameter,*handler
  "ATAK","Attack Time MS",0,9999,1,TYPE_INTEGER,0,&sound[3].attack,&setenvelopes,
  "DCAY","Decay Time MS",0,9999,1,TYPE_INTEGER,0,&sound[3].decay,&setenvelopes,
  "SUST","Sustain Level",0,1000,10,TYPE_FLOAT,0,&sound[3].sustain,&setenvelopes,
  "RELS","Release Time MS",0,9999,1,TYPE_INTEGER,0,&sound[3].release,&setenvelopes,
  "DELY","Delay Time MS",0,9999,1,TYPE_INTEGER,0,&sound[3].delay,&setenvelopes,
  "HOLD","HOLD Time MS",0,9999,1,TYPE_INTEGER,0,&sound[3].hold,&setenvelopes,
};

// top level menu structure - each top level menu contains one submenu
struct menu mainmenu[] = {
  // name,submenu *,initial submenu index,number of submenus
  "Sample 0",sample0params,0,sizeof(sample0params)/sizeof(submenu),
  "Envelope 0",env0params,0,sizeof(env0params)/sizeof(submenu),
  "Sample 1",sample1params,0,sizeof(sample1params)/sizeof(submenu),
  "Envelope 1",env1params,0,sizeof(env1params)/sizeof(submenu),
  "Sample 2",sample2params,0,sizeof(sample2params)/sizeof(submenu),
  "Envelope 2",env2params,0,sizeof(env2params)/sizeof(submenu),
  "Sample 3",sample3params,0,sizeof(sample3params)/sizeof(submenu),
  "Envelope 3",env3params,0,sizeof(env3params)/sizeof(submenu),
};

#define NUM_MAIN_MENUS sizeof(mainmenu)/ sizeof(menu)
menu * topmenu=mainmenu;  // points at current menu
int8_t topmenuindex=0;  // keeps track of which top menu item we are displaying

// ******* menu handling code ************

// display the top menu
void drawtopmenu( int8_t index) {
    display.setCursor ( 0, TOPMENU_Y ); 
    display.print("                    "); // kludgy line erase
    display.setCursor ( 0, TOPMENU_Y ); 
    display.print(topmenu[index].name);
    display.setCursor ( 0, FILENAME_Y );  // custom for this app - show the filename for the sample
    if (index < 4) display.print(sound[index].filename);
    display.display();
}

// display a sub menu item and its value
// index is the index into the current top menu's submenu array
// pos is the relative x location on the screen ie field 0,1,2 or 3 
void drawsubmenu( int8_t index, int8_t pos) {
    submenu * sub;
    // print the name text
    //display.setCursor ((DISPLAY_X/SUBMENU_FIELDS)*pos*DISPLAY_CHAR_WIDTH+DISPLAY_X_MENUPAD, SUBMENU_Y ); // set cursor to parameter name field - staggered short names
    display.setCursor (submenu_X[pos], SUBMENU_Y); // set cursor to parameter name field - staggered long names
    sub=topmenu[topmenuindex].submenus; //get pointer to the submenu array
    if (index < topmenu[topmenuindex].numsubmenus) display.print(sub[index].name); // make sure we aren't beyond the last parameter in this submenu
    else display.print("     ");
    
    // print the value
    display.setCursor (submenu_X[pos], SUBMENU_VALUE_Y ); // set cursor to parameter value field
    display.print("     "); // erase old value
    display.setCursor (submenu_X[pos], SUBMENU_VALUE_Y ); // set cursor to parameter value field
    if ((sub[index].step !=0) && (index < topmenu[topmenuindex].numsubmenus)) { // don't print dummy parameter or beyond the last submenu item
      int16_t val=*sub[index].parameter;  // fetch the parameter value   // 
      //if (val> sub[index].max) *sub[index].parameter=val=sub[index].max; // check the parameter range and limit if its out of range ie we loaded a bad patch
     // if (val< sub[index].min) *sub[index].parameter=val=sub[index].min; // check the parameter range and limit if its out of range ie we loaded a bad patch
      char temp[5];
      switch (sub[index].ptype) {
        case TYPE_INTEGER:   // print the value as an unsigned integer          
          sprintf(temp,"%4d",val); // lcd.print doesn't seem to print uint8 properly
          display.print(temp);  
          display.print(" ");  // blank out any garbage
          break;
        case TYPE_FLOAT:   // print the int value as a float 
          sprintf(temp,"%1.2f",(float)val/1000); // menu should have int value between -9999 +9999 so float is -9.99 to +9.99
          display.print(temp);  
          display.print(" ");  // blank out any garbage
          break;
        case TYPE_TEXT:  // use the value to look up a string
          if (val > sub[index].max) val=sub[index].max; // sanity check
          if (val < 0) val=0; // min index is 0 for text fields
          display.print(sub[index].ptext[val]); // parameter value indexes into the string array
          display.print(" ");  // blank out any garbage
          break;
        default:
        case TYPE_NONE:  // blank out the field
          display.print("     ");
          break;
      } 
    }

    display.display(); 
}

// display the sub menus of the current top menu

void drawsubmenus() {
  int8_t index = topmenu[topmenuindex].submenuindex;
  for (int8_t i=0; i< SUBMENU_FIELDS; ++i) drawsubmenu(index++,i);
}

//adjust the topmenu index and update the menus and submenus
// dir - int value to add to the current top menu index ie L-R scroll
void scrollmenus(int8_t dir) {
  topmenuindex+= dir;
  if (topmenuindex < 0) topmenuindex = NUM_MAIN_MENUS -1; // handle wrap around
  if (topmenuindex >= NUM_MAIN_MENUS ) topmenuindex = 0; // handle wrap around
  display.clearDisplay();
  drawtopmenu(topmenuindex);
  drawsubmenus();    
}

// same as above but scrolls submenus
void scrollsubmenus(int8_t dir) {
  if (dir !=0) { // don't redraw if there is no change
    dir= dir*SUBMENU_FIELDS; // sidescroll SUBMENU_FIELDS at a time
    topmenu[topmenuindex].submenuindex+= dir;
    if (topmenu[topmenuindex].submenuindex < 0) topmenu[topmenuindex].submenuindex = 0; // stop at first submenu
    if (topmenu[topmenuindex].submenuindex >= topmenu[topmenuindex].numsubmenus ) topmenu[topmenuindex].submenuindex -=dir; // stop at last submenu     
    display.clearDisplay();  // for now, redraw everything
    drawtopmenu(topmenuindex);
    drawsubmenus(); 
  }     
}

// show a message on 2nd line of display - it gets auto erased after a timeout
void showmessage(char * message) {
  display.setCursor(0, MSG_Y); 
  display.print(message);
  messagetimer=millis();
  message_displayed=true;
}

// clear the message on the message line
void erasemessage(void) {
    display.setCursor(0, MSG_Y); 
    display.print("                    "); 
    message_displayed=false; 
}

void domenus(void) {
  int16_t enc;
  int8_t index; 
  int16_t encodervalue[4]; 
  ClickEncoder::Button button; 
  // process the menu encoder - scroll submenus, scroll main menu when button down
  enc=P4Encoder.getValue(); // compiler bug - can't do this inside the if statement
  if (digitalRead(P4_SW) == 1)  {  // side scroll menus  - there is a bit of a delay detecting button state so read button directly
    if ( enc != 0) {
      scrollmenus(enc);
    }
  }
  else {  // we are scrolling submenus
    if ( enc != 0) {
        scrollsubmenus(enc);           
    }
  }

// process parameter encoder buttons - button gestures are used as shortcuts/alternatives to using main encoder
// hold button and rotate to scroll submenus 

  if (digitalRead(P0_SW) == 0) scrollsubmenus(P0Encoder.getValue());  // if button pressed, side scroll submenus
  if (digitalRead(P1_SW) == 0) scrollsubmenus(P1Encoder.getValue());
  if (digitalRead(P2_SW) == 0) scrollsubmenus(P2Encoder.getValue());
  if (digitalRead(P3_SW) == 0) scrollsubmenus(P3Encoder.getValue());
    
  index= topmenu[topmenuindex].submenuindex; // submenu field index
  submenu * sub=topmenu[topmenuindex].submenus; //get pointer to the current submenu array
  button= P0Encoder.getButton();

  
 // process parameter encoders
  encodervalue[0]=P0Encoder.getValue();  // read encoders
  encodervalue[1]=P1Encoder.getValue();
  encodervalue[2]=P2Encoder.getValue();
  encodervalue[3]=P3Encoder.getValue();

  for (int field=0; field<4;++field) { // loop thru the on screen submenus
    if (encodervalue[field]!=0) {  // if there is some input, process it
      int16_t temp=*sub[index].parameter + encodervalue[field]*sub[index].step; // menu code uses ints - convert to floats when needed
      if (temp < (int16_t)sub[index].min) temp=sub[index].min;
      if (temp > (int16_t)sub[index].max) temp=sub[index].max;
      *sub[index].parameter=temp;
      if (sub[index].handler != 0) (*sub[index].handler)();  // call the handler function
      erasemessage(); // undraw old longname
      showmessage(sub[index].longname);  // show the long name of what we are editing
      drawsubmenu(index,field);
    }
    ++index;
    if (index >= topmenu[topmenuindex].numsubmenus) break; // check that we have not run out of submenus
  }
}


