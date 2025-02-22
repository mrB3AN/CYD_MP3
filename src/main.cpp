#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "Audio.h"
#include "FS.h"
#include <vector>

Audio audio(true, I2S_DAC_CHANNEL_LEFT_EN);

#define SD_CS_PIN 5

const char* AUDIO_FILE = "/BohemianRhapsody.mp3"; 

std::vector<String> fileList;
int currentSongIndex = 0;
int playlistSize = 0;

// List the files in the directory
void listDir(fs::FS &fs, const char * dirname, uint8_t levels, std::vector<String> &fileList) {
  // Open the directory
  File root = fs.open(dirname);

  // Check if the directory exists
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  // Check if the file is a directory
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  // Move to the first file in the directory
  File file = root.openNextFile();
  while (file) {
    // If the file is not a directory
    if (!file.isDirectory()) 
    {      
      // If the file is not a directory, add it to the list
      fileList.push_back(String(file.name()));
    }

    // Move to the next file
    file = root.openNextFile();
  }
}

// Callback function for end of stream
void audio_eof_mp3(const char* info) {
  Serial.println("[Audio] Song finished!");

  currentSongIndex++;

  // Play the next song
  if (currentSongIndex < playlistSize) {
    Serial.printf("[Audio] Attempting to play %s...\n", fileList[currentSongIndex].c_str());
    bool ok = audio.connecttoSD(fileList[currentSongIndex].c_str());
    if (!ok) {
      Serial.println("[Audio] Could not open file!");
    }
  } 
  else {
    Serial.println("[Audio] No more songs in the list.");
  }
}

//////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);

  delay(5000);
  Serial.println("Starting...");
  delay(1000);

  // Mount the SD
  if (!SD.begin(SD_CS_PIN, SPI, 80000000)) {
    Serial.println("[SD] Card mount failed!");
    while (true) { delay(1000); }
  }


  // Create a vector to hold the file names
  listDir(SD, "/", 0, fileList);

  // Print the list of file names
  Serial.println("Files found:");
  for (const auto &fileName : fileList) {
    Serial.println(fileName);
  }

  playlistSize = fileList.size();

  // Set Audio Paramteres
  audio.forceMono(false);   
  audio.setVolume(20);


  // Play the first song
  if (!fileList.empty()) {
    Serial.printf("[Audio] Attempting to play %s...\n", fileList[currentSongIndex].c_str());
    bool ok = audio.connecttoSD(fileList[currentSongIndex].c_str());
    if (!ok) {
      Serial.println("[Audio] Could not open file!");
    }
  } else {
    Serial.println("[SD] No audio files found.");
  }
}

void loop() {
  audio.loop();
}

// Optional debug callbacks:
// void audio_info(const char *info) {
//   Serial.print("[Audio Info] ");
//   Serial.println(info);
// }

