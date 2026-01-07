#define BTN_UP 14
#define BTN_SEL 27
#define BTN_DOWN 26

void setup() {
  Serial.begin(115200);
  Serial.println("Testing");
}

void loop() {
  buttonUp();
}

//Debounces the up button
void buttonUp() {
    static bool prevState = HIGH;
    static unsigned long prevTime = 0;

    bool currentState = digitalRead(BTN_UP);
    unsigned long now = millis();

    if(prevState == HIGH && currentState == LOW && now - prevTime > 150) {
      int test = 0;
      Serial.println("Up Pressed");

      while(currentState == LOW) {
        currentState = digitalRead(BTN_UP);
        unsigned long heldTime = (millis() - now);

        if(heldTime > 450 && ((heldTime - 450) % 1000) == 0) {
          Serial.print("Up Held ");
          Serial.println(test);
          test += 1;
        }
      }

      prevTime = now;
    }

    prevState = currentState;
}