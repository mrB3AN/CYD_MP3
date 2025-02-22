#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "Audio.h"

Audio audio(true, I2S_DAC_CHANNEL_LEFT_EN);

#define SD_CS_PIN 5

const char* AUDIO_FILE = "/BohemianRhapsody.mp3"; 

void setup() {
  Serial.begin(115200);

  delay(5000);
  Serial.println("Starting...");
  delay(1000);


  if (!SD.begin(SD_CS_PIN, SPI, 80000000)) {
    Serial.println("[SD] Card mount failed!");
    while (true) { delay(1000); }
  }



  audio.forceMono(false);   
  audio.setVolume(20);

  if (SD.exists(AUDIO_FILE)) {
    Serial.printf("[Audio] Attempting to play %s...\n", AUDIO_FILE);
    bool ok = audio.connecttoSD(AUDIO_FILE);
    if (!ok) {
      Serial.println("[Audio] Could not open file!");
    }
  } else {
    Serial.printf("[SD] File %s not found.\n", AUDIO_FILE);
  }
}

void loop() {
  audio.loop();
}

// Optional debug callbacks:
void audio_info(const char *info) {
  Serial.print("[Audio Info] ");
  Serial.println(info);
}
