/* $Id: cursesmessageflowedtext.H,v 1.1 2003/05/27 14:09:03 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef cursesmessageflowedtext_H
#define cursesmessageflowedtext_H

#include "cursesmessage.H"
#include "rfc2646parse.H"

//////////////////////////////////////////////////////////////////////
// Helper classes for flowed text reformatting
//

class CursesMessage::FlowedTextParser : public rfc2646Parser,
		     public CursesMessage::reformatter {

	CursesMessage *messagePtr;
	const struct unicode_info *content_chset;
	const struct unicode_info *my_chset;
	std::string flowedTextBuffer;
	size_t displayWidth;
        bool rewrapFlowed(std::string text, bool partialFlag);

	void parse(std::string text);
	bool finish();

public:
	FlowedTextParser( CursesMessage *messagePtrArg,
			 const struct unicode_info *content_chsetArg,
			 const struct unicode_info *my_chsetArg,
			 size_t displayWidthArg);
	~FlowedTextParser();

	bool operator()(std::string text);
};

#endif
