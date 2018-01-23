/* $Id: curseslabel.C,v 1.1 2003/05/27 14:09:07 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "curses_config.h"
#include "curseslabel.H"

using namespace std;

CursesLabel::CursesLabel(CursesContainer *parent,
			 string textArg,
			 Curses::CursesAttr attributeArg)
	: Curses(parent), text(textArg), attribute(attributeArg)
{
}

CursesLabel::~CursesLabel()
{
}

void CursesLabel::setRow(int row)
{
	erase();
	Curses::setRow(row);
	draw();
}

void CursesLabel::setCol(int col)
{
	erase();
	Curses::setCol(col);
	draw();
}

void CursesLabel::setAlignment(Alignment newAlignment)
{
	erase();
	Curses::setAlignment(newAlignment);
	draw();
}

void CursesLabel::setAttribute(Curses::CursesAttr attr)
{
	attribute=attr;
	draw();
}

void CursesLabel::setText(string textArg)
{
	erase();
	text=textArg;
	draw();
}

int CursesLabel::getWidth() const
{
	return getTextLength(text.c_str());
}

int CursesLabel::getHeight() const
{
	return 1;
}

void CursesLabel::draw()
{
	writestr(text, attribute);
}

void CursesLabel::erase()
{
	string s="";

	s.append(getWidth(), ' ');

	writestr(s, CursesAttr());
}

void CursesLabel::writestr(string str, Curses::CursesAttr attr)
{
	writeText(str, 0, 0, attr);
}
