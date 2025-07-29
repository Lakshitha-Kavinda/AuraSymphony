#include <FastLED.h>

#define NUM_LEDS 288
#define DATA_PIN 5
#define WIDTH 18
#define HEIGHT 16
#define NUM_BANDS 18
#define SERIAL_TIMEOUT 100 // ms
#define SMOOTHING_FACTOR 0.3
#define INACTIVITY_TIMEOUT 500 // ms

CRGB leds[NUM_LEDS];
float bandValues[NUM_BANDS];
float prevBandValues[NUM_BANDS] = {0};
uint32_t lastUpdate = 0;
uint8_t history[NUM_BANDS][HEIGHT] = {0}; // Store brightness history

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(64);
  FastLED.clear();
  FastLED.show();
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
        drawWaterfall();
        lastUpdate = millis();
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
    memset(history, 0, sizeof(history)); // Clear history
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
    bandValues[i] = SMOOTHING_FACTOR * prevBandValues[i] + (1 - SMOOTHING_FACTOR) * values[i];
    prevBandValues[i] = bandValues[i];
    if (bandValues[i] > 1) significant = true;
  }
  return significant;
}

void drawWaterfall() {
  // Shift history down
  for (int band = 0; band < NUM_BANDS; band++) {
    for (int h = 0; h < HEIGHT - 1; h++) {
      history[band][h] = history[band][h + 1];
      history[band][h] = max(0, history[band][h] - 20); // Fade as it falls
    }
    // Add new value at the top
    int height = min((int)bandValues[band], HEIGHT);
    history[band][HEIGHT - 1] = map(height, 0, HEIGHT, 0, 255);
  }

  // Draw the grid
  FastLED.clear();
  for (int band = 0; band < NUM_BANDS; band++) {
    for (int h = 0; h < HEIGHT; h++) {
      if (history[band][h] > 0) {
        int ledIndex = XY(band, h);
        uint8_t hue = map(h, 0, HEIGHT - 1, 0, 96); // Red to blue (warm to cool)
        leds[ledIndex] = CHSV(hue, 255, history[band][h]);
      }
    }
  }
  FastLED.show();
  Serial.println("OK");
}