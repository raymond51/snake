//MAN LIB - https://invisible-island.net/ncurses/man/ncurses.3x.html

#include <ncurses.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG_PRINT
#define WELCOME "Welcome to SNAKE! (Press any key to start)"
#define GRID_ARRAY_SIZE 20
#define FPS 30

//CONTROLS
#define QUIT 'q'

//default starting pos
#define INIT_SNAKE_HEAD_X 10
#define INIT_SNAKE_HEAD_Y 10
#define INIT_FOOD_X 5
#define INIT_FOOD_Y 10

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

typedef struct Food
{
    int x_pos, y_pos;
} Food;

typedef struct snake_node
{
    int x_pos, y_pos;
    struct snake_node *next;
} node;

typedef struct Snake
{
    bool isRunning;
    short second_counter;
    short score;
    short curDir;

    int (*board)[GRID_ARRAY_SIZE];
    Food food;
    node *head;
} Snake;

bool init_game(Snake *g);
void init_curses();
void init_snake_body(Snake *g);
void generate_snake_border(Snake *g);
node *create_snake_head_node(int x, int y);
void append_snake_body_node(node *head, int x, int y);
void add_snake_to_board(Snake *g);
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
    draw_board(g);
    /*print score*/
    mvwprintw(stdscr, 20, 15, "Score:%i", (*g).score);
    printControls();
    /*Debug info*/
#ifdef DEBUG_PRINT
    mvwprintw(stdscr, 2, 40, "dir:%i", (*g).curDir);
#endif
    refresh();
}

void update(Snake *g)
{
    //using non-blocking 1 second tracking to
    //clear board via setting EMPTY
    //using new input e.g dir update snake pos/movement -shifting

    if ((*g).second_counter >= FPS)
    {
        (*g).second_counter = 0;
        //Action
        clear_board(g);
        move_snake(g);
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
            /*default food pos*/
            (*g).food.x_pos = INIT_FOOD_X;
            (*g).food.y_pos = INIT_FOOD_Y;
            (*g).board[INIT_FOOD_Y][INIT_FOOD_X] = FOOD;
            (*g).score = 0;
            (*g).second_counter = 0;
            (*g).curDir = NO_MOVEMENT;

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
}

void move_snake(Snake *g)
{
    node *current = (*g).head;
    short x_dir_temp = 0, y_dir_temp = 0; //no movement - no update
    short x_prev, y_prev;                 //store prev coordinates
    bool isSnakeHead = true;
    switch ((*g).curDir)
    {
    case UP:
        y_dir_temp = -1;
        break;
    case DOWN:
        y_dir_temp = 1;
        break;
    case LEFT:
        x_dir_temp = -1;
        break;
    case RIGHT:
        x_dir_temp = 1;
        break;
    }

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
                (*current).x_pos = x_prev;
                (*current).y_pos = y_prev;
                x_prev = x_new; // store new coords
                y_prev = y_new;
            }
        }
        current = (*current).next;
    }
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