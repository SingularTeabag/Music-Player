#include <SPI.h>
#include <Wire.h>

//SD card
#include <FS.h>
#include <SD.h>
#define SD_CS 5

//I2C Screen
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



void setup() {
  Serial.begin(115200);

  //Display Initialization
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  Serial.println("Initialized Screen");

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.clearDisplay();
  display.display();

  //SD Initialization
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Mount Failed!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("No SD Card");
    display.display();
    for (;;)
      ;
  } else
    listFiles("/");
}

void loop() {
}

void listFiles(const char *dirname) {
  File root = SD.open(dirname);
  // Serial.println("Entered");
  if (!root) {
    return;
  }

  //File entry = root.openNextFile();

  while (true) {
    File entry = root.openNextFile();

    if (!entry) {
      // No more files or directories
      Serial.println("No more files");
      break;
    }

    if (strcmp(entry.name(), "System Volume Information") == 0)
      entry = root.openNextFile();

    //display.setTextColor(BLACK, WHITE); //Select
    Serial.print(entry.name());
    display.println(entry.name());
    display.display();

    if (entry.isDirectory()) {

      Serial.println(" - Directory");

      char concat[128];

      snprintf(concat, sizeof(concat), "%s/%s", dirname, entry.name());

      Serial.println(concat);
      listFiles(concat);
    } else
      Serial.println(" - File ");

    entry.close();
  }
}
