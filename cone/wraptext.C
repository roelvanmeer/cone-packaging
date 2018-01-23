/* $Id: wraptext.C,v 1.1 2003/05/27 14:09:04 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "wraptext.H"
#include "gettext.H"
#include <errno.h>
#include <curses/mycurses.H>
#include "libmail/mail.H"

using namespace std;

WrapText::WrapText(string text, size_t toWidth)
{
	const struct unicode_info *u=Gettext::defaultCharset();

	vector<unicode_char> uv;

	unicode_char *uc= (u->c2u)(u, text.c_str(), NULL);

	if (!uc)
		LIBMAIL_THROW(strerror(errno));

	try {
		size_t i;

		for (i=0; uc[i]; i++)
			;

		uv.reserve(i);
		uv.insert(uv.end(), uc, uc+i);

		free(uc);
	} catch (...) {
		free (uc);
		LIBMAIL_THROW();
	}

	init(uv, toWidth);
}

WrapText::WrapText(vector<unicode_char> &text, size_t toWidth)
{
	init(text, toWidth);
}

void WrapText::init(vector<unicode_char> &text, size_t toWidth)
{
	// DEPENDENCY: unicode_char=wchar_t

	vector<wchar_t> wbuf;

	wbuf.reserve(text.size());
	wbuf.insert(wbuf.end(), text.begin(), text.end());

	vector< pair< vector<wchar_t>::iterator,
		vector<wchar_t>::iterator> > lines;

	// The curses library does the heavy lifting.

	Curses::wordWrap(wbuf.begin(), wbuf.end(), lines, toWidth);

	vector< pair< vector<wchar_t>::iterator,
		vector<wchar_t>::iterator> >::iterator
		b=lines.begin(),
		e=lines.end();

	// Convert back to unicode_chars

	while (b != e)
	{
		vector<unicode_char> cpy;

		cpy.insert(cpy.end(), b->first, b->second);

		this->lines.push_back(cpy);

		b++;

	}
}

WrapText::~WrapText()
{
}

WrapText::operator vector<string>() const
{
	vector<string> linesChar;

	vector< vector<unicode_char> >::const_iterator
		b=lines.begin(), e=lines.end();

	vector<unicode_char> buf;

	const struct unicode_info *u=Gettext::defaultCharset();

	// Convert each line to local charset

	while (b != e)
	{
		buf.clear();
		buf.reserve(b->size()+1);
		buf.insert(buf.end(), b->begin(), b->end());
		buf.push_back(0);

		char *p= (*u->u2c)(u, &buf[0], NULL);

		if (!p)
			LIBMAIL_THROW(strerror(errno));

		try {
			linesChar.push_back(string(p));
			free(p);
		} catch (...) {
			free(p);
			LIBMAIL_THROW();
		}
		b++;
	}

	return linesChar;
}

WrapText::operator string() const
{
	vector<string> linesChar= *this; // Get the lines

	// Combine the lines.

	string l="";

	vector<string>::const_iterator b=linesChar.begin(),
		e=linesChar.end();

	while (b != e)
	{
		l += *b++;

		l += "\n";
	}
	return l;
}
