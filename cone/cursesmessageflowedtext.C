/* $Id: cursesmessageflowedtext.C,v 1.2 2005/06/30 02:55:49 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "cursesmessageflowedtext.H"
#include "wraptext.H"

using namespace std;

CursesMessage::FlowedTextParser
::FlowedTextParser(CursesMessage *messagePtrArg,
		   const struct unicode_info *content_chsetArg,
		   const struct unicode_info *my_chsetArg,
		   size_t displayWidthArg)
	: messagePtr(messagePtrArg),
	  content_chset(content_chsetArg),
	  my_chset(my_chsetArg),
	  flowedTextBuffer(""),
	  displayWidth(displayWidthArg)
{
}

CursesMessage::FlowedTextParser::~FlowedTextParser()
{
}

bool CursesMessage::FlowedTextParser::operator()(string text)
{
	flowedTextBuffer += text;

	size_t n;

	while ((n=flowedTextBuffer.find('\n')) != flowedTextBuffer.npos)
	{
		text=flowedTextBuffer.substr(0, n);
		flowedTextBuffer=flowedTextBuffer.substr(n+1);

		if (!rewrapFlowed(text, false))
			return false;
	}

	// Don't accumulate a huge paragraph buffer.
	if (flowedTextBuffer.size() > displayWidth * 100)
		if (!rewrapFlowed(flowedTextBuffer, true))
			return false;

	return true;
}

bool CursesMessage::FlowedTextParser
::rewrapFlowed(string text, bool partialFlag)
{
	size_t pfix;
	string pfixStr="";

	// Count flowed format quoting depth.

	for (pfix=0; pfix < text.size(); pfix++)
	{
		if (text[pfix] != '>')
			break;
	}

	if (pfix > 0)
		pfixStr=text.substr(0, pfix) + ' ';

	if (pfix < text.size() && text[pfix] == ' ')
		pfix++;

	// Compute desired text width by subtracting quoting depth from
	// display width.

	size_t w=pfixStr.size();

	if (w + 20 < displayWidth)
		w=displayWidth-w;
	else
		w=20;

	string l=text.substr(pfix);

	// Convert message charset to display charset.

	if (!messagePtr->toMyCharset(content_chset, my_chset, l))
		return false;

	vector<string> lines=WrapText(l, w);

	// Prepend the oriqinal quoting prefix to each wrapped line.

	vector<string>::iterator b=lines.begin(), e=lines.end();

	while (b != e)
	{
		*b = pfixStr + *b;
		b++;
	}

	if (partialFlag) // Put back last line into the buffer.

	{
		flowedTextBuffer="";
		if (lines.size() > 0)
		{
			flowedTextBuffer=lines[lines.size()-1];
			lines.erase(lines.begin() + (lines.size()-1),
				    lines.end());
		}

		if (pfix == 0)
			flowedTextBuffer=" " + flowedTextBuffer;

		if (!messagePtr->toMyCharset(my_chset, content_chset,
					     flowedTextBuffer))
			return false; // flowedTextBuffer uses content chset.
	}

	messagePtr->reformatAddLines(lines);
	return true;
}

void CursesMessage::FlowedTextParser::parse(std::string text)
{
	rfc2646Parser::parse(text);
}

bool CursesMessage::FlowedTextParser::finish()
{
	return rfc2646Parser::finish();
}
