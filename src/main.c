#define EZ_IMPLEMENTATION
#define EZ_CURSES_IMPLEMENTATION
#define EZ_NO_CRT_LIB
#include "ez.h"
#include "ezcurses.h"

struct
todoitm
{
    uint32_t status;
    char name[128];
    struct todoitm *next;
    struct todoitm *prev;
};

struct
todolist
{
    uint32_t count;
    struct todoitm *items;
};

struct todoitm *
addtodo(struct todolist *list, char *name, int status)
{
    struct todoitm *itm;
    size_t namelen;
    struct todoitm *i;

    itm = (struct todoitm *)ez_mem_alloc(sizeof(struct todoitm));

    if(!itm)
    {
        return(0);
    }
    itm->status = status;
    namelen = ez_str_len(name);
    ez_str_copy_max(name, itm->name, ez_array_count(itm->name) - 1);
    itm->name[namelen] = 0;
    itm->prev = 0;
    itm->next = 0;

    if(list->count == 0)
    {
        list->items = itm;
    }
    else
    {
        i = list->items;
        while(i != 0 && i->next != 0)
        {
            i = i->next;
        }
        i->next = itm;
        itm->prev = i;
    }
    ++list->count;

    return(itm);
}

int
gettodopos(struct todolist *list, struct todoitm *itm)
{
    struct todoitm *i;
    int pos;

    if(!itm)
    {
        return(-1);
    }

    pos = 0;
    for(i = list->items;
        i != 0;
        i = i->next)
    {
        if(i == itm)
        {
            return(pos);
        }
        ++pos;
    }

    return(-1);
}

void
adjustscroll(
    struct todolist *list,
    struct todoitm  *itm,
    int rows,
    int *scroll)
{
    while(gettodopos(list, itm) >= rows + *scroll)
    {
        *scroll += 1;
    }
    while(gettodopos(list, itm) < *scroll)
    {
        *scroll -= 1;
    }
}

int
savelist(
    struct todolist *list,
    char *fname)
{
    char *buff;
    size_t buffsize;
    uint32_t *ip;
    char *cp;
    uint32_t i;
    size_t namelen;

    struct todoitm *itm;

    buffsize = 
        sizeof(uint32_t)
        + (sizeof(uint32_t) + 128*sizeof(char))*list->count;
    buff = (char *)ez_mem_alloc(buffsize);

    ip = (uint32_t *)buff;
    *ip++ = list->count;

    itm = list->items;
    cp = (char *)ip;
    for(i = 0;
        i < list->count;
        ++i)
    {
        ip = (uint32_t *)cp;
        *ip++ = itm->status;
        cp = (char *)ip;
        namelen = ez_str_len(itm->name);
        ez_str_copy_max(itm->name, cp, ez_array_count(itm->name) - 1);
        cp[namelen] = 0;
        itm = itm->next;
        cp += ez_array_count(itm->name);
    }

    ez_file_write(fname, (void *)buff, buffsize);
    ez_mem_free(buff);

    return(1);
}

int
loadlist(
    struct todolist *list,
    char *fname)
{
    char *buff;
    size_t buffsize;

    uint32_t *ip;
    char *cp;
    uint32_t i;
    uint32_t status;
    uint32_t listcount;

    struct todoitm *itm;

    if(!ez_file_exists(fname))
    {
        struct todolist tmplist = {0};
        savelist(&tmplist, fname);
    }

    buff = (char *)ez_file_read_bin(fname, &buffsize);

    ip = (uint32_t *)buff;
    listcount = *ip++;

    cp = (char *)ip;
    for(i = 0;
        i < listcount;
        ++i)
    {
        ip = (uint32_t *)cp;
        status = *ip++;
        cp = (char *)ip;
        itm = addtodo(list, cp, status);
        cp += ez_array_count(itm->name);
    }

    ez_file_free((void *)buff);

    return(1);
}

typedef enum
{
    MODE_NORMAL,
    MODE_ADD,
} mode_type;

struct todolist list;

#define CTRL_KEY(k) ((k) & 0x1f)

#define FILE_PATHNAME "C:\\tools\\todos.dat"

void
main(void)
{
    initscr();
    cbreak();
    noecho();

    int rows;
    int cols;

    getmaxyx(rows, cols);
    ++rows;
    ++cols;
    rows -= 2; /* Leave space for title bar */

    int cx;
    int cy;
    int scroll = 0;

    mode_type mode = MODE_NORMAL;

    curs_set(0);
    loadlist(&list, FILE_PATHNAME);

    struct todoitm *selitm = list.items;

    char c;
    int running;

    running = 1;
    c = 0;
    while(running)
    {
        clear();
        move(0, 0);

        char *apptitle =
            "jk move - SPACE toggle on/off - Ctrl+Q quit - gG move to bottom/top\r\n"
            "aAoO add - dD delete - Ctrl+S save\r\n"
        ;
        addstr(apptitle);
        getyx(cy, cx);


        move(cy, 0);
        struct todoitm *i;
        int count = -1;
        for(i = list.items;
            i != 0;
            i = i->next)
        {
            ++count;
            if(count < scroll)
            {
                continue;
            }
            else if(count >= scroll + rows)
            {
                break;
            }

            addstr("    ");
            if(i == selitm)
            {
                if(mode == MODE_ADD)
                {
                    attrset(FG_BLACK|BG_BRIGHT_RED);
                }
                else if(mode == MODE_NORMAL)
                {
                    attrset(FG_BLACK|BG_WHITE);
                }
            }
            addstr("[");
            if(i->status)
            {
                addstr("X");
            }
            else
            {
                addstr(" ");
            }
            addstr("] ");
            addstr(i->name);
            attrset(FG_BRIGHT_WHITE|BG_BLACK);
            addstr("\n\r");
        }

        refresh();
        c = getch();

        if(mode == MODE_NORMAL)
        {
            switch(c)
            {
                case CTRL_KEY('z'):
                case CTRL_KEY('q'):
                case CTRL_KEY('c'):
                {
                    savelist(&list, FILE_PATHNAME);
                    running = 0;
                } break;

                case CTRL_KEY('s'):
                {
                    savelist(&list, FILE_PATHNAME);
                } break;

                case 'j':
                case 'J':
                {
                    if(selitm && selitm->next)
                    {
                        selitm = selitm->next;
                    }
                    adjustscroll(&list, selitm, rows, &scroll);
                } break;

                case 'k':
                case 'K':
                {
                    if(selitm && selitm->prev)
                    {
                        selitm = selitm->prev;
                    }
                    adjustscroll(&list, selitm, rows, &scroll);
                } break;

                case ' ':
                {
                    if(selitm)
                    {
                        selitm->status = (selitm->status) ? 0 : 1;
                    }
                } break;

                case 'g':
                {
                    scroll = 0;
                    i = list.items;
                    while(i != 0 && i->next != 0)
                    {
                        i = i->next;
                    }
                    selitm = i;
                    adjustscroll(&list, selitm, rows, &scroll);
                } break;

                case 'G':
                {
                    selitm = list.items;
                    scroll = 0;
                } break;

                case 'a':
                case 'o':
                case 'A':
                {
                    struct todoitm *tmp;
                    struct todoitm *last;

                    last = list.items;
                    while(last != 0 && last->next != 0)
                    {
                        last = last->next;
                    }

                    mode = MODE_ADD;
                    i = addtodo(&list, "\0", 0);
                    if(selitm && selitm != last)
                    {
                        tmp = selitm->next;
                        selitm->next = i;
                        i->prev = selitm;
                        i->next = tmp;
                        tmp->prev = i;
                        if(last)
                        {
                            last->next = 0;
                        }
                    }
                    selitm = i;
                } break;

                case 'O':
                {
                    struct todoitm *tmp;
                    struct todoitm *last;

                    last = list.items;
                    while(last != 0 && last->next != 0)
                    {
                        last = last->next;
                    }

                    mode = MODE_ADD;
                    i = addtodo(&list, "\0", 0);
                    if(selitm)
                    {
                        if(selitm->prev)
                        {
                            tmp = selitm->prev;
                            tmp->next = i;
                            i->prev = tmp;
                            i->next = selitm;
                            selitm->prev = i;
                        }
                        else
                        {
                            list.items = i;
                            i->prev = 0;
                            i->next = selitm;
                            selitm->prev = i;
                        }
                        if(last)
                        {
                            last->next = 0;
                        }
                    }
                    selitm = i;
                } break;

                case 'd':
                case 'D':
                {
                    if(selitm)
                    {
                        i = selitm;
                        if(selitm->prev)
                        {
                            selitm->prev->next = selitm->next;
                            if(selitm->next)
                            {
                                selitm->next->prev = selitm->prev;
                                selitm = selitm->next;
                            }
                            else
                            {
                                selitm = selitm->prev;
                            }
                        }
                        else
                        {
                            list.items = selitm->next;
                            if(list.items)
                            {
                                list.items->prev = 0;
                            }
                            selitm = list.items;
                        }
                        --list.count;
                        ez_mem_free(i);
                    }
                } break;
            }
        }
        else if(mode == MODE_ADD)
        {
            if(ez_char_is_print(c))
            {
                size_t namelen = ez_str_len(selitm->name);
                if(namelen < ez_array_count(selitm->name) - 1)
                {
                    selitm->name[namelen] = c;
                    selitm->name[namelen+1] = 0;
                }
            }
            else if(c == '\r' || c == '\n')
            {
                mode = MODE_NORMAL;
            }
            else if(c == CTRL_KEY('q'))
            {
                if(selitm->prev && selitm->next)
                {
                    i = selitm;
                    selitm->prev->next = selitm->next;
                    selitm->next->prev = selitm->prev;
                    selitm = selitm->prev;
                    ez_mem_free(i);
                }
                else if(selitm->prev)
                {
                    i = selitm;
                    selitm->prev->next = 0;
                    selitm = selitm->prev;
                    ez_mem_free(i);
                }
                else if(selitm->next)
                {
                    i = selitm;
                    list.items = selitm->next;
                    selitm->next->prev = 0;
                    selitm = selitm->next;
                    ez_mem_free(i);
                }
                else
                {
                    list.items = 0;
                    ez_mem_free(selitm);
                    selitm = 0;
                }
                --list.count;
                mode = MODE_NORMAL;
            }
            else if(c == 0x08)
            {
                size_t namelen = ez_str_len(selitm->name);
                if(namelen > 0)
                {
                    selitm->name[namelen-1] = 0;
                }
            }
        }
    }

    endwin();
}
