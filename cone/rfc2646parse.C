/* $Id: rfc2646parse.C,v 1.1 2003/05/27 14:09:04 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "rfc2646parse.H"
#include "gettext.H"
#include "rfc2045/rfc2646.h"

rfc2646Parser::rfc2646Parser()
	: parser(NULL), quote_depth(-1), errFlag(false)
{

	parser=rfc2646_alloc( &rfc2646Parser::parser_cb_s, this);

	if (!parser)
		outofmemory();
}

rfc2646Parser::~rfc2646Parser()
{
	struct rfc2646parser *p;

	if ((p=parser) != NULL)
	{
		parser=NULL;
		rfc2646_free(p);
	}
}

bool rfc2646Parser::finish()
{
	struct rfc2646parser *p;
	
	if ((p=parser) != NULL)
	{
		parser=NULL;
		rfc2646_free(p);
	}

	if (quote_depth >= 0 && !errFlag)
		if (!(*this)("\n"))
			errFlag=true;

	return !errFlag;
}

int rfc2646Parser::parser_cb_s(struct rfc2646parser *parser, int flowed,
			       void *voidArg)
{
	((rfc2646Parser *)voidArg)->parser_cb(parser, flowed);
	return 0;
}

void rfc2646Parser::parser_cb(struct rfc2646parser *parser, int flowed)
{
	if (parser == NULL)
		return; // Destructing

	if (errFlag)
		return;

	std::string s;

	s.insert(s.end(), parser->line, parser->line + parser->linelen);

	int i;

	if (quote_depth >= 0 && quote_depth != parser->quote_depth)
	{
		// Insert a linebreak if quote_depth suddenly changes in
		// a middle of flowed text.

		if (!(*this)("\n"))
			errFlag=true;
		quote_depth= -1;
	}
	else if (!flowed && quote_depth >= 0 && parser->linelen == 0)
	{
		//    ...text<SP><EOL>
		//    <EOL>
		//
		// ... Insert an extra blank line between paragraphs

		s.push_back('\n');

		for (i=0; i<parser->quote_depth; i++)
			s.push_back('>');
		s.push_back(' ');
		s.push_back('\n');
		quote_depth= -1;

		if (!(*this)(s))
			errFlag=true;
		return;
	}

	if (errFlag)
		return;

	if (quote_depth >= 0)
	{
		if (!(*this)(" ")) // Flowed from previous line.
		{
			errFlag=true;
			return;
		}
	}
	else {
		// This is start of new line.

		s.insert(s.begin(), ' ');
		for (i=0; i<parser->quote_depth; i++)
			s.insert(s.begin(), '>');
	}

	quote_depth=parser->quote_depth;

	if (!flowed)
	{
		quote_depth=-1;
		s.push_back('\n');
	}

	if (s.size() > 0)
		if (!(*this)(s))
			errFlag=true;
}

void rfc2646Parser::parse(std::string text)
{
	if (parser && rfc2646_parse(parser, &*text.begin(), text.size()))
	{
		struct rfc2646parser *p=parser;

		parser=NULL;
		rfc2646_free(p);
	}
}

