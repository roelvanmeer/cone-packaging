/* $Id: cursesmessagehtmlparser.C,v 1.1 2003/05/27 14:09:03 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "cursesmessagehtmlparser.H"
#include "gettext.H"

using namespace std;

CursesMessage::HtmlParser
::HtmlParser( CursesMessage *messagePtrArg,
	      const struct unicode_info *content_chsetArg,
	      const struct unicode_info *my_chsetArg,
	      unicodeEntityAltList *txtAltListArg,
	      size_t displayWidthArg)
	: ::htmlParser( (content_chsetArg ? *content_chsetArg
			 : *my_chsetArg), *my_chsetArg,
			txtAltListArg, displayWidthArg),
	messagePtr(messagePtrArg),
	streamPtr(NULL),
	reply(false)
{
}

CursesMessage::HtmlParser
::HtmlParser( ostream &streamPtrArg,
	      bool replyArg,
	      const struct unicode_info *content_chsetArg,
	      const struct unicode_info *my_chsetArg,
	      unicodeEntityAltList *txtAltListArg,
	      size_t displayWidthArg)
	: ::htmlParser( (content_chsetArg ? *content_chsetArg
			 : *my_chsetArg), *my_chsetArg,
			txtAltListArg, displayWidthArg),
	messagePtr(NULL),
	streamPtr(&streamPtrArg),
	reply(replyArg)
{
	init();
}

void CursesMessage::HtmlParser::init()
{
	parse(_("<b>&laquo; HTML content follows &raquo;</b><br>"));
}

CursesMessage::HtmlParser::~HtmlParser()
{
}

void CursesMessage::HtmlParser::parse(string text)
{
	::htmlParser::parse(text);
}

bool CursesMessage::HtmlParser::finish()
{
	flush(); // remaining flotsam from htmlParser
	return true;
}

void CursesMessage::HtmlParser::parsedLine(string line, bool wrapped)
{
	if (reply)
	{
		if (line.size() == 0 || line[0] != '>')
			line.insert(line.begin(), ' ');
		line.insert(line.begin(), '>');
	}

	if (messagePtr)
		messagePtr->reformatAddLine(line,
					    CursesMessage::LineIndex::ATTRIBUTES);

	if (streamPtr) // When replying, drop all attributes
	{
		vector<pair<textAttributes, string> > parsedLine;

		textAttributes::getAttributedText(line, parsedLine);

		vector<pair<textAttributes, string> >::iterator b, e;

		b=parsedLine.begin();
		e=parsedLine.end();

		while (b != e)
		{
			(*streamPtr) << b->second;
			b++;
		}

		(*streamPtr) << (wrapped ? " ":"") << endl;
	}
}
