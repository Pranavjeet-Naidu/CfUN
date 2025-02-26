#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>

#define WIDTH 1200
#define HEIGHT 600
#define BATCH_SIZE 64
#define RAYS_NUMBER 500
#define RAY_THICKNESS 3
#define MAX_OBJECTS 10
#define MAX_REFLECTION_DEPTH 3

// Colors
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0x000000FF
#define COLOR_RAY 0xFFD43BFF
#define COLOR_RAY_BLUR 0xBD6800FF
#define COLOR_RED 0xFF0000FF
#define COLOR_BLUE 0x0000FFFF
#define COLOR_YELLOW 0xFFFF00FF
#define COLOR_CYAN 0x00FFFFFF
#define COLOR_PINK 0xFF69B4FF
#define min(a,b) ((a) < (b) ? (a) : (b))
// First define the types
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
    Uint32 color;
    double intensity;
    int depth;
} Ray;

typedef struct {
    Circle circles[MAX_OBJECTS];
    int circle_count;
    Vector2 light_source;
    Ray rays[RAYS_NUMBER];
} Scene;

// Now define global variables AFTER the types are declared
static Circle* selected_circle = NULL;
static bool dragging = false;
static bool simulation_running = true;

// Function declarations
void draw_circle(SDL_Surface* surface, Circle circle);
void trace_ray(SDL_Surface* surface, Scene* scene, Ray ray);
void trace_ray_batch(SDL_Surface* surface, Scene* scene, int start_index, int end_index);
void cleanup_scene(Scene* scene);
Vector2 normalize(Vector2 v);
bool ray_circle_intersection(Ray ray, Circle circle, double* t, Vector2* normal);
void generate_rays(Scene* scene);
void init_scene(Scene* scene);

// Function to draw a filled circle using the midpoint algorithm
void draw_circle(SDL_Surface* surface, Circle circle) {
    int x0 = (int)circle.position.x;
    int y0 = (int)circle.position.y;
    int radius = (int)circle.radius;
    
    // Draw filled circle using horizontal lines
    for (int y = -radius; y <= radius; y++) {
        int width = sqrt(radius*radius - y*y);
        for (int x = -width; x <= width; x++) {
            int drawX = x0 + x;
            int drawY = y0 + y;
            if (drawX >= 0 && drawX < WIDTH && drawY >= 0 && drawY < HEIGHT) {
                ((Uint32*)surface->pixels)[drawY * surface->w + drawX] = circle.color;
            }
        }
    }
}

void trace_ray_batch(SDL_Surface* surface, Scene* scene, int start_index, int end_index) {
    for (int i = start_index; i < end_index && i < RAYS_NUMBER; i++) {
        trace_ray(surface, scene, scene->rays[i]);
    }
}

void cleanup_scene(Scene* scene) {
    // Add any necessary cleanup code here
    scene->circle_count = 0;
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
        double angle = (2.0 * M_PI * i) / RAYS_NUMBER;
        Vector2 direction = {cos(angle), sin(angle)};
        scene->rays[i] = (Ray){
            scene->light_source,
            direction,
            COLOR_RAY,
            1.0,
            0
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
        // If no intersection, limit ray length
        Vector2 end = {
            ray.start.x + ray.direction.x * WIDTH,
            ray.start.y + ray.direction.y * WIDTH
        };
        max_distance = sqrt(WIDTH * WIDTH + HEIGHT * HEIGHT);
    }

    // Draw the ray
    for (double t = 0; t < max_distance; t += 1.0) {
        int x = (int)(ray.start.x + ray.direction.x * t);
        int y = (int)(ray.start.y + ray.direction.y * t);
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
            Uint32 color = ray.intensity < 0.5 ? COLOR_RAY_BLUR : ray.color;
            ((Uint32*)surface->pixels)[y * surface->w + x] = color;
        }
    }

    // Handle reflections
    if (closest_object != -1) {
        Vector2 reflection_dir = {
            ray.direction.x - 2 * closest_normal.x * (ray.direction.x * closest_normal.x + ray.direction.y * closest_normal.y),
            ray.direction.y - 2 * closest_normal.y * (ray.direction.x * closest_normal.x + ray.direction.y * closest_normal.y)
        };
        reflection_dir = normalize(reflection_dir);
        
        Vector2 intersection = {
            ray.start.x + ray.direction.x * closest_t,
            ray.start.y + ray.direction.y * closest_t
        };
        
        Ray reflected_ray = {
            intersection,
            reflection_dir,
            ray.color,
            ray.intensity * scene->circles[closest_object].reflectivity,
            ray.depth + 1
        };
        
        trace_ray(surface, scene, reflected_ray);
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
        {300, 300},  // position
        20,          // radius
        COLOR_PINK,  // color
        0.0          // reflectivity
    };
    
    // Add white circle
    scene->circles[scene->circle_count++] = (Circle){
        {600, 300},  // position
        50,          // radius
        COLOR_WHITE, // color
        0.0          // reflectivity
    };
    
    // Add reflective circle (cyan)
    scene->circles[scene->circle_count++] = (Circle){
        {900, 300},  // position
        70,          // radius
        COLOR_CYAN,  // color
        0.8          // reflectivity
    };
    
    // Generate initial rays
    generate_rays(scene);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Raytracing Simulation", 
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
    
    Scene scene;
    init_scene(&scene);
    
    SDL_Event event;
    Uint32 frameStart, frameTime;
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    double obstacle_speed_y = 2.0;
    
    while (simulation_running) {
        frameStart = SDL_GetTicks();
        
        // Event handling
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    simulation_running = false;
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (dragging && selected_circle != NULL) {
                        selected_circle->position.x = event.motion.x;
                        selected_circle->position.y = event.motion.y;
                        if (selected_circle == &scene.circles[0]) {
                            scene.light_source.x = event.motion.x;
                            scene.light_source.y = event.motion.y;
                            generate_rays(&scene);
                        }
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int mouse_x = event.button.x;
                        int mouse_y = event.button.y;
                        
                        for (int i = 0; i < scene.circle_count; i++) {
                            double dx = mouse_x - scene.circles[i].position.x;
                            double dy = mouse_y - scene.circles[i].position.y;
                            double distance = sqrt(dx*dx + dy*dy);
                            
                            if (distance <= scene.circles[i].radius) {
                                selected_circle = &scene.circles[i];
                                dragging = true;
                                obstacle_speed_y = 0;
                                break;
                            }
                        }
                    }
                    break;

                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        dragging = false;
                        selected_circle = NULL;
                    }
                    break;
                    
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_r:
                            init_scene(&scene);
                            break;
                        case SDLK_ESCAPE:
                            simulation_running = false;
                            break;
                    }
                    break;
            }
        }
        
        // Update game state
        SDL_FillRect(surface, NULL, COLOR_BLUE);
        
        scene.circles[1].position.y += obstacle_speed_y;
        if (scene.circles[1].position.y - scene.circles[1].radius < 0 ||
            scene.circles[1].position.y + scene.circles[1].radius > HEIGHT) {
            obstacle_speed_y = -obstacle_speed_y;
        }
        
        // Render in batches
        for (int i = 0; i < RAYS_NUMBER; i += BATCH_SIZE) {
            trace_ray_batch(surface, &scene, i, min(i + BATCH_SIZE, RAYS_NUMBER));
        }
        
        // Draw circles
        for (int i = 0; i < scene.circle_count; i++) {
            draw_circle(surface, scene.circles[i]);
        }
        
        SDL_UpdateWindowSurface(window);
        
        // Frame timing
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }
    
    // Cleanup
    cleanup_scene(&scene);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}