//#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <String.h>

//OLED Screen variables
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1
#define IIC_ADRESS 0x3c
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

//Button GPIO
#define BTN_UP 14
#define BTN_SEL 27
#define BTN_DOWN 26

//Colors
#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK

//Short song list
const char* menuItems[] = {
    "Song 1",
    "Song 2",
    "Song 3",
    "Song 4",
    "Song 5",
    "Song 6",
    "Song 7"
};

//Full songlist
const char* fullMenu[] = {
    "Song 1",
    "Song 2",
    "Song 3",
    "Song 4",
    "Song 5",
    "Song 6",
    "Song 7",
    "Song 8",
    "Song 9",
    "Song 10",
    "Song 11"
};

int menuSelection = 0;
int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
int fullMenuLength = sizeof(fullMenu) / sizeof(fullMenu[0]);
int menuOfset = 0;

void setup() {
    Serial.begin(115200);

    //Initialize OLED screen
    if(!display.begin(SSD1306_SWITCHCAPVCC, IIC_ADRESS)) {
        Serial.println(F("OLED failed"));
        while(true);
    }

    //Screen setup
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);    
    printMenu();
    display.display();

    //button Setup
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_SEL, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
}

void loop() {
    buttonUp();
    buttonDown();
    buttonSelect();
}

//Normal Text display
void printNormal(int height, const char* text) {
    display.setTextColor(WHITE);
    display.setCursor(1, height);
    display.print(text);
}

//Highlighted Text display
void printSelect(int height, const char* text) {
    display.fillRect(0, height, (strlen(text) * 5 + strlen(text) + 1), 9, WHITE);

    display.setTextColor(BLACK);
    display.setCursor(1, height + 1);
    display.print(text);
}

//prints the menu that the users sees (star of the show)
void printMenu(){
    int textHeight = 4;

    if(menuSelection < 0) {
        menuSelection = 0;
        shiftMenuUp();
    }

    if(menuSelection > menuLength - 1) {
        menuSelection = menuLength - 1;
        shiftMenuDown();
    }

    display.clearDisplay();
    for(int i = 0; i < menuLength; i++) {
        //Serial.println(menuItems[i]);

        

        if(i == menuSelection){
            printSelect(textHeight, menuItems[i]);
            textHeight += 10;
        }
        else {
            printNormal(textHeight, menuItems[i]); 
            textHeight += 8;
        }
    }
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

    if(prevState == HIGH && currentState == LOW && now - prevTime > 150) {
    
        //Serial.println("Down");
        menuSelection += 1;
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

    if(prevState == HIGH && currentState == LOW && now - prevTime > 150) {
    
        Serial.println(menuItems[menuSelection]);

        prevTime = now;
  }

  prevState = currentState;
}