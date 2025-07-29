#include <FastLED.h>

// LED Matrix Configuration
#define LED_PIN     5
#define NUM_LEDS    288  // 18x16 grid
#define WIDTH       18
#define HEIGHT      16
#define BRIGHTNESS  100

// Audio Configuration
#define NUM_BANDS   10
#define SMOOTHING   0.3
#define BEAT_THRESHOLD 1.2
#define INACTIVITY_TIMEOUT 1000 // ms
#define MIN_INTENSITY 5       // Minimum value to trigger ripple

// Ripple Structure
struct Ripple {
  uint8_t x, y;
  float radius;
  float intensity;
  uint8_t hue;
  bool active;
  uint8_t brightness;
};

#define MAX_RIPPLES 5
Ripple ripples[MAX_RIPPLES];

float bandValues[NUM_BANDS] = {0};
float prevBandValues[NUM_BANDS] = {0};
uint32_t lastUpdate = 0;

CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // for (int i = 0; i < MAX_RIPPLES; i++) {
  //   ripples[i].active = false;
  // }

  // testColors();
  Serial.println("Beat Ripple Visualizer Ready");
}

void testColors() {
  FastLED.clear();
  for (int x = 0; x < WIDTH; x++) {
    int ledIndex = XY(x, 0);
    uint8_t hue = map(x, 0, WIDTH - 1, 0, 255);
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
        updateRipples();
        renderRipples();
        FastLED.show();
        lastUpdate = millis();
        Serial.println("OK");
      } else {
        Serial.println("LOW - No ripple");
        clearAllRipples();
      }
      bufIdx = 0;
    } else if (bufIdx < sizeof(buffer) - 1) {
      buffer[bufIdx++] = c;
    } else {
      bufIdx = 0;
    }
  }

  if (millis() - lastUpdate > INACTIVITY_TIMEOUT && lastUpdate > 0) {
    clearAllRipples();
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

    if (bandValues[i] >= MIN_INTENSITY) {
      significant = true;

      if (bandValues[i] > prevBandValues[i] * BEAT_THRESHOLD) {
        spawnRipple(i, bandValues[i]);
        Serial.print("Beat detected for band ");
        Serial.println(i);
      }
    }

    prevBandValues[i] = bandValues[i];
  }

  return significant;
}

void spawnRipple(int band, float intensity) {
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (!ripples[i].active) {
      ripples[i].x = band;
      ripples[i].y = HEIGHT / 2;
      ripples[i].radius = 0;
      ripples[i].intensity = constrain(intensity, MIN_INTENSITY, (float)HEIGHT);
      ripples[i].hue = map(band, 0, NUM_BANDS - 1, 0, 255);
      ripples[i].brightness = 255;
      ripples[i].active = true;
      Serial.print("Spawning ripple for band ");
      Serial.println(band);
      return;
    }
  }
}

void updateRipples() {
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (ripples[i].active) {
      float growth = 0.1 + 0.05 * ripples[i].intensity;
      ripples[i].radius += growth;
      ripples[i].brightness -= 2;

      if (ripples[i].brightness < 30 || ripples[i].radius > max(WIDTH, HEIGHT) / 2.0) {
        ripples[i].active = false;
      }
    }
  }
}

void renderRipples() {
  FastLED.clear();
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      for (int i = 0; i < MAX_RIPPLES; i++) {
        if (ripples[i].active) {
          float dx = x - ripples[i].x;
          float dy = y - ripples[i].y;
          float distance = sqrt(dx * dx + dy * dy);
          if (abs(distance - ripples[i].radius) < 1.5) {
            int ledIndex = XY(x, y);
            leds[ledIndex] = CHSV(ripples[i].hue, 255, ripples[i].brightness);
          }
        }
      }
    }
  }
}

void clearAllRipples() {
  FastLED.clear();
  FastLED.show();
  for (int i = 0; i < MAX_RIPPLES; i++) {
    ripples[i].active = false;
  }
}
