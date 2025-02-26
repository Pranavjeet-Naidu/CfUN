# CNake - Classic Snake Game in C

A traditional Snake game implemented in C using SDL2 graphics library.

## Overview

CNake is a C implementation of the classic Snake game where you control a snake that grows longer as it eats apples. The game features:

- Grid-based gameplay
- Keyboard controls for movement
- Score tracking
- Regular and bonus (golden) apples
- Increasing difficulty (speed) as you progress
- Game over screen with final score

## Requirements

- SDL2
- SDL2_ttf
- C compiler (gcc recommended)
- Standard font (FreeSansBold.ttf or DejaVuSans-Bold.ttf)

## Building

```sh
# Compile the game
gcc cnake.c -o cnake -lSDL2 -lSDL2_ttf -lm

# Run the game
./cnake
```

## Gameplay

- Control the snake using the arrow keys (↑, ↓, ←, →)
- Eat red apples to grow and increase your score
- Regular apples are worth 1 point
- Golden apples (rarer) are worth 10 points
- Avoid hitting the walls or the snake's own body
- The snake moves faster as your score increases
- When game over, press ESC to exit

## Implementation Details

- Uses a linked list structure to represent the snake body
- Snake movement handled through directional updates
- Collision detection for boundaries and self-collision
- Random apple generation with special bonus apples
- Text rendering for score display and game over screen

## Code Structure

The game is implemented in a single C file (`cnake.c`) containing:

- Game initialization and setup
- Input handling
- Snake movement and growth logic
- Collision detection
- Rendering functions
- Memory management