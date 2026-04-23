//libraries
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <String.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

//Audio
#include "AudioFileSourceSD.h"
#include "AudioGeneratorFLAC.h"
#include "AudioOutputI2S.h"
#define VOL_STEP 0.005

//fonts
#include "FreeSerifItalic18pt7b.h"
#include "FreeMonoBold24pt7b.h"
#include "FreeSansBoldOblique12pt7b.h"

//OLED Screen variables
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1
#define IIC_ADRESS 0x3c
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

//SD
#define SD_CS 5

//Button GPIO
#define BTN_UP 14
#define BTN_SEL 27
#define BTN_DOWN 26
#define BTN_VOL_UP 12
#define BTN_VOL_DOWN 13

//Colors
#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK

//Short song list
char* menuItems[] = {
    "Songs For The Deaf - Queens Of The Stone Age",
    "The Real Song For The Deaf",
    "You think I Ain't Worth A Dollar, But I Feel Like A Millionaire",
    "No One Knows",
    "First It Giveth",
    "Songs For The Dead",
    "The Sky Is Fallin'"
};

//Full songlist
char* fullMenu[] = {
    "Songs For The Deaf - Queens Of The Stone Age",
    "The Real Song For The Deaf",
    "You think I Ain't Worth A Dollar, But I Feel Like A Millionaire",
    "No One Knows",
    "First It Giveth",
    "Songs For The Dead",
    "The Sky Is Fallin'",
    "Six Shooter",
    "Hanging Tree",
    "Go With The Flow",
    "Gonna Lieave You",
    "Do It Again",
    "God Is In The Radio",
    "Another Love Song",
    "Songs For The Deaf",
    "Mosquito Song"
};

//menu
int menuSelection = 0;
int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
int fullMenuLength = sizeof(fullMenu) / sizeof(fullMenu[0]);
int menuOfset = 0;

//menu text scrolling
int scrollOffset = 1;
int scrollDir = -1;
unsigned long lastScrollTime = 0;

//screen sleep
unsigned long lastInput = 0;
bool isScreenActive = false;

//Audio Stuff
bool isMusicPlaying = false;
float volume = 0.02; 
AudioGeneratorFLAC *flacAudio;
AudioFileSourceSD *musicFile;
AudioOutputI2S *output;

/* Todo 
    - Fully make menu working
        - Set both related arrays to empty
        - Finish sd card scaning method
            - have it save to either a) esp32 onboard txt document or b) txt document on SD card
                -probably sd card
        - Have a manual sd scan option
        - SD scan when txt document doesnt exist 
    - Rapid scrolling when button held
    - Make music playing work
        - Make sure that you cant play an album title
    - Now playing Screen
        - Song title
        - Artist 
        - Set Select to play / pause
        - Left & Right to Skip & Rewind 
        - Put volume indicator here
    - Possibly remeber volume and song
        - Stored Locally 
    - Album Autoplay 
    - looping 
    - shuffling 

*/


void setup() {
    Serial.begin(115200);

    //Initialize OLED screen
    if(!display.begin(SSD1306_SWITCHCAPVCC, IIC_ADRESS)) {
        Serial.println(F("OLED failed"));
        while(true)
            ;
    }

    //Screen setup
    isScreenActive = true;
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.clearDisplay();
    splashScreen();
    display.display();

    //Audio Stuff
    output = new AudioOutputI2S();
    output -> SetPinout(25, 32, 33); //WSEL - 32, DIN - 33, BCLK - 25
    output -> SetBuffers(16, 2304); //Crisp hifi audio, huzza!
    output -> SetGain(volume);
    flacAudio = new AudioGeneratorFLAC(); // flac decoder, flacAudio -> (musicFile (musicFile must be a new AudioFileSourceSD{Remember to delete the prior music data}), AUDIO_DESTINATION)

    //button Setup
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_SEL, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_VOL_UP, INPUT_PULLUP);
    pinMode(BTN_VOL_DOWN, INPUT_PULLUP);

    //Spash Screen delay
    delay(4000);

    //SD Initialization
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Card Mount Failed!");
        missingSD();
        while(true){
            if(SD.begin(SD_CS))
                break;
        }
    }

    //menu
    display.clearDisplay();
    lastScrollTime = millis();
    printMenu();
    display.display();
}

void loop() {
    validateSD();
    printMenu();
    buttonUp();
    buttonDown();
    buttonSelect();
    buttonVolUp();
    buttonVolDown();
    screenSleep();
}

void screenSleep() {
    if(millis() - lastInput > 60000) {
        isScreenActive = false;
        display.clearDisplay();
    }
    else 
        isScreenActive = true;
}

//Makes sure that the sd card is still there 
void validateSD() {
    if (!SD.open("/")) {
        Serial.println("SD Card removed!");
        missingSD();
        while(true){
            lastInput = millis();
            if(SD.open("/"))
                break;
        }
    }
    display.clearDisplay();
    printMenu();
}

//scrolls text that is too long to display, > 21 char
void scrollText(const char* text) {
    if(millis() - lastScrollTime > 25) {
        if(scrollOffset == 1) {
            if(millis() - lastScrollTime > 1500){
                scrollDir = -1;
                scrollOffset += scrollDir;
                lastScrollTime = millis();
            }
        }
        else if(-1 * scrollOffset >= ((strlen(text) * 5) + strlen(text) + 1) - 128)
            scrollDir = 1;
            
            if(millis() - lastScrollTime > 1500){
                scrollOffset += scrollDir;
                lastScrollTime = millis();
            }
        else if(scrollOffset != 1 && !(-1 * scrollOffset >= ((strlen(text) * 5) + strlen(text) + 1) - 128)) {
            //Serial.println("scroll");
            scrollOffset += scrollDir;
            lastScrollTime = millis();
        }
    }
}

//Highlighted Text display
void printSelect(int height, const char* text, int scrollOffset) {
    display.fillRect(0, height, (strlen(text) * 5 + strlen(text) + 1), 9, WHITE);

    display.setTextColor(BLACK);
    display.setCursor(scrollOffset, height + 1);

    display.print(text);
}

//Normal Text display
void printNormal(int height, const char* text) {
    display.setTextColor(WHITE);
    display.setCursor(1, height);

    //truncates song titles that are too long 
    if(strlen(text) > 21) {
        for(int i = 0; i < 21 && text[i] != '\0'; i++) {
            display.write(text[i]);
        }
    }
    else
        display.print(text);
}

//prints the menu that the users sees (star of the show)
void printMenu(){
    int textHeight = 4;

    //menu shift up
    if(menuSelection < 0) {
        menuSelection = 0;
        shiftMenuUp();
    }

    //menu shift down
    if(menuSelection > menuLength - 1) {
        menuSelection = menuLength - 1;
        shiftMenuDown();
    }

    //display the menu itself 
    display.clearDisplay();
    if(isScreenActive) {
        for(int i = 0; i < menuLength; i++) {
            //Serial.println(menuItems[i]);

            //is selected?
            if(i == menuSelection){
                if(strlen(menuItems[i]) > 21)
                    scrollText(menuItems[i]);
                printSelect(textHeight, menuItems[i], scrollOffset);
                textHeight += 10;
            }
            else {
                printNormal(textHeight, menuItems[i]); 
                textHeight += 8;
            }
        }
        
    }
    else 
        display.clearDisplay();
    display.display();
}

//Shifts the entire menuItems array down (technically up)
void shiftMenuDown() {
    if(menuOfset + menuLength < fullMenuLength) {
        for(int i = 0; i < menuLength - 1; i++) {
            menuItems[i] = menuItems[i + 1];
        }
    menuItems[6] = fullMenu[menuOfset + menuLength];
    menuOfset += 1;
    }
}

//Shifts the entire menuItems array up (technically down)
void shiftMenuUp() {
    if(menuOfset > 0) {
        for(int i = menuLength - 1; i > 0; i--) {
            menuItems[i] = menuItems[i - 1];
        }
    menuItems[0] = fullMenu[menuOfset - 1];
    menuOfset -= 1;
    }
}

//Debounces the up button
void buttonUp() {
    static bool prevState = HIGH;
    static unsigned long prevTime = 0;

    bool currentState = digitalRead(BTN_UP);
    unsigned long now = millis();

    if(prevState == HIGH && currentState == LOW && now - prevTime > 150) {
    
        //Serial.println("Up");
        menuSelection -= 1;
        scrollOffset = 1;
        lastScrollTime = millis();
        lastInput = millis();
        printMenu();

        prevTime = now;
    }

    prevState = currentState;
}

//Debounces the down button
void buttonDown() {
    static bool prevState = HIGH;
    static unsigned long prevTime = 0;

    bool currentState = digitalRead(BTN_DOWN);
    unsigned long now = millis();

    if(prevState == HIGH && currentState == LOW && now - prevTime > 130) {
        //Serial.println("Down");
        menuSelection += 1;
        scrollOffset = 1;
        lastScrollTime = millis();
        lastInput = millis();
        printMenu();

        prevTime = now;
  }

    prevState = currentState;
}

//Debounces the select button
void buttonSelect() {
    static bool prevState = HIGH;
    static unsigned long prevTime = 0;

    bool currentState = digitalRead(BTN_SEL);
    unsigned long now = millis();

    if(prevState == HIGH && currentState == LOW && now - prevTime > 130) {
        //Serial.println(menuItems[menuSelection]);

        loadMusic("/music2.flac"); //test file
        playMusic();

        lastInput = millis();

        prevTime = now;
    }

    prevState = currentState;
}

//Debounces the volume up button
void buttonVolUp() {
    static bool prevState = HIGH;
    static unsigned long prevTime = 0;

    bool currentState = digitalRead(BTN_VOL_UP);
    unsigned long now = millis();

    if(prevState == HIGH && currentState == LOW && now - prevTime > 130) {
        if(volume > 0.1)
            volume = 0.1;
        else 
            volume += VOL_STEP;
        output -> SetGain(volume);
        Serial.println(volume);
        lastInput = millis();

        prevTime = now;
    }

    prevState = currentState;
}

//Debounces the volume down button
void buttonVolDown() {
    static bool prevState = HIGH;
    static unsigned long prevTime = 0;

    bool currentState = digitalRead(BTN_VOL_DOWN);
    unsigned long now = millis();

    if(prevState == HIGH && currentState == LOW && now - prevTime > 130) {\
        if(volume < 0.02)
            volume = 0.02;
        else 
            volume -= VOL_STEP;
        output -> SetGain(volume);
        Serial.println(volume);
        lastInput = millis();

        prevTime = now;
    }

    prevState = currentState;
}

void loadMusic(const char *file) {
    if(flacAudio -> isRunning())
        flacAudio -> stop();

    if(flacAudio) {
        delete flacAudio;
        flacAudio = NULL;
    }

    if(musicFile) {
        delete musicFile;
        musicFile = NULL;
    }

    musicFile = new AudioFileSourceSD(file);

    flacAudio = new AudioGeneratorFLAC();
}

void playMusic() {
    isMusicPlaying = true;
    flacAudio -> begin(musicFile, output);
}

void scanMusic() {
    scanningFiles();
}

void listFiles(const char *dirName) {

    File root = SD.open(dirName);
    
    //if no more files or directories
    if(!root)
        return;
    

    while(true){
        File entry = root.openNextFile();

        //no more files
        if(!entry)
            break;

        
    }
}

//lopaka.app screens
void splashScreen() {
    //Thanks to this one website that i unfortunelty forgot :(
    //I REMEBER, its lopaka.app theres some paid features (subscription boooo) but its all good cuz you can make up to 6 screens at a time and reuse screens to we chill
    display.clearDisplay();

    static const unsigned char PROGMEM image_espressif_systems_logo_png_seeklogo_407805_407806_bits[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0x00,0x00,0x10,0x7f,0x00,0x02,0x3f,0x1f,0x80,0x06,0x7f,0xc7,0xe0,0x04,0xff,0xf3,0xf0,0x0c,0xff,0xf9,0xf0,0x18,0x07,0xfc,0xf8,0x18,0xe0,0xfe,0x7c,0x13,0xfe,0x3f,0x3c,0x37,0xff,0x1f,0x9c,0x37,0xff,0xcf,0xce,0x2f,0xff,0xe7,0xee,0x2f,0x87,0xf3,0xe6,0x2f,0xe3,0xf9,0xf6,0x27,0xf8,0xf9,0xf2,0x37,0xfe,0x7c,0xf0,0x33,0xff,0x7c,0xf8,0x10,0x7f,0x3e,0xf8,0x10,0x1f,0xbe,0x78,0x18,0x4f,0x9e,0x78,0x08,0xe7,0x9e,0x78,0x0d,0xf7,0xdf,0x70,0x06,0xe7,0xdf,0x00,0x03,0x47,0xdf,0x00,0x01,0x87,0x9e,0x18,0x00,0xc7,0x9e,0x30,0x00,0x70,0x00,0xc0,0x00,0x1e,0x0f,0x80,0x00,0x03,0xfc,0x00,0x00,0x00,0x00,0x00};

    static const unsigned char PROGMEM image_Layer_28_bits[] = {0x7f,0xff,0xff,0xff,0xff,0xc0,0x80,0x00,0x00,0x00,0x00,0x20,0x80,0x00,0x00,0x00,0x00,0x20,0x8f,0xff,0xff,0xff,0xfe,0x20,0x90,0x00,0x00,0x00,0x01,0x20,0x90,0xff,0xff,0xff,0xe1,0x20,0x91,0x04,0x00,0x04,0x11,0x20,0x92,0x03,0xff,0xf8,0x09,0x20,0x94,0x71,0xff,0xf1,0xc5,0x20,0x94,0x89,0xff,0xf2,0x25,0x20,0x94,0x89,0xff,0xf2,0x25,0x20,0x94,0x89,0xff,0xf2,0x25,0x20,0x94,0x71,0xff,0xf1,0xc5,0x20,0x92,0x03,0xff,0xf8,0x09,0x20,0x91,0x04,0x00,0x04,0x11,0x20,0x90,0xff,0xff,0xff,0xe1,0x20,0x90,0x00,0x00,0x00,0x01,0x20,0x8f,0xff,0xff,0xff,0xfe,0x20,0x80,0x00,0x00,0x00,0x00,0x20,0x80,0x0f,0xff,0xfe,0x00,0x20,0x80,0x10,0x00,0x01,0x00,0x20,0x80,0x27,0xff,0xfc,0x80,0x20,0x80,0x4b,0xf1,0xfa,0x40,0x20,0x7f,0x9f,0xff,0xff,0x3f,0xc0};


    display.setTextColor(1);
    display.setTextWrap(false);
    display.setFont(&FreeSerifItalic18pt7b);
    display.setCursor(36, 55);
    display.print("Player");

    display.drawBitmap(3, 29, image_espressif_systems_logo_png_seeklogo_407805_407806_bits, 32, 32, 1);

    display.setCursor(2, 24);
    display.print("ES");

    display.drawBitmap(64, 2, image_Layer_28_bits, 43, 24, 1);

    display.display();

    display.setFont(NULL);
}

void missingSD() {
    display.clearDisplay();

    static const unsigned char PROGMEM image_SDcardFail_bits[] = {0xff,0xe0,0xed,0xe0,0xff,0xe0,0xe1,0xe0,0xde,0xe0,0xff,0xe0,0xff,0xe0,0xe6,0x00};

    display.drawBitmap(3, 2, image_SDcardFail_bits, 11, 8, 1);

    display.setTextColor(1);
    display.setTextWrap(false);
    display.setFont(&FreeMonoBold24pt7b);
    display.setCursor(36, 29);
    display.print("SD");

    display.setFont(&FreeSansBoldOblique12pt7b);
    display.setCursor(19, 56);
    display.print("Missing");

    display.drawBitmap(115, 2, image_SDcardFail_bits, 11, 8, 1);

    display.drawBitmap(115, 54, image_SDcardFail_bits, 11, 8, 1);

    display.drawBitmap(3, 54, image_SDcardFail_bits, 11, 8, 1);

    display.display();

    display.setFont(NULL);
}

void scanningFiles() {
    display.clearDisplay();

    static const unsigned char PROGMEM image_folder_open_file_bits[] = {0x00,0x00,0x00,0x7c,0x00,0x00,0x83,0xfc,0x00,0x80,0x02,0x00,0x80,0x02,0x00,0x8f,0xff,0x00,0x90,0x00,0x80,0x97,0xff,0xc0,0xa4,0x00,0x40,0xa8,0x00,0x40,0xc8,0x00,    0x80,0xd0,0x00,0x80,0x90,0x01,0x00,0xa0,0x01,0x00,0x7f,0xfe,0x00,0x00,0x00,0x00};


    display.setTextColor(1);
    display.setTextWrap(false);
    display.setFont(&FreeSerifItalic18pt7b);
    display.setCursor(3, 24);
    display.print("Scanning");

    display.setCursor(32, 54);
    display.print("Files");

    display.drawBitmap(7, 36, image_folder_open_file_bits, 18, 16, 1);

    display.drawBitmap(105, 36, image_folder_open_file_bits, 18, 16, 1);

    display.display();
}