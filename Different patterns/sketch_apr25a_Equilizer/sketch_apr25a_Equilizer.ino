#include <FastLED.h>

#define NUM_LEDS 288
#define DATA_PIN 5
#define WIDTH 18
#define HEIGHT 16
#define NUM_BANDS 18
#define SERIAL_TIMEOUT 100 // ms
#define SMOOTHING_FACTOR 0.3
#define INACTIVITY_TIMEOUT 500 // ms
#define MAX_PARTICLES 50

struct Particle {
  float x, y;
  float vx, vy;
  uint8_t hue;
  uint8_t brightness;
  bool active;
};

CRGB leds[NUM_LEDS];
float bandValues[NUM_BANDS];
float prevBandValues[NUM_BANDS] = {0};
uint32_t lastUpdate = 0;
Particle particles[MAX_PARTICLES];

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(64);
  FastLED.clear();
  FastLED.show();
  // Initialize particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].active = false;
  }
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
        drawParticles();
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
    for (int i = 0; i < MAX_PARTICLES; i++) {
      particles[i].active = false;
    }
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
    // Spawn particles on smaller rising edges
    if (bandValues[i] > prevBandValues[i] + 1) { // Lowered threshold
      spawnParticle(i, bandValues[i]);
    }
  }
  return significant;
}

void spawnParticle(int band, float amplitude) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) {
      particles[i].x = band;
      particles[i].y = 0;
      particles[i].vx = random(-5, 6) / 10.0; // Reduced spread
      particles[i].vy = map(amplitude, 0, HEIGHT, 0.3, 1.0); // Slower speed
      particles[i].hue = map(band, 0, NUM_BANDS - 1, 0, 255);
      particles[i].brightness = 255;
      particles[i].active = true;
      Serial.print("Spawning particle for band ");
      Serial.println(band);
      break;
    }
  }
}

void drawParticles() {
  FastLED.clear();
  // Update particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      particles[i].brightness = max(0, particles[i].brightness - 5); // Slower fade
      particles[i].vy -= 0.02; // Reduced gravity
      // Deactivate if offscreen or faded
      if (particles[i].y < 0 || particles[i].y >= HEIGHT || particles[i].x < 0 || particles[i].x >= WIDTH || particles[i].brightness == 0) {
        particles[i].active = false;
      } else {
        Serial.print("Particle ");
        Serial.print(i);
        Serial.print(": x=");
        Serial.print(particles[i].x);
        Serial.print(", y=");
        Serial.print(particles[i].y);
        Serial.print(", brightness=");
        Serial.println(particles[i].brightness);
      }
    }
  }
  // Draw particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      int x = round(particles[i].x);
      int y = round(particles[i].y);
      if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        int ledIndex = XY(x, y);
        leds[ledIndex] = CHSV(particles[i].hue, 255, particles[i].brightness);
      }
    }
  }
  FastLED.show();
  Serial.println("OK");
}