/* $Id: cursesscreen.H,v 1.2 2004/02/19 04:20:09 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef cursesscreen_H
#define cursesscreen_H

#include "../curses/curses_config.h"
#include "cursescontainer.H"

#if HAVE_NCURSESW_CURSES_H
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif

//
// A libcurses implementation.  A CursesScreen object is typically the
// root object of the Curses hierarchy.  The constructor initializes libcurses,
// the destructor cleans it up.
//

class CursesScreen : public CursesContainer {

	int save_w, save_h; // Fix some libcurses.a resizing bugs.

	mbstate_t inputstate;

	int inputcounter;

public:
	CursesScreen();
	~CursesScreen();

	// Calculate max size.
	int getWidth() const;
	int getHeight() const;

	void flush();
	void draw();

	bool writeText(const char *text, int row, int col,
		       const Curses::CursesAttr &attr) const;
	bool writeText(const wchar_t *text, int row, int col,
		       const Curses::CursesAttr &attr) const;

	int getTextLength(const char *text) const;
	int getTextLength(const wchar_t *text) const;

	// Return keyboard input.  Returns Key::nokey() if no keyboard input
	// isavailable.
	Key getKey();

	void beepError();
private:

	Key doGetKey();
public:
	void resized();
};

#endif
