/* EEPROM  mostly from modulove A-RYTH-MATIK
*/
#define EEPROM_START_ADDRESS 50
#define NUM_PRESETS 1
#define CONFIG_SIZE (sizeof(ConfigSlot))
#define LAST_USED_SLOT_ADDRESS (EEPROM_START_ADDRESS + NUM_PRESETS * CONFIG_SIZE)

// we have 1 preset progmem and save altered/new settings to eeprom slots

struct ConfigSlot {

  byte plaits[8];
  byte rings[8];
  byte braids[8];
  byte clouds[8];
  byte adsr[4];
  int tempo;                      // Tempo for the preset
  bool internalClock;             // Clock source state
  byte lastUsedSlot;              // Last used slot
  byte selectedPreset;            // last used preset / slot > 10 is an eeprom slot
  byte voiceNumber;               // last used preset / slot > 10 is an eeprom slot
};

const ConfigSlot defaultSlots[NUM_PRESETS] = PROGMEM {
  {
    { 127, 127, 127, 127, 127, 127, 0, 0 },  //plaits
    { 127, 127, 127, 127, 127, 127, 0, 0 },  //rings
    { 127, 127, 127, 127, 127, 127, 0, 0 },  //braids
    { 127, 127, 127, 127, 127, 127, 0, 0 },  //clouds
    { 13, 77, 5, 204 },              //adsr
    120,
    0 ,
    0 ,
    0 ,
    0 }
};

ConfigSlot memorySlots[NUM_PRESETS], currentConfig;

float eeprom_error = 0.0f;
uint8_t internalClock = 0;
uint8_t lastUsedSlot = 0;
bool offset_buf[8][16];  //offset buffer , Stores the offset result

// selected_preset = (selected_preset + increment + sizeof(defaultSlots) / sizeof(ConfigSlot)) % (sizeof(defaultSlots) / sizeof(ConfigSlot));

// persist global
//float fm_mod = 0.0f ; //IN(7);
//float decay_in = 0.5f; // IN(10);
//float lpg_in = 0.2f ;// IN(11);
//float pitch_in = 32.0f;

uint8_t floatToU8(float value) {
    // Clamp the value to [0, 1] to ensure it's within the normalized range
    value = std::clamp(value, 0.0f, 1.0f);
    // Scale and round to the nearest integer in the [0, 255] range
    return static_cast<uint8_t>(std::round(value * 255.00f));
}
float u8ToFloat(uint8_t value) {
    return static_cast<float>(value / 255.0f);
}

void updateSlot() {
    // globals
    currentConfig.voiceNumber = voice_number;  // which engine
                           // adsr
    currentConfig.adsr[0] = floatToU8(envAttack);
    currentConfig.adsr[1] = floatToU8(envDecay );
    currentConfig.adsr[2] = envRelease;
    currentConfig.adsr[3] = floatToU8(envSustain );

    // voices
    currentConfig.plaits[0] = floatToU8(plaits_morph );
    currentConfig.plaits[1] = floatToU8(plaits_harm );
    currentConfig.plaits[2] = floatToU8(plaits_timbre );
    currentConfig.plaits[3] = floatToU8(plaits_position );
    currentConfig.plaits[4] = floatToU8(plaits_level );
    currentConfig.plaits[5] = plaits_engine;

    currentConfig.rings[0] = floatToU8(rings_morph );
    currentConfig.rings[1] = floatToU8(rings_harm );
    currentConfig.rings[2] = floatToU8(rings_timbre );
    currentConfig.rings[3] = floatToU8(rings_position );
    currentConfig.rings[4] = floatToU8(rings_level );
    currentConfig.rings[5] = rings_engine;

    currentConfig.braids[0] = floatToU8(braids_morph );
    currentConfig.braids[1] = floatToU8(braids_harm );
    currentConfig.braids[2] = floatToU8(braids_timbre );
    currentConfig.braids[3] = floatToU8(braids_position );
    currentConfig.braids[4] = floatToU8(braids_level );
    currentConfig.braids[5] = braids_engine;

    currentConfig.clouds[0] = floatToU8(clouds_morph );
    currentConfig.clouds[1] = floatToU8(clouds_harm );
    currentConfig.clouds[2] = floatToU8(clouds_timbre );
    currentConfig.clouds[3] = floatToU8(clouds_position );
    currentConfig.clouds[4] = floatToU8(clouds_level );
    currentConfig.clouds[5] = clouds_engine;
}

void updateValues(){

    voice_number = currentConfig.voiceNumber;  // which engine
    // adsr
    envAttack = u8ToFloat(currentConfig.adsr[0]);
    envDecay = u8ToFloat(currentConfig.adsr[1]);
    envRelease = currentConfig.adsr[2];
    envSustain = u8ToFloat(currentConfig.adsr[3]);

    env->setAttackRate(envAttack * SAMPLERATE);
    env->setDecayRate(envDecay * SAMPLERATE);
    env->setReleaseRate(envRelease * SAMPLERATE);
    env->setSustainLevel(envSustain);

    // voices
    plaits_morph = u8ToFloat(currentConfig.plaits[0]);
    plaits_harm  = u8ToFloat(currentConfig.plaits[1]);
    plaits_timbre = u8ToFloat(currentConfig.plaits[2]);
    plaits_position = u8ToFloat(currentConfig.plaits[3]);
    plaits_level = u8ToFloat(currentConfig.plaits[4]);
    plaits_engine = currentConfig.plaits[5] ;

    rings_morph = u8ToFloat(currentConfig.rings[0]);
    rings_harm  = u8ToFloat(currentConfig.rings[1]);
    rings_timbre = u8ToFloat(currentConfig.rings[2]);
    rings_position = u8ToFloat(currentConfig.rings[3]);
    rings_level = u8ToFloat(currentConfig.rings[4]);
    rings_engine = currentConfig.rings[5];

    braids_morph = u8ToFloat(currentConfig.braids[0]);
    braids_harm = u8ToFloat(currentConfig.braids[1]);
    braids_timbre = u8ToFloat(currentConfig.braids[2]);
    braids_position = u8ToFloat(currentConfig.braids[3]);
    braids_level = u8ToFloat(currentConfig.braids[4]);
    braids_engine = currentConfig.braids[5];

    clouds_morph = u8ToFloat(currentConfig.clouds[0]);
    clouds_harm = u8ToFloat(currentConfig.clouds[1]);
    clouds_timbre = u8ToFloat(currentConfig.clouds[2]);
    clouds_position = u8ToFloat(currentConfig.clouds[3]);
    clouds_level = u8ToFloat(currentConfig.clouds[4]);
    clouds_engine = currentConfig.clouds[5];

    // set globals from current voice parameters
    setVoiceParameters();

}

void printConfigSlot(ConfigSlot slot) {
  // Print plaits array
  Serial.println("plaits: ");
  for (int i = 0; i < 8; i++) {
    Serial.print(slot.plaits[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Print rings array
  Serial.println("rings: ");
  for (int i = 0; i < 8; i++) {
    Serial.print(slot.rings[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Print braids array
  Serial.println("braids: ");
  for (int i = 0; i < 8; i++) {
    Serial.print(slot.braids[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Print clouds array
  Serial.println("clouds: ");
  for (int i = 0; i < 8; i++) {
    Serial.print(slot.clouds[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Print adsr array
  Serial.println("adsr: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(slot.adsr[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Print tempo
  Serial.print("tempo: ");
  Serial.println(slot.tempo);

  // Print internalClock
  Serial.print("internalClock: ");
  Serial.println(slot.internalClock ? "true" : "false");

  // Print lastUsedSlot
  Serial.print("lastUsedSlot: ");
  Serial.println(slot.lastUsedSlot);

  // Print selectedPreset
  Serial.print("selectedPreset: ");
  Serial.println(slot.selectedPreset);

  // Print voiceNumber
  Serial.print("voiceNumber: ");
  Serial.println(slot.voiceNumber);
}

/* track of current preset and slot outside of the general config
* permit loading correct preset/slot at boot with less overhead
*/
void saveCurrentPreset(int preset) {
  uint8_t baseAddress = 10;
  EEPROM.write(baseAddress, preset);
  if (EEPROM.commit()) {
    if (debug) Serial.println("EEPROM wrote preset");
  } else {
      eeprom_error = 1.1;
    if (debug) Serial.println("ERROR! EEPROM commit failed");
  }
}

void loadLastPreset() {
  uint8_t baseAddress = 10;
  selected_preset = EEPROM.read(baseAddress);
  if (debug) Serial.print("Last preset: ");
  if (debug) Serial.println(selected_preset);
}

/* loadInit is a function to retrieve the init digit
 * which indicates the device has initialized once
 */
void loadInit() {
  uint8_t baseAddress = 20;
  device_initialized = EEPROM.read(baseAddress);
  if (debug) Serial.print("Last preset: ");
  if (debug) Serial.println(selected_preset);
}

/* writeInit is a function to set the init digit
 * which indicates the device has initialized once
 */
void writeInit() {
  uint8_t baseAddress = 20;
  EEPROM.write(baseAddress, 1);
  if (EEPROM.commit()) {
    if (debug) Serial.println("EEPROM wrote preset");
  } else {
    eeprom_error = 1.2;
    if (debug) Serial.println("ERROR! EEPROM commit failed");
  }
}

void saveToEEPROM(int slot) {
  int baseAddress = EEPROM_START_ADDRESS + (slot * CONFIG_SIZE);
  if (baseAddress + CONFIG_SIZE <= EEPROM.length()) {
    // first update from running values
    updateSlot();
    currentConfig.tempo = bpm;
    //currentConfig.internalClock = internalClock;
    currentConfig.lastUsedSlot = slot;
    currentConfig.selectedPreset = selected_preset;
    EEPROM.put(baseAddress, currentConfig);
  } else {
    // Handle error
    eeprom_error = 2.0;
    if (debug ) Serial.println("EEPROM Save Error");
    return;
  }
  if (EEPROM.commit()) {
    if (debug) Serial.println("EEPROM successfully committed");
    printConfigSlot(currentConfig);
    saveCurrentPreset(slot);  // update the currently set preset
  } else {
    eeprom_error = 1.3;
    if (debug) Serial.println("ERROR! EEPROM commit failed");
  }

}

/*
   This function takes the default progmem configs and copies them to eeprom
   Thereafter, we can read all eeprom settings at start to reduce noise on loading
*/
void initializeEEPROM() {
  //ConfigSlot conf;
  // only using one for now
  currentConfig = defaultSlots[0];
  memorySlots[0] = defaultSlots[0];
  saveToEEPROM(0);
  /*
  for (uint8_t slot = 0; slot < 8; slot++) {
    currentConfig = defaultSlots[slot];
    memorySlots[slot] = defaultSlots[slot];
    saveToEEPROM(slot);
  }
  */

}

/*
   On device start we read all eeprom slots to be able to access them without recourse to eeprom
*/
void loadMemorySlots() {
  // only ONE
  int slot = 0;

  //for (uint8_t slot = 0; slot < 8; slot++) {
    int baseAddress = EEPROM_START_ADDRESS + (slot * CONFIG_SIZE);

    if (baseAddress + CONFIG_SIZE <= EEPROM.length()) {
      EEPROM.get(baseAddress, currentConfig);
      memorySlots[0] = currentConfig;
        updateValues();
    } else {
      // Handle the error
      eeprom_error = 3.0;
      if (debug ) Serial.println("EEPROM load Error");
      return;
    }
  //}

}

/* 
 * unused
*/

void loadDefaultConfig(ConfigSlot *config, int index) {
// Loading ConfigSlot from PROGMEM
  if (index >= NUM_PRESETS) index = 0;
  memcpy_P(config, &defaultSlots[index], sizeof(ConfigSlot));

}

void loadFromPreset(int preset) {
  if (preset >= sizeof(defaultSlots) / sizeof(ConfigSlot)) preset = 0;
  loadDefaultConfig(&currentConfig, preset);
  bpm = currentConfig.tempo;
  internalClock = currentConfig.internalClock;
  if (debug) Serial.println(bpm);
  selected_preset = preset;
}

// Loading ConfigSlot from memorySlots
void loadMemoryConfig(ConfigSlot *config, int index) {
  if (index >= NUM_PRESETS) index = 0;
  memcpy(config, &memorySlots[index], sizeof(ConfigSlot));
}

void loadFromMemorySlot(int preset) {
  if (preset >= sizeof(memorySlots) / sizeof(ConfigSlot)) preset = 0;
  //loadMemoryConfig(&currentConfig, preset);
  currentConfig = memorySlots[preset];
  bpm = currentConfig.tempo;
  selected_preset = preset;
  internalClock = currentConfig.internalClock;
  // now load them
  updateValues();

}

void loadMemorySlotDefaults() {
  for (uint8_t i = 0; i < 8; i++) {
    //memorySlots[i] = defaultSlots[i];
    memcpy_P(&memorySlots[i], &defaultSlots[i], sizeof(ConfigSlot));
    if (debug) Serial.println(memorySlots[i].tempo);
  }
}
/*
   On save, update memorySlots
*/
void updateMemorySlot(int preset) {
  memorySlots[preset] = currentConfig;
}

void loadFromEEPROM(int slot) {
  int baseAddress = EEPROM_START_ADDRESS + (slot * CONFIG_SIZE);
  if (baseAddress + CONFIG_SIZE <= EEPROM.length()) {
    EEPROM.get(baseAddress, currentConfig);
    bpm = currentConfig.tempo;
    internalClock = currentConfig.internalClock;
    //lastUsedSlot = slot;
    //selected_preset = currentConfig.selectedPreset;
    if (debug) Serial.println(bpm);
    //period = 60000 / bpm / 4;  // Update period with loaded tempo
    //setup_complete = true;
    updateValues(); // and load to memory

  } else {
    // Handle the error
    if (debug ) Serial.println("EEPROM Load Error");
    return;
  }
}

void initializeCurrentConfig(bool loadDefaults = false) {
  if (loadDefaults) {
    // Load default configuration from PROGMEM
    memcpy(&currentConfig, &defaultSlots[0], sizeof(ConfigSlot));  // Load the first default slot as the initial configuration
  } else {
    // Load configuration from EEPROM
    int baseAddress = EEPROM_START_ADDRESS;  // Start address for the first slot
    EEPROM.get(baseAddress, currentConfig);
    bpm = currentConfig.tempo;                     // Load tempo
    internalClock = currentConfig.internalClock;     // Load clock state
    lastUsedSlot = currentConfig.lastUsedSlot;       // Load last used slot
    selected_preset = currentConfig.selectedPreset;  // Load last used preset
    if (selected_preset < NUM_PRESETS) {             // we use overflow to determine if we're in eeprom
      loadFromPreset(selected_preset);
    } else {
      loadFromEEPROM(lastUsedSlot);
    }
  }
}
