/* $Id: cursesfield.C,v 1.7 2010/04/29 00:34:49 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "curses_config.h"
#include "cursesfield.H"
#include "curseskeyhandler.H"
#include "cursesmoronize.H"
#include <algorithm>
#include <iostream>

using namespace std;

vector<wchar_t> CursesField::yesKeys, CursesField::noKeys;
wchar_t CursesField::yankKey='\x19';
wchar_t CursesField::clrEolKey='\x0B';

string CursesField::cutBuffer;

CursesField::CursesField(CursesContainer *parent,
			 size_t widthArg,
			 size_t maxlengthArg,
			 string initValue)
	: Curses(parent), width(widthArg), maxlength(maxlengthArg),
	  shiftoffset(0),
	  cursorpos(0), selectpos(0), passwordChar(0), yesnoField(false),
	  isUnderlined(true)
{
	if (yesKeys.size() == 0)
		mbtow("yY", yesKeys);

	if (noKeys.size() == 0)
		mbtow("nN", noKeys);

	mbtow(initValue.c_str(), text);
}

void CursesField::setAttribute(Curses::CursesAttr attr)
{
	attribute=attr;
	draw();
}

CursesField::~CursesField()
{
	if (optionHelp.size() > 0)
		CursesKeyHandler::handlerListModified=true;
}

void CursesField::setOptionHelp(const vector< pair<string, string> > &help)
{
	CursesKeyHandler::handlerListModified=true;
	optionHelp=help;
}

void CursesField::setRow(int row)
{
	erase();
	Curses::setRow(row);
	draw();
}

void CursesField::setCol(int col)
{
	erase();
	Curses::setCol(col);
	draw();
}

void CursesField::setText(string textArg)
{
	text.clear();
	mbtow(textArg.c_str(), text);
	shiftoffset=0;
	cursorpos=selectpos=text.size();
	draw();
}

string CursesField::getText() const
{
	vector<wchar_t> w=text;

	w.push_back(0);

	return wtomb(&*w.begin());
}

int CursesField::getWidth() const
{
	return width;
}

void CursesField::setWidth(int w)
{
	if (w > 0)
		width=w;
	draw();
}

int CursesField::getHeight() const
{
	return 1;
}

void CursesField::draw()
{
	vector<wchar_t> v;

	v.insert(v.end(), width, ' ');
	v.push_back(0);

	size_t i;

	for (i=0; i<width; i++)
		if (i + shiftoffset < text.size())
			v[i]=passwordChar ? passwordChar:text[i+shiftoffset];

	size_t a=selectpos;
	size_t b=cursorpos;

	if (a > b)
	{
		size_t c=a; a=b; b=c;
	}

	if (a < shiftoffset)
		a=shiftoffset;

	if (b < shiftoffset)
		b=shiftoffset;

	a -= shiftoffset;
	b -= shiftoffset;

	if (a > width)
		a=width;
	if (b > width)
		b=width;

	v.insert(v.begin() + b, (wchar_t)0);
	++b;

	v.insert(v.begin() + a, (wchar_t)0);

	++a;
	++b;

	bool underlined=isUnderlined;

	if (v[0])
	{
		Curses::CursesAttr aa=attribute;

		writeText(&v[0], 0, 0, aa.setUnderline(underlined));
	}

	if (v[a])
	{
		Curses::CursesAttr aa=attribute;

		writeText(&v[a], 0, a-1,
			  aa.setReverse().setUnderline(underlined));
	}

	if (v[b])
	{
		Curses::CursesAttr aa=attribute;

		writeText(&v[b], 0, b-2,
			  aa.setUnderline(underlined));
	}
}


void CursesField::erase()
{
	vector<wchar_t> v;

	v.insert(v.end(), width, ' ');
	v.push_back(0);

	writeText(&*v.begin(), 0, 0, CursesAttr());
}

bool CursesField::isFocusable()
{
	return true;
}

void CursesField::focusGained()
{
	cursorpos=text.size();
	selectpos=0;
	shiftoffset=0;
	if (cursorpos >= width)
		shiftoffset=cursorpos + 1 - width;
	draw();
}

void CursesField::focusLost()
{
	cursorpos=selectpos=shiftoffset=0;
	draw();
}


int CursesField::getCursorPosition(int &row, int &col)
{
	// Automatically scroll field to keep the cursor visible

	row=0;
	if (cursorpos < shiftoffset)
	{
		shiftoffset=cursorpos;
		if (cursorpos < width)
			shiftoffset=0;
		else shiftoffset -= width/2;

		draw();
	}

	if (cursorpos >= shiftoffset + width)
	{
		shiftoffset=cursorpos + 1 - width;
		draw();
	}

	col=cursorpos - shiftoffset;
	Curses::getCursorPosition(row, col);

	return cursorpos == selectpos;
}

bool CursesField::processKeyInFocus(const Key &key)
{
	if (key == key.HOME)
	{
		cursorpos=selectpos=0;
		draw();
		return true;
	}

	if (key == key.SHIFTHOME)
	{
		cursorpos=0;
		draw();
		return true;
	}

	if (key == key.END)
	{
		cursorpos=selectpos=text.size();
		draw();
		return true;
	}

	if (key == key.SHIFTEND)
	{
		cursorpos=text.size();
		draw();
		return true;
	}

	if (key == key.LEFT)
	{
		left();
		draw();
		return true;
	}

	if (key == key.SHIFTLEFT)
	{
		size_t savepos=selectpos;
		left();
		selectpos=savepos;
		draw();
		return true;
	}

	if (key == key.RIGHT)
	{
		right();
		draw();
		return true;
	}

	if (key == key.SHIFTRIGHT)
	{
		size_t savepos=selectpos;
		right();

		selectpos=savepos;
		draw();
		return true;
	}

	if (key == '\t' || key == key.DOWN || key == key.UP ||
	    key == key.SHIFTDOWN || key == key.SHIFTUP || key == key.ENTER ||
	    key == key.PGDN || key == key.SHIFTPGDN ||
	    key == key.PGUP || key == key.SHIFTPGUP)
	{
		if (cursorpos != selectpos)
		{
			selectpos=cursorpos;
			draw();
		}

#if 0
		if (key == key.PGDN || key == key.SHIFTPGDN)
		{
			Key k(Key::DOWN);

			return Curses::processKeyInFocus(k);
		}

		if (key == key.PGUP || key == key.SHIFTPGUP)
		{
			Key k(Key::UP);

			return Curses::processKeyInFocus(k);
		}
#endif
		return Curses::processKeyInFocus(key);
	}

	if (cursorpos != selectpos)
	{
		if (key != key.SHIFT)
		{
			if (cursorpos < selectpos)
			{
				if (key != yankKey)
				{
					vector<wchar_t> wcut;

					wcut.insert(wcut.end(),
						    text.begin() + cursorpos,
						    text.begin() + selectpos);

					wcut.push_back(0);

					cutBuffer=wtomb(&wcut[0]);
				}

				text.erase(text.begin() + cursorpos,
					   text.begin() + selectpos);
			}
			else
			{
				if (key != yankKey)
				{
					vector<wchar_t> wcut;

					wcut.insert(wcut.end(),
						    text.begin() + selectpos,
						    text.begin() + cursorpos);

					wcut.push_back(0);
					cutBuffer=wtomb(&wcut[0]);
				}

				text.erase(text.begin() + selectpos,
					   text.begin() + cursorpos);
				cursorpos=selectpos;
			}
		}

		selectpos=cursorpos;
		draw();

		if (key == key.BACKSPACE || key == key.DEL)
			return true;
	}

	if (key == key.BACKSPACE && cursorpos > 0)
	{
		left();

		vector<wchar_t>::iterator b=text.begin() + cursorpos;
		vector<wchar_t>::iterator e=b;

#if HAVE_WCWIDTH
		int w=wcwidth( *b );

		if (w <= 0)
			w=1;
#else
		int w=1;
#endif

		if (text.end() - e >= w)
			e += w;

		text.erase(b, e);
		draw();
		return true;
	}

	if (key == key.DEL && cursorpos < text.size())
	{
		vector<wchar_t>::iterator b=text.begin() + cursorpos;
		vector<wchar_t>::iterator e=b;

#if HAVE_WCWIDTH
		int w=wcwidth( *b );

		if (w <= 0)
			w=1;
#else
		int w=1;
#endif

		if (text.end() - e >= w)
			e += w;

		text.erase(b, e);
		draw();
		return true;
	}

	if (!key.plain())
	{
		return Curses::processKeyInFocus(key);
	}

	wchar_t k=key.key;

	if (k == yankKey && !yesnoField &&
	    optionField.size() == 0 && cutBuffer.size() > 0)
		; // Will paste later
	else if (k < ' ')
	{
		return Curses::processKeyInFocus(key);
	}

#if HAVE_WCWIDTH

	int k_w=wcwidth(k);

	if (k == yankKey)
		k_w=1;
	else if (k_w <= 0)
	{
		k_w=1;
		k=' ';
	}
#else
	int k_w=1;
#endif

	if (text.size() + k_w > maxlength)
		return true;

	bool doYesNoField=true;

	if (optionField.size() > 0)
	{
		vector<wchar_t>::iterator p=
			find(optionField.begin(),
			     optionField.end(), k);


		if (p == optionField.end())
		{
			if (!yesnoField)
				return true;
		}
		else doYesNoField=false;

	}

	if (doYesNoField && yesnoField)
	{
		vector<wchar_t>::iterator b=yesKeys.begin(), e=yesKeys.end();

		while (b != e)
		{
			if (*b == k)
				break;

			b++;
		}

		if (b != e)
			k='Y';
		else
		{
			b=noKeys.begin(), e=noKeys.end();

			while (b != e)
			{
				if (*b == k)
					break;

				b++;
			}

			if (b == e)
				return true;

			k='N';
		}
		text.clear();
		cursorpos=0;
		selectpos=0;
		shiftoffset=0;
	}

	if (k == yankKey)
	{
		string s="";

		size_t p=cutBuffer.find('\n');

		if (p == std::string::npos)
		{
			s=cutBuffer;
		}
		else
		{
			s=cutBuffer.substr(0, p);
			cutBuffer=cutBuffer.substr(p+1);
		}

		vector<wchar_t> w;

		mbtow(s.c_str(), w);

		if (text.size() + w.size() > maxlength)
		{
			beepError();
			w.clear();
		}

		text.insert(text.begin() + cursorpos,
			    w.begin(), w.end());
		cursorpos += w.size();
	}
	else
	{
		text.insert(text.begin() + cursorpos, k);
		++cursorpos;

		while (--k_w)
		{
			text.insert(text.begin() + cursorpos, MCPAD);
			++cursorpos;
		}
	}

	selectpos=cursorpos;
	draw();

	if (yesnoField || optionField.size() > 0)
		this->processKeyInFocus(Key(Key::ENTER)); // Automatic enter
	else if (CursesMoronize::enabled && !passwordChar)
	{
		size_t n=cursorpos > 4 ? 4:cursorpos;
		size_t i;
		char m_buf[5];

		for (i=0; i<n; i++)
		{
			wchar_t c=text[cursorpos - i - 1];

			if ( (unsigned char)c != c)
				break;

			m_buf[i]=c;
		}

		if (i > 0)
		{
			wchar_t repl_c;

			m_buf[i]=0;

			i=CursesMoronize::moronize(m_buf, repl_c);

			if (i > 0)
			{
				while (i)
				{
					CursesField::
						processKeyInFocus(Key(Key::
								      BACKSPACE
								      ));
					--i;
				}
				CursesField::processKeyInFocus(Key(repl_c));
			}
		}
	}

	return true;
}

void CursesField::left()
{
	do_left();

#if HAVE_WCWIDTH

	size_t k_w=0;

	while (cursorpos > k_w && text[cursorpos-k_w] == MCPAD)
		++k_w;

	int ww=cursorpos - k_w == 0 ? 0:wcwidth(text[cursorpos-k_w]);

	if (ww > 1 && (size_t)(ww - 1) == k_w)
		while ( --ww )
			do_left();
#endif

}

void CursesField::right()
{
	do_right();

#if HAVE_WCWIDTH

	if (cursorpos > 0)
	{
		int k_w=wcwidth(text[cursorpos-1]);

		if (k_w > 1)
			while (--k_w)
				do_right();
	}

	if (cursorpos < text.size())
	{
		int k_w=wcwidth(text[cursorpos]);

		if (k_w > 1)
		{
			--k_w;
			cursorpos += k_w;

			int dummy1, dummy2;

			getCursorPosition(dummy1, dummy2);
			cursorpos -= k_w;
		}
	}
#endif
}

void CursesField::do_left()
{
	if (cursorpos != selectpos)
	{
		if (cursorpos > selectpos)
			cursorpos=selectpos;
	}
	else if (cursorpos > 0)
		--cursorpos;
	selectpos=cursorpos;
}

void CursesField::do_right()
{
	if (cursorpos != selectpos)
	{
		if (cursorpos < selectpos)
			cursorpos=selectpos;
	}
	else if (cursorpos < text.size())
		++cursorpos;
	selectpos=cursorpos;
}
