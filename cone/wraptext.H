/* $Id: wraptext.H,v 1.1 2003/05/27 14:09:04 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef wraptext_H
#define wraptext_H

///////////////////////////////////////////////////////////////////
//
// Helper class that does word wrapping

#include <string>
#include <vector>

#include "unicode/unicode.h"

#define WRAPTABSIZE 8

class WrapText {

	std::vector< std::vector<unicode_char> > lines;

	void init(std::vector<unicode_char> &text, size_t toWidth);

public:

	// Constructor receives text to wrap up

	WrapText(std::string text, size_t toWidth);
	WrapText(std::vector<unicode_char> &text, size_t toWidth);
	~WrapText();

	// Your choice: strings (in the current charset), or unichars.

	operator std::vector<std::string>() const;
	operator std::vector< std::vector<unicode_char> > const()
	{
		return lines;
	}

	size_t size() const { return lines.size(); } // # of lines

	operator std::string() const;
	// strings, newline at the end of each string.
};

#endif
