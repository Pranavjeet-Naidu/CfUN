#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>  // Add TTF library for text rendering
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 900
#define HEIGHT 600

#define CELL_SIZE 30
#define ROWS HEIGHT/CELL_SIZE
#define COLUMNS WIDTH/CELL_SIZE
#define LINE_WIDTH 2

#define COLOR_BLACK 0x00000000
#define COLOR_GRID 0x1f1f1f1f
#define COLOR_WHITE 0xffffffff
#define COLOR_APPLE 0x00ff0000
#define COLOR_GOLDEN_APPLE 0xFFD700FF  // Golden apple for bonus points
#define COLOR_GAME_OVER 0xFF00FFFF     // Game over text color

#define SNAKE(x,y) fill_cell(psurface,x,y,COLOR_WHITE) 
#define APPLE(x,y,color) fill_cell(psurface,x,y,color)
#define DRAW_GRID draw_grid(psurface)

#define INITIAL_DELAY 200
#define MIN_DELAY 50
#define DELAY_DECREMENT 5
#define BONUS_APPLE_CHANCE 5  // 1 in 5 chance for bonus apple

/* Like a LinkedList */
struct SnakeElement
{
    int x, y;
    /* Can be NULL (in case of last element) */
    struct SnakeElement *pnext;
};

/* Only one value should be != 0 */
struct Direction
{
    int dx,dy;
};

struct Apple
{
    int x,y;
    int bonus;  // Whether this is a bonus apple (worth more points)
    Uint32 color;
};

struct GameState {
    int score;
    int delay;
    int is_game_over;
    int game_started;  // Added flag to track game start
};

// Function declarations
void render_text(SDL_Surface* psurface, TTF_Font* font, const char* text, SDL_Color color, int x, int y);
void show_game_over(SDL_Surface* psurface, TTF_Font* font, int score);

int draw_grid(SDL_Surface* psurface)
{
    SDL_Rect row_line = {0,0,900,LINE_WIDTH};
    for (row_line.y = 0; row_line.y < HEIGHT; row_line.y += CELL_SIZE )
        SDL_FillRect(psurface, &row_line, COLOR_GRID);

    SDL_Rect col_line = {0,0,LINE_WIDTH, HEIGHT};
    for (col_line.x = 0; col_line.x < WIDTH; col_line.x += CELL_SIZE )
        SDL_FillRect(psurface, &col_line, COLOR_GRID);
}

void fill_cell(SDL_Surface* psurface, int x, int y, Uint32 color)
{
    SDL_Rect rect = {x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_FillRect(psurface, &rect, color);
}

// Render text to the surface
void render_text(SDL_Surface* psurface, TTF_Font* font, const char* text, SDL_Color color, int x, int y)
{
    SDL_Surface* text_surface = TTF_RenderText_Solid(font, text, color);
    if (text_surface == NULL) {
        printf("Failed to render text: %s\n", TTF_GetError());
        return;
    }

    SDL_Rect dest = {x, y, text_surface->w, text_surface->h};
    SDL_BlitSurface(text_surface, NULL, psurface, &dest);
    SDL_FreeSurface(text_surface);
}

// Show game over screen
void show_game_over(SDL_Surface* psurface, TTF_Font* font, int score)
{
    SDL_Rect rect = {0, 0, WIDTH, HEIGHT};
    SDL_FillRect(psurface, &rect, COLOR_BLACK);

    SDL_Color color = {255, 0, 255, 255};
    char game_over_text[50];
    sprintf(game_over_text, "GAME OVER");
    render_text(psurface, font, game_over_text, color, WIDTH/2 - 150, HEIGHT/2 - 60);

    char score_text[50];
    sprintf(score_text, "Final Score: %d", score);
    render_text(psurface, font, score_text, color, WIDTH/2 - 120, HEIGHT/2);

    render_text(psurface, font, "Press ESC to quit", color, WIDTH/2 - 150, HEIGHT/2 + 60);
}

size_t snake_size(struct SnakeElement **ppsnake)
{
    assert(ppsnake != NULL);
    assert(*ppsnake != NULL);

    size_t list_size = 1;
    struct SnakeElement *current = *ppsnake;
    while (current->pnext != NULL)
    {
        current = current->pnext;
        list_size++;
    }
    return list_size;
}

void draw_snake(SDL_Surface* psurface, struct SnakeElement **ppsnake)
{
    assert(psurface != NULL);
    assert(ppsnake != NULL);
    assert(*ppsnake != NULL);

    struct SnakeElement *psnake = *ppsnake;
    int debug_snake_size = 0;
    while (psnake != NULL)
    {
        SNAKE(psnake->x, psnake->y);
        psnake = psnake->pnext;
        debug_snake_size++;
    }
    assert(snake_size(ppsnake) == debug_snake_size);
}

void print_snake(struct SnakeElement **ppsnake)
{
    struct SnakeElement *psnake = *ppsnake;
    int counter = 0;
    while (psnake != NULL)
    {
        printf("Element No. %d\n", counter);
        printf("Snake x=%d\n", psnake->x);
        printf("Snake y=%d\n", psnake->y);
        printf("Snake self=%p\n", psnake);
        printf("Snake pnext=%p\n", psnake->pnext);
        psnake = psnake->pnext;
        counter++;
    }
}

void move_snake(struct SnakeElement **ppsnake, struct Direction *pdirection)
{
    assert(ppsnake != NULL);
    assert(*ppsnake != NULL);

    printf("direction dx=%d, dy=%d\n", pdirection->dx, pdirection->dy);
    if(pdirection->dx == 0 && pdirection->dy == 0)
    {
        return;
    }

    size_t size = snake_size(ppsnake);
    if (size == 1)
    {
        (*ppsnake)->x += pdirection->dx;
        (*ppsnake)->y += pdirection->dy;
    }
    else
    {
        struct SnakeElement *pcurrent = *ppsnake;
        int previous_x = pcurrent->x;
        int previous_y = pcurrent->y;

        pcurrent->x += pdirection->dx;
        pcurrent->y += pdirection->dy;

        while ( pcurrent->pnext != NULL )
        {
            pcurrent = pcurrent->pnext;

            int temp_x = pcurrent->x; 
            int temp_y = pcurrent->y; 

            pcurrent->x = previous_x;
            pcurrent->y = previous_y;

            previous_x = temp_x;
            previous_y = temp_y;

        }
    }
    printf("Snake size =%zu\n", size);
}

void reset_apple(struct SnakeElement *psnake, struct Apple *papple)
{
    papple->x = COLUMNS * ((double) rand() / RAND_MAX);
    papple->y = ROWS * ((double) rand() / RAND_MAX);

    // Randomly decide if this is a bonus apple
    if (rand() % BONUS_APPLE_CHANCE == 0) {
        papple->bonus = 1;
        papple->color = COLOR_GOLDEN_APPLE;
    } else {
        papple->bonus = 0;
        papple->color = COLOR_APPLE;
    }

    /* If apple coordinates collide with snake, try again */
    struct SnakeElement *pcurrent = psnake;
    do
    {
        if (pcurrent->x == papple->x && pcurrent->y == papple->y)
        {
            reset_apple(psnake, papple);
            break; 
        }
        pcurrent = pcurrent->pnext;
    }
    while (pcurrent != NULL);
}

void lengthen_snake(struct SnakeElement **ppsnake, struct Direction *pdirection)
{
    struct SnakeElement *head = malloc(sizeof(struct SnakeElement *));
    head->x = (*ppsnake)->x + pdirection->dx;
    head->y = (*ppsnake)->y + pdirection->dy;
    head->pnext = *ppsnake;

    *ppsnake = head;
}

int check_collision(struct SnakeElement **ppsnake)
{
    assert(ppsnake != NULL);
    assert(*ppsnake != NULL);

    struct SnakeElement snake = **ppsnake;
    if (snake.x < 0 || snake.y < 0
            || snake.x > COLUMNS || snake.y > ROWS)
    {
        printf("Snake hit boundary.\n");
        return 1;
    }

    struct SnakeElement *psnake = *ppsnake;
    while(psnake->pnext != NULL)
    {
        psnake = psnake->pnext;
        if(snake.x == psnake->x && snake.y == psnake->y)
        {      
            printf("Snake hit itself.\n");
            return 1;
        }
    }

    return 0;
}

int main()
{
    printf("Hello Snake\n");
    srand(time(NULL));  // Initialize random seed

    // Initialize SDL and TTF
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    
    SDL_Window* window = SDL_CreateWindow("Classic Snake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Surface* psurface = SDL_GetWindowSurface(window);
    
    // Load font
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 28);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans-Bold.ttf", 28); // Try another common font
        if (!font) {
            printf("Failed to load backup font, continuing without text\n");
        }
    }

    SDL_Event event;

    struct SnakeElement *psnake = malloc(sizeof(struct SnakeElement));
    struct SnakeElement *psnakeTail = malloc(sizeof(struct SnakeElement));
    struct SnakeElement *psnakeTail2 = malloc(sizeof(struct SnakeElement));

    psnake->x = 5;
    psnake->y=5;
    psnake->pnext=psnakeTail;

    psnakeTail->x = 5;
    psnakeTail->y=6;
    psnakeTail->pnext=psnakeTail2;

    psnakeTail2->x = 5;
    psnakeTail2->y=7;
    psnakeTail2->pnext=NULL;

    struct SnakeElement **ppsnake = &psnake;
    struct Direction direction = {0,0};
    struct Direction *pdirection = &direction;
    struct Apple apple = {0, 0, 0, COLOR_APPLE};
    struct Apple *papple = &apple;
    reset_apple(psnake, papple);

    struct GameState gameState = {0, INITIAL_DELAY, 0, 0};  // Initialize game_started to 0

    SDL_Rect override_rect = {0,0,WIDTH,HEIGHT};
    SDL_Color score_color = {255, 255, 255, 255};

    int game = 1;
    while(game)
    {
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                game = 0;   
            if (event.type == SDL_KEYDOWN)
            {
                if (gameState.is_game_over) {
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        game = 0;
                } else {
                    // Only change direction when game is active
                    if (event.key.keysym.sym == SDLK_RIGHT && direction.dx != -1) {
                        direction = (struct Direction) {1, 0};
                        gameState.game_started = 1;
                    }
                    if (event.key.keysym.sym == SDLK_LEFT && direction.dx != 1) {
                        direction = (struct Direction) {-1, 0};
                        gameState.game_started = 1;
                    }
                    if (event.key.keysym.sym == SDLK_UP && direction.dy != 1) {
                        direction = (struct Direction) {0, -1};
                        gameState.game_started = 1;
                    }
                    if (event.key.keysym.sym == SDLK_DOWN && direction.dy != -1) {
                        direction = (struct Direction) {0, 1};
                        gameState.game_started = 1;
                    }
                }
            }
        }

        SDL_FillRect(psurface, &override_rect, COLOR_BLACK);

        if (!gameState.is_game_over) {
            if (gameState.game_started) {
                move_snake(ppsnake, pdirection);
                
                if(check_collision(ppsnake)) {
                    printf("Collision! Game Over\n");
                    gameState.is_game_over = 1;
                    /* Free memory handled at end of game */
                }
                else {
                    if (psnake->x == papple->x && psnake->y == papple->y) {
                        // Add score based on apple type
                        gameState.score += papple->bonus ? 10 : 1;
                        
                        // Make game faster as score increases
                        if (gameState.delay > MIN_DELAY) {
                            gameState.delay -= DELAY_DECREMENT;
                        }
                        
                        reset_apple(psnake, papple);
                        lengthen_snake(ppsnake, pdirection);
                    }
                }
            }

            APPLE(papple->x, papple->y, papple->color);
            draw_snake(psurface, ppsnake); 
            DRAW_GRID;

            // Draw score
            char score_text[20];
            sprintf(score_text, "Score: %d", gameState.score);
            if (font) {
                render_text(psurface, font, score_text, score_color, 10, 10);
            }
        } else {
            // Game over screen
            if (font) {
                show_game_over(psurface, font, gameState.score);
            }
        }

        SDL_UpdateWindowSurface(window);
        SDL_Delay(gameState.is_game_over ? 100 : gameState.delay);
    }

    // Free memory
    struct SnakeElement *pcurrent = *ppsnake;
    struct SnakeElement *pnext;
    while (pcurrent != NULL)
    {
        pnext = pcurrent->pnext;
        free(pcurrent);
        pcurrent = pnext;
    }

    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}