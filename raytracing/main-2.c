#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>

#define WIDTH 1200
#define HEIGHT 600
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0x00000000
#define COLOR_RAY 0xFFD43BFF
#define COLOR_RAY_BLUR 0xBD6800FF
#define RAYS_NUMBER 360
#define RAY_THICKNESS 2
#define BLUR_THICKNESS 5

struct Circle {
    double x;
    double y;
    double r;
};

struct Ray {
    double x_start, y_start;
    double angle;
};

void FillCircle(SDL_Surface* surface, struct Circle circle, Uint32 color) {
    int radius_squared = (int)(circle.r * circle.r);
    for (int x = (int)(circle.x - circle.r); x <= (int)(circle.x + circle.r); x++) {
        for (int y = (int)(circle.y - circle.r); y <= (int)(circle.y + circle.r); y++) {
            if ((x - circle.x) * (x - circle.x) + (y - circle.y) * (y - circle.y) <= radius_squared) {
                SDL_Rect pixel = {x, y, 1, 1};
                SDL_FillRect(surface, &pixel, color);
            }
        }
    }
}

void generate_rays(struct Circle circle, struct Ray rays[RAYS_NUMBER]) {
    for (int i = 0; i < RAYS_NUMBER; i++) {
        double angle = ((double)i / RAYS_NUMBER) * 2 * M_PI;
        rays[i] = (struct Ray){circle.x, circle.y, angle};
    }
}

void draw_thick_line(SDL_Surface* surface, int x0, int y0, int x1, int y1, Uint32 color, int thickness) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1; 
    int err = dx + dy, e2;
    int half_thick = thickness / 2;

    while (1) {
        for (int i = -half_thick; i <= half_thick; i++) {
            for (int j = -half_thick; j <= half_thick; j++) {
                if (x0 + i >= 0 && x0 + i < WIDTH && y0 + j >= 0 && y0 + j < HEIGHT) {
                    SDL_Rect pixel = {x0 + i, y0 + j, 1, 1};
                    SDL_FillRect(surface, &pixel, color);
                }
            }
        }

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void DrawRays(SDL_Surface* surface, struct Ray rays[RAYS_NUMBER], struct Circle object) {
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
        if (cos(angle) != 0) {
            double tx = cos(angle) > 0 ? (WIDTH - x0)/cos(angle) : (-x0)/cos(angle);
            if (tx > 0) {
                double y = y0 + tx * sin(angle);
                if (y >= 0 && y <= HEIGHT) t_screen = fmin(t_screen, tx);
            }
        }
        if (sin(angle) != 0) {
            double ty = sin(angle) > 0 ? (HEIGHT - y0)/sin(angle) : (-y0)/sin(angle);
            if (ty > 0) {
                double x = x0 + ty * cos(angle);
                if (x >= 0 && x <= WIDTH) t_screen = fmin(t_screen, ty);
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
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Raytracing 2.0", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface* surface = SDL_GetWindowSurface(window);

    struct Circle light = {200, 200, 15};
    struct Circle obstacle = {550, 300, 140};
    struct Ray rays[RAYS_NUMBER];
    generate_rays(light, rays);

    double obstacle_speed = 4;
    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_MOUSEMOTION && event.motion.state) {
                light.x = event.motion.x;
                light.y = event.motion.y;
                generate_rays(light, rays);
            }
        }

        // Update obstacle position
        obstacle.y += obstacle_speed;
        if (obstacle.y + obstacle.r > HEIGHT || obstacle.y - obstacle.r < 0)
            obstacle_speed *= -1;

        // Clear screen
        SDL_FillRect(surface, NULL, COLOR_BLACK);
        
        // Draw elements
        DrawRays(surface, rays, obstacle);
        FillCircle(surface, light, COLOR_WHITE);
        FillCircle(surface, obstacle, COLOR_WHITE);

        SDL_UpdateWindowSurface(window);
        SDL_Delay(10);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}