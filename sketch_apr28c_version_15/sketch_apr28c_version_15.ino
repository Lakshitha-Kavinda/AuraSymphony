#include <WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>

// Wi-Fi credentials
const char* ssid = "Kavinda";  // Replace with your Wi-Fi SSID
const char* password = "eytk1601";  // Replace with your Wi-Fi password

// LED matrix settings
#define DATA_PIN 19
#define MAX_LEDS 64  // 8x8 matrix
CRGB leds[MAX_LEDS];
int matrixWidth = 8;
int matrixHeight = 8;
int numLeds = matrixWidth * matrixHeight;

// Pattern definitions
#define NUM_PATTERNS 24
enum Pattern {
  ENHANCED_BAR_GRAPH,
  MIRRORED_SPECTRUM,
  WATERFALL_EFFECT,
  PULSE_WAVE,
  PULSE_EXPANSION,
  VORTEX,
  SIMPLE_BAR_GRAPH,
  COLOR_BASED,
  RIPPLE_PER_BAND,
  FIRE_EFFECT,
  PARTICLE_EFFECT,
  CIRCULAR_SPECTRUM,
  RADIAL_FREQUENCY_BARS,
  SAKURA_GLOW,
  SAKURA_FLOWER,
  VOICE_WAVEFORM,
  SHOCKWAVE_RINGS,
  PIXEL_RAIN,
  WAVEFORM_ANIMATION,
  HEARTBEAT_CIRCLE,
  CSE_SAKURA_WIND,
  SAKURA_PETAL_CHASE,
  FALLING_PETALS_HANAMI,
  CSE_SAKURA_BEAT_GLOW
};
Pattern currentPattern = ENHANCED_BAR_GRAPH;

// Color settings
int selectedHue = 0; // Default hue (0-360)
bool useDefaultColors = true; // Default to using pattern's default colors

// Default hues for each pattern (in degrees, 0-360)
const int defaultHues[NUM_PATTERNS] = {
  0,   // ENHANCED_BAR_GRAPH (gradient)
  0,   // MIRRORED_SPECTRUM (gradient)
  0,   // WATERFALL_EFFECT (gradient)
  0,   // PULSE_WAVE (gradient)
  0,   // PULSE_EXPANSION (rotating hue)
  0,   // VORTEX (gradient)
  0,   // SIMPLE_BAR_GRAPH (gradient)
  0,   // COLOR_BASED (RGB based on frequency)
  0,   // RIPPLE_PER_BAND (gradient)
  0,   // FIRE_EFFECT (fire hues)
  0,   // PARTICLE_EFFECT (gradient)
  0,   // CIRCULAR_SPECTRUM (gradient)
  0,   // RADIAL_FREQUENCY_BARS (gradient)
  330, // SAKURA_GLOW (pink)
  340, // SAKURA_FLOWER (pinkish-purple)
  0,   // VOICE_WAVEFORM (gradient)
  0,   // SHOCKWAVE_RINGS (gradient)
  0,   // PIXEL_RAIN (gradient)
  0,   // WAVEFORM_ANIMATION (gradient)
  0,   // HEARTBEAT_CIRCLE (red)
  330, // CSE_SAKURA_WIND (pink)
  330, // SAKURA_PETAL_CHASE (pink)
  330, // FALLING_PETALS_HANAMI (pink)
  330  // CSE_SAKURA_BEAT_GLOW (pink)
};

// Audio data
#define NUM_BANDS 18
uint8_t amplitudes[NUM_BANDS];

// Particle effect data
#define MAX_PARTICLES 50
struct Particle {
  float x, y;
  float vx, vy;
  uint8_t hue;
  uint8_t brightness;
  bool active;
  float phase; // For wobble/spin in Falling Petals
};
Particle particles[MAX_PARTICLES];

// Circular spectrum data
int peakValues[NUM_BANDS] = {0};
uint32_t peakTimes[NUM_BANDS] = {0};

// Pixel rain data
struct RainDrop {
  float x, y;
  float speed;
  bool active;
};
RainDrop rainDrops[MAX_PARTICLES];

// Web server and UDP
WiFiServer server(80);
WiFiUDP udp;
const int UDP_PORT = 21324;

// HTML content in PROGMEM
const char* html_part1 PROGMEM =
"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
"<!DOCTYPE html><html lang=\"en\"><head>"
"<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"<title>LED Matrix Control</title>"
"<style>"
"body {background-color: #1a202c; color: white; font-family: Arial, sans-serif; padding: 20px;}"
"a {color: #3182ce; text-decoration: none; padding: 10px; display: block; margin: 5px; background-color: #4a5568; border-radius: 5px; width: 200px; text-align: center;}"
"a:hover {background-color: #2d3748;}"
".input-group {margin: 20px 0;}"
"input[type=\"number\"] {background-color: #4a5568; color: white; padding: 5px; border-radius: 5px; border: none; width: 100px;}"
"input[type=\"submit\"], button {background-color: #3182ce; color: white; padding: 8px 16px; border-radius: 5px; border: none; cursor: pointer; margin-top: 10px;}"
"input[type=\"submit\"]:hover, button:hover {background-color: #2b6cb0;}"
"label {margin-right: 10px;}"
"#colorPreview {width: 50px; height: 50px; display: inline-block; border: 1px solid white; vertical-align: middle;}"
"#colorWheel {border: 2px solid #4a5568; border-radius: 50%;}"
"</style></head><body>"
"<h1>LED Matrix Control</h1>"
"<div class=\"input-group\">"
"<form action=\"/setMatrixSize\" method=\"get\">"
"<label>Matrix Width:</label><br><input type=\"number\" name=\"w\" value=\"";

const char* html_part2 PROGMEM =
"\" min=\"1\" max=\"255\"><br>"
"<label>Matrix Height:</label><br><input type=\"number\" name=\"h\" value=\"";

const char* html_part3 PROGMEM =
"\" min=\"1\" max=\"255\"><br>"
"<input type=\"submit\" value=\"Update Size\">"
"</form></div>"
"<h2>Select Effect</h2>"
"<div><a href=\"/setPattern?pattern=0\">Enhanced Bar Graph</a>"
"<a href=\"/setPattern?pattern=1\">Mirrored Spectrum</a>"
"<a href=\"/setPattern?pattern=2\">Waterfall Effect</a>"
"<a href=\"/setPattern?pattern=3\">Pulse Wave</a>"
"<a href=\"/setPattern?pattern=4\">Pulse Expansion</a>"
"<a href=\"/setPattern?pattern=5\">Vortex</a>"
"<a href=\"/setPattern?pattern=6\">Simple Bar Graph</a>"
"<a href=\"/setPattern?pattern=7\">Color Based</a>"
"<a href=\"/setPattern?pattern=8\">Ripple Per Band</a>"
"<a href=\"/setPattern?pattern=9\">Fire Effect</a>"
"<a href=\"/setPattern?pattern=10\">Particle Effect</a>"
"<a href=\"/setPattern?pattern=11\">Circular Spectrum</a>"
"<a href=\"/setPattern?pattern=12\">Radial Frequency Bars</a>"
"<a href=\"/setPattern?pattern=13\">Sakura Glow</a>"
"<a href=\"/setPattern?pattern=14\">Sakura Flower</a>"
"<a href=\"/setPattern?pattern=15\">Voice Waveform</a>"
"<a href=\"/setPattern?pattern=16\">Shockwave Rings</a>"
"<a href=\"/setPattern?pattern=17\">Pixel Rain</a>"
"<a href=\"/setPattern?pattern=18\">Waveform Animation</a>"
"<a href=\"/setPattern?pattern=19\">Heartbeat Circle</a>"
"<a href=\"/setPattern?pattern=20\">CSE Sakura Wind</a>"
"<a href=\"/setPattern?pattern=21\">Sakura Petal Chase</a>"
"<a href=\"/setPattern?pattern=22\">Falling Petals (Hanami)</a>"
"<a href=\"/setPattern?pattern=23\">CSE Sakura Beat Glow</a></div>"
"<h2>Color Settings</h2>"
"<div class=\"input-group\">"
"<form action=\"/setColor\" method=\"get\">"
"<label><input type=\"checkbox\" name=\"defaultColors\" value=\"1\" checked> Use Default Colors</label><br>"
"<canvas id=\"colorWheel\" width=\"200\" height=\"200\" style=\"cursor: pointer;\"></canvas><br>"
"<label>Selected Color: </label><div id=\"colorPreview\" style=\"background-color: hsl(";

const char* html_part4 PROGMEM =
", 100%, 50%);\"></div><br>"
"<input type=\"hidden\" name=\"hue\" id=\"hueInput\" value=\"";

const char* html_part5 PROGMEM =
"\">"
"<input type=\"submit\" value=\"Apply Color\">"
"</form>"
"<form action=\"/resetColor\" method=\"get\">"
"<button type=\"submit\">Reset to Pattern Default</button>"
"</form></div>"
"<script>"
"const canvas = document.getElementById('colorWheel');"
"const ctx = canvas.getContext('2d');"
"const preview = document.getElementById('colorPreview');"
"const hueInput = document.getElementById('hueInput');"
"const centerX = canvas.width / 2;"
"const centerY = canvas.height / 2;"
"const innerRadius = 80;"
"const outerRadius = 100;"
"for (let angle = 0; angle < 360; angle++) {"
"  for (let r = innerRadius; r <= outerRadius; r++) {"
"    const rad = (angle * Math.PI) / 180;"
"    const x = centerX + Math.cos(rad) * r;"
"    const y = centerY + Math.sin(rad) * r;"
"    ctx.beginPath();"
"    ctx.arc(x, y, 1, 0, 2 * Math.PI);"
"    ctx.fillStyle = `hsl(${angle}, 100%, 50%)`;"
"    ctx.fill();"
"  }"
"}"
"const selectedHue = parseInt(hueInput.value);"
"const selectedRad = (selectedHue * Math.PI) / 180;"
"const markerRadius = (innerRadius + outerRadius) / 2;"
"const markerX = centerX + Math.cos(selectedRad) * markerRadius;"
"const markerY = centerY + Math.sin(selectedRad) * markerRadius;"
"ctx.beginPath();"
"ctx.arc(markerX, markerY, 5, 0, 2 * Math.PI);"
"ctx.strokeStyle = 'white';"
"ctx.lineWidth = 2;"
"ctx.stroke();"
"canvas.addEventListener('click', (event) => {"
"  const rect = canvas.getBoundingClientRect();"
"  const x = event.clientX - rect.left - centerX;"
"  const y = event.clientY - rect.top - centerY;"
"  const distance = Math.sqrt(x * x + y * y);"
"  if (distance >= innerRadius && distance <= outerRadius) {"
"    const angle = Math.atan2(y, x) * 180 / Math.PI;"
"    const hue = (angle + 360) % 360;"
"    hueInput.value = Math.round(hue);"
"    preview.style.backgroundColor = `hsl(${hue}, 100%, 50%)`;"
"    ctx.clearRect(0, 0, canvas.width, canvas.height);"
"    for (let angle = 0; angle < 360; angle++) {"
"      for (let r = innerRadius; r <= outerRadius; r++) {"
"        const rad = (angle * Math.PI) / 180;"
"        const x = centerX + Math.cos(rad) * r;"
"        const y = centerY + Math.sin(rad) * r;"
"        ctx.beginPath();"
"        ctx.arc(x, y, 1, 0, 2 * Math.PI);"
"        ctx.fillStyle = `hsl(${angle}, 100%, 50%)`;"
"        ctx.fill();"
"      }"
"    }"
"    const newRad = (hue * Math.PI) / 180;"
"    const newX = centerX + Math.cos(newRad) * markerRadius;"
"    const newY = centerY + Math.sin(newRad) * markerRadius;"
"    ctx.beginPath();"
"    ctx.arc(newX, newY, 5, 0, 2 * Math.PI);"
"    ctx.strokeStyle = 'white';"
"    ctx.lineWidth = 2;"
"    ctx.stroke();"
"  }"
"});"
"</script>"
"</body></html>";

// XY mapping for zig-zag layout
uint16_t XY(uint16_t x, uint16_t y) {
  if (x >= matrixWidth || y >= matrixHeight || x < 0 || y < 0) return numLeds;
  uint16_t i;
  if (y % 2 == 0) {
    i = (y * matrixWidth) + x;
  } else {
    i = (y * matrixWidth) + (matrixWidth - 1 - x);
  }
  if (i >= numLeds) return numLeds;
  return i;
}

// XY mapping for straight layout
uint16_t XYstraight(uint16_t x, uint16_t y) {
  if (x >= matrixWidth || y >= matrixHeight || x < 0 || y < 0) return numLeds;
  uint16_t i = (y * matrixWidth) + x;
  if (i >= numLeds) return numLeds;
  return i;
}

// CSE bitmap (5x7)
const uint8_t cseBitmap[5][7] = {
  {1, 1, 1, 0, 1, 1, 1}, // C: ###, S: ###, E: ###
  {1, 0, 0, 0, 1, 0, 0}, // C: #,   S: #,   E: #
  {1, 0, 0, 0, 1, 1, 1}, // C: #,   S: ###, E: ###
  {1, 0, 0, 0, 0, 0, 1}, // C: #,   S:   #, E:   #
  {1, 1, 1, 0, 1, 1, 1}  // C: ###, S: ###, E: ###
};

// CSE Sakura Wind pattern
void cseSakuraWind() {
  FastLED.clear();

  // Use selectedHue for CSE bitmap (default pink hue: 330)
  uint8_t cseHue = useDefaultColors ? 330 : selectedHue;
  uint8_t cseMappedHue = map(cseHue, 0, 360, 0, 255);

  // Render "CSE" bitmap
  int startX = 0; // Center horizontally (x: 0)
  int startY = 1; // Center vertically (y: 1)
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 7; x++) {
      if (cseBitmap[y][x]) {
        int ledIndex = XYstraight(startX + x, startY + y);
        if (ledIndex < numLeds) {
          leds[ledIndex] = CHSV(cseMappedHue, 255, 255); // Use selected/default hue
        }
      }
    }
  }

  // Calculate average amplitude for petal spawn rate
  uint32_t amplitudeSum = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    amplitudeSum += amplitudes[i];
  }
  uint8_t avgAmplitude = amplitudeSum / NUM_BANDS;

  // Spawn new petals based on amplitude
  static unsigned long lastSpawn = 0;
  int spawnInterval = map(avgAmplitude, 0, 255, 1000, 200); // Spawn faster with higher amplitude
  if (millis() - lastSpawn > spawnInterval) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (!particles[i].active) {
        particles[i].x = random(0, matrixWidth);
        particles[i].y = 0;
        particles[i].vx = random(-5, 6) / 10.0; // Wind effect: -0.5 to 0.5
        particles[i].vy = map(avgAmplitude, 0, 255, 0.1, 0.4); // Fall speed
        // Apply gradient around selectedHue or default pink
        particles[i].hue = useDefaultColors ? 213 : getFrequencyHue(particles[i].x, matrixWidth - 1);
        particles[i].brightness = 255;
        particles[i].active = true;
        lastSpawn = millis();
        break;
      }
    }
  }

  // Update and render particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      // Update position
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      particles[i].brightness = max(0, particles[i].brightness - 20); // Faster fade for 8x8

      // Boundary checks
      if (particles[i].y >= matrixHeight || particles[i].x < 0 || particles[i].x >= matrixWidth || particles[i].brightness == 0) {
        particles[i].active = false;
        continue;
      }

      // Render particle if not overlapping "CSE"
      int x = round(particles[i].x);
      int y = round(particles[i].y);
      if (x >= 0 && x < matrixWidth && y >= 0 && y < matrixHeight) {
        int ledIndex = XYstraight(x, y);
        if (ledIndex < numLeds) {
          // Check if pixel is part of "CSE"
          bool isCSEPixel = false;
          if (x >= startX && x < startX + 7 && y >= startY && y < startY + 5) {
            if (cseBitmap[y - startY][x - startX]) {
              isCSEPixel = true;
            }
          }
          if (!isCSEPixel) {
            leds[ledIndex] = CHSV(particles[i].hue, 255, particles[i].brightness);
          }
        }
      }
    }
  }

  FastLED.show();
}

// CSE Sakura Beat Glow pattern
void cseSakuraBeatGlow() {
  FastLED.clear();

  // Use selectedHue for CSE bitmap and glow (default pink hue: 330)
  uint8_t cseHue = useDefaultColors ? 330 : selectedHue;
  uint8_t cseMappedHue = map(cseHue, 0, 360, 0, 255);

  // Render "CSE" bitmap
  int startX = 0; // Center horizontally (x: 0)
  int startY = 1; // Center vertically (y: 1)
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 7; x++) {
      if (cseBitmap[y][x]) {
        int ledIndex = XYstraight(startX + x, startY + y);
        if (ledIndex < numLeds) {
          leds[ledIndex] = CHSV(cseMappedHue, 255, 255); // Use selected/default hue
        }
      }
    }
  }

  // Calculate bass amplitude for glow pulse (bands 0-3)
  uint32_t bassSum = 0;
  for (int i = 0; i < 4; i++) {
    bassSum += amplitudes[i];
  }
  uint8_t bassAmplitude = bassSum / 4;

  // Calculate glow radius based on bass amplitude
  float centerX = (matrixWidth - 1) / 2.0;
  float centerY = (matrixHeight - 1) / 2.0;
  float minGlowRadius = 3.0; // Slightly larger than bitmap
  float maxGlowRadius = min(matrixWidth, matrixHeight) / 2.0; // Cover most of matrix
  float glowRadius = minGlowRadius + (bassAmplitude / 255.0) * (maxGlowRadius - minGlowRadius);

  // Render glow around "CSE"
  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      // Check if pixel is outside the "CSE" bitmap and within glow radius
      bool isCSEPixel = (x >= startX && x < startX + 7 && y >= startY && y < startY + 5 && cseBitmap[y - startY][x - startX]);
      if (!isCSEPixel && distance <= glowRadius && distance >= minGlowRadius) {
        int ledIndex = XYstraight(x, y);
        if (ledIndex < numLeds) {
          uint8_t brightness = map(distance, minGlowRadius, glowRadius, 255, 32);
          leds[ledIndex] = CHSV(cseMappedHue, 200, brightness); // Softer saturation for glow
        }
      }
    }
  }

  // Calculate average amplitude for petal spawn rate
  uint32_t amplitudeSum = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    amplitudeSum += amplitudes[i];
  }
  uint8_t avgAmplitude = amplitudeSum / NUM_BANDS;

  // Spawn new petals based on amplitude
  static unsigned long lastSpawn = 0;
  int spawnInterval = map(avgAmplitude, 0, 255, 1000, 200); // Spawn faster with higher amplitude
  if (millis() - lastSpawn > spawnInterval) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (!particles[i].active) {
        particles[i].x = random(0, matrixWidth);
        particles[i].y = 0;
        particles[i].vx = random(-5, 6) / 10.0; // Wind effect: -0.5 to 0.5
        particles[i].vy = map(avgAmplitude, 0, 255, 0.1, 0.4); // Fall speed
        // Apply gradient around selectedHue or default pink
        particles[i].hue = useDefaultColors ? 213 : getFrequencyHue(particles[i].x, matrixWidth - 1);
        particles[i].brightness = 255;
        particles[i].active = true;
        lastSpawn = millis();
        break;
      }
    }
  }

  // Update and render particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      // Update position
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      particles[i].brightness = max(0, particles[i].brightness - 20); // Faster fade for 8x8

      // Boundary checks
      if (particles[i].y >= matrixHeight || particles[i].x < 0 || particles[i].x >= matrixWidth || particles[i].brightness == 0) {
        particles[i].active = false;
        continue;
      }

      // Render particle if not overlapping "CSE"
      int x = round(particles[i].x);
      int y = round(particles[i].y);
      if (x >= 0 && x < matrixWidth && y >= 0 && y < matrixHeight) {
        int ledIndex = XYstraight(x, y);
        if (ledIndex < numLeds) {
          // Check if pixel is part of "CSE"
          bool isCSEPixel = false;
          if (x >= startX && x < startX + 7 && y >= startY && y < startY + 5) {
            if (cseBitmap[y - startY][x - startX]) {
              isCSEPixel = true;
            }
          }
          if (!isCSEPixel) {
            leds[ledIndex] = CHSV(particles[i].hue, 255, particles[i].brightness);
          }
        }
      }
    }
  }

  FastLED.show();
}

// Sakura Petal Chase pattern
void sakuraPetalChase() {
  FastLED.clear();

  // Calculate average amplitude for wave speed
  uint32_t amplitudeSum = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    amplitudeSum += amplitudes[i];
  }
  uint8_t avgAmplitude = amplitudeSum / NUM_BANDS;
  float speed = map(avgAmplitude, 0, 255, 0.1, 0.5); // Speed scales with music intensity

  // Spawn new petal groups periodically
  static unsigned long lastSpawn = 0;
  int spawnInterval = map(avgAmplitude, 0, 255, 1000, 200); // Spawn faster with louder music
  if (millis() - lastSpawn > spawnInterval) {
    for (int i = 0; i < MAX_PARTICLES; i += 3) { // Spawn groups of 3 petals
      if (!particles[i].active && i + 2 < MAX_PARTICLES) {
        for (int j = 0; j < 3; j++) {
          particles[i + j].x = -j; // Start just off-screen to the left
          particles[i + j].y = random(0, matrixHeight);
          particles[i + j].vx = speed; // Move right
          particles[i + j].vy = 0;
          particles[i + j].hue = useDefaultColors ? 213 : getFrequencyHue(particles[i + j].y, matrixHeight - 1);
          particles[i + j].brightness = 255;
          particles[i + j].active = true;
        }
        lastSpawn = millis();
        break;
      }
    }
  }

  // Update and render particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      // Update position
      particles[i].x += particles[i].vx;
      particles[i].brightness = max(0, particles[i].brightness - 10);

      // Wrap around to left edge
      if (particles[i].x >= matrixWidth) {
        particles[i].x -= matrixWidth;
      }

      // Deactivate if faded out
      if (particles[i].brightness == 0) {
        particles[i].active = false;
        continue;
      }

      // Render particle
      int x = round(particles[i].x);
      int y = round(particles[i].y);
      if (x >= 0 && x < matrixWidth && y >= 0 && y < matrixHeight) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          leds[ledIndex] = CHSV(particles[i].hue, 255, particles[i].brightness);
        }
      }
    }
  }

  FastLED.show();
}

// Falling Petals (Hanami Style) pattern
void fallingPetalsHanami() {
  FastLED.clear();

  // Calculate average amplitude for spawn rate
  uint32_t amplitudeSum = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    amplitudeSum += amplitudes[i];
  }
  uint8_t avgAmplitude = amplitudeSum / NUM_BANDS;

  // Spawn new petals
  static unsigned long lastSpawn = 0;
  int spawnInterval = map(avgAmplitude, 0, 255, 1500, 500); // Slower spawn for softer effect
  if (millis() - lastSpawn > spawnInterval) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (!particles[i].active) {
        particles[i].x = random(0, matrixWidth);
        particles[i].y = 0;
        particles[i].vx = random(-2, 3) / 20.0; // Slight horizontal drift: -0.1 to 0.1
        particles[i].vy = random(5, 21) / 100.0; // Fall speed: 0.05 to 0.2
        particles[i].phase = random(0, 256); // Random phase for wobble/spin
        // Alternate pink (330) and white (0,0)
        bool isPink = random(0, 2);
        particles[i].hue = useDefaultColors ? (isPink ? 213 : 0) : getFrequencyHue(particles[i].x, matrixWidth - 1);
        particles[i].brightness = useDefaultColors && !isPink ? 255 : 255; // White uses brightness, pink uses hue
        particles[i].active = true;
        lastSpawn = millis();
        break;
      }
    }
  }

  // Update and render particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      // Update position
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      // Add wobble/spin using phase
      particles[i].vx += sin8(particles[i].phase) / 255.0 * 0.05 - 0.025; // Oscillate ±0.025
      particles[i].phase += 5; // Slow phase change for gentle motion
      particles[i].brightness = max(0, particles[i].brightness - 5); // Slow fade

      // Boundary checks
      if (particles[i].y >= matrixHeight || particles[i].x < 0 || particles[i].x >= matrixWidth || particles[i].brightness == 0) {
        particles[i].active = false;
        continue;
      }

      // Render particle
      int x = round(particles[i].x);
      int y = round(particles[i].y);
      if (x >= 0 && x < matrixWidth && y >= 0 && y < matrixHeight) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          if (useDefaultColors && particles[i].hue == 0) {
            leds[ledIndex] = CRGB(particles[i].brightness, particles[i].brightness, particles[i].brightness); // White
          } else {
            leds[ledIndex] = CHSV(particles[i].hue, 255, particles[i].brightness);
          }
        }
      }
    }
  }

  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting setup...");

  Serial.print("Free heap at start: ");
  Serial.println(ESP.getFreeHeap());

  // Connect to Wi-Fi
  Serial.println("Calling WiFi.begin()...");
  WiFi.begin(ssid, password);
  Serial.println("WiFi.begin() called");

  Serial.println("Checking WiFi status...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi");
    return;
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Free heap after WiFi: ");
  Serial.println(ESP.getFreeHeap());

  // Initialize FastLED after WiFi
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, MAX_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000);
  FastLED.clear();
  FastLED.show();
  Serial.println("FastLED initialized");

  // Initialize amplitudes
  for (int i = 0; i < NUM_BANDS; i++) {
    amplitudes[i] = 0;
  }
  Serial.println("Amplitudes initialized");

  // Initialize particles and rain drops
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].active = false;
    rainDrops[i].active = false;
  }

  // Setup UDP
  udp.begin(UDP_PORT);
  Serial.print("UDP Listening on port ");
  Serial.println(UDP_PORT);

  // Start web server
  server.begin();
  Serial.println("Web server started");

  Serial.print("Free heap after setup: ");
  Serial.println(ESP.getFreeHeap());

  // Boot screen
  cseSakuraWind();
  delay(10000);
}

// Helper function to compute hue based on position/band
uint8_t getFrequencyHue(int index, int maxIndex) {
  if (useDefaultColors) {
    return map(index, 0, maxIndex, 0, 255); // Full spectrum for default colors
  } else {
    // Create a wider gradient around selectedHue
    int hueMin = (selectedHue - 60 + 360) % 360; // Wider range: ±60 degrees
    int hueMax = (selectedHue + 60) % 360;
    int hue;
    if (hueMin <= hueMax) {
      hue = map(index, 0, maxIndex, hueMin, hueMax);
    } else {
      // Handle wrap-around (e.g., hueMin=300, hueMax=60)
      hue = map(index, 0, maxIndex, hueMin, hueMax + 360);
      hue = hue % 360;
    }
    return map(hue, 0, 360, 0, 255); // Convert 0-360 to 0-255 for CHSV
  }
}

// Pattern implementations
void enhancedBarGraph() {
  FastLED.clear();
  for (int x = 0; x < matrixWidth; x++) {
    int band = (x * NUM_BANDS) / matrixWidth;
    uint8_t amplitude = amplitudes[band];
    int barHeight = (amplitude * matrixHeight) / 255;
    for (int y = 0; y < barHeight && y < matrixHeight; y++) {
      uint16_t index = XY(x, y);
      if (index < numLeds) {
        uint8_t hue = getFrequencyHue(x, matrixWidth - 1);
        leds[index] = CHSV(hue, 255, 255);
      }
    }
  }
}

void mirroredSpectrum() {
  FastLED.clear();
  int mid = matrixWidth / 2;
  for (int x = 0; x < mid; x++) {
    int band = (x * NUM_BANDS) / mid;
    uint8_t amplitude = amplitudes[band];
    int barHeight = (amplitude * matrixHeight) / 255;
    for (int y = 0; y < barHeight && y < matrixHeight; y++) {
      uint16_t index1 = XY(mid + x, y);
      uint16_t index2 = XY(mid - 1 - x, y);
      uint8_t hue = getFrequencyHue(x, mid - 1);
      if (index1 < numLeds) leds[index1] = CHSV(hue, 255, 255);
      if (index2 < numLeds) leds[index2] = CHSV(hue, 255, 255);
    }
  }
}

void waterfallEffect() {
  for (int y = matrixHeight - 1; y > 0; y--) {
    for (int x = 0; x < matrixWidth; x++) {
      uint16_t srcIndex = XY(x, y - 1);
      uint16_t dstIndex = XY(x, y);
      if (srcIndex < numLeds && dstIndex < numLeds) {
        leds[dstIndex] = leds[srcIndex];
      }
    }
  }
  for (int x = 0; x < matrixWidth; x++) {
    int band = (x * NUM_BANDS) / matrixWidth;
    uint8_t amplitude = amplitudes[band];
    uint16_t index = XY(x, 0);
    if (index < numLeds) {
      uint8_t hue = getFrequencyHue(x, matrixWidth - 1);
      leds[index] = CHSV(hue, 255, amplitude);
    }
  }
}

void pulseWave() {
  FastLED.clear();
  static uint8_t phase = 0;
  for (int x = 0; x < matrixWidth; x++) {
    int band = (x * NUM_BANDS) / matrixWidth;
    uint8_t amplitude = amplitudes[band];
    for (int y = 0; y < matrixHeight; y++) {
      int value = sin8(phase + (x + y) * 8) * amplitude / 255;
      uint16_t index = XY(x, y);
      if (index < numLeds) {
        uint8_t hue = getFrequencyHue(x, matrixWidth - 1);
        leds[index] = CHSV(hue, 255, value);
      }
    }
  }
  phase += 5;
}

void pulseExpansion() {
  FastLED.clear();
  static uint8_t phase = 0;
  static uint8_t hue = 0;

  float centerX = (matrixWidth - 1) / 2.0;
  float centerY = (matrixHeight - 1) / 2.0;
  uint32_t amplitudeSum = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    amplitudeSum += amplitudes[i];
  }
  uint8_t avgAmplitude = amplitudeSum / NUM_BANDS;
  float maxRadius = sqrt(centerX * centerX + centerY * centerY);

  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      float radius = (phase / 255.0) * maxRadius;
      float waveWidth = 1.0;
      float difference = abs(distance - radius);
      if (difference <= waveWidth) {
        uint8_t brightness = (1.0 - difference / waveWidth) * avgAmplitude;
        uint16_t index = XY(x, y);
        if (index < numLeds) {
          leds[index] = CHSV(hue, 255, brightness);
        }
      }
    }
  }
  phase += 5;
  hue += 3;
}

void vortex() {
  FastLED.clear();
  float totalEnergy = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    totalEnergy += amplitudes[i];
  }
  totalEnergy /= NUM_BANDS;
  totalEnergy /= 255.0;

  float centerX = (matrixWidth - 1) / 2.0;
  float centerY = (matrixHeight - 1) / 2.0;
  float maxRadius = min(centerX, centerY);

  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance <= maxRadius) {
        float angle = atan2(dy, dx) + (millis() / 1000.0);
        float spiral = sin(distance * 2.0 - angle * 2.0 + totalEnergy * 0.5);
        if (spiral > 0) {
          int ledIndex = XY(x, y);
          uint8_t hue = getFrequencyHue(distance * 10, maxRadius * 10);
          uint8_t brightness = map(spiral * totalEnergy * matrixHeight, 0, matrixHeight, 64, 255);
          leds[ledIndex] = CHSV(hue, 255, brightness);
        }
      }
    }
  }
}

void simpleBarGraph() {
  FastLED.clear();
  for (int x = 0; x < matrixWidth; x++) {
    int band = (x * NUM_BANDS) / matrixWidth;
    int barHeight = (amplitudes[band] * matrixHeight) / 255;
    for (int y = 0; y < barHeight && y < matrixHeight; y++) {
      int ledIndex = XY(x, matrixHeight - 1 - y);
      if (ledIndex < numLeds) {
        uint8_t hue = getFrequencyHue(x, matrixWidth - 1);
        leds[ledIndex] = CHSV(hue, 255, 255);
      }
    }
  }
}

void colorBased() {
  uint32_t intensitySum = 0;
  uint32_t lowSum = 0, midSum = 0, highSum = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    intensitySum += amplitudes[i];
    if (i < 6) lowSum += amplitudes[i];
    else if (i < 12) midSum += amplitudes[i];
    else highSum += amplitudes[i];
  }
  uint8_t intensity = (intensitySum / NUM_BANDS) * 10;
  uint8_t low = (lowSum / 6) * 10;
  uint8_t mid = (midSum / 6) * 10;
  uint8_t high = (highSum / 6) * 10;

  uint8_t red = constrain(low / 10, 0, 255);
  uint8_t green = constrain(mid / 10, 0, 255);
  uint8_t blue = constrain(high / 10, 0, 255);
  uint8_t brightness = constrain(intensity / 10, 50, 255);

  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      uint16_t index = XY(x, y);
      if (index < numLeds) {
        leds[index] = CRGB(red, green, blue);
      }
    }
  }
  FastLED.setBrightness(brightness);
}

void ripplePerBand() {
  FastLED.clear();
  static uint8_t phase = 0;

  for (int x = 0; x < matrixWidth; x++) {
    int band = (x * NUM_BANDS) / matrixWidth;
    float centerY = (matrixHeight - 1) / 2.0;
    float maxDistance = matrixHeight / 2.0;
    uint8_t amplitude = amplitudes[band];

    for (int y = 0; y < matrixHeight; y++) {
      for (int dx = -1; dx <= 1; dx++) {
        int nx = x + dx;
        if (nx < 0 || nx >= matrixWidth) continue;

        float dy = y - centerY;
        float distance = sqrt(dx * dx + dy * dy);
        float radius = (phase / 255.0) * maxDistance * (amplitude / 255.0);

        float waveWidth = 1.0;
        float difference = abs(distance - radius);
        if (difference <= waveWidth) {
          uint8_t brightness = (1.0 - difference / waveWidth) * amplitude;
          uint16_t index = XY(nx, y);
          if (index < numLeds) {
            uint8_t hue = getFrequencyHue(x, matrixWidth - 1);
            leds[index] = CHSV(hue, 255, brightness);
          }
        }
      }
    }
  }
  phase += 5;
}

void fireEffect() {
  FastLED.clear();
  for (int x = 0; x < matrixWidth; x++) {
    int band = (x * NUM_BANDS) / matrixWidth;
    int height = min((amplitudes[band] * matrixHeight) / 255, matrixHeight);
    uint8_t hue;
    if (x <= matrixWidth / 3) {
      hue = 85;
    } else if (x <= (2 * matrixWidth) / 3) {
      hue = map(x, matrixWidth / 3, (2 * matrixWidth) / 3, 170, 85);
    } else {
      hue = map(x, (2 * matrixWidth) / 3, matrixWidth - 1, 85, 0);
    }
    for (int y = 0; y < matrixHeight; y++) {
      int noise = random(0, 10);
      int fireHeight = height + noise - 5;
      if (y <= fireHeight) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          uint8_t brightness = map(y, 0, fireHeight, 64, 255);
          if (y > height) brightness = brightness / 2;
          leds[ledIndex] = CHSV(hue, 255, brightness);
        }
      }
    }
  }
}

void spawnParticle(int band, uint8_t amplitude) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) {
      particles[i].x = band * matrixWidth / NUM_BANDS;
      particles[i].y = 0;
      particles[i].vx = random(-10, 11) / 20.0;
      particles[i].vy = map(amplitude, 0, 255, 0.5, 1.5);
      particles[i].hue = map(band, 0, NUM_BANDS - 1, 0, 255);
      particles[i].brightness = 255;
      particles[i].active = true;
      break;
    }
  }
}

void particleEffect() {
  static uint8_t lastAmplitudes[NUM_BANDS] = {0};
  FastLED.clear();

  for (int i = 0; i < NUM_BANDS; i++) {
    if (amplitudes[i] > lastAmplitudes[i] + 10) {
      spawnParticle(i, amplitudes[i]);
    }
    lastAmplitudes[i] = amplitudes[i];
  }

  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      particles[i].brightness = max(0, particles[i].brightness - 10);
      particles[i].vy -= 0.05;
      if (particles[i].y < 0 || particles[i].y >= matrixHeight || particles[i].x < 0 || particles[i].x >= matrixWidth || particles[i].brightness == 0) {
        particles[i].active = false;
      }
    }
  }

  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      int x = round(particles[i].x);
      int y = round(particles[i].y);
      if (x >= 0 && x < matrixWidth && y >= 0 && y < matrixHeight) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          leds[ledIndex] = CHSV(particles[i].hue, 255, particles[i].brightness);
        }
      }
    }
  }
}

void circularSpectrum() {
  FastLED.clear();
  for (int band = 0; band < min(NUM_BANDS, matrixWidth); band++) {
    int height = min((amplitudes[band] * matrixHeight) / 255, matrixHeight);
    if (height > peakValues[band]) {
      peakValues[band] = height;
      peakTimes[band] = millis();
    } else if (millis() - peakTimes[band] > 500) {
      peakValues[band] = max(0, peakValues[band] - 1);
    }
    for (int h = 0; h < height; h++) {
      int ledIndex = XY(band, h);
      if (ledIndex < numLeds) {
        uint8_t brightness = map(h, 0, matrixHeight - 1, 128, 255);
        uint8_t hue = getFrequencyHue(band, min(NUM_BANDS, matrixWidth) - 1);
        leds[ledIndex] = CHSV(hue, 255, brightness);
      }
    }
    if (peakValues[band] > height) {
      int peakIndex = XY(band, peakValues[band] - 1);
      if (peakIndex < numLeds) {
        leds[peakIndex] = CHSV(0, 0, 255);
      }
    }
  }
}

void radialFrequencyBars() {
  FastLED.clear();

  float centerX = (matrixWidth - 1) / 2.0;
  float centerY = (matrixHeight - 1) / 2.0;
  float circleRadius = min(matrixWidth, matrixHeight) / 5.0;
  float maxBarLength = min(matrixWidth, matrixHeight) / 2.0 - circleRadius;

  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance <= circleRadius) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          leds[ledIndex] = CHSV(0, 0, 255);
        }
      }
    }
  }

  for (int band = 0; band < NUM_BANDS; band++) {
    float baseAngle = (band * 360.0 / NUM_BANDS) * (PI / 180.0);
    float amplitude = amplitudes[band];
    float barLength = (amplitude / 255.0) * maxBarLength;
    uint8_t hue = getFrequencyHue(band, NUM_BANDS - 1);

    for (int thickness = -1; thickness <= 1; thickness++) {
      float angleOffset = thickness * 5.0 * (PI / 180.0);
      float angle = baseAngle + angleOffset;

      float startX = centerX + circleRadius * cos(angle);
      float startY = centerY + circleRadius * sin(angle);

      for (float r = 0; r < barLength; r += 0.5) {
        float x = startX + r * cos(angle);
        float y = startY + r * sin(angle);
        int ix = round(x);
        int iy = round(y);
        if (ix >= 0 && ix < matrixWidth && iy >= 0 && iy < matrixHeight) {
          int ledIndex = XY(ix, iy);
          if (ledIndex < numLeds) {
            leds[ledIndex] = CHSV(hue, 255, 255);
          }
        }
      }
    }
  }
}

void sakuraGlow() {
  FastLED.clear();

  float centerX = (matrixWidth - 1) / 2.0;
  float centerY = (matrixHeight - 1) / 2.0;
  float flowerRadius = min(matrixWidth, matrixHeight) / 4.0;
  float maxGlowRadius = min(matrixWidth, matrixHeight) / 2.0;

  uint32_t amplitudeSum = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    amplitudeSum += amplitudes[i];
  }
  uint8_t avgAmplitude = amplitudeSum / NUM_BANDS;
  float glowRadius = flowerRadius + (avgAmplitude / 255.0) * (maxGlowRadius - flowerRadius);

  // Use selectedHue when useDefaultColors is false
  uint8_t glowHue = useDefaultColors ? 330 : selectedHue;

  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance > flowerRadius && distance <= glowRadius) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          uint8_t brightness = map(distance, flowerRadius, glowRadius, 128, 32);
          leds[ledIndex] = CHSV(map(glowHue, 0, 360, 0, 255), 80, brightness);
        }
      }
    }
  }

  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance <= flowerRadius) {
        float angle = atan2(dy, dx);
        float petalShape = sin(5 * angle);
        if (petalShape > 0.5 || distance < flowerRadius / 2.0) {
          int ledIndex = XY(x, y);
          if (ledIndex < numLeds) {
            leds[ledIndex] = CHSV(map(glowHue, 0, 360, 0, 255), 100, 255);
          }
        }
      }
    }
  }
}

void sakuraFlower() {
  FastLED.clear();

  float centerX = (matrixWidth - 1) / 2.0;
  float centerY = (matrixHeight - 1) / 2.0;
  float scaleX = matrixWidth / 512.0;
  float scaleY = matrixHeight / 512.0;
  float flowerRadius = min(matrixWidth, matrixHeight) / 3.0;
  float maxGlowRadius = min(matrixWidth, matrixHeight) / 2.0;

  uint32_t amplitudeSum = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    amplitudeSum += amplitudes[i];
  }
  uint8_t avgAmplitude = amplitudeSum / NUM_BANDS;
  float glowRadius = flowerRadius + (avgAmplitude / 255.0) * (maxGlowRadius - flowerRadius);

  // Use selectedHue when useDefaultColors is false
  uint8_t flowerHue = useDefaultColors ? 340 : selectedHue;

  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance > flowerRadius && distance <= glowRadius) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          uint8_t brightness = map(distance, flowerRadius, glowRadius, 128, 32);
          leds[ledIndex] = CHSV(map(flowerHue, 0, 360, 0, 255), 100, brightness);
        }
      }
    }
  }

  for (int petal = 0; petal < 5; petal++) {
    float angleOffset = (petal * 72.0) * (PI / 180.0);
    for (float t = 0; t <= 1.0; t += 0.05) {
      float r = 150 * (1 - t) + 50 * t;
      float theta = t * 2 * PI;
      float petalX = r * cos(theta);
      float petalY = r * sin(theta);

      float rotatedX = petalX * cos(angleOffset) - petalY * sin(angleOffset);
      float rotatedY = petalX * sin(angleOffset) + petalY * cos(angleOffset);

      int x = round(centerX + (rotatedX * scaleX));
      int y = round(centerY + (rotatedY * scaleY));
      if (x >= 0 && x < matrixWidth && y >= 0 && y < matrixHeight) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          leds[ledIndex] = CHSV(map(flowerHue, 0, 360, 0, 255), 100, 255);
        }
      }
    }
  }

  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = (x - centerX) / scaleX;
      float dy = (y - centerY) / scaleY;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance <= 12) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          leds[ledIndex] = CHSV(0, 0, 0);
        }
      }
    }
  }

  float stamenEnds[8][2] = {
    {256, 190}, {300, 200}, {320, 250}, {300, 310},
    {250, 320}, {210, 310}, {190, 250}, {210, 200}
  };
  for (int i = 0; i < 8; i++) {
    float x0 = centerX;
    float y0 = centerY;
    float x1 = centerX + (stamenEnds[i][0] - 256) * scaleX;
    float y1 = centerY + (stamenEnds[i][1] - 256) * scaleY;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float steps = max(abs(dx), abs(dy));
    for (float t = 0; t <= 1; t += 1.0 / steps) {
      int x = round(x0 + t * dx);
      int y = round(y0 + t * dy);
      if (x >= 0 && x < matrixWidth && y >= 0 && y < matrixHeight) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          leds[ledIndex] = CHSV(0, 0, 0);
        }
      }
    }
    for (int x = 0; x < matrixWidth; x++) {
      for (int y = 0; y < matrixHeight; y++) {
        float dx = (x - (centerX + (stamenEnds[i][0] - 256) * scaleX)) / scaleX;
        float dy = (y - (centerY + (stamenEnds[i][1] - 256) * scaleY)) / scaleY;
        float distance = sqrt(dx * dx + dy * dy);
        if (distance <= 5) {
          int ledIndex = XY(x, y);
          if (ledIndex < numLeds) {
            leds[ledIndex] = CHSV(0, 0, 0);
          }
        }
      }
    }
  }
}

void voiceWaveform() {
  FastLED.clear();
  static uint8_t phase = 0;

  int centerY = (matrixHeight - 1) / 2;

  for (int x = 0; x < matrixWidth; x++) {
    int band = (x * NUM_BANDS) / matrixWidth;
    uint8_t amplitude = amplitudes[band];
    uint8_t effectiveAmplitude = amplitude;

    if (effectiveAmplitude < 10) {
      effectiveAmplitude = 10 + (sin8(phase + x * 10) / 64);
    }

    int barHeight = (effectiveAmplitude * (matrixHeight / 2)) / 255;
    barHeight = max(1, barHeight);

    for (int dy = -barHeight; dy <= barHeight; dy++) {
      int y = centerY + dy;
      if (y >= 0 && y < matrixHeight) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          uint8_t hue = getFrequencyHue(x, matrixWidth - 1);
          uint8_t brightness = 255 - (abs(dy) * 255 / (barHeight + 1));
          leds[ledIndex] = CHSV(hue, 255, brightness);
        }
      }
    }
  }

  phase += 5;
}

void shockwaveRings() {
  FastLED.clear();
  static uint8_t phase = 0;

  float centerX = (matrixWidth - 1) / 2.0;
  float centerY = (matrixHeight - 1) / 2.0;
  float maxRadius = sqrt(centerX * centerX + centerY * centerY);

  uint32_t bassSum = 0;
  for (int i = 0; i < 4; i++) {
    bassSum += amplitudes[i];
  }
  uint8_t bassStrength = bassSum / 4;

  for (int ring = 0; ring < 3; ring++) {
    float ringPhase = (phase + (ring * 85)) % 255;
    float radius = (ringPhase / 255.0) * maxRadius * (bassStrength / 255.0);
    float waveWidth = 1.0;

    for (int x = 0; x < matrixWidth; x++) {
      for (int y = 0; y < matrixHeight; y++) {
        float dx = x - centerX;
        float dy = y - centerY;
        float distance = sqrt(dx * dx + dy * dy);
        float difference = abs(distance - radius);
        if (difference <= waveWidth) {
          uint8_t brightness = (1.0 - difference / waveWidth) * 255;
          brightness = brightness * (255 - ringPhase) / 255;
          uint16_t index = XY(x, y);
          if (index < numLeds) {
            uint8_t hue = getFrequencyHue(ring, 2);
            leds[index] = CHSV(hue, 255, brightness);
          }
        }
      }
    }
  }

  phase += 5;
}

void spawnRainDrop(float intensity) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!rainDrops[i].active) {
      rainDrops[i].x = random(0, matrixWidth);
      rainDrops[i].y = 0;
      rainDrops[i].speed = map(intensity, 0, 255, 0.1, 0.5);
      rainDrops[i].active = true;
      break;
    }
  }
}

void pixelRain() {
  FastLED.clear();

  uint32_t intensitySum = 0;
  for (int i = 0; i < NUM_BANDS; i++) {
    intensitySum += amplitudes[i];
  }
  uint8_t intensity = intensitySum / NUM_BANDS;

  static unsigned long lastSpawn = 0;
  if (millis() - lastSpawn > map(intensity, 0, 255, 500, 50)) {
    spawnRainDrop(intensity);
    lastSpawn = millis();
  }

  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (rainDrops[i].active) {
      rainDrops[i].y += rainDrops[i].speed;
      if (rainDrops[i].y >= matrixHeight) {
        rainDrops[i].active = false;
        continue;
      }
      int x = round(rainDrops[i].x);
      int y = round(rainDrops[i].y);
      if (x >= 0 && x < matrixWidth && y >= 0 && y < matrixHeight) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          uint8_t hue = getFrequencyHue(x, matrixWidth - 1);
          uint8_t brightness = 255 - (y * 255 / matrixHeight);
          leds[ledIndex] = CHSV(hue, 255, brightness);
        }
      }
    }
  }
}

void waveformAnimation() {
  FastLED.clear();
  static uint8_t phase = 0;

  uint32_t beatSum = 0;
  for (int i = 6; i < 12; i++) {
    beatSum += amplitudes[i];
  }
  uint8_t beatStrength = beatSum / 6;

  float centerY = (matrixHeight - 1) / 2.0;
  float amplitude = (beatStrength / 255.0) * (matrixHeight / 4.0);

  for (int x = 0; x < matrixWidth; x++) {
    float wave = sin((x * 2.0 * PI / matrixWidth) + (phase / 255.0) * 2 * PI);
    int y = round(centerY + wave * amplitude);
    for (int dy = -1; dy <= 1; dy++) {
      int waveY = y + dy;
      if (waveY >= 0 && waveY < matrixHeight) {
        int ledIndex = XY(x, waveY);
        if (ledIndex < numLeds) {
          uint8_t hue = getFrequencyHue(x, matrixWidth - 1);
          uint8_t brightness = (dy == 0) ? 255 : 128;
          leds[ledIndex] = CHSV(hue, 255, brightness);
        }
      }
    }
  }

  phase += 5;
}

void heartbeatCircle() {
  FastLED.clear();
  static uint8_t phase = 0;

  float centerX = (matrixWidth - 1) / 2.0;
  float centerY = (matrixHeight - 1) / 2.0;
  float maxRadius = min(centerX, centerY);

  uint32_t beatSum = 0;
  for (int i = 6; i < 12; i++) {
    beatSum += amplitudes[i];
  }
  uint8_t beatStrength = beatSum / 6;

  float baseRadius = maxRadius * 0.5;
  float pulse = (sin8(phase) / 255.0) * (beatStrength / 255.0) * (maxRadius * 0.5);
  float radius = baseRadius + pulse;

  // Use selectedHue when useDefaultColors is false
  uint8_t circleHue = useDefaultColors ? 0 : map(selectedHue, 0, 360, 0, 255);

  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      float difference = abs(distance - radius);
      if (difference <= 1.0) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          uint8_t brightness = (1.0 - difference) * 255;
          leds[ledIndex] = CHSV(circleHue, 255, brightness);
        }
      }
    }
  }

  static uint8_t ripplePhase = 0;
  float rippleRadius = (ripplePhase / 255.0) * maxRadius;
  if (rippleRadius >= maxRadius) ripplePhase = 0;
  for (int x = 0; x < matrixWidth; x++) {
    for (int y = 0; y < matrixHeight; y++) {
      float dx = x - centerX;
      float dy = y - centerY;
      float distance = sqrt(dx * dx + dy * dy);
      float difference = abs(distance - rippleRadius);
      if (difference <= 1.0 && distance > radius) {
        int ledIndex = XY(x, y);
        if (ledIndex < numLeds) {
          uint8_t brightness = (1.0 - difference) * (255 - ripplePhase);
          leds[ledIndex] = CHSV(circleHue, 255, brightness);
        }
      }
    }
  }

  phase += 10;
  ripplePhase += 5;
}

void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  unsigned long timeout = millis();
  while (client.connected() && !client.available()) {
    if (millis() - timeout > 1000) {
      client.stop();
      return;
    }
    delay(1);
  }

  String request = "";
  while (client.available()) {
    char c = client.read();
    request += c;
    if (request.endsWith("\r\n\r\n")) break;
  }

  if (request.startsWith("GET /setPattern")) {
    int patternIndex = request.indexOf("pattern=");
    if (patternIndex != -1) {
      int pattern = request.substring(patternIndex + 8, request.indexOf(" ", patternIndex)).toInt();
      if (pattern >= 0 && pattern < NUM_PATTERNS) {
        currentPattern = static_cast<Pattern>(pattern);
        Serial.print("Pattern set to: ");
        Serial.println(pattern);
      }
    }
  } else if (request.startsWith("GET /setMatrixSize")) {
    int wIndex = request.indexOf("w=");
    int hIndex = request.indexOf("h=");
    if (wIndex != -1 && hIndex != -1) {
      int newWidth = request.substring(wIndex + 2, request.indexOf("&", wIndex)).toInt();
      int newHeight = request.substring(hIndex + 2, request.indexOf(" ", hIndex)).toInt();
      if (newWidth > 0 && newHeight > 0 && newWidth * newHeight <= MAX_LEDS) {
        matrixWidth = newWidth;
        matrixHeight = newHeight;
        numLeds = matrixWidth * matrixHeight;
        FastLED.clear();
        Serial.print("Matrix size set to: ");
        Serial.print(matrixWidth);
        Serial.print("x");
        Serial.println(matrixHeight);
      } else {
        Serial.println("Invalid matrix size");
      }
    }
  } else if (request.startsWith("GET /setColor")) {
    int hueIndex = request.indexOf("hue=");
    int defaultColorsIndex = request.indexOf("defaultColors=");
    if (hueIndex != -1) {
      selectedHue = request.substring(hueIndex + 4, request.indexOf("&", hueIndex) != -1 ? request.indexOf("&", hueIndex) : request.indexOf(" ", hueIndex)).toInt();
      if (selectedHue < 0 || selectedHue > 360) selectedHue = 0;
      Serial.print("Selected hue: ");
      Serial.println(selectedHue);
    }
    useDefaultColors = (defaultColorsIndex != -1 && request.substring(defaultColorsIndex + 13, defaultColorsIndex + 14) == "1");
    Serial.print("Use default colors: ");
    Serial.println(useDefaultColors ? "true" : "false");
  } else if (request.startsWith("GET /resetColor")) {
    selectedHue = defaultHues[currentPattern];
    useDefaultColors = true;
    Serial.print("Reset hue to pattern default: ");
    Serial.println(selectedHue);
  }

  client.print(html_part1);
  client.print(matrixWidth);
  client.print(html_part2);
  client.print(matrixHeight);
  client.print(html_part3);
  client.print(selectedHue);
   client.print(html_part4);
  client.print(selectedHue);
  client.print(html_part5);
  client.stop();
}

void loop() {
  handleClient();

  int packetSize = udp.parsePacket();
  if (packetSize) {
    uint8_t buffer[2 + NUM_BANDS];
    int len = udp.read(buffer, 2 + NUM_BANDS);
    if (len == 2 + NUM_BANDS && buffer[0] == 0x01 && buffer[1] == NUM_BANDS) {
      for (int i = 0; i < NUM_BANDS; i++) {
        amplitudes[i] = buffer[i + 2];
      }
      Serial.println("Received UDP data");
    }
  }

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 20) {
    switch (currentPattern) {
      case ENHANCED_BAR_GRAPH: enhancedBarGraph(); break;
      case MIRRORED_SPECTRUM: mirroredSpectrum(); break;
      case WATERFALL_EFFECT: waterfallEffect(); break;
      case PULSE_WAVE: pulseWave(); break;
      case PULSE_EXPANSION: pulseExpansion(); break;
      case VORTEX: vortex(); break;
      case SIMPLE_BAR_GRAPH: simpleBarGraph(); break;
      case COLOR_BASED: colorBased(); break;
      case RIPPLE_PER_BAND: ripplePerBand(); break;
      case FIRE_EFFECT: fireEffect(); break;
      case PARTICLE_EFFECT: particleEffect(); break;
      case CIRCULAR_SPECTRUM: circularSpectrum(); break;
      case RADIAL_FREQUENCY_BARS: radialFrequencyBars(); break;
      case SAKURA_GLOW: sakuraGlow(); break;
      case SAKURA_FLOWER: sakuraFlower(); break;
      case VOICE_WAVEFORM: voiceWaveform(); break;
      case SHOCKWAVE_RINGS: shockwaveRings(); break;
      case PIXEL_RAIN: pixelRain(); break;
      case WAVEFORM_ANIMATION: waveformAnimation(); break;
      case HEARTBEAT_CIRCLE: heartbeatCircle(); break;
      case CSE_SAKURA_WIND: cseSakuraWind(); break;
      case SAKURA_PETAL_CHASE: sakuraPetalChase(); break;
      case FALLING_PETALS_HANAMI: fallingPetalsHanami(); break;
      case CSE_SAKURA_BEAT_GLOW: cseSakuraBeatGlow(); break;
      default: enhancedBarGraph(); break;
    }
    FastLED.show();
    lastUpdate = millis();
  }

  static unsigned long lastHeapCheck = 0;
  if (millis() - lastHeapCheck > 5000) {
    Serial.print("Free heap in loop: ");
    Serial.println(ESP.getFreeHeap());
    lastHeapCheck = millis();
  }
}