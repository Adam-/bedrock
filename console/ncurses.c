#include "console/console.h"
#include "console/socket.h"

#include <curses.h>

static WINDOW *win;
static WINDOW *text_window;
static int max_y, max_x; /* Terminal dimensions, max_y is used to determine how long message_list can grow */

void ncurses_init()
{
	win = initscr();

	getmaxyx(win, max_y, max_x);
	text_window = newwin(max_y - 2, max_x - 2, 0, 0);
	scrollok(text_window, TRUE);

	wmove(win, max_y - 2, 0);
	wprintw(win, "> ");

	wrefresh(win);
	wrefresh(text_window);
}

void ncurses_shutdown()
{
	delwin(text_window);
	text_window = NULL;

	endwin();
	win = NULL;
}

void ncurses_print(const char *data)
{
	wprintw(text_window, "%s", data);
	wrefresh(text_window);
}

void ncurses_read()
{
	static char buf[512];
	static size_t bufsize = 0;

	if (bufsize >= sizeof(buf) - 1)
		bufsize = 0;
	
	buf[bufsize++] = getch();
	buf[bufsize] = 0;

	if (buf[bufsize - 1] == '\n')
	{
		if (!strcasecmp(buf, "QUIT\n"))
		{
			console_running = false;
			bufsize = 0;
		}
		else
		{
			socket_send(buf);
			bufsize = 0;

			wmove(win, max_y - 2, 0);
			wprintw(win, "> ");
			clrtoeol();
			wrefresh(win);
		}
	}
}

