/* $Id: curseslabel.H,v 1.1 2003/05/27 14:09:07 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef curseslabel_H
#define curseslabel_H

#include "mycurses.H"

////////////////////////////////////////////////////////////////////////
//
// A plain, garden variety, label.  Centered, or right-aligned, perhaps.
//
// A lot of work, just for this

class CursesLabel : public Curses {

	std::string text;

protected:
	Curses::CursesAttr attribute;
public:
	CursesLabel(CursesContainer *parent,
		    std::string textArg,
		    Curses::CursesAttr attributeArg=Curses::CursesAttr());
	~CursesLabel();

	void setRow(int row);
	void setCol(int col);
	void setAlignment(Alignment alignment);
	void setAttribute(Curses::CursesAttr attr);
	virtual void setText(std::string textArg);

	int getWidth() const;
	int getHeight() const;

	void draw();
	void erase();
private:
	void writestr(std::string str, Curses::CursesAttr attr);
};

#endif
