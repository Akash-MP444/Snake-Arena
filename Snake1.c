#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <conio.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// User Structure
typedef struct {
    char username[50];
    char password[50];
    int high_score;
} User;

// Game Configuration
#define MAX_USERS 100
#define GRID_SIZE 20
#define MAX_SNAKE_LENGTH 100
#define FOODS_PER_LEVEL 5
#define SCORE_PER_FOOD 10
#define BONUS_SCORE 25
#define BONUS_LIFETIME_TICKS 35
#define BONUS_SPAWN_INTERVAL 3
#define BASE_DELAY_MS 220
#define LEVEL_SPEEDUP_MS 12
#define MIN_DELAY_MS 70
#define MAX_OBSTACLES 40

// User Management Functions
void register_user();
int login();
void save_users();
void load_users();
void update_high_score(int score);
int get_high_score();
void show_leaderboard();
void flush_input_buffer();
int find_user_index(const char *username);

// Game Structures
typedef enum {
    UP,
    DOWN,
    LEFT,
    RIGHT
} Direction;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position body[MAX_SNAKE_LENGTH];
    int length;
} Snake;

typedef enum {
    MODE_CLASSIC,
    MODE_WRAP
} GameMode;

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_NORMAL,
    DIFFICULTY_HARD
} Difficulty;

typedef struct {
    char username[50];
    Snake snake;
    Position food;
    Position bonus_food;
    Position obstacles[MAX_OBSTACLES];
    Direction direction;
    int score;
    int level;
    int foods_eaten;
    int obstacle_count;
    int bonus_active;
    int bonus_timer;
    int paused;
    int game_over;
    int speed_modifier_ms;
    GameMode mode;
    Difficulty difficulty;
} GameState;

// Global Variables
User users[MAX_USERS];
int user_count = 0;
char current_user[50];

// Function Prototypes
void initialize_game(GameState *game);
void draw_board(GameState *game);
void move_snake(GameState *game);
void generate_food(GameState *game);
int check_collision(GameState *game);
void handle_input(GameState *game);
void gotoxy(int x, int y);
void set_cursor_visibility(char visible);
void setup_console();
void restore_console();
int get_frame_delay(const GameState *game);
void setup_level_obstacles(GameState *game);
int is_on_snake(const GameState *game, int x, int y);
int is_on_obstacle(const GameState *game, int x, int y);
void configure_run(GameState *game);
int choose_menu_option(int min_choice, int max_choice);
void play_sound_event(int event_id);
void show_splash_screen();
void show_countdown();

enum {
    SFX_EAT = 1,
    SFX_BONUS = 2,
    SFX_PAUSE = 3,
    SFX_GAME_OVER = 4
};

// Cursor Positioning Function for Windows
void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Cursor Visibility Function
void set_cursor_visibility(char visible) {
    CONSOLE_CURSOR_INFO cursor_info;
    cursor_info.dwSize = 100;
    cursor_info.bVisible = visible;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}

void setup_console() {
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h_out, &mode)) {
        SetConsoleMode(h_out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    set_cursor_visibility(0);
}

void restore_console() {
    set_cursor_visibility(1);
}

void flush_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* discard */
    }
}

int choose_menu_option(int min_choice, int max_choice) {
    int choice;
    while (1) {
        if (scanf("%d", &choice) != 1) {
            flush_input_buffer();
            printf("Invalid input. Please enter a number: ");
            continue;
        }
        if (choice < min_choice || choice > max_choice) {
            printf("Choose between %d and %d: ", min_choice, max_choice);
            continue;
        }
        return choice;
    }
}

void play_sound_event(int event_id) {
    switch (event_id) {
        case SFX_EAT:
            Beep(880, 40);
            break;
        case SFX_BONUS:
            Beep(1200, 60);
            Beep(1600, 80);
            break;
        case SFX_PAUSE:
            Beep(700, 45);
            break;
        case SFX_GAME_OVER:
            Beep(420, 120);
            Beep(300, 170);
            break;
        default:
            break;
    }
}

void show_splash_screen() {
    system("cls");
    printf("\x1b[1;36m");
    printf("   _____ _   _    _    _  _______     _    ____  _____ _   _    _\n");
    printf("  / ____| \\ | |  / \\  | |/ / ____|   / \\  |  _ \\| ____| \\ | |  / \\ \n");
    printf(" | (___ |  \\| | / _ \\ | ' /|  _|    / _ \\ | |_) |  _| |  \\| | / _ \\ \n");
    printf("  \\___ \\| . ` |/ ___ \\| . \\| |___  / ___ \\|  _ <| |___| |\\  |/ ___ \\ \n");
    printf("  ____) | |\\  /_/   \\_\\_|\\_\\_____|_/   \\_\\_| \\_\\_____|_| \\_/_/   \\_\\\n");
    printf(" |_____/|_| \\_|\n");
    printf("\x1b[0m\n");
    printf("Arcade Edition | Eat, Dodge, Survive\n");
    printf("Press any key to continue...\n");
    _getch();
}

void show_countdown() {
    for (int i = 3; i >= 1; i--) {
        system("cls");
        printf("Starting in %d...\n", i);
        Beep(800 + (3 - i) * 120, 80);
        Sleep(350);
    }
    system("cls");
    printf("GO!\n");
    Beep(1300, 100);
    Sleep(220);
}

int find_user_index(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

// User Management Implementation
void register_user() {
    system("cls");
    if (user_count >= MAX_USERS) {
        printf("Maximum users reached!\n");
        system("pause");
        return;
    }

    User new_user;
    printf("\n=== USER REGISTRATION ===\n");
    
    // Username Input
    while (1) {
        printf("Enter Username: ");
        scanf("%49s", new_user.username);

        // Check if username already exists
        int exists = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, new_user.username) == 0) {
                exists = 1;
                break;
            }
        }

        if (!exists) break;
        printf("Username already exists. Try again.\n");
    }

    // Password Input
    printf("Enter Password: ");
    scanf("%49s", new_user.password);

    // Initialize high score
    new_user.high_score = 0;

    // Add user
    users[user_count++] = new_user;
    save_users();
    printf("Registration Successful!\n");
    system("pause");
}

int login() {
    system("cls");
    char username[50], password[50];
    
    printf("\n=== SNAKE GAME LOGIN ===\n");
    printf("Username: ");
    scanf("%49s", username);
    printf("Password: ");
    scanf("%49s", password);

    int idx = find_user_index(username);
    if (idx >= 0 && strcmp(users[idx].password, password) == 0) {
        strcpy(current_user, username);
        return 1;
    }

    printf("Invalid Credentials!\n");
    system("pause");
    return 0;
}

void save_users() {
    FILE *file = fopen("users.dat", "wb");
    if (file == NULL) {
        printf("Error saving users!\n");
        return;
    }
    
    fwrite(&user_count, sizeof(int), 1, file);
    fwrite(users, sizeof(User), user_count, file);
    fclose(file);
}

void load_users() {
    FILE *file = fopen("users.dat", "rb");
    if (file == NULL) return;
    
    fread(&user_count, sizeof(int), 1, file);
    if (user_count < 0 || user_count > MAX_USERS) {
        user_count = 0;
        fclose(file);
        return;
    }
    fread(users, sizeof(User), user_count, file);
    fclose(file);
}

// Function to update high score
void update_high_score(int score) {
    int idx = find_user_index(current_user);
    if (idx >= 0 && score > users[idx].high_score) {
        users[idx].high_score = score;
        save_users();
    }
}

// Function to get current user's high score
int get_high_score() {
    int idx = find_user_index(current_user);
    if (idx >= 0) {
        return users[idx].high_score;
    }
    return 0;
}

void show_leaderboard() {
    system("cls");
    printf("=== LEADERBOARD (TOP 10) ===\n\n");

    if (user_count == 0) {
        printf("No users found. Register and play to appear here!\n\n");
        system("pause");
        return;
    }

    User sorted[MAX_USERS];
    memcpy(sorted, users, sizeof(User) * user_count);

    for (int i = 0; i < user_count - 1; i++) {
        for (int j = i + 1; j < user_count; j++) {
            if (sorted[j].high_score > sorted[i].high_score) {
                User temp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temp;
            }
        }
    }

    int limit = user_count < 10 ? user_count : 10;
    for (int i = 0; i < limit; i++) {
        printf("%2d. %-20s %5d\n", i + 1, sorted[i].username, sorted[i].high_score);
    }

    printf("\n");
    system("pause");
}

// Game Initialization
void initialize_game(GameState *game) {
    strcpy(game->username, current_user);

    // Initialize Snake
    game->snake.length = 1;
    game->snake.body[0].x = GRID_SIZE / 2;
    game->snake.body[0].y = GRID_SIZE / 2;

    // Initial Direction
    game->direction = RIGHT;

    // Initial Game Parameters
    game->obstacle_count = 0;
    game->bonus_active = 0;
    game->bonus_timer = 0;
    game->mode = MODE_CLASSIC;
    game->difficulty = DIFFICULTY_NORMAL;
    game->speed_modifier_ms = 0;
    game->score = 0;
    game->level = 1;
    game->foods_eaten = 0;
    game->paused = 0;
    game->game_over = 0;

    // Generate First Food
    setup_level_obstacles(game);
    generate_food(game);
}

// Game Board Drawing
void draw_board(GameState *game) {
    gotoxy(0, 0);
    
    int high_score = get_high_score();
    int next_level_in = FOODS_PER_LEVEL - (game->foods_eaten % FOODS_PER_LEVEL);
    if (game->foods_eaten == 0 || next_level_in == FOODS_PER_LEVEL) {
        next_level_in = FOODS_PER_LEVEL;
    }

    printf("\x1b[1;36mSNAKE GAME\x1b[0m | Player: %s | Mode: %s | Difficulty: %s\n",
           game->username,
           game->mode == MODE_WRAP ? "Wrap" : "Classic",
           game->difficulty == DIFFICULTY_EASY ? "Easy" : (game->difficulty == DIFFICULTY_HARD ? "Hard" : "Normal"));
    printf("Level: %d | Score: %d | High Score: %d | Obstacles: %d | Next level in: %d food\n",
           game->level, game->score, high_score, game->obstacle_count, next_level_in);
    printf("Controls: WASD / Arrow Keys | P: Pause | Q: Quit | Bonus: * (%d pts)\n", BONUS_SCORE);
    if (game->paused) {
        printf("\x1b[1;33m[PAUSED]\x1b[0m Press P to resume.\n");
    } else {
        printf("                                                  \n");
    }

    // Top Border
    for (int i = 0; i < GRID_SIZE + 2; i++)
        printf("=");
    printf("\n");

    // Game Board
    for (int y = 0; y < GRID_SIZE; y++) {
        printf("|");
        for (int x = 0; x < GRID_SIZE; x++) {
            int is_snake = 0;
            int is_food = 0;
            int is_head = 0;
            int is_obstacle = 0;
            int is_bonus = 0;

            // Check Snake Segments
            for (int i = 0; i < game->snake.length; i++) {
                if (game->snake.body[i].x == x && game->snake.body[i].y == y) {
                    is_snake = 1;
                    if (i == 0) {
                        is_head = 1;
                    }
                    break;
                }
            }

            // Check Food Position
            if (game->food.x == x && game->food.y == y)
                is_food = 1;
            if (game->bonus_active && game->bonus_food.x == x && game->bonus_food.y == y)
                is_bonus = 1;
            if (is_on_obstacle(game, x, y))
                is_obstacle = 1;

            // Render Snake and Food
            if (is_head)
                printf("\x1b[1;32m@\x1b[0m");
            else if (is_snake)
                printf("\x1b[32mo\x1b[0m");
            else if (is_obstacle)
                printf("\x1b[1;35m#\x1b[0m");
            else if (is_bonus)
                printf("\x1b[1;33m*\x1b[0m");
            else if (is_food)
                printf("\x1b[1;31m$\x1b[0m");
            else
                printf(" ");
        }
        printf("|\n");
    }

    // Bottom Border
    for (int i = 0; i < GRID_SIZE + 2; i++)
        printf("=");
    printf("\n");
}

// Food Generation
int is_on_snake(const GameState *game, int x, int y) {
    for (int i = 0; i < game->snake.length; i++) {
        if (game->snake.body[i].x == x && game->snake.body[i].y == y) {
            return 1;
        }
    }
    return 0;
}

int is_on_obstacle(const GameState *game, int x, int y) {
    for (int i = 0; i < game->obstacle_count; i++) {
        if (game->obstacles[i].x == x && game->obstacles[i].y == y) {
            return 1;
        }
    }
    return 0;
}

void setup_level_obstacles(GameState *game) {
    int target = game->level - 1;
    if (target > MAX_OBSTACLES) {
        target = MAX_OBSTACLES;
    }

    game->obstacle_count = 0;
    while (game->obstacle_count < target) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        /* keep the spawn zone safe */
        if (abs(x - (GRID_SIZE / 2)) <= 1 && abs(y - (GRID_SIZE / 2)) <= 1) {
            continue;
        }
        if (is_on_obstacle(game, x, y)) {
            continue;
        }

        game->obstacles[game->obstacle_count].x = x;
        game->obstacles[game->obstacle_count].y = y;
        game->obstacle_count++;
    }
}

void generate_food(GameState *game) {
    int x, y;
    int valid_position;

    do {
        valid_position = 1;
        x = rand() % GRID_SIZE;
        y = rand() % GRID_SIZE;

        if (is_on_snake(game, x, y) || is_on_obstacle(game, x, y)) {
            valid_position = 0;
        }
        if (game->bonus_active && game->bonus_food.x == x && game->bonus_food.y == y) {
            valid_position = 0;
        }
    } while (!valid_position);

    game->food.x = x;
    game->food.y = y;
}

// Snake Movement
void move_snake(GameState *game) {
    Position previous_tail = game->snake.body[game->snake.length - 1];

    // Move Body Segments
    for (int i = game->snake.length - 1; i > 0; i--) {
        game->snake.body[i] = game->snake.body[i-1];
    }

    // Move Head
    switch (game->direction) {
        case UP:    game->snake.body[0].y--; break;
        case DOWN:  game->snake.body[0].y++; break;
        case LEFT:  game->snake.body[0].x--; break;
        case RIGHT: game->snake.body[0].x++; break;
    }

    if (game->mode == MODE_WRAP) {
        if (game->snake.body[0].x < 0) game->snake.body[0].x = GRID_SIZE - 1;
        if (game->snake.body[0].x >= GRID_SIZE) game->snake.body[0].x = 0;
        if (game->snake.body[0].y < 0) game->snake.body[0].y = GRID_SIZE - 1;
        if (game->snake.body[0].y >= GRID_SIZE) game->snake.body[0].y = 0;
    }

    if (game->bonus_active) {
        game->bonus_timer--;
        if (game->bonus_timer <= 0) {
            game->bonus_active = 0;
        }
    }

    // Food Consumption
    if (game->snake.body[0].x == game->food.x && 
        game->snake.body[0].y == game->food.y) {
        play_sound_event(SFX_EAT);
        // Increase Snake Length
        if (game->snake.length < MAX_SNAKE_LENGTH) {
            game->snake.body[game->snake.length] = previous_tail;
            game->snake.length++;
        }

        // Update Score and Level
        game->score += SCORE_PER_FOOD;
        game->foods_eaten++;

        if (game->foods_eaten % BONUS_SPAWN_INTERVAL == 0 && !game->bonus_active) {
            int x, y;
            do {
                x = rand() % GRID_SIZE;
                y = rand() % GRID_SIZE;
            } while (is_on_snake(game, x, y) || is_on_obstacle(game, x, y) ||
                     (game->food.x == x && game->food.y == y));
            game->bonus_food.x = x;
            game->bonus_food.y = y;
            game->bonus_active = 1;
            game->bonus_timer = BONUS_LIFETIME_TICKS;
        }
        
        // Level Progression
        if (game->foods_eaten % FOODS_PER_LEVEL == 0) {
            game->level++;
            setup_level_obstacles(game);
        }

        // Generate New Food
        generate_food(game);
    }

    if (game->bonus_active &&
        game->snake.body[0].x == game->bonus_food.x &&
        game->snake.body[0].y == game->bonus_food.y) {
        play_sound_event(SFX_BONUS);
        game->score += BONUS_SCORE;
        game->bonus_active = 0;
        game->bonus_timer = 0;
    }
}

// Collision Detection
int check_collision(GameState *game) {
    // Wall Collision
    if (game->mode == MODE_CLASSIC &&
        (game->snake.body[0].x < 0 || game->snake.body[0].x >= GRID_SIZE ||
         game->snake.body[0].y < 0 || game->snake.body[0].y >= GRID_SIZE))
        return 1;

    if (is_on_obstacle(game, game->snake.body[0].x, game->snake.body[0].y))
        return 1;

    // Self Collision
    for (int i = 1; i < game->snake.length; i++) {
        if (game->snake.body[0].x == game->snake.body[i].x &&
            game->snake.body[0].y == game->snake.body[i].y)
            return 1;
    }

    return 0;
}

// Input Handling
void handle_input(GameState *game) {
    if (_kbhit()) {
        char ch = _getch();

        if (ch == 0 || ch == (char)224) {
            char arrow = _getch();
            switch (arrow) {
                case 72:
                    if (game->direction != DOWN) game->direction = UP;
                    break;
                case 80:
                    if (game->direction != UP) game->direction = DOWN;
                    break;
                case 75:
                    if (game->direction != RIGHT) game->direction = LEFT;
                    break;
                case 77:
                    if (game->direction != LEFT) game->direction = RIGHT;
                    break;
            }
            return;
        }

        switch (ch) {
            case 'w': 
            case 'W':
                if (game->direction != DOWN) game->direction = UP; 
                break;
            case 's': 
            case 'S':
                if (game->direction != UP) game->direction = DOWN; 
                break;
            case 'a': 
            case 'A':
                if (game->direction != RIGHT) game->direction = LEFT; 
                break;
            case 'd': 
            case 'D':
                if (game->direction != LEFT) game->direction = RIGHT; 
                break;
            case 'p':
            case 'P':
                game->paused = !game->paused;
                play_sound_event(SFX_PAUSE);
                break;
            case 'q': 
            case 'Q':
                game->game_over = 1; 
                break;
        }
    }
}

int get_frame_delay(const GameState *game) {
    int delay = BASE_DELAY_MS - ((game->level - 1) * LEVEL_SPEEDUP_MS) + game->speed_modifier_ms;
    if (delay < MIN_DELAY_MS) {
        delay = MIN_DELAY_MS;
    }
    return delay;
}

void configure_run(GameState *game) {
    int mode_choice, difficulty_choice;
    system("cls");
    printf("=== MATCH SETUP ===\n");
    printf("Choose mode:\n");
    printf("1. Classic (wall hit = game over)\n");
    printf("2. Wrap (pass through walls)\n");
    printf("Enter choice: ");
    mode_choice = choose_menu_option(1, 2);
    game->mode = (mode_choice == 2) ? MODE_WRAP : MODE_CLASSIC;

    printf("\nChoose difficulty:\n");
    printf("1. Easy\n");
    printf("2. Normal\n");
    printf("3. Hard\n");
    printf("Enter choice: ");
    difficulty_choice = choose_menu_option(1, 3);
    if (difficulty_choice == 1) {
        game->difficulty = DIFFICULTY_EASY;
        game->speed_modifier_ms = 45;
    } else if (difficulty_choice == 3) {
        game->difficulty = DIFFICULTY_HARD;
        game->speed_modifier_ms = -35;
    } else {
        game->difficulty = DIFFICULTY_NORMAL;
        game->speed_modifier_ms = 0;
    }
}

// Main Menu
void main_menu() {
    int choice;
    
    while (1) {
        system("cls");
        printf("\x1b[1;36mS N A K E   A R E N A\x1b[0m\n");
        printf("====================\n");
        printf("1. Login\n");
        printf("2. Register\n");
        printf("3. Leaderboard\n");
        printf("4. How to Play\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        choice = choose_menu_option(1, 5);

        switch (choice) {
            case 1:
                if (login()) {
                    // Start Game
                    GameState game;
                    srand(time(NULL));
                    initialize_game(&game);
                    configure_run(&game);
                    show_countdown();
                    system("cls");

                    // Game Loop
                    while (!game.game_over) {
                        handle_input(&game);

                        if (!game.paused) {
                            move_snake(&game);

                            if (check_collision(&game)) {
                                game.game_over = 1;
                                play_sound_event(SFX_GAME_OVER);
                            }
                        }

                        draw_board(&game);
                        
                        // Adjust speed based on level
                        Sleep(get_frame_delay(&game));
                    }

                    // Game Over
                    system("cls");
                    printf("\x1b[1;31mGame Over!\x1b[0m\n");
                    printf("Final Score: %d | Level Reached: %d\n", game.score, game.level);
                    
                    // Update high score
                    int old_high_score = get_high_score();
                    update_high_score(game.score);
                    
                    if (game.score > old_high_score) {
                        printf("\x1b[1;32mNew High Score: %d!\x1b[0m\n", game.score);
                    } else {
                        printf("High Score: %d\n", old_high_score);
                    }
                    
                    system("pause");
                }
                break;
            case 2:
                register_user();
                break;
            case 3:
                show_leaderboard();
                break;
            case 4:
                system("cls");
                printf("=== HOW TO PLAY ===\n");
                printf("- Eat $ to grow and score points.\n");
                printf("- Grab * quickly for bonus points.\n");
                printf("- Avoid your body and # obstacles.\n");
                printf("- In Classic mode, walls end the game.\n");
                printf("- In Wrap mode, you pass through walls.\n\n");
                printf("Controls: WASD or Arrow Keys, P pause, Q quit.\n\n");
                system("pause");
                break;
            case 5:
                restore_console();
                exit(0);
            default:
                printf("Invalid choice!\n");
                Sleep(700);
                break;
        }
    }
}

// Main Function
int main() {
    atexit(restore_console);
    setup_console();
    load_users(); // Load existing users
    show_splash_screen();
    main_menu();
    return 0;
}
