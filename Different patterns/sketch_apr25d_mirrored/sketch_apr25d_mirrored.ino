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
int peakValues[NUM_BANDS] = {0};
uint32_t peakTimes[NUM_BANDS] = {0};

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
        drawMirroredSpectrum();
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

void drawMirroredSpectrum() {
  FastLED.clear();
  for (int band = 0; band < NUM_BANDS; band++) {
    int height = min((int)bandValues[band], HEIGHT);
    // Scale height to half the grid (0–8)
    height = map(height, 0, HEIGHT, 0, 8);
    // Update peak
    if (height > peakValues[band]) {
      peakValues[band] = height;
      peakTimes[band] = millis();
    } else if (millis() - peakTimes[band] > 500) {
      peakValues[band] = max(0, peakValues[band] - 1);
    }
    // Draw bottom half (upward)
    for (int h = 0; h < height; h++) {
      int ledIndex = XY(band, h);
      uint8_t brightness = map(h, 0, 7, 128, 255);
      uint8_t hue = map(band, 0, NUM_BANDS - 1, 0, 255);
      leds[ledIndex] = CHSV(hue, 255, brightness);
    }
    // Draw top half (downward)
    for (int h = 0; h < height; h++) {
      int ledIndex = XY(band, 15 - h);
      uint8_t brightness = map(h, 0, 7, 128, 255);
      uint8_t hue = map(band, 0, NUM_BANDS - 1, 0, 255);
      leds[ledIndex] = CHSV(hue, 255, brightness);
    }
    // Draw peaks
    if (peakValues[band] > height) {
      int peakBottom = XY(band, peakValues[band] - 1);
      int peakTop = XY(band, 16 - (peakValues[band] - 1));
      leds[peakBottom] = CHSV(0, 0, 255);
      leds[peakTop] = CHSV(0, 0, 255);
    }
  }
  FastLED.show();
  Serial.println("OK");
}