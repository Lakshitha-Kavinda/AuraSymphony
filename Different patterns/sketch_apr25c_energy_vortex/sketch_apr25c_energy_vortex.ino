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
        drawVortex();
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

void drawVortex() {
  FastLED.clear();
  // Calculate total energy to drive vortex intensity
  float totalEnergy = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    totalEnergy += bandValues[i];
  }
  totalEnergy /= NUM_BANDS;

  float centerX = WIDTH / 2.0;
  float centerY = HEIGHT / 2.0;
  float maxRadius = min(centerX, centerY);

  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance <= maxRadius) {
        // Calculate angle and spiral effect
        float angle = atan2(dy, dx) + (millis() / 1000.0); // Rotate over time
        float spiral = sin(distance * 2.0 - angle * 2.0 + totalEnergy * 0.5);
        if (spiral > 0) {
          int ledIndex = XY(x, y);
          uint8_t hue = map(distance, 0, maxRadius, 0, 32); // Red to yellow
          uint8_t brightness = map(spiral * totalEnergy, 0, HEIGHT, 64, 255);
          leds[ledIndex] = CHSV(hue, 255, brightness);
        }
      }
    }
  }
  FastLED.show();
  Serial.println("OK");
}