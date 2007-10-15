/* $Id: cursesscreen.C,v 1.15 2007/07/30 02:47:52 mrsam Exp $
**
** Copyright 2002-2006, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "curses_config.h"
#include "cursesscreen.H"
#include "cursesfield.H"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

using namespace std;

static char termStopKey= 'Z' & 31;

static RETSIGTYPE bye(int dummy)
{
	endwin();
	kill( getpid(), SIGKILL);
	_exit(0);
}

int Curses::getColorCount()
{
	if (has_colors())
		return COLORS;
	return 0;
}

CursesScreen::CursesScreen() : inputcounter(0)
{
	initscr();
	start_color();

#if HAS_USE_DEFAULT_COLORS
	use_default_colors();
#endif

	// Assign meaningful colors

	if (has_colors())
	{
		short f;
		short b;

		int i;

		pair_content(0, &f, &b);

		int colorcount=getColorCount();

		if (colorcount > 0)
			--colorcount;

		for (i=1; i<COLOR_PAIRS; i++)
		{
			if (colorcount <= 1)
				f=b=0;
			else
			{
				f=(i / COLORS) % COLORS;
				b=(i % COLORS);
			}

#if HAS_USE_DEFAULT_COLORS
			if (b == 0)
				b= -1;
#endif
			init_pair(i, f, b);
		}
	}

	raw();
	noecho();
	nodelay(stdscr, true);
	timeout(0);
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	//	fcntl(0, F_SETFL, O_NONBLOCK);

	signal(SIGHUP, bye);
	signal(SIGTERM, bye);

	memset(&inputstate, 0, sizeof(inputstate));
	save_w=COLS;
	save_h=LINES;
	shiftmode=false;

	const char *underline_hack_env=getenv("UNDERLINE_HACK");

	underline_hack=underline_hack_env && atoi(underline_hack_env);
}

CursesScreen::~CursesScreen()
{
	endwin();
}

int CursesScreen::getWidth() const
{
	return COLS;
}

int CursesScreen::getHeight() const
{
	return LINES;
}

void CursesScreen::draw()
{
	attroff(A_STANDOUT | A_BOLD | A_UNDERLINE | A_REVERSE);
	clear();
	CursesContainer::draw();
}

void CursesScreen::resized()
{
	CursesContainer::resized();
	draw();
}

bool CursesScreen::writeText(const char *text, int row, int col,
			     const Curses::CursesAttr &attr) const
{
	vector<wchar_t> wbuf;

	mbtow(text, wbuf);
	wbuf.push_back(0);

	return writeText(&wbuf[0], row, col, attr);
}

bool CursesScreen::writeText(const wchar_t *text, int row, int col,
			     const Curses::CursesAttr &attr) const
{

	// Truncate text to fit within the display

	if (row < 0 || row >= getHeight() || col >= getWidth())
		return false;

	while (col < 0)
	{
		if (!*text)
			return true;

		text++;
		col++;
	}

	if (!*text)
		return true;

	int w=getWidth();
	size_t i;

	for (i=0; text[i]; i++)
		;

	if ((size_t)(w - col) < i)
		i=w-col;

	if (i == 0)
		return true;

	vector<wchar_t> buf;

	buf.insert(buf.end(), text, text+i);

	// Replace unprintable characters with question marks.
	for (i=0; i < buf.size(); )
	{
#if HAVE_WCWIDTH
		int ww=wcwidth(buf[i]);
#endif

		if ((buf[i] >= 0 && buf[i] < ' ')
		    || (buf[i] >= 127 && buf[i] < 160)

#if HAVE_WCWIDTH
		    || ww <= 0
#endif
		    )
		{
			buf[i]='?';
		}

#if HAVE_WCWIDTH
		if (ww > 1)
		{
			if (i + ww > buf.size()) // Truncated
			{
				while (i < buf.size())
					buf[i++]=' ';
				continue;
			}

			buf.erase(buf.begin() + i + 1,
				  buf.begin() + i + ww);
		}
#endif

		i++;
	}

	buf.push_back(0);

	text=&buf[0];

	int c=0;

	// If color was requested, then the colors will wrap if the requested
	// color number exceeds the # of colors reported by curses.

	int ncolors=getColorCount();

	if (ncolors > 2 && COLOR_PAIRS > 0)
	{
		int bg=attr.getBgColor();
		int fg=attr.getFgColor();

		if (fg || bg)
		{
			if (bg > 0)
				bg= ((bg - 1) % (ncolors-1)) + 1;

			fg %= ncolors;

			if (fg == bg)
				fg = (fg + ncolors/2) % ncolors;
		}

		c=bg + ncolors * fg;

		c %= COLOR_PAIRS;
	}

	if (c > 0)
		attron(COLOR_PAIR(c));

	if (attr.getHighlight())
		attron(A_BOLD);
	if (attr.getReverse())
		attron(A_REVERSE);

	if (attr.getUnderline())
	{
		if (termattrs() & A_UNDERLINE)
		{
			if (underline_hack)
			{
				size_t i=0;

				for (i=0; buf[i]; i++)
					if (buf[i] == ' ')
						buf[i]=0xA0;
			}
			attron(A_UNDERLINE);
		}
		else
		{
			size_t i;

			for (i=0; buf[i]; i++)
				if (buf[i] == ' ')
					buf[i]='_';
		}
	}

#if WIDECURSES
	mvaddwstr(row, col, text);
#else
	//	mvaddwstr(row, col, text);

	string mb=wtomb(text);

	if (col == 0 && mb.size() == (size_t)getWidth() &&
	    mb.find_first_not_of(' ') == mb.npos &&
	    attr.getBgColor() == 0 &&
	    !attr.getHighlight() &&
	    !attr.getReverse() &&
	    !attr.getUnderline())
	{
		// Hope that curses can optimize this...

		move(row, col);
		clrtoeol();
	}
	else
	{
		for (i=0; i<mb.size(); i++)
		{
			if (mb[i] == 127 || buf[i] < ' ')
			{
				buf[i]=' ';
			}
		}
#if 0
		cerr << "ADDSTR " << row << "x" << col << ": "
		     << mb.c_str() << endl;
#endif
		mvaddstr(row, col, mb.c_str());
	}
#endif

	if (c)
		attroff(COLOR_PAIR(c));
	attroff(A_STANDOUT | A_BOLD | A_UNDERLINE | A_REVERSE);
	return true;
}

int CursesScreen::getTextLength(const char *text) const
{
	vector<wchar_t> wbuf;

	mbtow(text, wbuf);
	wbuf.push_back(0);

	return getTextLength(&wbuf[0]);
}

int CursesScreen::getTextLength(const wchar_t *text) const
{
	size_t l=0;

	while (text && *text)
	{
#if HAVE_WCWIDTH
		int w=wcwidth(*text);

		if (w <= 0)
			w=1;

		l += w;
#else
		l++;
#endif
		text++;
	}
	return l;
}

void CursesScreen::flush()
{
	refresh();
}

void (*Curses::suspendedHook)(void)=&Curses::suspendedStub;

void Curses::suspendedStub(void)
{
}

void Curses::setSuspendHook(void (*func)(void))
{
	suspendedHook=func;
}

Curses::Key CursesScreen::getKey()
{
	for (;;)
	{
		Key k=doGetKey();

		if (k.plain())
		{
			if (k.key == termStopKey)
			{
				if (suspendCommand.size() > 0)
				{
					vector<const char *> argv;

					const char *p=getenv("SHELL");

					if (!p || !*p)
						p="/bin/sh";

					argv.push_back(p);
					argv.push_back("-c");
					argv.push_back(suspendCommand.c_str());
					argv.push_back(NULL);
					Curses::runCommand(argv, -1, "");
					draw();
					(*suspendedHook)();
					continue;
				}

				endwin();
				kill( getpid(), SIGSTOP );
				draw();
				refresh();
				(*suspendedHook)();
				continue;
			}
		}
		else if (k == Key::RESIZE)
		{
			save_w=COLS;
			save_h=LINES;

			::erase(); // old window
			resized();
			touchwin(stdscr);
			flush();
			draw();
			flush();
		}
		return k;
	}
}

int Curses::runCommand(vector<const char *> &argv,
		       int stdinpipe,
		       string continuePrompt)
{
	endwin();
	if (continuePrompt.size() > 0)
	{
		cout << endl << endl;
		cout.flush();
	}

	pid_t editor_pid, p2;

#ifdef WIFSTOPPED

	RETSIGTYPE (*save_tstp)(int)=signal(SIGTSTP, SIG_IGN);
	RETSIGTYPE (*save_cont)(int)=signal(SIGCONT, SIG_IGN);
#endif

	editor_pid=fork();
	int waitstat;

	signal(SIGCHLD, SIG_DFL);

	if (editor_pid < 0)
	{
#ifdef WIFSTOPPED
		signal(SIGTSTP, save_tstp);
		signal(SIGCONT, save_cont);
#endif
		beep();
		return -1;
	}

	if (editor_pid == 0)
	{
		signal(SIGTSTP, SIG_DFL);
		signal(SIGCONT, SIG_DFL);
		if (continuePrompt.size() > 0)
		{
			dup2(1, 2);
		}
		signal(SIGINT, SIG_DFL);
		if (stdinpipe >= 0)
		{
			dup2(stdinpipe, 0);
			close(stdinpipe);
		}
		execvp(argv[0], (char **)&argv[0]);
		kill(getpid(), SIGKILL);
		_exit(1);
	}

	if (stdinpipe >= 0)
		close(stdinpipe);

	signal(SIGINT, SIG_IGN);

	for (;;)
	{

#ifdef WIFSTOPPED

		p2=waitpid(editor_pid, &waitstat, WUNTRACED);

		if (p2 != editor_pid)
			continue;

		if (WIFSTOPPED(waitstat))
		{
			kill(getpid(), SIGSTOP);
			kill(editor_pid, SIGCONT);
			continue;
		}
#else

		p2=wait(&waitstat);

		if (p2 != editor_pid)
			continue;
#endif
		break;
	}

	signal(SIGINT, SIG_DFL);

#ifdef WIFSTOPPED
	signal(SIGTSTP, save_tstp);
	signal(SIGCONT, save_cont);
#endif

	if (continuePrompt.size() > 0)
	{
		char buffer[80];

		cout << endl << continuePrompt;
		cout.flush();
		cin.getline(buffer, sizeof(buffer));
	}

	refresh();

	return WIFEXITED(waitstat) ? WEXITSTATUS(waitstat):-1;
}

Curses::Key CursesScreen::doGetKey()
{
	// Position cursor first.

	int row=getHeight();
	int col=getWidth();

	if (row > 0) --row;
	if (col > 0) --col;

	if (currentFocus != NULL)
	{
		int new_row=currentFocus->getHeight();
		if (new_row > 0) --new_row;

		int new_col=currentFocus->getWidth();
		if (new_col > 0) --new_col;
		curs_set(currentFocus->getCursorPosition(new_row, new_col));

		if (new_row < row)
			row=new_row;

		if (new_col < col)
			col=new_col;
	}
	else
		curs_set(0);

	//	flush(); // BUG ncurses-5.2, blocking read after sigwinch.

	int k=mvgetch(row, col);

	if (k == ERR)
	{
		flush();

		if (++inputcounter >= 100)
		{
			// Detect closed xterm without SIGHUP

			if (write(1, "", 1) < 0)
				bye(0);
			inputcounter=0;
		}

		if (LINES != save_h || COLS != save_w)
			return (Key(Key::RESIZE));

		return Key((wchar_t)0);
	}
	inputcounter=0;

#ifdef KEY_SUSPEND

	if (k == KEY_SUSPEND)
		k=termStopKey;
#endif

	if ( (k & 0xFF) != k)
	{
		static const struct {
			int keyval;
			const char *keycode;
		} keys[]={

#define K(v,c) { v, Curses::Key::c }

			K(KEY_LEFT, LEFT),
			K(KEY_RIGHT, RIGHT),
			K(KEY_SLEFT, SHIFTLEFT),
			K(KEY_SRIGHT, SHIFTRIGHT),
			K(KEY_UP, UP),
			K(KEY_DOWN, DOWN),
			K(KEY_ENTER, ENTER),
			K(KEY_PPAGE, PGUP),
			K(KEY_NPAGE, PGDN),
			K(KEY_HOME, HOME),
			K(KEY_END, END),
			K(KEY_SHOME, SHIFTHOME),
			K(KEY_SEND, SHIFTEND),
			K(KEY_RESIZE, RESIZE),
			K(KEY_BACKSPACE, BACKSPACE),
			K(KEY_DC, DEL),
			K(KEY_EOL, CLREOL),
#undef K
		};

		if (shiftmode)
		{
			switch (k) {
			case KEY_PPAGE:
				return (Key(Key::SHIFTPGUP));
			case KEY_NPAGE:
				return (Key(Key::SHIFTPGDN));
			case KEY_UP:
				return (Key(Key::SHIFTUP));
			case KEY_DOWN:
				return (Key(Key::SHIFTDOWN));
			case KEY_LEFT:
				k=KEY_SLEFT;
				break;
			case KEY_RIGHT:
				k=KEY_SRIGHT;
				break;
			case KEY_HOME:
				k=KEY_SHOME;
				break;
			case KEY_END:
				k=KEY_SEND;
				break;
			default:
				shiftmode=0;
			}
		}

		size_t i;

		for (i=0; i<sizeof(keys)/sizeof(keys[0]); i++)
			if (keys[i].keyval == k)
				return Key(keys[i].keycode);

#if HAS_FUNCTIONKEYS
		if (k >= KEY_F(0) && k <= KEY_F(63))
		{
			Key kk("FKEY");
			kk.key=k-KEY_F(0);
			return kk;
		}
#endif

		return Key( (wchar_t)0);
	}

	if (k == 0)
	{
		shiftmode= !shiftmode;

		return Key(Key::SHIFT);
	}

	if (k == CursesField::clrEolKey)
		return Key(Key::CLREOL);

	if (k == '\r')
		return Key(Key::ENTER);

	shiftmode=false;

	char inputChar=(char)k;

	wchar_t w;

	size_t n=mbrtowc(&w, &inputChar, 1, &inputstate);

	if (n != 1)
	{
		if (n != (size_t)-2)
		{
			memset(&inputstate, 0, sizeof(inputstate));
			// -2 - not enough chars, it's ok, else reset
		}

		return (Key((wchar_t)0));
	}

	return (Key((wchar_t)w));
}

void CursesScreen::beepError()
{
	beep();
}
