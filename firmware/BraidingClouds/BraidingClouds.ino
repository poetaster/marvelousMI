/*
  (c) 2024 blueprint@poetaster.de
  GPLv3

      Some sources, including the stmlib, rings, braids and plaits libs are
      MIT License Copyright (c)  2020 (emilie.o.gillet@gmail.com)
*/

bool debug = false;

#include <Arduino.h>
#include <math.h>
#include "stdio.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include <hardware/pwm.h>
#include <EEPROM.h>

#include <iostream>
#include <vector>
#include <algorithm>

#include <SoftwareSerial.h>

#include <MIDI.h>

//MIDI_CREATE_DEFAULT_INSTANCE();
// this method permits us to use arbitrary pins
// in pico land this is also a PIO statemachine (uart)

using Transport = MIDI_NAMESPACE::SerialMIDI<SoftwareSerial>;
int rxPin = 1;
int txPin = 3;
SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin); //thruActivated
Transport serialMIDI(mySerial);
MIDI_NAMESPACE::MidiInterface<Transport> MIDI((Transport&)serialMIDI);


// start with CV input, switch on midi in or setting.
bool midi_switch = false;
bool midi_switch_setting = false;

#include <I2S.h>
#define SAMPLERATE 32000

#define pBCLK 8
#define pWS (pBCLK+1)
#define pDOUT 10
I2S DAC(OUTPUT, pBCLK, pDOUT);

// create ADSR env
#include "ADSR.h"
ADSR *env = new ADSR();
float envAttack;
float envDecay;
int envRelease;
float envSustain;
long envTimer = 0;



// utility
double randomDouble( double minf, double maxf) {
  return minf + random(1UL << 31) * (maxf - minf) / (1UL << 31);  // use 1ULL<<63 for max double values)
}

double median(std::vector<int>& numbers) {
  std::sort(numbers.begin(), numbers.end());
  int n = numbers.size();
  return n % 2 == 0 ? (numbers[n / 2 - 1] + numbers[n / 2]) / 2.0 : numbers[n / 2];
}

float mapf(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
  float result;
  result = (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
  return result;
}


// volts to octave for 3.3 volts
// based on https://little-scale.blogspot.com/2018/05/pitch-cv-to-frequency-conversion-via.html
float data;
float pitch;
float pitch_offset = 0; // 36 in mmm 10volt input
float freq;

float max_voltage_of_adc = 3.3;
float voltage_division_ratio = 0.3333333333333;
float notes_per_octave = 12;
float volts_per_octave = 0.54;
float mapping_upper_limit = 4090; //60.0f; //(max_voltage_of_adc / voltage_division_ratio) * notes_per_octave * volts_per_octave;
float mapping_lower_limit = 5; //36.0f;


// trigger is on gp0

// cv input
#define CV1 (42u)
#define CV2 (43u)
#define CV3 (44u)
#define CV4 (45u)
#define CV5 (46u)
#define CV6 (47u)

int cv_ins[6] = {CV1, CV2, CV3, CV4, CV5, CV6};
int cv_avg = 15;

// buffer for input to rings exciter
float CV1_buffer[32];

// button inputs
#define SW1 34
#define SW2 35
#define SW3 38
#define SW4 2

#include <Bounce2.h>
Bounce2::Button btn_one = Bounce2::Button();
Bounce2::Button btn_two = Bounce2::Button();
Bounce2::Button btn_four = Bounce2::Button();
Bounce2::Button btn_three = Bounce2::Button();

int btn_three_state = 0;
int btn_two_state = 0;

// Generic pin state variable
byte pinState;

// common output buffers
int16_t out_bufferL[32];
int16_t out_bufferR[32];

// sample buffer for clouds
int16_t sample_buffer[32]; // used while we play samples for demo

// voice params
int voice_number = 0; // for switching  between modules

// we are reusing the plaits nomenclature for all modules
// Plaits modulation vars
float morph_in = 0.5f; // IN(4);
float trigger_in = 0.0f; //IN(5);
float level_in = 0.0f; //IN(6);
float harm_in = 0.3f;
float timbre_in = 0.3f;
float position_in = 0.5f;
float pos_mod = 0.0f;

int engine_in;
char engine_name;
int bpm;
int selected_preset;

// persist plaits 
float plaits_morph = morph_in;
float plaits_harm = harm_in;
float plaits_timbre = timbre_in;
float plaits_position = position_in;
float plaits_level = 1.0f;
int   plaits_engine = 0;
// persist Rings 
float rings_position = 0.25f; // position
float rings_morph = morph_in;
float rings_harm = harm_in;
float rings_timbre = timbre_in;
float rings_level = 1.0f;
int   rings_engine = 0;
// persist braids
float braids_timbre = timbre_in;
float braids_morph = morph_in;
float braids_harm = harm_in;
float braids_position = position_in;
int   braids_engine = 0;
float braids_level = 1.0f;
// persist global
float fm_mod = 0.0f ; //IN(7);
float timb_mod = 0.0f; //IN(8);
float morph_mod = 0.0f; //IN(9);
float decay_in = 0.5f; // IN(10);
float lpg_in = 0.2f ;// IN(11);
float pitch_in = 32.0f;
int max_engines = 21; // varies per backend
// persist clouds
float clouds_morph = morph_in;
float clouds_timbre = timbre_in;
float clouds_harm = harm_in;
float clouds_position = position_in;
float clouds_mode = engine_in;
float clouds_dw_in = 1.0f;
float clouds_reverb = 0.3f;
float clouds_spread = 0.5f;
float clouds_fb = 0.3f;
float clouds_pos_in = 0.0f;
int   clouds_engine = 0;
float clouds_level = 1.0f;
bool  freeze_in = false;
int   voice_in = 4;

bool trigger_on = false;
bool easterEgg = false;



#include <STMLIB.h> // 
//#include <RINGS.h>
//#include "rings.h"

//#include <PLAITS.h>
//#include "plaits.h"

#include <BRAIDS.h>
#include "braids.h"

// clouds dsp not used pushes ram
#include <CLOUDS.h>
#include "clouds.h"

#include "Midier.h"
// midi related functions
#include "midi.h"

#include "names.h"

int device_initialized = 0 ; // used to identify if the eeprom has been written.
#include "eeprom.h"

bool writing = false;
bool reading = false;

const int encoderSW_pin = 28;

// encoder related // 2,3 8,9


// ugly, but using both ranges does not work
// this depends on poetasters version of the arduino library
#include "pio_encoder.h"
PioEncoder enc1(39, PIO pio1);
PioEncoder enc2(32, PIO pio1);
PioEncoder enc3(36, PIO pio1);

// PioEncoder enc4(6, PIO pio0);
// sadly we need a second approach.

#include <RotaryEncoder.h>
// Setup a RotaryEncoder without pio/ the meta: 6, 7 usually
RotaryEncoder enc4( 6,  7,  RotaryEncoder::LatchMode::FOUR3);

int enc1_pos_last = 0;
int enc1_delta = 0;
int enc2_pos_last = 0;
int enc2_delta = 0;
int enc3_pos_last = 0;
int enc3_delta = 0;
int enc4_pos_last = 0;
int enc4_delta = 0;


uint32_t enc1_push_millis;
uint32_t step_push_millis;
bool encoder_held = false;


// display related
const int oled_sda_pin = 4;
const int oled_scl_pin = 5;
const int oled_i2c_addr = 0x3C;
const int dw = 128;
const int dh = 32;

#include <Adafruit_SSD1306.h>
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(dw, dh, &Wire, OLED_RESET);

#include "font.h"
#include "helvnCB6pt7b.h"
#define myfont helvnCB6pt7b // Org_01 looks better but is small.
#define smallfont Org_01
#include "display.h"


// buttons & knobs defines/functions
//#include "control.h"


// audio related defines

int current_track;
int32_t update_timer;
int32_t button_timer;
int update_interval = 30;
int engineCount = 0;
bool button_state = true;

// last time btn_one release
unsigned long btnOneLastTime;
unsigned long btnTwoLastTime;
unsigned long btnFourLastTime;
int32_t previous_pitch = 40;

bool just_booting = true;

//File settingsFile;


void setup() {
  if (debug) {
    Serial.begin(57600);
    Serial.println(F("YUP"));
  }

  // start encoders MUST be first because of PIO init?
  //pio_set_gpio_base(PIO pio1, 16); this fails, do it in begin.
  //pio_set_gpio_base(PIO pio0, 0);

  enc1.begin();
  //enc1.flip(); // only on the green blue encoders
  enc2.begin();
  //enc2.flip(); // only on the green blue encoders
  enc3.begin();
  //enc3.flip(); // only on the green blue encoders

  delay(100);

  // set up Pico PWM audio output the DAC2 stereo approach works./
  DAC.setBuffers(4, 32); //plaits::kBlockSize * 4); // DMA buffers
  //DAC.onTransmit(cb);
  DAC.setFrequency(SAMPLERATE);
  // now start the dac
  DAC.begin();

  delay(100);


  // doesn't really get us anything
  analogReadResolution(12);

  // thi is to switch to PWM for power to avoid ripple noise
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);

  // mute mute is LOW
  pinMode(11, OUTPUT);
  digitalWrite(11, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);

  // CV
  pinMode(CV1, INPUT);
  pinMode(CV2, INPUT);
  pinMode(CV3, INPUT);
  pinMode(CV4, INPUT);
  pinMode(CV5, INPUT);
  pinMode(CV6, INPUT);
  // trigger in
  pinMode(41u, INPUT);

  // DISPLAY
  Wire.setSDA(oled_sda_pin);
  Wire.setSCL(oled_scl_pin);
  Wire.begin();
  // SSD1306 --  or SH1106 in this case
  if (!display.begin(SSD1306_SWITCHCAPVCC, oled_i2c_addr)) {
    //if (!display.begin( oled_i2c_addr)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) ;  // Don't proceed, loop forever
  }

  displaySplash();

  // buttons

  btn_four.attach( SW4 , INPUT_PULLUP);
  btn_four.interval(5);
  btn_four.setPressedState(LOW);

  btn_one.attach( SW1 , INPUT_PULLUP);
  btn_one.interval(5);
  btn_one.setPressedState(LOW);

  btn_two.attach( SW2 , INPUT_PULLUP);
  btn_two.interval(5);
  btn_two.setPressedState(LOW);


  btn_three.attach( SW3 , INPUT_PULLUP);
  btn_three.interval(5);
  btn_three.setPressedState(LOW);

  // initialize envelope settings
  envAttack = 0.05f;
  envDecay = 0.3f;
  envRelease = 5;
  envSustain = 0.8f;
  env->setAttackRate(envAttack * SAMPLERATE);  // .01 second
  env->setDecayRate(envDecay * SAMPLERATE);
  env->setReleaseRate(envRelease * SAMPLERATE);
  env->setSustainLevel(envSustain);


  // initialize a mode to play
  //mode = midier::Mode::Ionian;
  //makeScale( roots[scaleRoot], mode);

  // init synth engines

  //initPlaits();
  //delay(50);
  //initRings();
  //delay(50);
  initBraids();
  delay(100);
  initClouds();

  // Initialize wave switch states
  update_timer = millis();
  button_timer = millis();

  // used to indicated we can fetch saved settings
  just_booting = true;
  EEPROM.begin(2048);
  delay(150);

  if (just_booting) {
    loadInit();
    if (device_initialized == 1 ) {
      // only one for now 
      //loadLastPreset(); // sets selected_preset from base eeprom save point
      loadMemorySlots();
    } else {
      // first use of device, so prep it.
      selected_preset = 0;
      currentConfig = defaultSlots[0]; // current config is default 0
      initializeEEPROM(); // first run, get settings into all 8 slots
      //bpm = currentConfig.tempo;
      //internalClock = currentConfig.internalClock;
      writeInit(); // set flag for next boot
    }

    just_booting = false;
  }

  // set up midi last, debugging
  MIDI.setHandleNoteOn(HandleMidiNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(HandleMidiNoteOff);  // Put only the name of the function
  MIDI.setHandleControlChange(HandleControlChange);  // Put only the name of the function
  // Initiate MIDI communications, listen to all channels (not needed with Teensy usbMIDI)
  MIDI.begin(MIDI_CHANNEL_OMNI);

  /*
  int sensorValue;

  // calibrate note in CV1
  while (millis() < 5000) {
    sensorValue = analogRead(CV1);

    // record the maximum sensor value
    if (sensorValue > mapping_upper_limit) {
      mapping_upper_limit = sensorValue;
    }

    // record the minimum sensor value
    if (sensorValue < mapping_lower_limit) {all tabs to spaces
      mapping_lower_limit = sensorValue;
    }
  }
  */

}

// main loop for audio rendering
void loop() {

  if ( DAC.availableForWrite()) {
    if ( ! writing) {
      if (voice_number == 0) {
        /*
        updatePlaitsAudio();
        // now apply the envelope
        for (size_t i = 0; i < plaits::kBlockSize; ++i) {
          int16_t sampleL = (int16_t) ( (float) outputPlaits[i].out * env->process() ) ;
          int16_t sampleR = (int16_t) ( (float) outputPlaits[i].aux * env->process() ) ;
          //out_bufferL[i] = sample;
          DAC.write( sampleL );
          DAC.write( sampleR );
        }*/

      } else if (voice_number == 1) {
        // we're not doing stereo because we get neat poly output with note ins like this
        /*
        updateRingsAudio();
        for (size_t i = 0; i < 32; i++) {
          DAC.write( out_bufferL[i] );
          DAC.write( out_bufferL[i] );
        }*/

      } else if (voice_number == 2) {
        // just mono for now
        updateBraidsAudio();
        for (size_t i = 0; i < 32; i++) {
          int16_t sample =   (int16_t) ( (float) inst[0].pd.buffer[i] * env->process() ) ;
          DAC.write( sample );
          DAC.write( sample );
        }
      } else if (voice_number == 3) {
        // clouds, samplebuffer at same time
        // or braids into buffer directly.
        updateBraidsAudio();
        // copy the braids audio to the clouds input buffer
        clouds::FloatFrame  *input = cloud[0].input;
        for (int i = 0; i < 32; i++) {
          float sample;
          sample = (float) ( analogRead(CV6) )/4095.0f ;
          //if (sample == 0.51f) {
          if (easterEgg) {
            sample = (float) ( inst[0].pd.buffer[i] / 32768.0f   * 0.5f ) ;
          } else {
            sample = sample;// * 1.5f;
          }
          input[i].l = sample;
          input[i].r = sample;  // Mono input
        }
        updateCloudsAudio();
        clouds::FloatFrame  *output = cloud[0].output;
        for (int i = 0; i < 32; i++) {
          int16_t sampleL =  stmlib::Clip16( static_cast<int32_t>(  (  output[i].l )  * 32768.0f  ) ) ;
          int16_t sampleR =  stmlib::Clip16( static_cast<int32_t>(  (  output[i].r )  * 32768.0f  ) ) ;
          DAC.write( sampleL );
          DAC.write( sampleR );
        }
      }
      } // end writing
   } // end available for write

}

void setup1() {
  delay (1000); // wait for main core to start up perhipherals
}

// second core deals with ui / control rate updates
void loop1() {

  if (! writing) { // don't do shit when eeprom is being written1.00
    // we need these on boot so the second loop can catch the startup button.
    btn_one.update();
    btn_two.update();
    btn_three.update();
    btn_four.update();

    unsigned long now = millis();

    // first control elements
    if (voice_number == 0) {
      //updatePlaitsControl();
    } else if (voice_number == 1) {
      //updateRingsControl();
    }
    // need faster updates
    read_encoders();

    if ( midi_switch == false && midi_switch_setting == false ) {
      voct_midi(CV1);
    }
    read_trigger();
    MIDI.read();

    if ( now - update_timer > 5 ) {
      read_cv();
      read_buttons();

      // display updates
      if (btn_two_state == 1) {
        if (voice_number == 2) {
          displayADSR();
        } else {
          // secondary clouds 
          displayReverb();
        }
      } else {
        if (voice_number == 0) {
          //displayPlaits();
        } else if (voice_number == 1) {
          //displayRings();
        } else if (voice_number == 2) {
          displayBraids();
        } else {
          displayClouds();
        }
      }
      update_timer = now;
    }
  }
}

void read_buttons() {

  bool doublePressMode = false;
  bool longPress = false;
  int oneState = btn_one.read();
  int twoState = btn_two.read();

  int fourState = btn_four.read();
  btnOneLastTime = btn_one.previousDuration();
  btnTwoLastTime = btn_two.previousDuration();

  // we toggle button three for either position or timber encoder action
  if ( btn_three.pressed() ) {
      btn_three_state = !btn_three_state;
  }
  // we toggle button two state for either  adsr or normal parameters
  if ( btn_two.pressed() ) {
      //btn_two_state = !btn_two_state;
  }

  if ( btn_two.rose() ) {
    if ( btnTwoLastTime > 350 ) {
      if ( voice_number == 3 ) {
        easterEgg = !easterEgg;
        longPress = true;
      }
    } else if (btnTwoLastTime < 350) {
      btn_two_state = !btn_two_state;

    }
  }

  // if button one was held for more than 300 millis and we're in rings toggle easteregg
  if ( btn_one.rose() ) {
    if ( btnTwoLastTime > 350 ) {
      if ( voice_number == 3 && ! btn_two.pressed()) {
        freeze_in = !freeze_in;
        longPress = true;
      }
    }
  }

  // meta button pressed to cycle through voices
  if (btn_four.rose()) {
    // first record our last settings
    if (voice_number == 0) {
      plaits_morph = morph_in;
      plaits_timbre = timbre_in;
      plaits_harm = harm_in;
      plaits_engine = engine_in;
      plaits_position = position_in;
    }
    if (voice_number == 1) {
      rings_morph = morph_in;
      rings_timbre = timbre_in;
      rings_harm = harm_in;
      rings_position = position_in;
      rings_engine = engine_in;
    }
    if (voice_number == 2) {
      braids_morph = morph_in;
      braids_timbre = timbre_in;
      braids_engine = engine_in;
    }
    /* not used */
    if (voice_number == 3) {
      clouds_morph = morph_in;
      clouds_timbre = timbre_in;
      clouds_harm = harm_in;
      clouds_position = position_in;
      clouds_engine = engine_in;
    }

    voice_number++;
    if (voice_number > 3) voice_number = 2;

    // Save to eeprom
    // set mute then save
    // send null data for (21ms @ 48kHz)

    writing = true;
    digitalWrite(11, HIGH);
    digitalWrite(11, LOW);
    int16_t sampleL = 0;
    unsigned long now = millis();

    while ( millis() - now < 35) {
        DAC.write(sampleL);
        DAC.write(sampleL);
    }
    delay(21);

    saveToEEPROM(0);
    setVoiceParameters();

    delay(1);
    digitalWrite(11, LOW);
    digitalWrite(11, HIGH);
    writing = false;
  }

  if (!doublePressMode && !longPress) {
    // being tripple shure :)

  }
}

/*
 *  set the globals to values from last saved voice parameters
 */

void setVoiceParameters(){
  if (voice_number == 0) {
      engine_in = plaits_engine; // engine_in % 17;
      max_engines = 21;
      morph_in = plaits_morph;
      timbre_in = plaits_timbre;
      harm_in = plaits_harm;
      position_in = plaits_position;

    } else if (voice_number == 1) {
      engine_in = rings_engine;
      max_engines = 5;
      morph_in = rings_morph;
      harm_in = rings_harm;
      timbre_in = rings_timbre;
      position_in = rings_position;

    } else if (voice_number == 2 ) {
      engine_in = braids_engine;
      max_engines = 45;
      morph_in = braids_morph;
      timbre_in = braids_timbre;

    } else if (voice_number == 3 ) {
      engine_in = clouds_engine;
      max_engines = 3;
      morph_in = clouds_morph;
      timbre_in = clouds_timbre;
      harm_in = clouds_harm;
    }
}

float voct_midiBraids(int cv_in) {

  int val = 0;
  for (int j = 0; j < cv_avg; ++j) val += analogRead(cv_in); // read the A/D a few times and average for a more stable value
  val = val / cv_avg;
  pitch = pitch_offset + map(val, 0.0, 4095.0, mapping_upper_limit, 0.0); // convert pitch CV data value to a MIDI note number
  return pitch - 37; // don't know why, probably tuned to A so -5 + -36 to drop two octaves
}


void voct_midi(int cv_in) {
  int val = 0;
  for (int j = 0; j < 15; ++j) val += analogRead(cv_in); // read the A/D a few times and average for a more stable value
  val = val / 15;

  // data = (float) val * 1.0f;
  pitch = map(val, mapping_lower_limit, mapping_upper_limit, 36, 84); // convert pitch CV data value to a MIDI note number

  //pitch = pitch - pitch_offset;

  pitch_in = pitch;

  // this is a temporary move to get around clicking on trigger + note cv in
  if (pitch != previous_pitch) {
    previous_pitch = pitch;
    // this is the plaits version

  }
}

// in marvelous, moved to digital pin tx/gp0
void read_trigger() {
  int16_t trig ;
  for (int j = 0; j < 5; ++j) trig += analogRead(41u); // read the A/D a few times and average for a more stable value
  trig = trig / 5 ;

  // tricky conflicts with default uarts
  if ( midi_switch == false && midi_switch_setting == false ) {

    if (trig  > 500 ) {
      trigger_in = 1.0f;
      //trigger_on = true;
      //if (millis() - envTimer > 50) {
      envTimer = millis();
      env->gate(true);
      //}

    } else  {
      //don't turn off here?
      trigger_in = 0.0f;
      // don't retrigger ADSR too quickly
      //if (millis() - envTimer > 50) {
      envTimer = 0;
      env->gate(false);
      //}
      //trigger_on = false;
    }
    //if (voice_number == 0) updateVoicetrigger();

  }

}

void read_cv() {
  // CV updates
  // braids wants 0 - 32767, plaits 0-1

  //plaits and rings cv
  int16_t timbre = avg_cv(CV2);
  timb_mod = (float)timbre;
  timb_mod = timbre /4095.0f * 0.5f; //mapf( timb_mod, 5.0f, 4090.0f, 0.00f, 1.00f);
  timb_mod = constrain(timb_mod, 0.00f, 1.00f);

  int16_t morph = avg_cv(CV3) ;
  morph_mod = (float) morph;
  morph_mod = morph / 4095.0f * 0.5f; //mapf ( (float) morph_mod, 5.0f, 4090.0f, 0.00f, 1.00f);
  morph_mod = constrain(morph_mod, 0.00f, 1.00f);

  // don't remember if this was important
  float pos = avg_cv(CV4) ; // f&d noise floor
  pos_mod = pos/4095.0f * 0.5f;//mapf (  pos, 5.0f, 4090.0f, 0.00f, 1.00f);
  pos_mod = constrain(pos, 0.00f, 1.00f);

  // plaits
  //int16_t lpgColor =  avg_cv(CV5);
  //lpg_in = mapf( (float) lpgColor, 180.0f, 4095.0f, 0.00f, 1.000f);

  if (voice_number == 0 || voice_number == 1) {
    // plaits

    timb_mod = mapf(timb_mod, 0.0f, 1.0f, 0.0f, 0.8f);

    //if (debug) Serial.print(timb_mod);

    //voices[0].modulations.timbre_patched = true;
    //voices[0].modulations.timbre_patched = false;
    //morph_mod = mapf(morph_mod, 0.02f, 1.0f, -1.0f, 1.0f);

    morph_mod = mapf(morph_mod, 0.0f, 1.0f, 0.0f, 0.8f);

    //voices[0].modulations.morph_patched = true;

  }
  if (voice_number == 3) {

  }

}

// either avg or median, both suck :)
int16_t avg_cv(int cv_in) {

  //std::vector<int> data;
  int16_t val = 0;

  //for (int j = 0; j < cv_avg; ++j) data.push_back(analogRead(cv_in)); // val += analogRead(cv_in); // read the A/D a few times and average for a more stable value
  for (int j = 0; j < cv_avg; ++j) val += analogRead(cv_in); // read the A/D a few times and average for a more stable value
  val = val / cv_avg;

  //return median(data);

  return val;
}

void read_encoders() {

  // enc4 meta is an exceptoin
  enc4.tick();

  // first encoder actually, third
  int enc1_pos = enc1.getCount() / 4;
  if ( enc1_pos != enc1_pos_last ) {
    enc1_delta = (enc1_pos - enc1_pos_last) ;
  }
  // encoder 3 does double duty
  // on timbre and position
  if ( enc1_delta) {
    if (btn_three_state == 0 && btn_two_state == 0) {
      float turn = ( enc1_delta * 0.005f ) + timbre_in;
      CONSTRAIN(turn, 0.f, 1.0f)
      timbre_in = turn;
    } else if (btn_three_state == 1 && btn_two_state == 0) {
      float turn = ( enc1_delta * 0.005f ) + position_in;
      CONSTRAIN(turn, 0.f, 1.0f)
      position_in = turn;
    } else if ( btn_two_state == 1 && voice_number == 2) {
      float turn =  enc1_delta  + envRelease;
      CONSTRAIN(turn, 1, 10)
      envRelease = turn;
      env->setReleaseRate(envRelease * SAMPLERATE);
    } else if (btn_two_state == 1 && voice_number == 3) {
      float turn = ( enc1_delta * 0.005f ) + clouds_dw_in;
      CONSTRAIN(turn, 0.f, 1.0f)
      clouds_dw_in = turn;
    }
  }
  enc1_delta = 0;
  enc1_pos_last = enc1_pos;

  // second encoder actually first
  int enc2_pos = enc2.getCount() / 4;
  if ( enc2_pos != enc2_pos_last ) {
    enc2_delta = (enc2_pos - enc2_pos_last) ;
  }
  if (enc2_delta) {
    if (btn_two_state == 0) {
      float turn = ( enc2_delta * 0.005f ) + morph_in;
      CONSTRAIN(turn, 0.f, 1.0f)
      morph_in = turn;
    } else if (btn_two_state == 1 && voice_number ==2) {
      float turn = ( enc2_delta * 0.005f ) + envDecay;
      CONSTRAIN(turn, 0.f, 1.0f)
      envDecay = turn;
      env->setDecayRate(envDecay * SAMPLERATE);  // .01 second
    } else if (btn_two_state == 1 && voice_number ==3) {
      float turn = ( enc2_delta * 0.005f ) + clouds_reverb;
      CONSTRAIN(turn, 0.f, 1.0f)
      clouds_reverb = turn;
    }
  }
  enc2_pos_last = enc2_pos;
  enc2_delta = 0;

  // third encoder, actually second
  int enc3_pos = enc3.getCount() / 4;
  if ( enc3_pos != enc3_pos_last ) {
    enc3_delta = (enc3_pos - enc3_pos_last);
  }
  if (enc3_delta) {
    if (btn_two_state == 0) {
      float turn = ( enc3_delta * 0.005f ) + harm_in;
      CONSTRAIN(turn, 0.f, 1.0f)
      harm_in = turn;
    } else if (btn_two_state == 1 && voice_number ==2) {
      float turn = ( enc3_delta * 0.005f ) + envSustain;
      CONSTRAIN(turn, 0.f, 1.0f)
      envSustain = turn;
      env->setSustainLevel(envSustain);
    } else if (btn_two_state == 1 && voice_number ==3) {
      float turn = ( enc3_delta * 0.005f ) + clouds_fb;
      CONSTRAIN(turn, 0.f, 1.0f)
      clouds_fb = turn;
    }
  }
  enc3_pos_last = enc3_pos;
  enc3_delta = 0;


  // meta encoder turned to cycle through voice engines
  int enc4_pos = enc4.getPosition();
  if ( enc4_pos != enc4_pos_last ) {
    if (btn_two_state == 0) {
      engineCount =  (int) enc4.getDirection()  + engineCount ;
      if (engineCount < 0) {
        engineCount = max_engines;
      } else if (engineCount > max_engines) {
        engineCount = 0;
      }
      engine_in = engineCount;
    } else if (btn_two_state == 1 && voice_number ==2) {
      float turn = ( (int) enc4.getDirection() * 0.005f ) + envAttack;
      CONSTRAIN(turn, 0.f, 1.0f)
      envAttack = turn;
      env->setAttackRate(envAttack * SAMPLERATE);  // .01 second
    } else if (btn_two_state == 1 && voice_number ==3) {
      float turn = ( (int) enc4.getDirection() * 0.005f ) + clouds_spread;
      CONSTRAIN(turn, 0.f, 1.0f)
      clouds_spread = turn;

    }
  }
  enc4_pos_last = enc4_pos;

}
