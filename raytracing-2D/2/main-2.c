#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>

#define WIDTH 1200
#define HEIGHT 600
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0x00000000
#define COLOR_RAY 0xFFD43BFF
#define COLOR_RAY_BLUR 0xBD6800FF
#define RAYS_NUMBER 360
#define RAY_THICKNESS 2
#define BLUR_THICKNESS 5

typedef struct {
    double x;
    double y;
    double r;
    Uint32 color;
} Circle;

typedef struct {
    double x_start, y_start;
    double angle;
} Ray;

// Function declarations
void fill_circle(SDL_Surface* surface, Circle circle);
void generate_rays(Circle light, Ray rays[RAYS_NUMBER]);
void draw_thick_line(SDL_Surface* surface, int x0, int y0, int x1, int y1, Uint32 color, int thickness);
void draw_rays(SDL_Surface* surface, Ray rays[RAYS_NUMBER], Circle object);

void fill_circle(SDL_Surface* surface, Circle circle) {
    // More efficient circle drawing algorithm
    int radius = (int)circle.r;
    int centerX = (int)circle.x;
    int centerY = (int)circle.y;
    
    // Use horizontal scanlines for better performance
    for (int y = -radius; y <= radius; y++) {
        int height = sqrt(radius*radius - y*y);
        int drawY = centerY + y;
        
        // Skip if outside screen bounds
        if (drawY < 0 || drawY >= HEIGHT)
            continue;
            
        int startX = centerX - height;
        int endX = centerX + height;
        
        // Clamp to screen bounds
        startX = startX < 0 ? 0 : startX;
        endX = endX >= WIDTH ? WIDTH-1 : endX;
        
        // Draw horizontal line
        SDL_Rect line = {startX, drawY, endX - startX + 1, 1};
        SDL_FillRect(surface, &line, circle.color);
    }
}

void generate_rays(Circle light, Ray rays[RAYS_NUMBER]) {
    for (int i = 0; i < RAYS_NUMBER; i++) {
        double angle = ((double)i / RAYS_NUMBER) * 2 * M_PI;
        rays[i] = (Ray){light.x, light.y, angle};
    }
}

void draw_thick_line(SDL_Surface* surface, int x0, int y0, int x1, int y1, Uint32 color, int thickness) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1; 
    int err = dx + dy, e2;
    int half_thick = thickness / 2;

    while (1) {
        // Draw square centered at current pixel
        for (int i = -half_thick; i <= half_thick; i++) {
            for (int j = -half_thick; j <= half_thick; j++) {
                int drawX = x0 + i;
                int drawY = y0 + j;
                
                if (drawX >= 0 && drawX < WIDTH && drawY >= 0 && drawY < HEIGHT) {
                    ((Uint32*)surface->pixels)[drawY * surface->w + drawX] = color;
                }
            }
        }

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void draw_rays(SDL_Surface* surface, Ray rays[RAYS_NUMBER], Circle object) {
    for (int i = 0; i < RAYS_NUMBER; i++) {
        double x0 = rays[i].x_start;
        double y0 = rays[i].y_start;
        double angle = rays[i].angle;
        
        // Calculate intersection with object
        double dx = object.x - x0;
        double dy = object.y - y0;
        double a = 1.0;
        double b = 2.0 * (dx * cos(angle) + dy * sin(angle));
        double c = dx*dx + dy*dy - object.r*object.r;
        double disc = b*b - 4*a*c;
        
        double t_obj = -1;
        if (disc >= 0) {
            double sqrt_d = sqrt(disc);
            double t1 = (-b + sqrt_d) / (2*a);
            double t2 = (-b - sqrt_d) / (2*a);
            if (t1 > 0) t_obj = t1;
            if (t2 > 0 && t2 < t1) t_obj = t2;
        }

        // Calculate intersection with screen boundaries
        double t_screen = INFINITY;
        
        // Check horizontal boundaries
        if (cos(angle) != 0) {
            double tx = cos(angle) > 0 ? (WIDTH - x0)/cos(angle) : (-x0)/cos(angle);
            if (tx > 0) {
                double y = y0 + tx * sin(angle);
                if (y >= 0 && y <= HEIGHT) {
                    t_screen = fmin(t_screen, tx);
                }
            }
        }
        
        // Check vertical boundaries
        if (sin(angle) != 0) {
            double ty = sin(angle) > 0 ? (HEIGHT - y0)/sin(angle) : (-y0)/sin(angle);
            if (ty > 0) {
                double x = x0 + ty * cos(angle);
                if (x >= 0 && x <= WIDTH) {
                    t_screen = fmin(t_screen, ty);
                }
            }
        }

        // Determine end point
        double t_end = t_screen;
        if (t_obj > 0 && t_obj < t_screen) t_end = t_obj;
        if (t_end == INFINITY || t_end <= 0) continue;

        int x_end = (int)(x0 + t_end * cos(angle));
        int y_end = (int)(y0 + t_end * sin(angle));
        
        // Clamp coordinates to screen bounds
        x_end = fmax(0, fmin(WIDTH-1, x_end));
        y_end = fmax(0, fmin(HEIGHT-1, y_end));

        // Draw blur and main ray
        draw_thick_line(surface, (int)x0, (int)y0, x_end, y_end, COLOR_RAY_BLUR, BLUR_THICKNESS);
        draw_thick_line(surface, (int)x0, (int)y0, x_end, y_end, COLOR_RAY, RAY_THICKNESS);
    }
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Raytracing 2.0", 
                                        SDL_WINDOWPOS_CENTERED, 
                                        SDL_WINDOWPOS_CENTERED, 
                                        WIDTH, HEIGHT, 0);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    if (!surface) {
        printf("Surface could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Circle light = {200, 200, 15, COLOR_WHITE};
    Circle obstacle = {550, 300, 140, COLOR_WHITE};
    Ray rays[RAYS_NUMBER];
    generate_rays(light, rays);

    double obstacle_speed = 4;
    bool running = true;
    bool light_dragging = false;
    
    // Frame timing variables
    Uint32 frameStart, frameTime;
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    
    while (running) {
        frameStart = SDL_GetTicks();
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int mouseX = event.button.x;
                        int mouseY = event.button.y;
                        
                        // Check if clicked on light
                        double dx = mouseX - light.x;
                        double dy = mouseY - light.y;
                        if (sqrt(dx*dx + dy*dy) <= light.r) {
                            light_dragging = true;
                        }
                    }
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        light_dragging = false;
                    }
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (light_dragging) {
                        light.x = event.motion.x;
                        light.y = event.motion.y;
                        generate_rays(light, rays);
                    }
                    break;
                    
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
            }
        }

        // Update obstacle position
        obstacle.y += obstacle_speed;
        if (obstacle.y + obstacle.r > HEIGHT || obstacle.y - obstacle.r < 0) {
            obstacle_speed *= -1;
        }

        // Clear screen
        SDL_FillRect(surface, NULL, COLOR_BLACK);
        
        // Draw elements
        draw_rays(surface, rays, obstacle);
        fill_circle(surface, obstacle);
        fill_circle(surface, light);

        SDL_UpdateWindowSurface(window);
        
        // Frame timing
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}