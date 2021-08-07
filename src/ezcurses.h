#ifndef EZ_CURSES
#define EZ_CURSES

#define CHAR_FULL_BLOCK   '\xdb'
#define CHAR_DARK_SHADE   '\xb2'
#define CHAR_MEDIUM_SHADE '\xb1'
#define CHAR_LIGHT_SHADE  '\xb0'

#define COLOR_BLACK          0
#define COLOR_BLUE           1
#define COLOR_GREEN          2
#define COLOR_CYAN           3
#define COLOR_RED            4
#define COLOR_MAGENTA        5
#define COLOR_YELLOW         6
#define COLOR_WHITE          7
#define COLOR_BRIGHT_BLACK   8
#define COLOR_BRIGHT_BLUE    9
#define COLOR_BRIGHT_GREEN   10
#define COLOR_BRIGHT_CYAN    11
#define COLOR_BRIGHT_RED     12
#define COLOR_BRIGHT_MAGENTA 13
#define COLOR_BRIGHT_YELLOW  14
#define COLOR_BRIGHT_WHITE   15

#define FG_BLACK          COLOR_BLACK
#define FG_BLUE           COLOR_BLUE
#define FG_GREEN          COLOR_GREEN
#define FG_CYAN           COLOR_CYAN
#define FG_RED            COLOR_RED
#define FG_MAGENTA        COLOR_MAGENTA
#define FG_YELLOW         COLOR_YELLOW
#define FG_WHITE          COLOR_WHITE
#define FG_BRIGHT_BLACK   COLOR_BRIGHT_BLACK
#define FG_BRIGHT_BLUE    COLOR_BRIGHT_BLUE
#define FG_BRIGHT_GREEN   COLOR_BRIGHT_GREEN
#define FG_BRIGHT_CYAN    COLOR_BRIGHT_CYAN
#define FG_BRIGHT_RED     COLOR_BRIGHT_RED
#define FG_BRIGHT_MAGENTA COLOR_BRIGHT_MAGENTA
#define FG_BRIGHT_YELLOW  COLOR_BRIGHT_YELLOW
#define FG_BRIGHT_WHITE   COLOR_BRIGHT_WHITE

#define BG_BLACK          (COLOR_BLACK << 4)
#define BG_BLUE           (COLOR_BLUE << 4)
#define BG_GREEN          (COLOR_GREEN << 4)
#define BG_CYAN           (COLOR_CYAN << 4)
#define BG_RED            (COLOR_RED << 4)
#define BG_MAGENTA        (COLOR_MAGENTA << 4)
#define BG_YELLOW         (COLOR_YELLOW << 4)
#define BG_WHITE          (COLOR_WHITE << 4)
#define BG_BRIGHT_BLACK   (COLOR_BRIGHT_BLACK << 4)
#define BG_BRIGHT_BLUE    (COLOR_BRIGHT_BLUE << 4)
#define BG_BRIGHT_GREEN   (COLOR_BRIGHT_GREEN << 4)
#define BG_BRIGHT_CYAN    (COLOR_BRIGHT_CYAN << 4)
#define BG_BRIGHT_RED     (COLOR_BRIGHT_RED << 4)
#define BG_BRIGHT_MAGENTA (COLOR_BRIGHT_MAGENTA << 4)
#define BG_BRIGHT_YELLOW  (COLOR_BRIGHT_YELLOW << 4)
#define BG_BRIGHT_WHITE   (COLOR_BRIGHT_WHITE << 4)

extern int  initscr(void);
extern void endwin(void);
extern void clear(void);
extern void refresh(void);
extern void attrset(int attr);
extern void attron(int attr);
extern void attroff(int attr);

/* CURSOR */
extern void curs_set(int visibility);
extern void move(int y, int x);
extern int  gety(void);
extern int  getx(void);
extern int  getmaxy(void);
extern int  getmaxx(void);

#define getyx(y, x) ((y) = gety(), (x) = getx())
#define getmaxyx(y, x) ((y) = getmaxy(), (x) = getmaxx())

/* MODES */
extern int  raw(void);
extern int  cbreak(void);
extern int  echo(void);
extern int  noecho(void);

/* I/O */
extern void addstr(char *);
extern void addch(char c);
extern char getch();

#define printw(s) addstr(s)

#endif

#ifdef EZ_CURSES_IMPLEMENTATION

#ifdef EZ_IMPLEMENTATION
#undef EZ_IMPLEMENTATION
#include "ez.h"
#define EZ_IMPLEMENTATION
#else
#include "ez.h"
#endif

#include <windows.h>

HANDLE ezcurs_stdout;
HANDLE ezcurs_stdin;
CHAR_INFO *ezcurs_stdout_buff;
int ezcurs_cols;
int ezcurs_rows;
int ezcurs_cx;
int ezcurs_cy;
int ezcurs_echo;
WORD ezcurs_dflt_attr;
WORD ezcurs_curr_attr;

int
initscr(void)
{
    /* Create screen buffer */
    ezcurs_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    ezcurs_stdin  = GetStdHandle(STD_INPUT_HANDLE);
    if(ezcurs_stdout == INVALID_HANDLE_VALUE ||
       ezcurs_stdin  == INVALID_HANDLE_VALUE)
    {
        return(0);
    }
    int cols;
    int rows;
    COORD coord;
    CONSOLE_SCREEN_BUFFER_INFO sbinfo = {0};
    if(!GetConsoleScreenBufferInfo(ezcurs_stdout, &sbinfo))
    {
        return(0);
    }
    ezcurs_stdout = CreateConsoleScreenBuffer(
        GENERIC_READ|GENERIC_WRITE, 0, 0,
        CONSOLE_TEXTMODE_BUFFER, 0);
    cols = sbinfo.srWindow.Right  - sbinfo.srWindow.Left + 1;
    rows = sbinfo.srWindow.Bottom - sbinfo.srWindow.Top + 1;
    ezcurs_cols = cols;
    ezcurs_rows = rows;
    coord.X = (SHORT)cols;
    coord.Y = (SHORT)rows;
    if(!SetConsoleScreenBufferSize(ezcurs_stdout, coord))
    {
        return(0);
    }
    size_t buffsize = cols*rows;
    ezcurs_stdout_buff = ez_mem_alloc(buffsize*sizeof(CHAR_INFO));
    if(!ezcurs_stdout_buff)
    {
        return(0);
    }
    ezcurs_dflt_attr = FG_BRIGHT_WHITE|BG_BLACK;
    ezcurs_curr_attr = ezcurs_dflt_attr;
    clear();
    if(!SetConsoleActiveScreenBuffer(ezcurs_stdout))
    {
        return(0);
    }

    ezcurs_echo = 1;

    /* Set console position to (0, 0) */
    move(0, 0);
    curs_set(1);

    return(1);
}

void
endwin(void)
{
}

void
curs_set(int visibility)
{
    CONSOLE_CURSOR_INFO cursinfo = {0};
    if(visibility == 0)
    {
        cursinfo.dwSize = 100;
        cursinfo.bVisible = 0;
    }
    else if(visibility == 1)
    {
        cursinfo.dwSize = 1;
        cursinfo.bVisible = 1;
    }
    else if(visibility == 2)
    {
        cursinfo.dwSize = 100;
        cursinfo.bVisible = 1;
    }
    else
    {
        cursinfo.dwSize = 1;
        cursinfo.bVisible = 1;
    }
    SetConsoleCursorInfo(ezcurs_stdout, &cursinfo);
}

void
move(int y, int x)
{
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(ezcurs_stdout, coord);
    ezcurs_cx = x;
    ezcurs_cy = y;
}

int
gety(void)
{
    return(ezcurs_cy);
}

int
getx(void)
{
    return(ezcurs_cx);
}

int
getmaxy(void)
{
    return(ezcurs_rows - 1);
}

int
getmaxx(void)
{
    return(ezcurs_cols - 1);
}

void
clear(void)
{
    size_t buffsize = ezcurs_cols*ezcurs_rows;
    for(size_t i = 0;
        i < buffsize;
        ++i)
    {
        ezcurs_stdout_buff[i].Char.AsciiChar = ' ';
        ezcurs_stdout_buff[i].Attributes     = ezcurs_curr_attr;
    }
}

void
refresh(void)
{
    COORD coord;
    COORD stdout_coord;
    SMALL_RECT write_region;

    /* Set cursor position */
    move(ezcurs_cy, ezcurs_cx);

    /* Write screen buffer to the console */
    stdout_coord.X = (SHORT)ezcurs_cols;
    stdout_coord.Y = (SHORT)ezcurs_rows;
    coord.X = 0;
    coord.Y = 0;
    write_region.Top    = 0;
    write_region.Left   = 0;
    write_region.Right  = (SHORT)ezcurs_cols;
    write_region.Bottom = (SHORT)ezcurs_rows;
    WriteConsoleOutput(
        ezcurs_stdout, ezcurs_stdout_buff, stdout_coord,
        coord, &write_region);
}

void
attrset(int attr)
{
    ezcurs_curr_attr = (WORD)attr;
}

void
attron(int attr)
{
    ezcurs_curr_attr |= (WORD)attr;
}

void
attroff(int attr)
{
    ezcurs_curr_attr &= ~((WORD)attr);
}

int
raw(void)
{
    DWORD stdin_mode;
    if(!GetConsoleMode(ezcurs_stdin, &stdin_mode))
    {
        return(0);
    }
    stdin_mode &= ~(ENABLE_LINE_INPUT|ENABLE_PROCESSED_INPUT);
    if(!SetConsoleMode(ezcurs_stdin, stdin_mode))
    {
        return(0);
    }
    return(1);
}

int
cbreak(void)
{
    DWORD stdin_mode;
    if(!GetConsoleMode(ezcurs_stdin, &stdin_mode))
    {
        return(0);
    }
    stdin_mode &= ~(ENABLE_LINE_INPUT);
    stdin_mode |= ENABLE_PROCESSED_INPUT;
    if(!SetConsoleMode(ezcurs_stdin, stdin_mode))
    {
        return(0);
    }
    return(1);
}

int
echo(void)
{
    /* Enable raw mode */
    DWORD stdin_mode;
    if(!GetConsoleMode(ezcurs_stdin, &stdin_mode))
    {
        return(0);
    }
    stdin_mode |= ENABLE_ECHO_INPUT;
    ezcurs_echo = 1;
    if(!SetConsoleMode(ezcurs_stdin, stdin_mode))
    {
        return(0);
    }
    return(1);
}

int
noecho(void)
{
    /* Enable raw mode */
    DWORD stdin_mode;
    if(!GetConsoleMode(ezcurs_stdin, &stdin_mode))
    {
        return(0);
    }
    stdin_mode &= ~(ENABLE_ECHO_INPUT);
    ezcurs_echo = 0;
    if(!SetConsoleMode(ezcurs_stdin, stdin_mode))
    {
        return(0);
    }
    return(1);
}

void
addstr(char *s)
{
    /* TODO OPTIMIZE: Transform this info a switch statement */
    while(*s)
    {
        if(ez_char_is_print(*s))
        {
            ezcurs_stdout_buff[ezcurs_cy*ezcurs_cols + ezcurs_cx].Char.AsciiChar = *s;
            ezcurs_stdout_buff[ezcurs_cy*ezcurs_cols + ezcurs_cx].Attributes     = ezcurs_curr_attr;
            ++ezcurs_cx;
            if(ezcurs_cx >= ezcurs_cols)
            {
                ++ezcurs_cy;
                ezcurs_cx = 0;
                if(ezcurs_cy >= ezcurs_rows)
                {
                    ezcurs_cy = ezcurs_rows - 1;
                }
            }
        }
        else if(*s == '\r')
        {
            ezcurs_cx = 0;
        }
        else if(*s == '\n')
        {
            ++ezcurs_cy;
            if(ezcurs_cy >= ezcurs_rows)
            {
                ezcurs_cy = ezcurs_rows - 1;
            }
        }
        ++s;
    }
}

void
addch(char c)
{
    char b[2];
    b[0] = c;
    b[1] = 0;
    addstr(b);
}

char
getch()
{
    char c;
    DWORD bread;
    if(!ReadFile(ezcurs_stdin, &c, 1, &bread, 0) || bread != 1)
    {
        c = 0;
    }
    else
    {
        if(ezcurs_echo)
        {
            addch(c);
        }
    }
    return(c);
}

#endif
