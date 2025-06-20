#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <SD.h>
#include <vector>
#include <driver/i2s.h>

#include "Audio.h"
#include "FS.h"
#include "lvgl.h"
#include "ui.h"
#include "screens.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/************************* AUDIO ******************************/
Audio audio(false, I2S_DAC_CHANNEL_DISABLE, I2S_NUM_0);  // External DAC

#define SD_CS_PIN 5  // SD card CS pin

SPIClass sdSPI(HSPI);  // Use HSPI for the SD card to isolate from VSPI (display + touch)

std::vector<String> fileList;
int currentSongIndex = 0;
int playlistSize = 0;

// 1-byte commands for audio control
enum AudioCommand : uint8_t {
  CMD_PLAY_PAUSE,
  CMD_NEXT,
  CMD_PREV,
  CMD_VOL_UP,
  CMD_VOL_DOWN
};

// Queue for audio commands 
QueueHandle_t audioCmdQ = nullptr;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels, std::vector<String> &fileList) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open directory or not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) fileList.push_back(String(file.name()));
    file = root.openNextFile();
  }
}

// End of mp3
void audio_eof_mp3(const char* info) {
  Serial.println("[Audio] Song finished!");
  currentSongIndex++;
  if (currentSongIndex < playlistSize) {
    Serial.printf("[Audio] Attempting to play %s...\n", fileList[currentSongIndex].c_str());
    bool ok = audio.connecttoSD(fileList[currentSongIndex].c_str());
    if (!ok) Serial.println("[Audio] Could not open file!");
  } else {
    Serial.println("[Audio] No more songs in the list.");
  }
}

/*********************** TOUCHSCREEN ********************************/
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

// Create a display buffer for LVGL
enum { SCREENBUFFER_SIZE_PIXELS = screenWidth * screenHeight / 10 };
static lv_color_t buf[SCREENBUFFER_SIZE_PIXELS];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight);
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass tsSpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
uint16_t touchScreenMinimumX = 150, touchScreenMaximumX = 3700, touchScreenMinimumY = 300, touchScreenMaximumY = 3900;

/* Display flushing */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  if (LV_COLOR_16_SWAP) {
    size_t len = lv_area_get_size(area);
    lv_draw_sw_rgb565_swap(pixelmap, len);
  }
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t*)pixelmap, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

/* Read touchscreen input */
void my_touch_read(lv_indev_t *indev_drv, lv_indev_data_t *data) {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();

    // Offset to shift the touch input for fine tuning
    const int16_t xOffset = 200; 
    p.x -= xOffset;

    // Ensure the adjusted value stays within bounds
    if (p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
    if (p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
    if (p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
    if (p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;

    data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 1, screenWidth);
    data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 1, screenHeight);
    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

/*Set tick routine needed for LVGL internal timings*/
static uint32_t my_tick_get_cb(void) { return millis(); }


static int counter = 0;

extern "C" {
  // Button event callbacks
  void action_volume_up(lv_event_t *e) {
    counter++;
    char counter_text[16];
    snprintf(counter_text, sizeof(counter_text), "Count: %d", counter);
    lv_label_set_text(objects.song_title, counter_text);

    //audio.setVolume(min(22, audio.getVolume() + 1));
    AudioCommand cmd = CMD_VOL_UP;
    xQueueSend(audioCmdQ, &cmd, /*ticksToWait=*/ 0);
  }

  void action_volume_down(lv_event_t *e) {
    counter--;
    char counter_text[16];
    snprintf(counter_text, sizeof(counter_text), "Count: %d", counter);
    lv_label_set_text(objects.song_title, counter_text);

    //audio.setVolume(max(0, audio.getVolume() - 1));
    AudioCommand cmd = CMD_VOL_DOWN;
    xQueueSend(audioCmdQ, &cmd, 0);
  }

  void action_next_song(lv_event_t * e){
  
  };

  void action_play_pause_song(lv_event_t * e){

  };

  void action_prev_song(lv_event_t * e){

  };
}

/*********************** FREERTOS ********************************/
void guiTask(void *pv) {
  for (;;) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void audioTask(void* pv) {
  AudioCommand cmd;
  for (;;) {
    
    // 1) Check if there’s any command waiting
    while (xQueueReceive(audioCmdQ, &cmd, 0) == pdTRUE) {
      switch (cmd) {
        case CMD_PLAY_PAUSE:
          // replace with actual audio play/pause calls
          
          break;
        case CMD_NEXT:
          // reuse existing end-of-file handler
          
          break;
        case CMD_PREV:
          
          break;
        case CMD_VOL_UP:
          audio.setVolume(min(22, audio.getVolume() + 1));
          break;
        case CMD_VOL_DOWN:
          audio.setVolume(max(0, audio.getVolume() - 1));
          break;
      }
    }

    // 2) Always keep the audio engine pumping
    audio.loop();

    // 3) Yield for a bit
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}


/*********************** SETUP ********************************/
void setup() {
  Serial.begin(115200);
  lv_init();

  // Initialize touch (VSPI)
  tsSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(tsSpi);
  ts.setRotation(3);

  // Initialize display (VSPI)
  tft.begin();
  tft.setRotation(3);

  // Setup LVGL display buffer
  static lv_disp_t *disp = lv_display_create(screenWidth, screenHeight);
  lv_display_set_buffers(
    disp,
    buf, nullptr,
    SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t),
    LV_DISPLAY_RENDER_MODE_PARTIAL
  );
  lv_display_set_flush_cb(disp, my_disp_flush);

  // Setup touch input
  lv_indev_t *touch_indev = lv_indev_create();
  lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(touch_indev, my_touch_read);

  lv_tick_set_cb(my_tick_get_cb);

  ui_init();
  Serial.println("Screen Setup done");

  delay(5000);
  Serial.println("Starting Audio...");
  delay(1000);

  /*
  i2s_pin_config_t myPins = {
    .bck_io_num = 4,     // BCLK (Bit Clock)
    .ws_io_num = 22,     // LRCK (Left/Right Clock), also called WS (Word Select)
    .data_out_num = 27,  // DATA Out
    .data_in_num = -1    // Not used
  };
  i2s_set_pin(I2S_NUM_0, &myPins); // Set the I2S pins for I2S_NUM_0
  */

  // Initialize HSPI for SD card
  sdSPI.begin(18, 19, 23, SD_CS_PIN);  //replace with defines

  // Mount SD on HSPI
  if (!SD.begin(SD_CS_PIN, sdSPI, 25000000)) {
    Serial.println("[SD] Card mount failed on HSPI!");
    while (true) { delay(1000); }
  }

  listDir(SD, "/", 0, fileList);
  Serial.println("Files found:");
  for (const auto &fileName : fileList) Serial.println(fileName);

  playlistSize = fileList.size();
  audio.forceMono(false);
  // Set I2S Pins: BCLK (Bit Clock), LRCK (Left/Right Clock), also called WS (Word Select), DATA Out
  audio.setPinout(4, 22, 27); // replace with defines
  audio.setVolume(1);

  if (!fileList.empty()) {
    Serial.printf("[Audio] Attempting to play %s...\n", fileList[currentSongIndex].c_str());
    bool ok = audio.connecttoSD(fileList[currentSongIndex].c_str());
    if (!ok) Serial.println("[Audio] Could not open file!");
  } else {
    Serial.println("[SD] No audio files found.");
  }

  audioCmdQ = xQueueCreate(10, sizeof(AudioCommand));
  if (!audioCmdQ) {
    Serial.println("Audio command queue creation failed!");
    // Stop here to avoid running without a queue
    while (true) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  // Create GUI task pinned to core 1
  xTaskCreatePinnedToCore(
    guiTask,          // entry function
    "LVGL",           // debug name
    8 * 1024,         // 8 KB stack, recomended minimum by LVGL 
    nullptr,          // no parameters
    1,                // priority 1
    nullptr,          // on’t need the task handle
    1                 // pin to CPU core 1
  );

  // Create Audio task pinned to core 0
  xTaskCreatePinnedToCore(
    audioTask,
    "Audio",
    4096,
    nullptr,
    2,
    nullptr,
    0                 
  );
}

void loop() {
  // Tasks are running in the background
}
