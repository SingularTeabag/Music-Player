#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorFLAC.h"
#include "AudioOutputI2S.h"

#define SD_CS 5

// Volume button pins
#define VOL_UP 14
#define VOL_DOWN 27
#define VOL_STEP 0.005  // Increment/decrement per button press

AudioGeneratorFLAC *flac;
AudioFileSourceSD *file;
AudioOutputI2S *out;

// Initial volume (very low to prevent clipping)
float volume = 0.02;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting ESP32 Audio Player...");

  // Initialize buttons
  pinMode(VOL_UP, INPUT_PULLUP);
  pinMode(VOL_DOWN, INPUT_PULLUP);

  // Initialize SD card
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR: SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized successfully!");

  // Check file
  if (!SD.exists("/music.flac")) {
    Serial.println("ERROR: File /music.flac not found!");
    return;
  }
  Serial.println("Audio file found!");
  delete file;
  file = new AudioFileSourceSD("/music.flac");

  // Setup I2S output
  out = new AudioOutputI2S();

  // Pin mapping for WCMCU-1334
  out->SetPinout(25, 32, 33);  // BCLK, LRCLK, DATA

  // Correct buffer configuration
  out->SetBuffers(16, 2304);

  // Set initial volume
  out->SetGain(volume);

  Serial.println("I2S audio output configured.");

  // Start FLAC decoder
  flac = new AudioGeneratorFLAC();

  Serial.println("Starting FLAC playback...");
  if (!flac->begin(file, out)) {
    Serial.println("ERROR: Failed to start FLAC decoder!");
  } else {
    Serial.println("playing show me how to live!");
  }
}

void loop() {
  // Handle volume buttons
  if (digitalRead(VOL_UP) == LOW) {
    volume += VOL_STEP;
    if (volume > 0.1) volume = 0.1;  // max volume
    out->SetGain(volume);
    Serial.print("Volume: ");
    Serial.println(volume);
    delay(150);  // simple debounce
  }

  if (digitalRead(VOL_DOWN) == LOW) {
    volume -= VOL_STEP;
    if (volume < 0.02) volume = 0.02;  // min volume
    out->SetGain(volume);
    Serial.print("Volume: ");
    Serial.println(volume);
    delay(150);  // simple debounce
  }

  // Run the FLAC player
  if (flac && flac->isRunning()) {
    if (!flac->loop()) {
      // Stop old FLAC
      if (flac->isRunning()) flac->stop();
      delete flac;

      // Delete old file
      delete file;

      // Create new file
      file = new AudioFileSourceSD("/music2.flac");

      // Create new FLAC decoder
      flac = new AudioGeneratorFLAC();
      flac->begin(file, out);
    }
  } else {
    delay(1000);
  }
}