/* $Id: htmlparser.C,v 1.10 2010/04/29 00:34:49 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "htmlparser.H"
#include "htmlentity.h"
#include <string.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <cctype>

#include "libmail/mail.H"

using namespace std;

/////////////////////////////////////////////////////////////////////////////

textAttributes::textAttributes()
	: bold(0), underline(0), reverse(0), italic(0), bgcolor(0), fgcolor(0)
{
}

textAttributes::textAttributes(string s)
	: bold(0), underline(0), reverse(0), italic(0), bgcolor(0), fgcolor(0)
{
	string::iterator b=s.begin(), e=s.end();
	unsigned *c= &bgcolor;

	while (b != e)
	{
		static const char d[]="0123456789";

		const char *p=strchr(d, *b);

		if (p)
			*c=(*c) * 10 + p-d;
		else switch (*b) {
		case 'B':
			bold=1;
			break;
		case 'I':
			italic=1;
			break;
		case 'U':
			underline=1;
			break;
		case 'R':
			reverse=1;
			break;
		case '.':
			c= &fgcolor;
			break;
		case 'L':

			// \x7Fs are encoded as \x80\x80, and \x80 is encoded
			// as \x80\x81

			{
				ostringstream o;

				while (++b != e)
				{
					if ((char)*b != (char)'\x80')
					{
						o << *b;
						continue;
					}

					if (++b == e)
						break;
					o << (char) ((unsigned char)*b-1);
				}

				url=o.str();
			}
			return;
		}
		b++;
	}
}

textAttributes::~textAttributes()
{
}

bool textAttributes::operator==(const textAttributes &a) const
{
	return bold == a.bold &&
		underline == a.underline &&
		reverse == a.reverse &&
		italic == a.italic && bgcolor == a.bgcolor &&
		fgcolor == a.fgcolor &&
		url == a.url;
}

bool textAttributes::operator!=(const textAttributes &a) const
{
	return ! operator==(a);
}

textAttributes::operator string() const
{
	ostringstream o;
	char normal='N';

	if (bold)
	{
		o << "B";
		normal=0;
	}

	if (underline)
	{
		o << "U";
		normal=0;
	}

	if (reverse)
	{
		o << "R";
		normal=0;
	}

	if (italic)
	{
		o << "I";
		normal=0;
	}

	if (normal)
		o << (char)normal;

	if (bgcolor)
		o << bgcolor;
	if (fgcolor)
		o << '.' << fgcolor;

	if (url.size() > 0)
	{
		string::const_iterator b=url.begin(), e=url.end();

		o << 'L';

		// \x7Fs are encoded as \x80\x80, and \x80 is encoded as
		// \x80\x81

		while (b != e)
		{
			if (*b == '\x7F' || *b == '\x80')
				o << '\x80' << (char)((unsigned char)*b+1);
			else
				o << *b;
			++b;
		}
	}

	return  "\x7F" + o.str() + "\x7F";
}

htmlParser::enhanced_char::enhanced_char(unicode_char chArg,
					 textAttributes attrArg)
	: ch(chArg), attr(attrArg)
{
}

htmlParser::enhanced_char::~enhanced_char()
{
}

//
// Take a string with embedded attribute codes and return an array of
// attribute/text pairs.
//

void textAttributes::getAttributedText(std::string s,
				       std::vector< std::pair<textAttributes,
				       std::string> > &vec)
{
	textAttributes currentAttr;
	string currentString;

	string::iterator b=s.begin(), e=s.end();

	while (b != e)
	{
		if (*b != '\x7F')
		{
			string::iterator c=b;

			while (c != e && *c != '\x7F')
				c++;

			currentString += string(b, c);
			b=c;
			continue;
		}

		++b;
		string::iterator c=b;
		while (c != e && *c != '\x7F')
			c++;

		textAttributes nextAttr(string(b, c));

		if (nextAttr != currentAttr && currentString.size() > 0)
		{
			vec.push_back(make_pair(currentAttr, currentString));
			currentString="";
		}
		currentAttr=nextAttr;

		if (c != e)
			c++;
		b=c;
	}

	if (currentString.size() > 0)
		vec.push_back(make_pair(currentAttr, currentString));
}

/////////////////////////////////////////////////////////////////////////////

htmlParser::tag::tag() : name(0), preformatted(0)
{
}

htmlParser::tag::~tag()
{
}

void htmlParser::tag::getAttribute(string name,
				   vector<unicode_char> &w)
{
	list< pair<string, vector<unicode_char> > >::iterator
		ab=attributes.begin(),
		ae=attributes.end();

	while (ab != ae)
	{
		if (ab->first == name)
		{
			w=ab->second;
			return;
		}

		++ab;
	}

	w.clear();
}

/////////////////////////////////////////////////////////////////////////////


htmlParser::htmlParser(const struct unicode_info &htmlCharsetArg,
		       const struct unicode_info &txtCharsetArg,
		       unicodeEntityAltList *txtAltListArg,
		       size_t desiredLineLengthArg)
	: wordBreak(0), desiredLineLength(desiredLineLengthArg), inTag(0),
	  nblanklines(0), paragraphBreak(true),

	  htmlCharset(htmlCharsetArg),
	  txtCharset(txtCharsetArg),
	  txtAltList(txtAltListArg)
{
}

htmlParser::~htmlParser()
{
}

// Get indentation prefix for a BLOCKQUOTE

static const char *
get_blockquote_pfix(list< pair<string, vector<unicode_char> > > &attr)
{
	list< pair<string, vector<unicode_char> > >::iterator
		p=attr.begin(), e=attr.end();

	while (p != e)
	{
		if (strcasecmp(p->first.c_str(), "TYPE") == 0)
			break;
		p++;
	}

	if (p != e)
	{
		string s;

		s.insert(s.end(), p->second.begin(), p->second.end());

		if (strcasecmp(s.c_str(), "CITE") == 0)
			return ">";
	}

	return "  ";
}

#define IS_INDENTED(b) \
	( strcmp((b)->name->name, BLOCKQUOTE.name) == 0 || \
	  strcmp((b)->name->name, TABLE.name) == 0)

//
// Take desired line length, subtract current indentation, and arrive at
// current target line length.

size_t htmlParser::getLineLength()
{
	size_t n=0;

	list<tag>::iterator b=tagStack.begin(), e=tagStack.end();

	while (b != e)
	{
		if ( IS_INDENTED(b))
		{
			n += strlen(get_blockquote_pfix(b->attributes));
		}
		b++;
	}


	if (n)
		++n;

	return n + 20 < desiredLineLength ? desiredLineLength - n:20;
}

/* TAG OPTIONS */

#define OPT_FLUSH_C 'F'
#define OPT_FLUSH   "F"   /* Single line break */

#define OPT_STACK_C 'S'
#define OPT_STACK   "S"   /* This tag can stack up */

#define OPT_CRUFT_C 'C'
#define OPT_CRUFT   "C"   /* Text inside these tags should be eaten */

#define OPT_NOBREAK_C 'N'
#define OPT_NOBREAK "N"   /* No break at all */

//
// Take whatever's accumulated in currentLine, and spit it out.
//
// Apply the current alignment.

void htmlParser::fmtline(bool wrapped)
{
	if (currentLine.size() == 0)
		++nblanklines;  // # of consecutive blank lines.
	else
		nblanklines=0;

	vector<unicode_char> s;

	s.reserve(currentLine.size());

	list<tag>::iterator b=tagStack.begin(), e=tagStack.end();

	vector<unicode_char> align;

	if (b != e)
	{
		list<tag>::iterator epp=e;

		--epp;

		epp->getAttribute("ALIGN", align);

	}

	// Throw out <STYLE>, or <SCRIPT>

	while (b != e)
	{
		if ( strchr((*b).name->options, OPT_CRUFT_C))
		{
			currentLine.clear();
			return;
		}
		b++;
	}

	b=tagStack.begin();
	e=tagStack.end();

	tou(align.begin(), align.end());

	if (align.size() == 6 &&
	    align[0] == 'C' &&
	    align[1] == 'E' &&
	    align[2] == 'N' &&
	    align[3] == 'T' &&
	    align[4] == 'E' &&
	    align[5] == 'R')

	{
		size_t n=getLineLength();

		if (n > currentLine.size())
			currentLine.insert(currentLine.begin(),
					   (n - currentLine.size())/2,
					   enhanced_char(' ',
							 textAttributes()));
	}

	// Prepend indentation prefix.

	while (b != e)
	{
		if ( IS_INDENTED(b))
		{
			const char *p=get_blockquote_pfix(b->attributes);

			s.insert(s.end(), p, p+strlen(p));
		}
		b++;
	}

	if (s.size() > 0)
		s.push_back(' ');

	// Scan currentLine.  Each time attributes change, insert an
	// attribute escape sequence into the text.

	std::vector<enhanced_char>::iterator cb, ce;

	cb=currentLine.begin();
	ce=currentLine.end();

	while (cb != ce)
	{
		if (cb == currentLine.begin() ||
		    cb->attr != cb[-1].attr)
		{
			string a=(string)cb->attr;
			s.insert(s.end(), a.begin(), a.end());
		}

		if (cb->ch != '\x7F')
			s.push_back(cb->ch);
		cb++;
	}

	currentLine.clear();
	wordBreak=0;
	paragraphBreak=false;

	s.push_back(0);

	char *p=unicode_fromCharset(&txtCharset, &s[0], txtAltList);

	if (!p)
		LIBMAIL_THROW((strerror(errno)));

	try {
		parsedLine(string(p), wrapped);
		free(p);
	} catch (...) {
		free(p);
		LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
	}
}

const struct htmlParser::taginfo htmlParser::BLOCKQUOTE
={"BLOCKQUOTE", 0, OPT_FLUSH OPT_STACK};
const struct htmlParser::taginfo htmlParser::H1={"H1", 1, ""};
const struct htmlParser::taginfo htmlParser::H2={"H2", 1, ""};
const struct htmlParser::taginfo htmlParser::H3={"H3", 1, ""};
const struct htmlParser::taginfo htmlParser::H4={"H4", 1, ""};
const struct htmlParser::taginfo htmlParser::H5={"H5", 1, ""};
const struct htmlParser::taginfo htmlParser::H6={"H6", 1, ""};
const struct htmlParser::taginfo htmlParser::H7={"H7", 1, ""};
const struct htmlParser::taginfo htmlParser::H8={"H8", 1, ""};
const struct htmlParser::taginfo htmlParser::H9={"H9", 1, ""};
const struct htmlParser::taginfo htmlParser::P={"P", 2, OPT_FLUSH};
const struct htmlParser::taginfo htmlParser::PRE={"PRE", 2, OPT_FLUSH};
const struct htmlParser::taginfo htmlParser::STYLE={"STYLE", 3, OPT_CRUFT OPT_FLUSH};
const struct htmlParser::taginfo htmlParser::SCRIPT={"SCRIPT", 4, OPT_CRUFT OPT_FLUSH};
const struct htmlParser::taginfo htmlParser::TABLE={"TABLE", 5, ""};
const struct htmlParser::taginfo htmlParser::TR={"TR", 6, ""};
const struct htmlParser::taginfo htmlParser::A={"A", 99, OPT_STACK OPT_NOBREAK};
const struct htmlParser::taginfo htmlParser::B={"B", 99, OPT_STACK OPT_NOBREAK};
const struct htmlParser::taginfo htmlParser::U={"U", 99, OPT_STACK OPT_NOBREAK};
const struct htmlParser::taginfo htmlParser::I={"I", 99, OPT_STACK OPT_NOBREAK};

const struct htmlParser::taginfo * const htmlParser::knownTags[]={
	&BLOCKQUOTE, &H1, &H2, &H3, &H4, &H5, &H6, &H7, &H8,
	&H9, &P, &PRE, &STYLE, &SCRIPT, &TABLE, &TR, &A, &B, &U, &I, NULL};

//
// Process incoming HTML.  Incoming HTML is converted to unicode as the first
// order of business.  As such, we have to wait until we've read a newline
// character before doing the unicode conversion.  Since this is E-mail,
// E-mail should not have excessive line length, so we're comfortable with
// chopping off excessively long buffers.

void htmlParser::parse(string s)
{
	size_t n;

	charBuffer += s;

	while ((n=charBuffer.find('\n')) != std::string::npos)
	{
		string line=charBuffer.substr(0, n+1);

		charBuffer=charBuffer.substr(n+1);

		vector<unicode_char> w;

		unicode_char *u= (htmlCharset.c2u)(&htmlCharset,
						   line.c_str(), NULL);

		if (!u)
			LIBMAIL_THROW(strerror(errno));

		try {
			size_t i;

			for (i=0; u[i]; i++)
				;

			if (i > 0)

				w.insert(w.end(), u, u+i);

			free(u);
		} catch (...) {
			free(u);
			LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
		}

		parseline(w);
	}

	if (charBuffer.size() > 8000)
		charBuffer.erase(charBuffer.begin() + 8000, charBuffer.end());
}

// Process HTML converted to unicode chars.

void htmlParser::parseline(vector<unicode_char> &s)
{
	vector<unicode_char>::iterator b=s.begin(), e=s.end();

	size_t targetLength=getLineLength();
	list<tag>::iterator sb=tagStack.begin(), se=tagStack.end();

	while (b != e)
	{
		if (inTag == '<') // Processing a tag.

		{
			if (currentTag.size() >= 3
			    && currentTag[0] == '!'
			    && currentTag[1] == '-' && currentTag[2] == '-')
			{
				// SGML comment

				if (currentTag.size() >= 5 &&
				    currentTag[currentTag.size()-2] == '-' &&
				    currentTag[currentTag.size()-1] == '-' &&
				    *b == '>')
				{
					newTag(); // Seen a tag.

					// Some stuff we cached might've
					// changed, refetch it:
					targetLength=getLineLength();
					sb=tagStack.begin();
					se=tagStack.end();
				}

				if (currentTag.size() >= 8000)
					currentTag.erase(currentTag.begin()+3,
							 currentTag.begin()
							 +1000);

				currentTag.push_back(*b);
			}
			else if (*b == '>')
			{
				newTag();

				// Some stuff we cached might've
				// changed, refetch it:

				targetLength=getLineLength();
				sb=tagStack.begin();
				se=tagStack.end();
			}
			else if (currentTag.size() < 8000)
				currentTag.push_back(*b);
			b++;
			continue;
		}

		if (inTag == '&') // Fetching an entity.
		{
			currentTag.push_back(*b);

			if (currentTag.size() > 16 || *b == ';')
			{
				inTag=0;

				string sch;

				sch.insert(sch.end(),
					   currentTag.begin(),
					   currentTag.end());

				if ( (*b=getHtmlEntity(sch.substr(0,
								  sch.size
								  ()-1)
						       )) == 0)
				{
					// Unknown entity, show verbatim code.

					sch="&#38;"; // The amp

					currentTag.insert(currentTag.begin(),
							  sch.begin(),
							  sch.end());

					parseline(currentTag);
					b++;
					sb=tagStack.begin();
					se=tagStack.end();
					continue;
				}
			}
			else
			{
				b++;
				continue;
			}
		}
		else if (*b == '<' || *b == '&')
		{
			currentTag.clear();
			inTag= *b;
			b++;
			continue;
		}

		// If we've just seen an <LI>, punt it:

		if (leadin_prefix.size() > 0)
		{
			vector<unicode_char> pfix_copy=leadin_prefix;

			leadin_prefix.clear();
			parseline(pfix_copy);
		}

		if (currentLine.size() >= targetLength)
		{
			vector<enhanced_char> wrapped;

			if (wordBreak > 0)
			{
				wrapped.insert(wrapped.begin(),
					       currentLine.begin()
					       + wordBreak + 1,
					       currentLine.end());

				currentLine.erase(currentLine.begin()
						  + wordBreak,
						  currentLine.end());
			}


			fmtline(true);
			currentLine.insert(currentLine.end(),
					   wrapped.begin(),
					   wrapped.end());
			sb=tagStack.begin();
			se=tagStack.end();
		}

		if (isb(*b)) // Is this a blank?
		{
			list<tag>::iterator p;

			string dummy;
			vector<unicode_char> dummyw;

			if (sb != se && (--(p=se))->preformatted)
			{

				// In preformatted section:

				if (*b == '\n')
				{
					dummy="<BR>";

					dummyw.insert(dummyw.end(),
						      dummy.begin(),
						      dummy.end());

					parseline(dummyw);
					b++;
					continue;
				}

				// Other spaces are converted to &nbsp;'s

				int n=1;

				if (*b == '\t')
					n= 8 - (currentLine.size() % 8);

				dummy="&nbsp;";
				dummyw.clear();

				while (n--)
					dummyw.insert(dummyw.end(),
						      dummy.begin(),
						      dummy.end());

				parseline(dummyw);
				b++;
				continue;
			}

			// Collapse multiple spaces into one.

			if (currentLine.size() == 0 ||
			    currentLine[currentLine.size()-1].ch == ' ')
			{
				b++;
				continue;
			}

			// Get active attribute.

			list<tag>::iterator t=se;
			textAttributes a= sb == se ? textAttributes()
				: (--t)->attr;

			wordBreak=currentLine.size();
			currentLine.push_back(enhanced_char(' ', a));
		}
		else // Not a blank char.
		{
			list<tag>::iterator t=se;
			textAttributes a= sb == se ? textAttributes()
				: (--t)->attr;

			currentLine.push_back(enhanced_char(*b, a));
		}
		b++;
	}
}

unicode_char htmlParser::getHtmlEntity(string entityName)
{
	const char *cp=entityName.c_str();

	if (*cp++ == '#') // Decimal/Hex unicode #
	{
		unicode_char c=0;

		if (*cp == 'x' || *cp == 'X')
		{
			istringstream i(entityName.substr(2));

			unsigned long uln=0;

			i >> hex >> uln;

			c=(unicode_char)uln;
		}
		else
		{
			c=atol(cp);
		}

		if (!c)
			c=' ';

		return c;
	}


	const char *p=entityName.c_str();
	size_t i;

	for (i=0; unicodeEntityList[i].name; i++)
		if (strcmp(unicodeEntityList[i].name, p) == 0)
			return unicodeEntityList[i].iso10646;

	return 0;
}

void htmlParser::urlDecode(string url, vector<unicode_char> &uc)
{
	uc.reserve(uc.size()+url.size());

	string::iterator b=url.begin(), e=url.end();

	while (b != e)
	{
		if (*b == '+')
		{
			uc.push_back(' ');
			++b;
			continue;
		}

		if (*b == '%')
		{
			if (++b != e && *b == 'u')
			{
				++b;
				if (e-b >= 4)
				{
					istringstream i(string(b, b+4));

					unsigned long u=0;

					i >> hex >> u;

					if (u)
						uc.push_back(u);
					b += 4;
				}
				continue;
			}

			if (e-b >= 2)
			{
				istringstream i(string(b, b+2));

				unsigned long u=0;

				i >> hex >> u;

				if (u)
					uc.push_back(u);
				b += 2;
			}
			continue;
		}

		if (*b != '&')
		{
			uc.push_back((unsigned char)*b);
			++b;
			continue;
		}

		string::iterator p=++b;

		while (p != e && *p != ';')
		{
			if (!isalnum(*p) && *p != '#')
				break;
			if (p-b > 16)
				break;
			++p;
		}

		unicode_char u;

		if (p != e && *p == ';' && (u=getHtmlEntity(string(b, p)))!=0)
		{
			uc.push_back(u);
			b= ++p;
			continue;
		}

		uc.push_back('&');
	}
}

//
// Parse CGI args:  name=value&name2=value2...
//

void htmlParser::cgiDecode(const std::vector<unicode_char> &uc,
			   std::map<std::string,
			   std::vector<unicode_char> > &args)
{
	std::vector<unicode_char>::const_iterator
		b=uc.begin(),
		e=uc.end();

	while (b != e)
	{
		std::vector<unicode_char>::const_iterator p=b;

		std::vector<unicode_char>::const_iterator ee=e;

		while (b != e && *b != '&' && *b != ';')
		{
			if (*b == '=' && ee == e)
				ee=b;
			++b;
		}

		if (ee == e)
			ee=b;

		string n;

		n.insert(n.end(), p, ee);
		if (ee != b)
			++ee;
		args[n]=std::vector<unicode_char>(ee, b);

		if (b != e)
			++b;
	}
}


// Opening a new tag.

void htmlParser::newTag()
{
	inTag=0;

	vector<unicode_char> tagStr=currentTag;

	currentTag.clear();

	tag new_tag;

	vector<unicode_char>::iterator b=tagStr.begin(), e=tagStr.end();

	skipspc(b, e);

	string tagname="";

	while (b != e)
	{
		if (isb(*b))
			break;

		if (*b == '/' && tagname.size() > 0)
			break; // /X goes into tagname, but not X/

		tagname += *b++;
	}

	skipspc(b, e);
	tou(tagname.begin(), tagname.end()); // Tag name to ucase.

	// Process tag attributes before doing anything else.

	while (b != e)
	{
		string name="";
		vector<unicode_char> value;

		while (b != e && !isb(*b))
		{
			if (*b == '=')
				break;

			name += *b++;
		}


		if (b != e && *b == '=') // Attribute has value.
		{
			b++;

			while (b != e)
			{
				if (value.size() == 0 ||
				    (value[0] != '\'' && value[0] != '"'))
				{
					// Unquoted attr value ends at next
					// blank.
					if (isb(*b))
						break;
				}
				else if (*b == value[0]) // Closing quote.
				{
					b++;
					value.erase(value.begin());
					break;
				}

				value.push_back(*b);
				++b;
			}
		}

		// Attribute name to uppercase.

		tou(name.begin(), name.end());
		new_tag.attributes.push_back( make_pair(name, value));

		skipspc(b, e);
	}

	if (tagname.size() == 0)
		return;

	// Process some aliases.

	if (tagname == "STRONG")
		tagname="B";

	if (tagname == "EM")
		tagname="I";

	if (tagname == "/STRONG")
		tagname="/B";

	if (tagname == "/EM")
		tagname="/I";

	// Convert <CENTER> to <P ALIGN=CENTER>

	if (tagname == "CENTER")
	{
		static unicode_char centerStr[]={'C', 'E', 'N', 'T', 'E', 'R'};

		vector<unicode_char> center;

		center.insert(center.end(), centerStr,
			      centerStr
			      + sizeof(centerStr)/sizeof(centerStr)[0]);

		new_tag.attributes.push_back( make_pair("ALIGN", center));
		tagname="P";
	}

	if (tagname == "/CENTER")
		tagname="/P";

	// Convert <DT> to <P STYLE='font-style: italic;'>

	if (tagname == "DT")
	{
		tagname="P";

		string s("font-style: italic;");
		vector<unicode_char> v;
		v.insert(v.end(), s.begin(), s.end());

		new_tag.attributes
			.push_back( make_pair(string("STYLE"), v));
	}

	if (tagname == "/DT")
		tagname="/P";

	// <DD> and <DL> can be simple blockquotes

	if (tagname == "DD" || tagname == "DL")
		tagname="BLOCKQUOTE";

	if (tagname == "/DD" || tagname == "/DL")
		tagname="/BLOCKQUOTE";

	if (tagname[0] == '/') // CLosing a tag.
	{
		const char *p=tagname.c_str()+1;

		list<tag>::iterator b=tagStack.begin(),
			e=tagStack.end();

		while (b != e)
		{
			--e;

			if (strcmp(e->name->name, p) == 0) // Found opened tag
			{
				if (strchr(e->name->options, OPT_NOBREAK_C))
				{
					// Stuff like <B> or <I> that just
					// updates the active attributes

					closetag(e);
				}
				else if (strchr(e->name->options, OPT_FLUSH_C))
				{
					// Tags that imply end of paragraph,
					// any partial line needs to be
					// flushed.

					if (currentLine.size() > 0)
						fmtline(false);
					closetag(e);
				}
				else
				{
					// Tags that imply end of paragraph,
					// and start of new one.

					if (currentLine.size() > 0)
						fmtline(false);
					closetag(e);
					newParagraph();
				}
				return;
			}
		}
		return;
	}

	// Visually show link targets.

	if (tagname == "A")
	{
		vector<unicode_char> href;

		new_tag.getAttribute("HREF", href);

		if (href.size() == 0)
			return;

		string s("font-style: italic;");
		vector<unicode_char> v;
		v.insert(v.end(), s.begin(), s.end());

		new_tag.attributes
			.push_back( make_pair(string("STYLE"), v));
	}

	// Visually show embedded images.

	if (tagname == "IMG")
	{
		vector<unicode_char> txt;

		new_tag.getAttribute("ALT", txt);

		if (txt.size() == 0)
			new_tag.getAttribute("TITLE", txt);

		if (txt.size() == 0)
		{
			string s="[IMAGE]";

			txt.insert(txt.end(), s.begin(), s.end());
		}
		else
		{
			txt.insert(txt.begin(), '[');
			txt.push_back(']');
		}

		parseline(txt);
		return;
	}

	if (tagname == "BR" || tagname == "DIV")
	{
		bool saveParagraphBreak=paragraphBreak;
		fmtline(false);

		paragraphBreak=saveParagraphBreak;
		return;
	}

	// Well, we ain't smart enough to do table layout, but at least make
	// sure that cells are separated by spaces.

	if (tagname == "TD")
	{
		vector<unicode_char> w;

		w.push_back(' ');
		parseline(w);
		return;
	}

	// Explicitly specify various styles as STYLE attributes, so we
	// know to look in one place.

	if (tagname == "B")
	{
		string s("font-weight: bold;");
		vector<unicode_char> v;
		v.insert(v.end(), s.begin(), s.end());

		new_tag.attributes
			.push_back( make_pair(string("STYLE"), v));
	}

	if (tagname == "U")
	{
		string s("text-decoration: underline;");
		vector<unicode_char> v;
		v.insert(v.end(), s.begin(), s.end());

		new_tag.attributes
			.push_back( make_pair(string("STYLE"), v));
	}

	if (tagname == "I")
	{
		string s("font-style: italic;");
		vector<unicode_char> v;
		v.insert(v.end(), s.begin(), s.end());

		new_tag.attributes
			.push_back( make_pair(string("STYLE"), v));
	}

	if (tagname == "HR")
	{
		if (currentLine.size() > 0)
			fmtline(false);

		vector<unicode_char> dummyw;

		size_t n=getLineLength();

		dummyw.insert(dummyw.end(), n, (unicode_char)'_');
		parseline(dummyw);
		return;
	}

	if (tagname == "LI")
	{
		if (currentLine.size() > 0)
			fmtline(false);

		if (nblanklines == 0)
			fmtline(false);

		string pfix="<B>&bull;</B>&nbsp;";

		leadin_prefix.clear();
		leadin_prefix.insert(leadin_prefix.begin(),
				     pfix.begin(), pfix.end());
	}

	size_t i;

	for (i=0; knownTags[i]; i++)
		if (strcmp(knownTags[i]->name, tagname.c_str()) == 0)
			break;

	if ((new_tag.name=knownTags[i]) == NULL)
		return;

	if (!strchr(new_tag.name->options, OPT_STACK_C))
	{
		list<tag>::iterator b, e;

		while ((b=tagStack.begin()) != (e=tagStack.end()) &&
		       (--e)->name->priority == new_tag.name->priority)
			closetag(e);
	}

	if (strcmp(new_tag.name->name, "PRE") == 0)
	{
		new_tag.preformatted=true;
		if (currentLine.size() > 0)
			fmtline(false);
	}
	else if (!strchr(new_tag.name->options, OPT_NOBREAK_C))
		newParagraph();
	opentag(tagname, new_tag);
}

// Push a new tag on the opened tag stack.

void htmlParser::opentag(std::string tagname,
			 htmlParser::tag &tagRef)
{
	if (!tagStack.empty())
	{
		list<tag>::iterator last=--tagStack.end();

		tagRef.attr=last->attr;

		// Inherit preformatted flag.

		if (last->preformatted)
			tagRef.preformatted=true;
	}

	if (tagname == "A")
	{
		vector<unicode_char> href;

		tagRef.getAttribute("HREF", href);
		tagRef.attr.url.insert(tagRef.attr.url.begin(),
				       href.begin(),
				       href.end());
	}

	std::list< std::pair<std::string,
		std::vector<unicode_char> > >::iterator b, e;

	// Activate any styles that we recognize.

	for (b=tagRef.attributes.begin(), e=tagRef.attributes.end();
	     b != e; b++)
	{
		if (strcasecmp(b->first.c_str(), "STYLE") == 0)
		{
			string s;

			s.insert(s.end(), b->second.begin(), b->second.end());

			updateTagStyle(tagRef, s);
			break;
		}
	}

	if (tagStack.size() < 20)
		tagStack.push_back(tagRef);

	if (tagname == "A")
	{
		ostringstream o;

		o << "&lt;URL:";

		string::iterator b=tagRef.attr.url.begin(),
			e=tagRef.attr.url.end();

		while (b != e)
		{
			if (*b == '&')
				o << "&amp;";
			else if (*b == '<')
				o << "&lt;";
			else if (*b == '>')
				o << "&gt;";
			else if (*b >= ' ' && *b < 0x7F)
				o << *b;
			++b;
		}
 
		o << "&gt;";

		std::string os=o.str();

		vector<unicode_char> u;

		u.reserve(os.size());
		u.insert(u.end(), os.begin(), os.end());

		parseline(u);
	}
}

// Parse a style.

void htmlParser::updateTagStyle(tag &tagRef, string style)
{
	while (style.size() > 0)
	{
		string property;

		// Get next property

		size_t n=style.find(';');

		if (n == std::string::npos)
		{
			property=style;
			style="";
		}
		else
		{
			property=style.substr(0, n);
			style.erase(style.begin(), style.begin()+n+1);
		}

		// Parse property name, value

		string propName;
		string propValue;

		n=property.find(':');

		if (n == std::string::npos)
		{
			propName=property;
			propValue="";
		}
		else
		{
			propName=property.substr(0, n);
			propValue=property.substr(n+1);
		}

		// Strip blanks

		for (n=0; n<propName.size() && isb(propName[n]); n++)
			;

		propName.erase(propName.begin(), propName.begin()+n);
		for (n=0; n<propValue.size() && isb(propValue[n]); n++)
			;
		propValue.erase(propValue.begin(), propValue.begin()+n);

		n=propValue.size();

		while (n > 0)
		{
			if (!isb(propValue[--n]))
			{
				n++;
				break;
			}
		}

		propValue.erase(propValue.begin()+n, propValue.end());

		if (strcasecmp(propName.c_str(), "font-weight") == 0)
		{
			tagRef.attr.reverse=0;

			if (strcasecmp(propValue.c_str(), "bold") == 0 ||
			    atoi(propValue.c_str()) > 700)
				tagRef.attr.reverse=1;
		}

		if (strcasecmp(propName.c_str(), "font-style") == 0)
		{
			if (strcasecmp(propValue.c_str(), "none") == 0)
				tagRef.attr.italic=0;
			if (strcasecmp(propValue.c_str(), "italic") == 0 ||
			    strcasecmp(propValue.c_str(), "oblique") == 0)
				tagRef.attr.italic=1;

		}

		if (strcasecmp(propName.c_str(), "text-decoration") == 0)
		{
			if (strcasecmp(propValue.c_str(), "none") == 0)
				tagRef.attr.underline=0;
			if (strcasecmp(propValue.c_str(), "underline") == 0)
				tagRef.attr.underline=1;
		}

	}
}

void htmlParser::closetag(list<tag>::iterator stackPtr)
{
	leadin_prefix.clear();
	tagStack.erase(stackPtr, tagStack.end());
}

void htmlParser::skipspc(vector<unicode_char>::iterator &b,
			 vector<unicode_char>::iterator &e)
{
	while (b != e && isb(*b))
		b++;
}

void htmlParser::newParagraph()
{
	if (paragraphBreak && currentLine.size() == 0)
		return;

	if (currentLine.size() > 0)
		fmtline(true);

	fmtline(false);
	paragraphBreak=true;
}

static const char l[]="abcdefghijklmnopqrstuvwxyz";
static const char u[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";

bool htmlParser::isb(unicode_char c)
{
	return (unicode_char)((unsigned char)c) == c &&
		strchr(" \t\r\n\v", c) != NULL;
}

bool htmlParser::isu(unicode_char c)
{
	return ((unicode_char)((unsigned char)c) == c && strchr(u, c) != 0);
}

void htmlParser::tou(string::iterator b, string::iterator e)
{
	while (b != e)
	{
		const char *p=strchr(l, *b);

		if (p != NULL)
			*b= u[p - l];
		b++;
	}
}

void htmlParser::tou(vector<unicode_char>::iterator b,
		     vector<unicode_char>::iterator e)
{
	while (b != e)
	{
		if ( ((unicode_char)(unsigned char)*b) == *b)
		{
			const char *p=strchr(l, (char)*b);

			if (p != NULL)
				*b= u[p - l];
		}
		b++;
	}
}
