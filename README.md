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

![WhatsApp Image 2025-07-29 at 21 35 45_3603966f](https://github.com/user-attachments/assets/56850e13-5d5c-4c3e-bbb2-d58df664b038)

![WhatsApp Image 2025-07-29 at 21 35 45_af4115d5](https://github.com/user-attachments/assets/740ac3ae-7104-4e5c-82ef-fd6f4c2d1e14)

![WhatsApp Image 2025-07-29 at 21 35 46_417f859e](https://github.com/user-attachments/assets/60d81e66-b516-4d62-b72f-0e9f89669171)

![WhatsApp Image 2025-07-29 at 21 35 46_f70b61b3](https://github.com/user-attachments/assets/3f726b33-4c38-43b7-b965-dc04c511c359)





