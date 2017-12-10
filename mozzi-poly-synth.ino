
/*
 *  Poly Synth for Arduino and Mozzi
 *  
 *  A simple mozzi synthesiser that supports polyphony.
 *  
 *  When limited by the maximum number of polyphony, the algorithm
 *  cuts the oldest sustained note and replaces it with a newly played
 *  note. The algorithm also prioritises bass notes to not be cut from
 *  polyphony.

 *  Runs with Arduino MIDI Library.
 *  Added on top of Mozzi Exmple Mozzi_MIDI_Input.
 *  
 *  I've sent midi notes through the serial using hairless-midiserial tool.
 *  To use this in a normal way, delete the line where the serial begins
 *  on the port 115200.
 *  
 *  @jidagraphy
 */
 
#include <MIDI.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <mozzi_midi.h>
#include <ADSR.h>
#include <tables/triangle_analogue512_int8.h>

#define CONTROL_RATE 256

//set maximum number of polyphony
#define MAX_POLY 4
#define OSC_NUM_CELLS 512

//Envelope controllers
#define ATTACK 22
#define DECAY 3000
#define SUSTAIN 8000
#define RELEASE 300
#define ATTACK_LEVEL 64
#define DECAY_LEVEL 2

// audio oscillator
Oscil<OSC_NUM_CELLS, AUDIO_RATE> osc[MAX_POLY];

// envelope generator
ADSR<CONTROL_RATE, AUDIO_RATE> env[MAX_POLY];

//polyphonic notes
byte poly[MAX_POLY] = {};

//order of polyphony
int pressed = 0;
int orderArray[MAX_POLY] = {};
int order = 0; //0 not pressed, order goes from 1 to MAX + 1
byte bassFinger = 0;

//optional midi monitor
#define LED 13


void HandleNoteOn(byte channel, byte note, byte velocity) { 
  if(pressed < MAX_POLY){
    for(unsigned int i = 0; i < MAX_POLY; i++){
      if(poly[i] == 0){
        osc[i].setFreq(mtof(float(note)));
        env[i].noteOn();
        poly[i] = note;
        order ++;
        orderArray[i] = order;
        if(poly[bassFinger] == 0 || note < poly[bassFinger]){
          bassFinger = i;
        }
        break;
      }
    }
  }else{
    //bass sustainer - lowest note does not get effected by poly
    //limit, unless the new note is lower than the lowest.
    if(note < poly[bassFinger]){
      //if lower, then reassign bass.
      env[bassFinger].noteOff();
      osc[bassFinger].setFreq(mtof(float(note)));
      env[bassFinger].noteOn();
      poly[bassFinger] = note;
    }else{
      int lowest = order;
      int oldestFinger = 0;
      
      for(unsigned int i = 0; i < MAX_POLY; i++){
        if(orderArray[i] <= lowest && i != bassFinger){
          lowest = orderArray[i];
          oldestFinger = i;
        }
      }
      env[oldestFinger].noteOff();
      osc[oldestFinger].setFreq(mtof(float(note)));
      env[oldestFinger].noteOn();
      poly[oldestFinger] = note;
      order++;
      orderArray[oldestFinger] = order;
    }
  }
  pressed++;
  digitalWrite(LED, HIGH);
}


void HandleNoteOff(byte channel, byte note, byte velocity) {
  byte handsOffChecker = 0;
  
  for(unsigned int i = 0; i < MAX_POLY; i++){
    if(note == poly[i]){
      env[i].noteOff();
      poly[i] = 0;
      
      if(i == bassFinger){
        //if released note is the bass, find the next bass and reassign
        byte lowest = poly[0];

        for(unsigned int j = 0; j < MAX_POLY; j++){
          if((lowest == 0) || (poly[j] != 0 && poly[j] <= lowest)){
            lowest = poly[j];
            bassFinger = j;
          }
        }
      }
      orderArray[i] = 0;
    }
    handsOffChecker += poly[i];
  }

//reset order counter if hands are off
  if(handsOffChecker == 0){
    order = 0;
    bassFinger = 0;
    digitalWrite(LED, LOW);
  }
  pressed--;
}

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  pinMode(LED, OUTPUT);  

  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  // Initiate MIDI communications, listen to all channels (not needed with Teensy usbMIDI)
  MIDI.begin(MIDI_CHANNEL_OMNI);  

  //to use it with Hairless-midiserial
  Serial.begin(115200);

  for(unsigned int i = 0; i < MAX_POLY; i++){
    env[i].setADLevels(ATTACK_LEVEL,DECAY_LEVEL);
    env[i].setTimes(ATTACK,DECAY,SUSTAIN,RELEASE);
    osc[i].setTable(TRIANGLE_ANALOGUE512_DATA);
  }
    
  //aSin0.setFreq(440); // default frequency
  startMozzi(CONTROL_RATE); 
}


void updateControl(){
  MIDI.read();
  for(unsigned int i = 0; i < MAX_POLY; i++){
    env[i].update();
  }
}


int updateAudio(){
  int currentSample = 0;
  
  for(unsigned int i = 0; i < MAX_POLY; i++){
    currentSample += env[i].next() * osc[i].next();
  }
  return (int) (currentSample)>>8;
}


void loop() {
  audioHook();
} 


