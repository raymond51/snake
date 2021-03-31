#include <ncurses.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define DEBUG_PRINT
#define WELCOME "Welcome to SNAKE! (Press any key to start)"
#define GRID_ARRAY_SIZE 20
#define FPS 30

//CONTROLS
#define QUIT 'q'

//default starting pos
#define INIT_SNAKE_HEAD_X 10
#define INIT_SNAKE_HEAD_Y 10

//Graphics
#define gEMPTY ' '
#define gBORDER '*'
#define gFOOD 'O'
#define gSNAKE_H 'X'
#define gSNAKE_B '#'

enum assets
{
    EMPTY = 0,
    BORDER,
    FOOD,
    SNAKE_HEAD,
    SNAKE_BODY
};

enum dir
{
    NO_MOVEMENT = 0,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

enum collision_type
{
    NO_COLLISION = 0,
    FOOD_COLLISION,
    BODY_COLLISION,
    BORDER_COLLISION
};

typedef struct Food
{
    int x_pos, y_pos;
} Food;

typedef struct snake_node
{
    int x_pos, y_pos;
    struct snake_node *next;
} node;

typedef struct Tail
{
    int x_pos, y_pos;
} Tail;

typedef struct Snake
{
    bool isRunning;
    short second_counter;
    short score;
    short curDir;
    short collision_id;

    int (*board)[GRID_ARRAY_SIZE];
    Food food;
    node *head;
    Tail tail;

} Snake;

bool init_game(Snake *g);
void init_curses();
void init_snake_body(Snake *g);
void generate_snake_border(Snake *g);
node *create_snake_head_node(int x, int y);
void append_snake_body_node(node *head, int x, int y);
void add_snake_to_board(Snake *g);
void generate_food(Snake *g);
void detect_snake_collision(Snake *g);
short movement_dir_x(Snake *g);
short movement_dir_y(Snake *g);
void clear_board(Snake *g);
void move_snake(Snake *g);
void draw(Snake *g);
void draw_board(Snake *g);
void update(Snake *g);
void printControls();
void getInput(Snake *g);
void print_in_middle(int startx, int starty, int width, char *string, WINDOW *win);
void free_list(node **head);

int main(int argc, char const *argv[])
{
    Snake *game = malloc(sizeof(Snake));
    init_curses();

    if (init_game(game))
    {
        /*Game loop*/

        while (game->isRunning)
        {
            getInput(game);
            draw(game);
            update(game);
            usleep(32 * 1000); //FPS
        }
    }
    free_list(&(*game).head);
    free((*game).board);
    free(game);
    endwin(); //end curses mode
    return EXIT_SUCCESS;
}

void draw(Snake *g)
{
    wclear(stdscr);
    //clear board via setting EMPTY
    add_snake_to_board(g);
    (*g).board[(*g).food.y_pos][(*g).food.x_pos] = FOOD; // add food to board
    draw_board(g);
    /*print score*/
    mvwprintw(stdscr, 20, 15, "Score:%i", (*g).score);
    printControls();
    /*Debug info*/
#ifdef DEBUG_PRINT
    mvwprintw(stdscr, 2, 40, "dir:%i", (*g).curDir);
    mvwprintw(stdscr, 3, 40, "Collision ID::%i", (*g).collision_id);
#endif
    refresh();
}

void update(Snake *g)
{
    if ((*g).second_counter >= FPS)
    {
        (*g).second_counter = 0;
        //Action
        detect_snake_collision(g);
        if ((*g).collision_id == NO_COLLISION)
        {
            clear_board(g);
            move_snake(g);
        }
        else if ((*g).collision_id == BODY_COLLISION || (*g).collision_id == BORDER_COLLISION)
        {
            wclear(stdscr);
            mvwprintw(stdscr, 10, 20, "GAME OVER!");
            mvwprintw(stdscr, 20, 15, "Score:%i", (*g).score);
            printControls();
            refresh();
            timeout(-1); //blocking input
            getch();
            (*g).isRunning = false;
        }
        else if ((*g).collision_id == FOOD_COLLISION)
        {
            (*g).collision_id = NO_COLLISION; //reset collision
            (*g).score++;
            clear_board(g);
            move_snake(g); //update snake body
            generate_food(g);
            append_snake_body_node((*g).head, (*g).tail.x_pos, (*g).tail.x_pos); //add new snake body
        }
    }
    (*g).second_counter++;
}

void draw_board(Snake *g)
{
    for (int i = 0; i < GRID_ARRAY_SIZE; i++)
    {
        for (int j = 0; j < GRID_ARRAY_SIZE; j++)
        {
            switch ((*g).board[i][j])
            {
            case EMPTY:
                printw("%c ", gEMPTY);
                break;
            case BORDER:
                printw("%c ", gBORDER);
                break;
            case FOOD:
                printw("%c ", gFOOD);
                break;
            case SNAKE_HEAD:
                printw("%c ", gSNAKE_H);
                break;
            case SNAKE_BODY:
                printw("%c ", gSNAKE_B);
                break;
            }
        }
        printw("\n");
    }
}

bool init_game(Snake *g)
{
    bool status = true;
    if (g == NULL)
    {
        status = false;
#ifdef DEBUG_PRINT
        printf("Error allocating memory to Game");
#endif
    }
    else
    {
        (*g).board = malloc(sizeof(int[GRID_ARRAY_SIZE][GRID_ARRAY_SIZE]));
        if ((*g).board != NULL)
        {
            (*g).isRunning = true;
            /*Create snake border*/
            generate_snake_border(g);
            init_snake_body(g);
            add_snake_to_board(g);

            srand(time(NULL)); //init rnd
            generate_food(g);

            (*g).score = 0;
            (*g).second_counter = 0;
            (*g).curDir = NO_MOVEMENT;
            (*g).collision_id = NO_COLLISION;

            timeout(0); //non-blocking read input for getch()
        }
    }
    return status;
}

void generate_snake_border(Snake *g)
{
    for (int i = 0; i < GRID_ARRAY_SIZE; i++)
    {
        for (int j = 0; j < GRID_ARRAY_SIZE; j++)
        {
            //COL 0, ROW 0, LAST COL, LAST ROW
            if (i == 0 || j == 0 || i == GRID_ARRAY_SIZE - 1 || j == GRID_ARRAY_SIZE - 1)
                (*g).board[i][j] = BORDER;
            else
                (*g).board[i][j] = EMPTY;
        }
    }
}

void generate_food(Snake *g)
{
    //generate num between 1 & 19
    int x_new_food, y_new_food;

    do
    {
        x_new_food = 1 + (rand() % (GRID_ARRAY_SIZE - 1));
        y_new_food = 1 + (rand() % (GRID_ARRAY_SIZE - 1));

    } while ((*g).board[y_new_food][x_new_food] != EMPTY);

    /*update the struct value here with new coords*/
    (*g).food.x_pos = x_new_food;
    (*g).food.y_pos = y_new_food;
}
void init_curses()
{
    char *welcome_string = WELCOME;
    initscr(); //init curses screen and manipulation routines
    cbreak();  //Immediate pass user input to program
    noecho();
    keypad(stdscr, TRUE);                                      //Enable arrow function keys
    curs_set(FALSE);                                           //hide cursors
    print_in_middle(0, LINES / 2, COLS, welcome_string, NULL); //COLS - specify width of screen in characters
    getch();
}

void clear_board(Snake *g)
{
    /*Clear within border*/
    for (int i = 1; i < GRID_ARRAY_SIZE - 1; i++)
    {
        for (int j = 1; j < GRID_ARRAY_SIZE - 1; j++)
        {
            (*g).board[i][j] = EMPTY;
        }
    }
}

/*Create snake structure of 3 pieces*/
void init_snake_body(Snake *g)
{
    (*g).head = create_snake_head_node(INIT_SNAKE_HEAD_X, INIT_SNAKE_HEAD_Y); //head
    append_snake_body_node((*g).head, INIT_SNAKE_HEAD_X + 1, INIT_SNAKE_HEAD_Y);
    append_snake_body_node((*g).head, INIT_SNAKE_HEAD_X + 2, INIT_SNAKE_HEAD_Y);
    append_snake_body_node((*g).head, INIT_SNAKE_HEAD_X + 3, INIT_SNAKE_HEAD_Y);
    append_snake_body_node((*g).head, INIT_SNAKE_HEAD_X + 4, INIT_SNAKE_HEAD_Y);
}

void move_snake(Snake *g)
{
    node *current = (*g).head;
    short x_dir_temp = movement_dir_x(g), y_dir_temp = movement_dir_y(g); //no movement - no update
    short x_prev, y_prev;                                                 //store prev coordinates
    bool isSnakeHead = true;

    while (current != NULL)
    {
        /*Update only if there is movement*/
        if ((*g).curDir != NO_MOVEMENT)
        {
            if (isSnakeHead)
            {
                isSnakeHead = false;
                x_prev = (*current).x_pos;
                y_prev = (*current).y_pos;
                (*current).x_pos += x_dir_temp;
                (*current).y_pos += y_dir_temp;
            }
            else
            {
                int x_new = (*current).x_pos, y_new = (*current).y_pos;
                /*Storing last element as tail*/
                (*g).tail.x_pos = (*current).x_pos;
                (*g).tail.y_pos = (*current).y_pos;
                (*current).x_pos = x_prev;
                (*current).y_pos = y_prev;
                x_prev = x_new; // store new coords
                y_prev = y_new;
            }
        }
        current = (*current).next;
    }
}

void detect_snake_collision(Snake *g)
{
    node *current = (*g).head;
    /*Store new head position based on moving dir*/
    short x_head_new_coord = (*current).x_pos + movement_dir_x(g),
          y_head_new_coord = (*current).y_pos + movement_dir_y(g); //no movement - no update
    bool isSnakeHead = true;

    /*Body collision*/

    while (current != NULL)
    {
        /*Update only if there is movement*/
        if ((*g).curDir != NO_MOVEMENT)
        {
            if (isSnakeHead)
            {
                isSnakeHead = false;
            }
            else
            {
                if ((*current).x_pos == x_head_new_coord && (*current).y_pos == y_head_new_coord)
                {
                    (*g).collision_id = BODY_COLLISION; //set collision
                    return;
                }
            }
        }
        current = (*current).next;
    }
    /*Border collision*/
    if (x_head_new_coord <= 0 || x_head_new_coord >= (GRID_ARRAY_SIZE - 1) || y_head_new_coord <= 0 || y_head_new_coord >= (GRID_ARRAY_SIZE - 1))
    {
        (*g).collision_id = BORDER_COLLISION; //set collision
        return;
    }
    else if ((*g).food.x_pos == x_head_new_coord && (*g).food.y_pos == y_head_new_coord)
        (*g).collision_id = FOOD_COLLISION; //set collision
}

short movement_dir_x(Snake *g)
{
    short tmp;
    switch ((*g).curDir)
    {
    case LEFT:
        tmp = -1;
        break;
    case RIGHT:
        tmp = 1;
        break;
    default: //No movement
        tmp = 0;
    }
    return tmp;
}

short movement_dir_y(Snake *g)
{
    short tmp;
    switch ((*g).curDir)
    {
    case UP:
        tmp = -1;
        break;
    case DOWN:
        tmp = 1;
        break;
    default: //No movement
        tmp = 0;
    }
    return tmp;
}

void add_snake_to_board(Snake *g)
{
    node *current = (*g).head;
    bool isSnakeHead = true;
    while (current != NULL)
    {
        if (isSnakeHead)
        {
            isSnakeHead = false; //first instance of snake head
            (*g).board[(*current).y_pos][(*current).x_pos] = SNAKE_HEAD;
        }
        else
        {
            (*g).board[(*current).y_pos][(*current).x_pos] = SNAKE_BODY;
        }
        current = (*current).next;
    }
}

node *create_snake_head_node(int x, int y)
{
    /*New node*/
    node *newNode = malloc(sizeof(*newNode));
    (*newNode).next = NULL;
    (*newNode).x_pos = x;
    (*newNode).y_pos = y;
    return newNode;
}

void append_snake_body_node(node *head, int x, int y)
{
    /*New node*/
    node *newNode = malloc(sizeof(*newNode));
    (*newNode).x_pos = x;
    (*newNode).y_pos = y;

    node *lastNode = head;
    while ((*lastNode).next != NULL)
    {
        lastNode = (*lastNode).next;
    }
    (*lastNode).next = newNode;
}

void free_list(node **head)
{
    node *curr = *head;
    node *next;
    while (curr != NULL)
    {
        next = (*curr).next;
        free(curr);
        curr = next;
    }
    *head = NULL; //deref real head in caller
}

void printControls()
{
    mvprintw(21, 0, "Controls:");
    mvprintw(22, 0, "   Movements - Arrow keys");
    mvprintw(23, 0, "   Quit - q");
}

void getInput(Snake *g)
{
    int c = getch();
    switch (c)
    {
    case KEY_UP:
        (*g).curDir = UP;
        break;
    case KEY_DOWN:
        (*g).curDir = DOWN;
        break;
    case KEY_LEFT:
        (*g).curDir = LEFT;
        break;
    case KEY_RIGHT:
        (*g).curDir = RIGHT;
        break;
    case QUIT:
        (*g).isRunning = false;
        break;
    }
}

void print_in_middle(int startx, int starty, int width, char *string, WINDOW *win)
{
    int length_Str, x, y;
    float temp;

    if (win == NULL)
        win = stdscr;
    getyx(win, y, x); //store current beginning coordinates and size of window
    if (startx != 0)
        x = startx;
    if (starty != 0)
        y = starty;
    if (width == 0)
        width = 80; //arb

    length_Str = strlen(string);
    temp = (width - length_Str) / 2; //middle allignment
    x = startx + (int)temp;
    mvwprintw(win, y, x, "%s", string); //print formatted output
    refresh();
}