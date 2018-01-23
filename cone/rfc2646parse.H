/* $Id: rfc2646parse.H,v 1.1 2003/05/27 14:09:04 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef rfc2646parser_h
#define rfc2646praser_h

#include "config.h"

//
// This class is a wrapper for rfc2045/rfc2646.h.
//
// The operator<<(string) method parses the string.
//
// The operator(string) method should be subclassed, it will be called
// repeatedly, to receive a contiguous stream of reformatted text.
//
// The text is reformatted so that the \n characters mark end of paragraphs
// only.  Each paragraph will begin with '>' marking the quote depth,
// followed by a single space.  Note that paragraphs with 0 quote depth
// will also have a single leading space character
//
// Note that a single invocation of operator(string) may not necessarily
// supply the entire paragraph.

#include <string>

class rfc2646Parser {

	struct rfc2646parser *parser; // See rfc2045/rfc2646.h

	int quote_depth;

	static int parser_cb_s(struct rfc2646parser *parser, int flowed,
			     void *voidArg);

	void parser_cb(struct rfc2646parser *p, int flowed);

	bool errFlag;

 public:
	rfc2646Parser();
	virtual ~rfc2646Parser();

	void parse(std::string text);

	bool finish(); // Call betfore destruction to flush things out

	virtual bool operator()(std::string)=0;
};

#endif
