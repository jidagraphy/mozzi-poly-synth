
/*
 *  Poly Synth for Arduino and Mozzi *
 *  @jidagraphy
 */

#include <MIDI.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <mozzi_midi.h>
#include <ADSR.h>
#include <tables/saw512_int8.h>

#define CONTROL_RATE 256

//set maximum number of polyphony
#define MAX_POLY 6
#define OSC_NUM_CELLS 512
#define WAVE_DATA SAW512_DATA

//Envelope controllers
#define ATTACK 22
#define DECAY 3000
#define SUSTAIN 8000
#define RELEASE 300
#define ATTACK_LEVEL 64
#define DECAY_LEVEL 0

//Voices
struct Voice{
  Oscil<OSC_NUM_CELLS, AUDIO_RATE> osc;  // audio oscillator
  ADSR<CONTROL_RATE, AUDIO_RATE> env;  // envelope generator
  byte note = 0;
  byte velocity = 0;
};

Voice voices[MAX_POLY];

//optional midi monitor
#define LED 13


void HandleNoteOn(byte channel, byte note, byte velocity) {
  if (velocity > 0) {

    int activeVoice = 0;
    int voiceToSteal = 0;
    int lowestVelocity = 128;

    for (unsigned int i = 0 ; i < MAX_POLY; i++) {
      if(!voices[i].env.playing()){
        voices[i].env.noteOff();
        voices[i].osc.setFreq(mtof(float(note)));
        voices[i].env.noteOn();
        voices[i].note = note;
        voices[i].velocity = velocity;
        break;
      }else{
        activeVoice++;
        if(lowestVelocity >= voices[i].velocity){
          lowestVelocity = voices[i].velocity;
          voiceToSteal = i;
        }
      }
    }

    if(activeVoice == MAX_POLY){
        voices[voiceToSteal].env.noteOff();
        voices[voiceToSteal].osc.setFreq(mtof(float(note)));
        voices[voiceToSteal].env.noteOn();
        voices[voiceToSteal].note = note;
        voices[voiceToSteal].velocity = velocity;
    }
    digitalWrite(LED, HIGH);

  } else {
    usbMIDI.sendNoteOff(note, velocity, channel);
  }
}


void HandleNoteOff(byte channel, byte note, byte velocity) {
  byte handsOffChecker = 0;
  for (unsigned int i = 0; i < MAX_POLY; i++) {
    if (note == voices[i].note) {
      voices[i].env.noteOff();
      voices[i].note = 0;
      voices[i].velocity = 0;
    }
    handsOffChecker += voices[i].note;
  }

  if (handsOffChecker == 0) {
    digitalWrite(LED, LOW);
  }
}

void HandleControlChange(byte channel, byte control, byte value) {

}

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  pinMode(LED, OUTPUT);

  usbMIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  usbMIDI.setHandleNoteOff(HandleNoteOff);  // Put only the name of the function
  // Initiate MIDI communications, listen to all channels (not needed with Teensy usbMIDI)
  // usbMIDI.begin(MIDI_CHANNEL_OMNI);

  //to use it with Hairless-midiserial, uncomment below
  // Serial.begin(115200);

  for(unsigned int i = 0; i < MAX_POLY; i++){
    voices[i].env.setADLevels(ATTACK_LEVEL,DECAY_LEVEL);
    voices[i].env.setTimes(ATTACK,DECAY,SUSTAIN,RELEASE);
    voices[i].osc.setTable(WAVE_DATA);
  }

  //aSin0.setFreq(440); // default frequency
  startMozzi(CONTROL_RATE);
}


void updateControl(){
  usbMIDI.read();
  for(unsigned int i = 0; i < MAX_POLY; i++){
    voices[i].env.update();
  }
}


int updateAudio(){
  int currentSample = 0;

  for(unsigned int i = 0; i < MAX_POLY; i++){
    currentSample += voices[i].env.next() * voices[i].osc.next();
  }
  return (int) (currentSample)>>8;
}


void loop() {
  audioHook();
}
