# Advanced Raytracing with Reflections

A sophisticated 2D raytracing simulation featuring multiple objects and ray reflection.

## Features
- Multiple objects with different properties
- Ray reflection physics
- Configurable object reflectivity
- Interactive dragging of all objects
- This version implements proper reflection physics allowing rays to bounce off objects based on their reflectivity properties. The code uses batch processing to improve performance and supports up to 3 reflection bounces per ray


![250226_16h43m28s_screenshot](https://github.com/user-attachments/assets/88859f86-d393-4fc6-89f7-a8ac0e60d175)


## How to Run
```bash
# Compile the code
gcc main-3.c -o main-3 $(sdl2-config --cflags --libs) -lm

# Run the simulation
./main-3
