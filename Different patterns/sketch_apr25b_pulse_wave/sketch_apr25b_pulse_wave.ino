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
        drawPulseWave();
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

void drawPulseWave() {
  FastLED.clear();
  // Calculate average amplitude to modulate wave speed
  float avgAmplitude = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    avgAmplitude += bandValues[i];
  }
  avgAmplitude /= NUM_BANDS;
  float waveSpeed = map(avgAmplitude, 0, HEIGHT, 1, 3);

  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      // Create a sine wave that moves left to right
      float wave = sin((x + millis() / (500.0 / waveSpeed)) * 0.5) * (HEIGHT / 2.0);
      wave += HEIGHT / 2.0; // Center the wave
      // Modulate wave height with amplitude of this band
      int band = x; // Map x to band
      wave = wave * (bandValues[band] / HEIGHT); // Scale wave height
      int waveHeight = round(wave);
      if (y <= waveHeight) {
        int ledIndex = XY(x, y);
        uint8_t hue = map(x, 0, WIDTH - 1, 0, 255); // Rainbow gradient
        uint8_t brightness = map(y, 0, waveHeight, 128, 255);
        leds[ledIndex] = CHSV(hue, 255, brightness);
      }
    }
  }
  FastLED.show();
  Serial.println("OK");
}