#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>

#define WIDTH 1200
#define HEIGHT 600

// Colors in SDL format (ARGB on little endian systems)
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0x000000FF
#define COLOR_RAY 0xFFD43BFF
#define COLOR_RAY_BLUR 0xBD6800FF
#define COLOR_RED 0xFF0000FF
#define COLOR_BLUE 0x0000FFFF
#define COLOR_YELLOW 0xFFFF00FF
#define COLOR_CYAN 0x00FFFFFF
#define COLOR_PINK 0xFF69B4FF

#define RAYS_NUMBER 500
#define RAY_THICKNESS 3
#define MAX_OBJECTS 10
#define MAX_REFLECTION_DEPTH 3

typedef struct {
    double x;
    double y;
} Vector2;

typedef struct {
    Vector2 position;
    double radius;
    Uint32 color;
    double reflectivity;
} Circle;

typedef struct {
    Vector2 start;
    Vector2 direction;
    Uint32 color;  // Ensures ray has a color field
    double intensity;
    int depth;
} Ray;

typedef struct {
    Circle circles[MAX_OBJECTS];
    int circle_count;
    Vector2 light_source;
    Ray rays[RAYS_NUMBER];
} Scene;

// Function to draw a filled circle using the midpoint algorithm
void draw_circle(SDL_Surface* surface, Circle circle) {
    int x0 = (int)circle.position.x;
    int y0 = (int)circle.position.y;
    int radius = (int)circle.radius;
    
    // Draw filled circle using horizontal lines
    for (int y = -radius; y <= radius; y++) {
        int x_extent = (int)sqrt(radius * radius - y * y);
        
        SDL_Rect line = {x0 - x_extent, y0 + y, x_extent * 2 + 1, 1};
        SDL_FillRect(surface, &line, circle.color);
    }
}

// Function to normalize a vector
Vector2 normalize(Vector2 v) {
    double length = sqrt(v.x * v.x + v.y * v.y);
    if (length < 0.0001) {
        return (Vector2){0, 0};
    }
    return (Vector2){v.x / length, v.y / length};
}

// Function to check ray-circle intersection
bool ray_circle_intersection(Ray ray, Circle circle, double* t, Vector2* normal) {
    Vector2 oc = {ray.start.x - circle.position.x, ray.start.y - circle.position.y};
    
    double a = ray.direction.x * ray.direction.x + ray.direction.y * ray.direction.y;
    double b = 2.0 * (oc.x * ray.direction.x + oc.y * ray.direction.y);
    double c = oc.x * oc.x + oc.y * oc.y - circle.radius * circle.radius;
    double discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) {
        return false;
    }
    
    double t1 = (-b - sqrt(discriminant)) / (2.0 * a);
    double t2 = (-b + sqrt(discriminant)) / (2.0 * a);
    
    // Get the closest intersection point in front of the ray
    if (t1 > 0.001) {
        *t = t1;
    } else if (t2 > 0.001) {
        *t = t2;
    } else {
        return false;
    }
    
    // Calculate the intersection point
    Vector2 intersection = {
        ray.start.x + ray.direction.x * (*t),
        ray.start.y + ray.direction.y * (*t)
    };
    
    // Calculate the normal at the intersection point
    *normal = normalize((Vector2){
        intersection.x - circle.position.x,
        intersection.y - circle.position.y
    });
    
    return true;
}

// Function to generate rays from the light source in all directions
void generate_rays(Scene* scene) {
    for (int i = 0; i < RAYS_NUMBER; i++) {
        double angle = ((double)i / RAYS_NUMBER) * 2 * M_PI;
        scene->rays[i] = (Ray){
            .start = scene->light_source,
            .direction = {cos(angle), sin(angle)},
            .color = COLOR_RAY,  // Initial color
            .intensity = 1.0,
            .depth = 0
        };
    }
}


// Function to trace a ray and draw it on the surface
void trace_ray(SDL_Surface* surface, Scene* scene, Ray ray) {
    if (ray.depth >= MAX_REFLECTION_DEPTH || ray.intensity < 0.1) {
        return;
    }

    double closest_t = INFINITY;
    int closest_object = -1;
    Vector2 closest_normal;

    // Skip checking intersection with the light source itself (index 0)
    for (int i = 1; i < scene->circle_count; i++) {
        double t;
        Vector2 normal;
        if (ray_circle_intersection(ray, scene->circles[i], &t, &normal)) {
            if (t < closest_t) {
                closest_t = t;
                closest_object = i;
                closest_normal = normal;
            }
        }
    }

    // Determine max distance to render the ray
    double max_distance = closest_t;
    if (isinf(max_distance)) {
        double t_min = INFINITY;
        if (ray.direction.x < 0) t_min = fmin(t_min, -ray.start.x / ray.direction.x);
        if (ray.direction.x > 0) t_min = fmin(t_min, (WIDTH - ray.start.x) / ray.direction.x);
        if (ray.direction.y < 0) t_min = fmin(t_min, -ray.start.y / ray.direction.y);
        if (ray.direction.y > 0) t_min = fmin(t_min, (HEIGHT - ray.start.y) / ray.direction.y);
        max_distance = t_min;
    }

    // Draw the ray
    for (double t = 0; t < max_distance; t += 1.0) {
        int x = (int)(ray.start.x + ray.direction.x * t);
        int y = (int)(ray.start.y + ray.direction.y * t);
        if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) continue;

        // Use the ray's color
        Uint32 color = ray.color;
        for (int dx = -RAY_THICKNESS/2; dx <= RAY_THICKNESS/2; dx++) {
            for (int dy = -RAY_THICKNESS/2; dy <= RAY_THICKNESS/2; dy++) {
                if (x + dx >= 0 && x + dx < WIDTH && y + dy >= 0 && y + dy < HEIGHT) {
                    SDL_Rect pixel = {x + dx, y + dy, 1, 1};
                    SDL_FillRect(surface, &pixel, color);
                }
            }
        }
    }

    // Handle reflections
    if (closest_object != -1) {
        Circle* hit_circle = &scene->circles[closest_object];
        if (hit_circle->reflectivity > 0.1 && ray.depth < MAX_REFLECTION_DEPTH - 1) {
            Vector2 hit_point = {
                ray.start.x + ray.direction.x * closest_t,
                ray.start.y + ray.direction.y * closest_t
            };

            // Compute reflection direction
            double dot_product = ray.direction.x * closest_normal.x + ray.direction.y * closest_normal.y;
            Vector2 reflection_dir = {
                ray.direction.x - 2.0 * dot_product * closest_normal.x,
                ray.direction.y - 2.0 * dot_product * closest_normal.y
            };
            reflection_dir = normalize(reflection_dir);

            // Create reflection ray
            Ray reflection = {
                .start = {hit_point.x + closest_normal.x * 0.1, hit_point.y + closest_normal.y * 0.1},
                .direction = reflection_dir,
                .color = hit_circle->color,  // Reflected ray adopts the object's color
                .intensity = ray.intensity * hit_circle->reflectivity,
                .depth = ray.depth + 1
            };

            // Trace the reflected ray
            trace_ray(surface, scene, reflection);
        }
    }
}

// Initialize scene with objects
void init_scene(Scene* scene) {
    // Clear scene
    scene->circle_count = 0;
    
    // Set light source
    scene->light_source = (Vector2){300, 300};
    
    // Add light source circle (pink)
    scene->circles[scene->circle_count++] = (Circle){
        .position = scene->light_source,
        .radius = 20,
        .color = COLOR_PINK,
        .reflectivity = 0.0
    };
    
    // Add white circle
    scene->circles[scene->circle_count++] = (Circle){
        .position = {550, 300},
        .radius = 80,
        .color = COLOR_WHITE,
        .reflectivity = 0.2
    };
    
    // Add reflective circle (cyan)
    scene->circles[scene->circle_count++] = (Circle){
        .position = {800, 300},
        .radius = 100,
        .color = COLOR_CYAN,
        .reflectivity = 0.8
    };
    
    // Generate initial rays
    generate_rays(scene);
}

int main() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow("Raytracing Simulation", 
                                         SDL_WINDOWPOS_CENTERED, 
                                         SDL_WINDOWPOS_CENTERED, 
                                         WIDTH, HEIGHT, 0);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Get window surface
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    if (!surface) {
        printf("Surface could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Initialize scene
    Scene scene;
    init_scene(&scene);
    
    // Main game loop variables
    bool simulation_running = true;
    SDL_Event event;
    Uint32 frameStart, frameTime;
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    
    // Movement parameters
    double obstacle_speed_y = 2.0;
    
    // Main loop
    while (simulation_running) {
        frameStart = SDL_GetTicks();
        
        // Process events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    simulation_running = false;
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (event.motion.state != 0) {
                        // Update light source position
                        scene.light_source.x = event.motion.x;
                        scene.light_source.y = event.motion.y;
                        
                        // Update the first circle (light source visual)
                        scene.circles[0].position = scene.light_source;
                        
                        // Regenerate rays
                        generate_rays(&scene);
                    }
                    break;
                    
                case SDL_KEYDOWN:
                    // Add keyboard controls
                    switch (event.key.keysym.sym) {
                        case SDLK_r:
                            // Reset scene
                            init_scene(&scene);
                            break;
                            
                        case SDLK_ESCAPE:
                            simulation_running = false;
                            break;
                    }
                    break;
            }
        }
        
        // Clear the screen with blue background
        SDL_FillRect(surface, NULL, COLOR_BLUE);
        
        // Update obstacle position
        scene.circles[1].position.y += obstacle_speed_y;
        if (scene.circles[1].position.y - scene.circles[1].radius < 0 ||
            scene.circles[1].position.y + scene.circles[1].radius > HEIGHT) {
            obstacle_speed_y = -obstacle_speed_y;
        }
        
        // Trace all rays (do this before drawing circles)
        for (int i = 0; i < RAYS_NUMBER; i++) {
            trace_ray(surface, &scene, scene.rays[i]);
        }
        
        // Draw all circles
        for (int i = 0; i < scene.circle_count; i++) {
            draw_circle(surface, scene.circles[i]);
        }
        
        // Update the screen
        SDL_UpdateWindowSurface(window);
        
        // Cap the frame rate
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }
    
    // Clean up
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}