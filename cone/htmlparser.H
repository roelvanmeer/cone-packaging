/* $Id: htmlparser.H,v 1.5 2005/11/24 01:45:18 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef htmlparser_H
#define htmlparser_H

#include "config.h"

#include "htmlentity.h"

#include <string>
#include <cstring>
#include <vector>
#include <list>
#include <map>

#include <iostream>
#include "unicode/unicode.h"

/////////////////////////////////////////////////////////////////////////
//
// This class converts HTML to flowed-format plain text.


// ... with some extra jazz, namely rendering attributes.  In the converted
// plain text format there will be embedded escape sequences that specify
// attributes for the following text.  The attributes are reset at the start
// of each line; each line begins with unattributed text.
//
// The textAttributes() object can be converted to/from string, which is
// the escape sequence.
//
// The escape sequnce is \x7Fzzzz\x7F, where zzz specify the encoded
// attributes.  The converted HTML text never uses \x7Fs, so there's no
// ambiguity.

class textAttributes {  // Parsed character attributes
public:
	unsigned bold:1;
	unsigned underline:1;
	unsigned reverse:1;
	unsigned italic:1;
	unsigned bgcolor;
	unsigned fgcolor;

	std::string url; // A URL is treated as an attribute

	textAttributes();
	textAttributes(std::string);
	~textAttributes();

	bool operator==(const textAttributes &a) const;
	bool operator!=(const textAttributes &a) const;
	operator std::string() const;

	// Convert text with embedded attribute escape codes to an array
	// of attribute/text pairs.

	static void getAttributedText(std::string,
				      std::vector< std::pair<textAttributes,
				      std::string> > &);
};

class htmlParser {

	class enhanced_char {
	public:
		unicode_char ch;
		textAttributes attr;

		enhanced_char(unicode_char chArg, textAttributes attrArg);
		~enhanced_char();
	};

	std::string charBuffer; // Accumulate raw HTML not yet swallowed.

	std::vector<enhanced_char> currentLine; // Current line being built.

	std::vector<unicode_char> leadin_prefix; // Used by <LI>

	size_t wordBreak; // Index of last seen space character.

	size_t desiredLineLength; // Wanted line length.

	char inTag;
	// 0 - eating CDATA, or '<' -- eating tag,
	// or '&' -- eating ext character.

	size_t nblanklines;

	std::vector<unicode_char> currentTag; // Current tag being eaten.

	void newTag();

	// Various tags we recognize, they get pushed on a stack as we
	// see them.  When closing a tag, priority figures out which
	// unclosed tags are popped off.

	struct taginfo {
		const char *name;
		int priority;
		const char *options; // Describes the tag, defined elsewhere.
	};


	static const struct taginfo BLOCKQUOTE, H1, H2, H3, H4, H5, H6, H7, H8,
		H9, P, STYLE, SCRIPT, TABLE, TR, PRE, A, B, U, I;

	static const struct taginfo * const knownTags[];

	class tag {
	public:
		const struct taginfo *name;
		std::list< std::pair<std::string,
				     std::vector<unicode_char> > > attributes;
		// Attributes on a tag.

		textAttributes attr; // Cached current text attrs.
		bool preformatted; // This is a PRE, or a derivative.

		tag();
		~tag();
		void getAttribute(std::string, std::vector<unicode_char> &);
		// Search for an attribute in attributes, and return the
		// attribute's value.


	};

	// Utility functions:

	// Skip whispace in b, up to e.
	static void skipspc(std::vector<unicode_char>::iterator &b,
			    std::vector<unicode_char>::iterator &e);

	static bool isb(unicode_char); // True if this is a blank char.
	static bool isu(unicode_char); // True if char is latin uppercase.

	static void tou(std::string::iterator, std::string::iterator);
	// Convert char string to uppercase.

	static void tou(std::vector<unicode_char>::iterator,
			std::vector<unicode_char>::iterator);
	// Ditto.

	std::list<tag> tagStack; // Current stack of open tags.

	void closetag(std::list<tag>::iterator);
	void opentag(std::string, tag &);
	void updateTagStyle(tag &, std::string);

	bool paragraphBreak; // Paragraph break just seen.
	void newParagraph();

	size_t getLineLength(); // Compute desired line length, for this line.

	const struct unicode_info &htmlCharset; // HTML charset
	const struct unicode_info &txtCharset; // Convert to this charset.
	unicodeEntityAltList *txtAltList; // HTML entity aliases.

	static unicode_char getHtmlEntity(std::string entityName);

public:
	static void urlDecode(std::string url, std::vector<unicode_char> &uc);

	static void cgiDecode(const std::vector<unicode_char> &uc,
			      std::map<std::string,
			      std::vector<unicode_char> > &args);
	htmlParser(const struct unicode_info &htmlCharsetArg,
		   // HTML character set

		   const struct unicode_info &txtCharsetArg,
		   // Our character set.  This class does the necessary
		   // translation

		   unicodeEntityAltList *txtAltListArg,
		   // Alternate representation of characters for
		   // txtCharset (smart quotes, and other ms-crap)

		   size_t desiredLineLengthArg=79);

	virtual ~htmlParser();

	virtual void parsedLine(std::string Line, bool wrapped)=0;
	//
	// The subclass should define parsedLine().
	// wrapped: true if this line is wrapped text (false - last line in
	// the paragraph).
	//

	void parse(std::string); // Feed HTML to here.

	void flush()
	{
		if (currentLine.size() > 0)
			fmtline(false);
	}

private:
	void fmtline(bool);

	void parseline(std::vector<unicode_char> &w);
};

#endif
