# AuraSymphony
ESP32 LED Matrix Display with Audio-Reactive Patterns

### Overview

-This project is an audio-reactive LED matrix display controlled by an ESP32 microcontroller, featuring vibrant patterns like Enhanced Bar Graph, Sakura Bloom, Falling Petals, Shockwave Rings, and Pixel Rain. It uses real-time audio processing via a Python script and a web interface hosted on the ESP32 for remote control of colors and patterns.


### Features

- **Audio-Reactive Visuals:** Processes real-time audio using Python (pyaudio, numpy) to map 18 frequency bands across a 31x31 WS2812B LED matrix.
- **Dynamic Patterns:** Includes Enhanced Bar Graph, Sakura Bloom, Falling Petals, Shockwave Rings, Pixel Rain, and more, optimized for performance.
- **Web Interface:** ESP32-hosted web server with a color wheel for remote control of patterns and colors.
- **Efficient Design:** Uses precomputed tables, separate ESP32 cores for UDP/web tasks, and optimized serial communication at 115200 baud.
- **Customizable:** Easily extendable with new patterns and adjustable parameters like the matrix size and different gradients of colours
- **Remote control ability:** The ESP32 is hosting a server and a frontend which can be controlled locally and change dimension sizes and patterns.

#### Remote Control interface

![WhatsApp Image 2025-07-29 at 21 35 45_5a8f9ef6](https://github.com/user-attachments/assets/95081333-1d8b-42e1-91d2-e7bf3bdbe58b)

![WhatsApp Image 2025-07-29 at 21 35 45_68e7d4b3](https://github.com/user-attachments/assets/7b83581b-a3b2-447e-82b9-926a82e4f1e4)

![WhatsApp Image 2025-07-29 at 21 35 46_911b26bf](https://github.com/user-attachments/assets/f6eee48d-5336-4488-be9e-d45157d2ed07)

![WhatsApp Image 2025-07-29 at 21 35 46_6095ad0a](https://github.com/user-attachments/assets/d1a6388b-57fd-4fa1-8d28-c304bb574a03)



