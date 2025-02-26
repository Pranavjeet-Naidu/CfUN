# Improved Raytracing Simulation

An enhanced version of the raytracing simulation with better performance and visuals.

## Features
- More efficient circle drawing algorithm using scanlines
- Optimized ray-circle intersection calculation
- Rays with visual effects (thickness and blur)
- Automatic obstacle movement
- Interactive light source ( click and drag to move the light source )
- Press ESC to exit the simulation
- Improved Performance 
- This version uses mathematical calculations to determine ray-circle intersections directly, rather than tracing pixels step by step, resulting in better performance and more accurate results.

## How to Run
```bash
# Compile the code
gcc main-2.c -o main-2 $(sdl2-config --cflags --libs) -lm

# Run the simulation
./main-2