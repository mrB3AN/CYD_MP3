#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <SD.h>
#include <vector>

#include "Audio.h"
#include "FS.h"
#include "lvgl.h"
#include "ui.h"
#include "screens.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/************************* AUDIO ******************************/
Audio audio(true, I2S_DAC_CHANNEL_LEFT_EN);
#define SD_CS_PIN 5
// Use HSPI for the SD card to isolate from VSPI (display + touch)
SPIClass sdSPI(HSPI);

std::vector<String> fileList;
int currentSongIndex = 0;
int playlistSize = 0;

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
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

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

/*========== Read touchscreen input ==========*/
void my_touch_read(lv_indev_t *indev_drv, lv_indev_data_t *data) {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
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

// Button event callbacks
void next_button_event_cb(lv_event_t *e) {
  counter++;
  char counter_text[16];
  snprintf(counter_text, sizeof(counter_text), "Count: %d", counter);
  lv_label_set_text(objects.song_label, counter_text);
}

void prev_button_event_cb(lv_event_t *e) {
  counter--;
  char counter_text[16];
  snprintf(counter_text, sizeof(counter_text), "Count: %d", counter);
  lv_label_set_text(objects.song_label, counter_text);
}


/*********************** FREERTOS ********************************/
void guiTask(void *pv) {
  for (;;) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void audioTask(void *pv) {
  for (;;) {
    audio.loop();
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
  Serial.println("Starting...");
  delay(1000);

  // Initialize HSPI for SD card
  sdSPI.begin(18, 19, 23, SD_CS_PIN);

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
  audio.setVolume(1);

  if (!fileList.empty()) {
    Serial.printf("[Audio] Attempting to play %s...\n", fileList[currentSongIndex].c_str());
    bool ok = audio.connecttoSD(fileList[currentSongIndex].c_str());
    if (!ok) Serial.println("[Audio] Could not open file!");
  } else {
    Serial.println("[SD] No audio files found.");
  }

  // Create GUI task pinned to core 1
  xTaskCreatePinnedToCore(
    guiTask,          // entry function
    "LVGL",           // debug name
    8 * 1024,         // 8 KB stack 
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
  // Idle; tasks handle GUI and audio
  vTaskDelay(pdMS_TO_TICKS(1000));
}
