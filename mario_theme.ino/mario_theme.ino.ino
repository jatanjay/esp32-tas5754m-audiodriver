#include <ESP_I2S.h>
#include <Wire.h>

// I2C Configuration
#define TAS5754M_I2C_ADDRESS 0x4C // 7-bit I2C address (0x98 >> 1)

// I2S Configuration
#define I2S_BCLK_PIN 5
#define I2S_WS_PIN 25
#define I2S_DOUT_PIN 26

#define TAS5754M_STANDBY_DISABLE 0x00
#define TAS5754M_UNMUTE_BOTH 0x00
#define TAS5754M_PLL_REF_MCLK 0x10
#define TAS5754M_CLOCK_AUTO_SET 0x08
#define TAS5754M_FORMAT_I2S_16BIT 0x00

#define TAS5754M_REG_PAGE_SELECT 0x00  // P0-R0: Page Select
#define TAS5754M_REG_STANDBY 0x02      // P0-R2: Standby Control
#define TAS5754M_REG_MUTE 0x03         // P0-R3: Mute Control
#define TAS5754M_REG_PLL_REF 0x0D      // P0-R13: PLL Reference
#define TAS5754M_REG_CLOCK_CTRL 0x25   // P0-R37: Clock Control
#define TAS5754M_REG_AUDIO_FORMAT 0x28 // P0-R40: Audio Format
#define TAS5754M_REG_VOLUME_B 0x3D     // P0-R61: Channel B Volume
#define TAS5754M_REG_VOLUME_A 0x3E     // P0-R62: Channel A Volume

// Note frequencies (in Hz)
#define NOTE_E4 330
#define NOTE_C4 262
#define NOTE_G4 392
#define NOTE_G3 196
#define NOTE_E3 165
#define NOTE_A3 220
#define NOTE_B3 247
#define NOTE_AS3 233
#define NOTE_F4 349
#define NOTE_A4 440
#define NOTE_D4 294

// Mario theme notes and durations
const int marioNotes[] = {
    NOTE_E4, NOTE_E4, 0,       NOTE_E4, 0,        NOTE_C4,  NOTE_E4, 0,
    NOTE_G4, 0,       0,       0,       NOTE_G3,  0,        0,       0,
    NOTE_C4, 0,       0,       NOTE_G3, 0,        0,        NOTE_E3, 0,
    0,       NOTE_A3, 0,       NOTE_B3, 0,        NOTE_AS3, NOTE_A3, 0,
    NOTE_G3, NOTE_E4, NOTE_G4, NOTE_A4, 0,        NOTE_F4,  NOTE_G4, 0,
    NOTE_E4, 0,       NOTE_C4, NOTE_D4, NOTE_B3,  0,        0,       NOTE_C4,
    0,       0,       NOTE_G3, 0,       0,        NOTE_E3,  0,       0,
    NOTE_A3, 0,       NOTE_B3, 0,       NOTE_AS3, NOTE_A3,  0,       NOTE_G3,
    NOTE_E4, NOTE_G4, NOTE_A4, 0,       NOTE_F4,  NOTE_G4,  0,       NOTE_E4,
    0,       NOTE_C4, NOTE_D4, NOTE_B3, 0,        0};

const int marioDurations[] = {8, 8, 8, 8, 8, 8, 8, 8, 4, 8, 8, 8, 4, 8, 8, 8,
                              4, 8, 8, 4, 8, 8, 4, 8, 8, 4, 8, 4, 8, 8, 4, 8,
                              4, 8, 8, 4, 8, 8, 4, 8, 4, 8, 8, 4, 4, 8, 8, 4,
                              8, 8, 4, 8, 8, 4, 8, 8, 4, 8, 4, 8, 8, 4, 8, 4,
                              8, 8, 4, 8, 8, 4, 8, 4, 8, 8, 4, 4, 8, 8};

I2SClass i2s;

void setup() {
  Serial.begin(115200);

  // Initialize I2C
  Wire.begin(21, 22); // SDA=21, SCL=22
  delay(100);

  // Initialize TAS5754M
  initTAS5754M();

  // Initialize I2S
  i2s.setPins(I2S_BCLK_PIN, I2S_WS_PIN, I2S_DOUT_PIN, -1, -1);
  if (!i2s.begin(I2S_MODE_STD, 44100, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_STEREO)) {
    Serial.println("Failed to initialize I2S!");
    while (1)
      ;
  }
}

void initTAS5754M() {
  // Select page 0
  writeRegister(0x00, 0x00);
  delay(1);

  // Disable standby
  writeRegister(TAS5754M_REG_STANDBY, TAS5754M_STANDBY_DISABLE);
  delay(1);

  // Unmute both channels
  writeRegister(TAS5754M_REG_MUTE, TAS5754M_UNMUTE_BOTH);
  delay(1);

  // Configure PLL reference to MCLK
  writeRegister(TAS5754M_REG_PLL_REF, TAS5754M_PLL_REF_MCLK);
  delay(1);

  // Enable clock auto set
  writeRegister(TAS5754M_REG_CLOCK_CTRL, TAS5754M_CLOCK_AUTO_SET);
  delay(1);

  // Configure I2S format, 16-bit
  writeRegister(TAS5754M_REG_AUDIO_FORMAT, TAS5754M_FORMAT_I2S_16BIT);
  delay(1);

  // Set volume (0x30 = -9dB -- )
  writeRegister(TAS5754M_REG_VOLUME_B, 0x60);
  writeRegister(TAS5754M_REG_VOLUME_A, 0x60);
  delay(1);
}

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(TAS5754M_I2C_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void playNote(int frequency, int duration) {
  if (frequency == 0) {
    // Play silence
    for (int i = 0; i < duration * 100; i++) {
      i2s.write(0);
      i2s.write(0);
    }
    return;
  }

  const int sampleRate = 44100;
  const int amplitude = 12000; // Increased amplitude
  const int totalSamples = duration * 100;

  float phase = 0.0;
  const float phaseIncrement = 2.0 * PI * frequency / sampleRate;

  for (int i = 0; i < totalSamples; i++) {
    // Generate square wave
    float sample = 0;
    if (sin(phase) > 0) {
      sample = 1.0;
    } else {
      sample = -1.0;
    }

    // Convert to 16-bit
    int16_t output = (int16_t)(amplitude * sample);

    i2s.write(output); // Left channel
    i2s.write(output); // Right channel

    phase += phaseIncrement;
    if (phase >= 2.0 * PI)
      phase -= 2.0 * PI;
  }
}

void loop() {
  // Play the Mario theme
  for (int i = 0; i < sizeof(marioNotes) / sizeof(int); i++) {
    int noteDuration = 1000 / marioDurations[i];
    playNote(marioNotes[i], noteDuration);
    delay(noteDuration / 8);
  }

  delay(1000);
}