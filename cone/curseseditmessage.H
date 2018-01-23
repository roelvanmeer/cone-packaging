/* $Id: curseseditmessage.H,v 1.4 2004/05/01 04:59:52 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#ifndef curseseditmessage_H
#define curseseditmessage_H

///////////////////////////////////////////////////////////////////////////////
//
// The message editor

#include "config.h"
#include "curses/cursescontainer.H"
#include "curses/curseskeyhandler.H"

#include "libmail/objectmonitor.H"
#include "unicode/unicode.h"
#include "spellcheckerbase.H"
#include <time.h>

#include <vector>
#include <string>

#define LINEW 77

namespace mail {
	class Search;
};

class CursesEditMessage : public Curses,
			  public CursesKeyHandler,
			  public mail::obj {

	// Text that's being edited is saved, always in UTF-8

	std::vector<std::string> text_UTF8;

	//
	// Whenever cursor row changes, leaf wants to update the status bar.
	// Rather then remembering to call a function each time the cursor
	// row variable gets modified, the cursor row # is this object, which
	// automatically invokes cursorRowChanged() each time it's poked.
	//

	class CursorRowUpdate {

		size_t r;
	public:
		CursesEditMessage *me;

		CursorRowUpdate() : r(0), me(NULL) {}
		~CursorRowUpdate() {}

		operator size_t()
		{
			return r;
		}

		size_t operator=(size_t newRow)
		{
			r=newRow;
			if (me)
				me->cursorRowChanged();
			return r;
		}

		size_t operator--()
		{
			return ((*this)=r-1);
		}

		size_t operator++()
		{
			return ((*this)=r+1);
		}

	};

	CursorRowUpdate cursorRow;
	size_t cursorCol; // Current cursor column
	size_t cursorLineHorizShift; // Current horiz scroll offset

	bool marked;	// Region being marked
	size_t markRow;	// The other end of the region.
	size_t markCursorPos;

	std::string defaultSearchStr; // Prev search string
	std::string defaultReplaceStr; // Prev replace string

	//
	// Given horizontal position #col, return an iterator to the character
	// in "line" that's shown at the given horizontal position.
	//
	std::vector<unicode_char>
	::iterator getIndexOfCol(std::vector<unicode_char> &line,
				 size_t col);

	//
	// Same thing, except that the starting position of each character
	// in 'line' is precomputed in pos
	//
	static std::vector<unicode_char>
	::iterator getIndexOfCol(std::vector<unicode_char> &line,
				 std::vector<size_t> &pos,
				 size_t colNum);

	//
	// For each character in line, create pos giving each character's
	// column.
	//

	static void getTextPos(std::vector<unicode_char> &line,
			       std::vector<size_t> &pos);

	//
	// Replace tabs in line (horizontal positions in pos) with U+00A0
	//

	static void expandTabs(std::vector<unicode_char> &line,
			       std::vector<size_t> &pos);

	class SpellCheckerManager;
	class DefineMacroHelper;

public:
	virtual void cursorRowChanged();

	CursesEditMessage(CursesContainer *parent);
	~CursesEditMessage();

	static time_t autosaveInterval;

	int getWidth() const;
	int getHeight() const;

	void load(std::istream &i);
	void save(std::ostream &o);
	bool isFocusable();
	bool processKeyInFocus(const Key &key);
	int getCursorPosition(int &row, int &col);

	void draw();
	void erase();

	void eraseAllText();

	void setCurrentLine(size_t row)
	{
		if (row < text_UTF8.size())
		{
			cursorRow=row;
			cursorCol=0;
		}
	}

	size_t currentLine() { return cursorRow; }
	size_t numLines() { return text_UTF8.size(); }
	std::string line_utf8(size_t n) { return text_UTF8[n]; }

private:
	virtual void macroDefined();
	void processMacroKey(size_t macroNameSize,
			     std::string &repl_utf8);

	void getText(size_t line, std::vector<unicode_char> &textRef);
	void setText(size_t line, std::vector<unicode_char> &textRef);

	void inserted(size_t, std::vector<unicode_char> &line);
	void drawLine(size_t lineNum);
	void left(bool);
	void right();
	void deleteChar();
	bool checkReplace(bool &, std::string &, SpellCheckerBase::Manager *);
	class ReplacePrompt;

	bool search(bool doUpdate, bool doWrap, mail::Search &engine,
		    size_t searchStrSize);
	void mark();
	void end();

	class yankHelper;

	void yank(wchar_t key, bool doUpdate, yankHelper &);

	bool getMarkedRegion(size_t &row1, size_t &row2,
			     size_t &col1, size_t &col2);

	static size_t colToPos(std::string &, std::vector<size_t> &, size_t,
			       bool);

	Curses::Key lastKey;
	bool processKey(const Curses::Key &key);
	bool listKeys( std::vector< std::pair<std::string, std::string> >
		       &list);

	virtual void modified()=0;
	virtual std::string getConfigDir();
	virtual void extedited();

public:
	static std::string externalEditor;
};

#endif
