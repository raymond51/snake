//MAN LIB - https://invisible-island.net/ncurses/man/ncurses.3x.html

#include <ncurses.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG_PRINT
#define QUIT 'q'

typedef struct Food
{

} Food;

typedef struct Snake
{
    bool isRunning;
    Food food;
} Snake;

bool init_game(Snake *g);
void getInput(Snake *g);
void print_in_middle(int startx, int starty, int width, char *string, WINDOW *win);

int main(int argc, char const *argv[])
{
    Snake *game = malloc(sizeof(game));
    if (init_game(game))
    {
        /*Game loop*/
        while (game->isRunning)
        {
            getInput(game);
            usleep(32 * 1000); //FPS
        }
    }

    free(game);
    endwin(); //end curses mode
    return EXIT_SUCCESS;
}

bool init_game(Snake *g)
{
    bool status = true;
    (*g).isRunning = true;
    char *welcome_string = "Welcome to SNAKE! (Press Any Key to start)";

    initscr(); //init curses screen and manipulation routines
    cbreak();  //Immediate pass user input to program
    noecho();
    keypad(stdscr, TRUE); //Enable arrow function keys
    curs_set(FALSE);      //hide cursors

    print_in_middle(0, LINES / 2, 0, welcome_string, NULL); //COLS - specify width of screen in characters
    getch();                                                //wait for user input

#ifdef DEBUG_PRINT
    printf("Game error\n");
#endif

    return status;
}

void getInput(Snake *g)
{
    char c;
    c = getch();
    switch (c)
    {
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