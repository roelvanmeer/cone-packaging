/* $Id: curses.C,v 1.4 2004/03/27 15:54:16 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "curses_config.h"

#include "mycurses.H"
#include "cursescontainer.H"
#include "curseskeyhandler.H"

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <iostream>

using namespace std;

bool Curses::keepgoing=false;
bool Curses::shiftmode=false;

string Curses::suspendCommand;

#define TABSIZE 8

const char Curses::Key::LEFT[]="LEFT",
	Curses::Key::RIGHT[]="RIGHT",
	Curses::Key::SHIFTLEFT[]="SHIFTLEFT",
	Curses::Key::SHIFTRIGHT[]="SHIFTRIGHT",
	Curses::Key::UP[]="UP",
	Curses::Key::DOWN[]="DOWN",
	Curses::Key::SHIFTUP[]="SHIFTUP",
	Curses::Key::SHIFTDOWN[]="SHIFTDOWN",
	Curses::Key::DEL[]="DEL",	
	Curses::Key::CLREOL[]="CLREOL",
	Curses::Key::BACKSPACE[]="BACKSPACE",
	Curses::Key::ENTER[]="ENTER",
	Curses::Key::PGUP[]="PGUP",
	Curses::Key::PGDN[]="PGDN",
	Curses::Key::SHIFTPGUP[]="SHIFTPGUP",
	Curses::Key::SHIFTPGDN[]="SHIFTPGDN",
	Curses::Key::HOME[]="HOME",
	Curses::Key::END[]="END",
	Curses::Key::SHIFTHOME[]="SHIFTHOME",
	Curses::Key::SHIFTEND[]="SHIFTEND",
	Curses::Key::SHIFT[]="SHIFT",
	Curses::Key::RESIZE[]="RESIZE";

bool Curses::Key::operator==(const std::vector<wchar_t> &v) const
{
	return keycode == 0 &&
		std::find(v.begin(), v.end(), key) != v.end();
}

Curses::Curses(CursesContainer *parentArg) : parent(parentArg),
					     row(0), col(0),
					     alignment(Curses::LEFT)
{
	if (parent != NULL)
		parent->addChild(this);
}

Curses::~Curses()
{
	CursesContainer *p=getParent();

	if (p)
		p->deleteChild(this);

	if (hasFocus())
		currentFocus=0;
}

int Curses::getScreenWidth() const
{
	const CursesContainer *p=getParent();

	while (p && p->getParent())
		p=p->getParent();

	return (p ? p->getWidth():0);
}

int Curses::getScreenHeight() const
{
	const CursesContainer *p=getParent();

	while (p && p->getParent())
		p=p->getParent();

	return (p ? p->getHeight():0);
}

int Curses::getRow() const
{
	return row;
}

int Curses::getCol() const
{
	return col;
}

void Curses::setRow(int r)
{
	row=r;
}

void Curses::scrollTo(size_t r)
{
	if (parent)
		parent->scrollTo(r + row);
}

void Curses::setCol(int c)
{
	col=c;
}

void Curses::resized()
{
	draw();
}

void Curses::getVerticalViewport(size_t &first_row,
				 size_t &nrows)
{
	CursesContainer *p=getParent();

	if (!p)
	{
		first_row=0;
		nrows=getHeight();
		return;
	}

	p->getVerticalViewport(first_row, nrows);
	size_t r=getRow();
	size_t h=getHeight();


	if (r + h <= first_row || r >= first_row + nrows)
	{
		first_row=nrows=0;
		return;
	}

	if (first_row < r)
	{
		nrows -= r-first_row;
		first_row=r;
	}

	first_row -= r;

	if (first_row + nrows > h)
		nrows=h-first_row;
}

void Curses::setAlignment(Alignment alignmentArg)
{
	alignment=alignmentArg;
}

Curses::Alignment Curses::getAlignment()
{
	return alignment;
}

int Curses::getRowAligned() const
{
	return row;
}

int Curses::getColAligned() const
{
	int c=col;

	if (alignment == PARENTCENTER)
	{
		const CursesContainer *p=getParent();

		if (p != NULL)
		{
			c=p->getWidth()/2;
			c -= getWidth()/2;
		}
	}
	else if (alignment == CENTER)
		c -= getWidth() / 2;
	else if (alignment == RIGHT)
		c -= getWidth();
	return c;
}

Curses *Curses::getDialogChild() const
{
	return NULL;
}

bool Curses::writeText(const char *text, int r, int c,
		       const CursesAttr &attr) const
{
	CursesContainer *p=parent;

	if (!isDialog() && p && p->getDialogChild())
		return false; // Parent has a dialog and it ain't us

	if (p)
		return p->writeText(text, r+getRowAligned(),
				    c+getColAligned(), attr);
	return false;
}


bool Curses::writeText(const wchar_t *text, int r, int c,
			       const CursesAttr &attr) const
{
	CursesContainer *p=parent;

	if (!isDialog() && p && p->getDialogChild())
		return false; // Parent has a dialog and it ain't us

	if (p)
		return p->writeText(text, r+getRowAligned(),
				    c+getColAligned(), attr);
	return false;
}

void Curses::writeText(string text, int row, int col,
		       const CursesAttr &attr) const
{
	writeText(text.c_str(), row, col, attr);
}

bool Curses::isDialog() const
{
	return false;
}

void Curses::erase()
{
	const CursesContainer *p=getParent();

	if (!isDialog() && p && p->getDialogChild())
		return; // Parent has a dialog and it ain't us

	size_t w=getWidth();
	size_t h=getHeight();

	vector<wchar_t> spaces;

	spaces.insert(spaces.end(), w, ' ');
	spaces.push_back(0);

	size_t i;

	CursesAttr attr;

	for (i=0; i<h; i++)
		writeText(&*spaces.begin(), i, 0, attr);
}

int Curses::getTextLength(const char *text) const
{
	const CursesContainer *p=getParent();

	if (p)
		return p->getTextLength(text);
	return 0;
}

int Curses::getTextLength(const wchar_t *text) const
{
	const CursesContainer *p=getParent();

	if (p)
		return p->getTextLength(text);
	return 0;
}

void Curses::beepError()
{
	CursesContainer *p=getParent();

	if (p)
		p->beepError();
}

Curses *Curses::currentFocus=0;

void Curses::requestFocus()
{
	Curses *oldFocus=currentFocus;

	currentFocus=NULL;

	cursesPtr<Curses> ptr=this;

	if (oldFocus)
		oldFocus->focusLost();

	if (!ptr.isDestroyed() && currentFocus == NULL)
	{
		currentFocus=this;
		focusGained();
	}
}

void Curses::dropFocus()
{
	Curses *oldFocus=currentFocus;

	currentFocus=NULL;

	if (oldFocus)
		oldFocus->focusLost();
}

void Curses::focusGained()
{
	draw();
}

void Curses::focusLost()
{
	draw();
}

void Curses::flush()
{
	CursesContainer *p=getParent();

	if (p)
		p->flush();
}

bool Curses::hasFocus()
{
	return currentFocus == this;
}

int Curses::getCursorPosition(int &r, int &c)
{
	r += getRowAligned();
	c += getColAligned();

	CursesContainer *parent=getParent();
	if (parent)
		return parent->getCursorPosition(r, c);

	return 1;
}


bool Curses::processKey(const Key &k)
{
	if (k == Key::RESIZE)
		return true; // No-op.

	return CursesKeyHandler::handle(k, currentFocus);
}

bool Curses::processKeyInFocus(const Key &key)
{
	if (key == '\t' || key == key.DOWN || key == key.SHIFTDOWN ||
	    key == key.ENTER)
	{
		transferNextFocus();
		return true;
	}

	if (key == key.UP || key == key.SHIFTUP)
	{
		transferPrevFocus();
		return true;
	}

	if (key == key.PGUP || key == key.SHIFTPGUP)
	{
		int h=getScreenHeight();

		if (h > 5)
			h -= 5;

		if (h < 5)
			h=5;

		while (h)
		{
			processKey(Key(key ==
				       Key::PGUP ? Key::UP:Key::SHIFTUP));
			--h;
		}
		return true;
	}

	if (key == key.PGDN || key == key.SHIFTPGDN)
	{
		int h=getScreenHeight();

		if (h > 5)
			h -= 5;

		if (h < 5)
			h=5;

		while (h)
		{
			processKey(Key(key ==
				       Key::PGDN ? Key::DOWN:Key::SHIFTDOWN));
			--h;
		}
		return true;
	}

	return false;
}

void Curses::mbtow(const char *text, vector<wchar_t> &wbuf)
{
	mbstate_t ps, ps_save;

	memset(&ps, 0, sizeof(ps));

	wbuf.clear();

	size_t l=strlen(text);

	while (l)
	{
		wchar_t wc;
		ps_save=ps;

		size_t r=mbrtowc(&wc, text, l, &ps);

		if (r > l) // -1 or -2
		{
			wc='?';
			r=1;
			ps=ps_save;
		}

		if (wc == '\t')
			wc=' ';

#if HAVE_WCWIDTH
		int nn=wcwidth(wc);

		if (nn <= 0)
		{
			nn=1;
			wc='?';
		}
#endif

		wbuf.push_back(wc);

#if HAVE_WCWIDTH
		while (--nn)
			wbuf.push_back(MCPAD);
#endif

		l -= r;
		text += r;
	}
}


string Curses::wtomb(const wchar_t *w)
{
	vector<char> cbuf;
	int pass;
	size_t ll=0;

#ifdef MB_LEN_MAX
	char scratchBuf[MB_LEN_MAX*2]; // Be safe
#else
	char scratchBuf[64]; // Be safe
#endif

	for (pass=0; pass<2; pass++)
	{
		size_t cnt=0;

		if (pass)
			cbuf.insert(cbuf.begin(), ll, ' ');

		char *p= &cbuf[0];

		const wchar_t *wptr=w;

		mbstate_t mbs;
		memset(&mbs, 0, sizeof(mbs));

		while (*wptr)
		{
			if (cnt && *wptr == MCPAD)
				// Padding after multicell widechar.
			{
				--cnt;
				wptr++;
				continue;
			}

			wchar_t wc= *wptr;

#if HAVE_WCWIDTH
			cnt=wcwidth(wc);

			if (cnt <= 0)
				wc=' ';
			else
				--cnt;
#endif

			size_t l=wcrtomb(pass ? p:scratchBuf,
					 wc ? wc:' ', &mbs);

			if (l == (size_t)-1)
			{
				l=1;
				if (pass)
					*p++='?';
			}
			else
			{
				if (pass)
					p += l;
			}

			ll += l;
			wptr++;
		}

		size_t l=wcrtomb(pass ? p:NULL, 0, &mbs);

		ll += l;
	}

	if (cbuf.size() > 0 && cbuf.end()[-1] == 0)
		cbuf.erase(cbuf.end()-1); // Drop trailing '\0

	return ( string(cbuf.begin(), cbuf.end()));
}

bool Curses::isFocusable()
{
	return 0;
}

void Curses::transferNextFocus()
{
	Curses *p=this;

	do
	{
		p=p->getNextFocus();

	} while (p != NULL && !p->isFocusable());

	if (p)
		p->requestFocus();
}

void Curses::transferPrevFocus()
{
	Curses *p=this;

	do
	{
		p=p->getPrevFocus();
	} while (p != NULL && !p->isFocusable());

	if (p)
		p->requestFocus();
}

bool Curses::childPositionCompareFunc(Curses *a, Curses *b)
{
	if (a->getRow() < b->getRow())
		return true;

	if (a->getRow() == b->getRow())
	{
		if (a->getCol() < b->getCol())
			return true;
		if (a->getCol() == b->getCol())
		{
			CursesContainer *p=a->getParent();
			size_t ai, bi;

			for (ai=0; ai<p->children.size(); ai++)
				if (a == p->children[ai].child)
				{
					for (bi=0; bi<p->children.size(); bi++)
						if (b == p->children[bi].child)
							return ai < bi;
					break;
				}
		}
	}
	return false;
}

Curses *Curses::getNextFocus()
{
	CursesContainer *p=getParent();
	Curses *child=this;

	while (p != NULL)
	{
		size_t i;
		Curses *nextFocusPtr=NULL;

		for (i=0; i<p->children.size(); i++)
			if ( childPositionCompareFunc(child,
						      p->children[i].child) &&
			     (nextFocusPtr == NULL ||
			      childPositionCompareFunc(p->children[i].child,
						       nextFocusPtr)))
				nextFocusPtr=p->children[i].child;

		if (nextFocusPtr)
			return nextFocusPtr;

		if (p->isDialog())
			break; // Do not cross dialog box boundaries.

		child=p;
		p=p->getParent();
	}
	return NULL;
}

Curses *Curses::getPrevFocus()
{
	CursesContainer *p=getParent();
	Curses *child=this;

	while (p != NULL)
	{
		size_t i;

		Curses *prevFocusPtr=NULL;

		for (i=0; i<p->children.size(); i++)
			if ( childPositionCompareFunc(p->children[i].child,
						      child) &&
			     (prevFocusPtr == NULL ||
			      childPositionCompareFunc(prevFocusPtr,
						       p->children[i].child)))
				prevFocusPtr=p->children[i].child;

		if (prevFocusPtr)
			return prevFocusPtr;

		if (p->isDialog())
			break; // Do not cross dialog box boundaries.

		child=p;
		p=p->getParent();
	}
	return NULL;
}

void Curses::getTextPos(vector<wchar_t> &line,
			vector<size_t> &pos)
{
	pos.clear();
	pos.insert(pos.begin(), line.size()+1, 0);

	size_t i=0;

	vector<wchar_t>::iterator b=line.begin(), e=line.end();

	size_t col=0;

	while (b != e)
	{
		pos[i]=col;

		if (*b == '\t')
		{
			col += TABSIZE;
			col -= (col % TABSIZE);
		}
		else
		{
			size_t n=1;

#if HAVE_WCWIDTH
			int nn=wcwidth(*b);

			if (nn > 1)
				n=nn;
#endif
			col += n;
		}

		b++;
		i++;
	}
	pos[i]=col;
}

void Curses::expandTabs(vector<wchar_t> &line,
			vector<size_t> &pos)
{
	size_t i=line.size();

	while (i > 0)
	{
		if (line[--i] != '\t')
		{
			if (pos[i+1] > pos[i]+1) // Multicell char
				line.insert(line.begin() + i + 1,
					    pos[i+1] - pos[i] - 1, MCPAD);

			continue;
		}

		size_t nSpaces=pos[i+1] - pos[i]; // That's how big this tab is

		line[i]=' ';
		if (nSpaces > 1)
			line.insert(line.begin() + i, nSpaces - 1, ' ');
	}
}

void Curses::unexpandMulticell(vector<wchar_t> &line)
{
#if HAVE_WCWIDTH

	size_t i;

	for (i=0; i<line.size(); )
	{
		int wc=wcwidth(line[i]);

		if (wc <= 0)
			line[i]='?';
		else if (wc > 1)
		{
			if (i + wc < line.size()) // Truncated
			{
				while (i < line.size())
					line[i++]='?';
				continue;
			}

			line.erase(line.begin() + i + 1,
				   line.begin() + i + wc);
		}
		i++;
	}
#endif
}

void Curses::wordWrap(vector<wchar_t>::iterator b,
		      vector<wchar_t>::iterator e,
		      vector< pair <vector<wchar_t>::iterator,vector<wchar_t>::iterator
		      > > &lines,
		      size_t toWidth)
{
	bool needBlankLine=false;

	do
	{
		vector<wchar_t>::iterator start=b;
		// Remember where line started

		vector<wchar_t>::iterator lastspc=e;
		// Assume entire line will fit, unless told otherwise

		size_t cnt=0;

		for (;;)
		{
			if (b == e) // Reached end of line, break here
			{
				lastspc=b;
				break;
			}

			// Remember where most recent space was
			if (*b == ' ' || *b == '\t')
				lastspc=b;


			// Calculate how much real estate this char needs

			size_t w=1;

			if (*b == '\t')
				w= TABSIZE - (cnt % TABSIZE);
			else
#if HAVE_WCWIDTH
			{
				int wc=wcwidth(*b);

				if (wc > 1)
					w=1;
			}
#endif

			cnt += w;

			if (cnt >= toWidth)
				break;
			b++;
		}

		vector<wchar_t>::iterator resume;

		if (lastspc == e)
			resume=lastspc=b;
		// If we didn't see a space, break at end of line

		else
		{
			resume=lastspc;

			if (resume != e)
			{
				resume++;

				if (resume == e)
					needBlankLine=true;
			}
		}

		lines.push_back( make_pair(start, lastspc));
		b=resume;
	} while (b != e);

	if (needBlankLine)
		lines.push_back(make_pair(e, e));
	// The last character was a space and we broke on it.
	// We need to be able to rebuild the original text by reconcatenating
	// all the lines, with a single space separator character.  Do that
	// by appending a 0-length line.
}
