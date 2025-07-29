#include <FastLED.h>

// LED Matrix Configuration
#define LED_PIN     5
#define NUM_LEDS    288  // 18x16 grid
#define WIDTH       18
#define HEIGHT      16
#define BRIGHTNESS  100

// Audio Configuration
#define NUM_BANDS   18
#define SMOOTHING   0.3
#define INACTIVITY_TIMEOUT 500 // ms

CRGB leds[NUM_LEDS];
float bandValues[NUM_BANDS] = {0};
float prevBandValues[NUM_BANDS] = {0};
uint32_t lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  testColors();
  Serial.println("Fire Effect Visualizer Ready");
}

void testColors() {
  FastLED.clear();
  for (int x = 0; x < WIDTH; x++) {
    int ledIndex = XY(x, 0);
    uint8_t hue;
    if (x <= 5) {
      hue = 85; // Green for low frequencies (bands 0-5)
    } else if (x <= 11) {
      hue = map(x, 6, 11, 170, 85); // Blue to green (bands 6-11)
    } else {
      hue = map(x, 12, WIDTH - 1, 85, 0); // Green to red (bands 12-17)
    }
    leds[ledIndex] = CHSV(hue, 255, 255);
  }
  FastLED.show();
  delay(2000);
}

int XY(int x, int y) {
  if (y % 2 == 0) {
    return y * WIDTH + x;
  } else {
    return y * WIDTH + (WIDTH - 1 - x);
  }
}

void loop() {
  static char buffer[64];
  static uint8_t bufIdx = 0;

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' && bufIdx > 2 && buffer[0] == 'S') {
      buffer[bufIdx] = '\0';
      if (parseInput(buffer + 2)) {
        drawFire();
        FastLED.show();
        lastUpdate = millis();
        Serial.println("OK");
      } else {
        Serial.println("ERROR");
      }
      bufIdx = 0;
    } else if (bufIdx < sizeof(buffer) - 1) {
      buffer[bufIdx++] = c;
    } else {
      bufIdx = 0;
    }
  }

  if (millis() - lastUpdate > INACTIVITY_TIMEOUT && lastUpdate > 0) {
    FastLED.clear();
    FastLED.show();
    Serial.println("Silence detected, clearing display");
  }
}

bool parseInput(const char* input) {
  int values[NUM_BANDS];
  int count = 0;
  char* ptr = (char*)input;
  char* token;

  while ((token = strtok_r(ptr, ",", &ptr)) && count < NUM_BANDS) {
    int val = atoi(token);
    if (val >= 0 && val <= HEIGHT) {
      values[count++] = val;
    } else {
      return false;
    }
  }

  if (count != NUM_BANDS) {
    return false;
  }

  bool significant = false;
  for (int i = 0; i < NUM_BANDS; i++) {
    bandValues[i] = SMOOTHING * prevBandValues[i] + (1 - SMOOTHING) * values[i];
    if (bandValues[i] > 1) significant = true;
    prevBandValues[i] = bandValues[i];
  }
  return significant;
}

void drawFire() {
  FastLED.clear();
  for (int x = 0; x < WIDTH; x++) {
    int height = min((int)bandValues[x], HEIGHT);
    uint8_t hue;
    if (x <= 5) {
      hue = 85; // Green for low frequencies (bands 0-5)
    } else if (x <= 11) {
      hue = map(x, 6, 11, 170, 85); // Blue to green (bands 6-11)
    } else {
      hue = map(x, 12, WIDTH - 1, 85, 0); // Green to red (bands 12-17)
    }
    for (int y = 0; y < HEIGHT; y++) {
      int noise = random(0, 10);
      int fireHeight = height + noise - 5;
      if (y <= fireHeight) {
        int ledIndex = XY(x, y);
        uint8_t brightness = map(y, 0, fireHeight, 64, 255);
        if (y > height) brightness = brightness / 2; // Flicker at the top
        leds[ledIndex] = CHSV(hue, 255, brightness);
      }
    }
  }
}