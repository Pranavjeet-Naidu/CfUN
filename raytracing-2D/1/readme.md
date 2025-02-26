# Basic Raytracing Simulation

This is a simple 2D raytracing simulation using SDL2.

## Features
- Light rays emitted from a source point in all directions
- Ray-object intersection detection
- Moving obstacle that bounces within the screen boundaries
- Uses a simple approach where rays are traced pixel by pixel until they hit an object or reach the screen boundary.
- Interactive light source - click and drag to move the light source 
- Close the window to exit the simulation 

## How to Run
```bash
# Compile the code
gcc main.c -o main $(sdl2-config --cflags --libs) -lm

# Run the simulation
./main