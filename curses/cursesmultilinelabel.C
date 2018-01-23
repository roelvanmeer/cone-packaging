/* $Id: cursesmultilinelabel.C,v 1.1 2004/03/21 05:30:01 mrsam Exp $
**
** Copyright 2004, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "curses_config.h"
#include "cursesmultilinelabel.H"

using namespace std;

CursesMultilineLabel::CursesMultilineLabel(CursesContainer *parent,
					   string textArg,
					   Curses::CursesAttr attributeArg)
	: Curses(parent), width(0), attribute(attributeArg)
{
	mbtow(textArg.c_str(), text);
}

void CursesMultilineLabel::init()
{
	int w=getWidth();

	if (w < 10)
		w=10;

	lines.clear();
	wordWrap(text.begin(), text.end(), lines, w);
}

CursesMultilineLabel::~CursesMultilineLabel()
{
}

void CursesMultilineLabel::setRow(int row)
{
	erase();
	Curses::setRow(row);
	draw();
}

void CursesMultilineLabel::setCol(int col)
{
	erase();
	Curses::setCol(col);
	draw();
}

void CursesMultilineLabel::setText(string newText)
{
	erase();
	text.clear();
	mbtow(newText.c_str(), text);
	init();
	draw();
}

void CursesMultilineLabel::setAlignment(Alignment newAlignment)
{
	erase();
	Curses::setAlignment(newAlignment);
	draw();
}

void CursesMultilineLabel::setAttribute(Curses::CursesAttr attr)
{
	attribute=attr;
	draw();
}

int CursesMultilineLabel::getWidth() const
{
	return width;
}

void CursesMultilineLabel::setWidth(int w)
{
	erase();
	width=w;
	init();
	draw();
}

int CursesMultilineLabel::getHeight() const
{
	int n=(int)lines.size();

	if (n <= 0)
		n=1;
	return n;
}

void CursesMultilineLabel::resized()
{
	erase();
	init();
	draw();
}

void CursesMultilineLabel::draw()
{
	int i, n;

	n=getHeight();
	int w=getWidth();

	for (i=0; i<n; i++)
	{
		vector<wchar_t> v;

		if (i < (int)lines.size())
			v.insert(v.begin(),
				 lines[i].first,
				 lines[i].second);
		if ((int)v.size() < w)
			v.insert(v.end(), w-v.size(), ' ');
		v.push_back(0);
		writeText(&v[0], i, 0, attribute);
	}
}

void CursesMultilineLabel::erase()
{
	string s="";
	int i, n;
	s.append(getWidth(), ' ');

	n=getHeight();
	for (i=0; i<n; i++)
		writeText(s.c_str(), i, 0, attribute);
}

