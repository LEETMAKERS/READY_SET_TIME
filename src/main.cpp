#include <Arduino.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW 
#define MAX_DEVICES   4 

#define CLK_PIN    13 
#define DATA_PIN   11 
#define CS_PIN     10 
#define BUTTON_PIN 6
#define BUZZER_PIN 5

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

enum State { RESET, RUNNING, STOPPED };
State currentState = RESET;

unsigned long startTime = 0;
unsigned long elapsedTime = 0;
unsigned long previousMillis = 0;
const long updateInterval = 30; // ~33 FPS is smooth and prevents SPI bus saturation

char currentTimeStr[10] = " 00.00 ";
char lastTimeStr[10]    = "";

// Button debouncing variables
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Shorter durations (10-25ms) make the active buzzer sound much quieter and crisper
void quietChirp(unsigned long durationMs) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(durationMs);
  digitalWrite(BUZZER_PIN, LOW);
}

void formatTime(unsigned long durationMs, char* outBuf) {
  unsigned long totalSeconds = (durationMs / 1000) % 100; // Keep to 2 digits (00-99)
  unsigned int hundredths = (durationMs % 1000) / 10;     // 2 digits (00-99)
  
  // Padded with spaces on both sides to lock alignment across the 32 columns
  sprintf(outBuf, " %02lu.%02u ", totalSeconds, hundredths);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  myDisplay.begin();
  myDisplay.setIntensity(2); // Moderate brightness
  // myDisplay.setInvert(true);
  myDisplay.displayClear();

  // Draw initial state
  formatTime(0, currentTimeStr);
  myDisplay.print(currentTimeStr);
  strcpy(lastTimeStr, currentTimeStr);
}

void loop() {
  // --- Button Handling ---
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    static bool buttonProcessed = false;
    
    if (reading == LOW && !buttonProcessed) {
      buttonProcessed = true;

      if (currentState == RESET) {
        startTime = millis();
        currentState = RUNNING;
        quietChirp(60); // Short tick on start
      } 
      else if (currentState == RUNNING) {
        elapsedTime = millis() - startTime;
        currentState = STOPPED;
        quietChirp(60); // Slight click on stop
      } 
      else if (currentState == STOPPED) {
        elapsedTime = 0;
        currentState = RESET;
        formatTime(0, currentTimeStr);
        myDisplay.print(currentTimeStr);
        strcpy(lastTimeStr, currentTimeStr);

        quietChirp(15);
        delay(120);
        quietChirp(15);
      }
    } 
    else if (reading == HIGH) {
      buttonProcessed = false;
    }
  }
  lastButtonState = reading;

  // --- Display Update (Only when changed) ---
  if (currentState == RUNNING) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= updateInterval) {
      previousMillis = currentMillis;
      elapsedTime = currentMillis - startTime;

      formatTime(elapsedTime, currentTimeStr);

      // Only push to display if the string content actually changed
      if (strcmp(currentTimeStr, lastTimeStr) != 0) {
        myDisplay.print(currentTimeStr); // Direct write: no clear, zero flicker
        strcpy(lastTimeStr, currentTimeStr);
      }
    }
  }
}