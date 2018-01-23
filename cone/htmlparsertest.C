/* $Id: htmlparsertest.C,v 1.2 2003/07/09 21:33:20 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "htmlparser.H"
#include "htmlentity.h"
#include <iomanip>

using namespace std;

/*
** Regression tests for the HTML parser.
*/

class myParser: public htmlParser {

	void parsedLine(string l, bool dummy);
public:
	myParser();
	~myParser();
};

myParser::myParser() : htmlParser(unicode_ISO8859_1, unicode_ISO8859_1,
				  NULL)
{
}

myParser::~myParser()
{
}

void myParser::parsedLine(string l, bool dummy)
{
	vector<pair<textAttributes, string> > parsedLine;

	textAttributes::getAttributedText(l, parsedLine);

	vector<pair<textAttributes, string> >::iterator b, e;

	b=parsedLine.begin();
	e=parsedLine.end();

	while (b != e)
	{
		cout << b->second;
		b++;
	}

	cout << endl;
}

int main(int argc, char *argv[])
{
	if (argc > 1 && strcmp(argv[1], "entities") == 0)
	{
		size_t i;
		for (i=0; unicodeEntityList[i].name; i++)
		{
			string s=unicodeEntityList[i].name;

			s += ":";

			size_t l=s.size();

			while (l < 16)
			{
				s += "&nbsp;";
				++l;
			}

			cout << s << "&" << unicodeEntityList[i].name
			     << ";&nbsp;(" << unicodeEntityList[i].iso10646
			     << ")<BR>" << endl;
		}

		exit(0);
	}

	if (argc > 1 && strcmp(argv[1], "utf8") == 0)
	{
		size_t i;
		for (i=0; unicodeEntityList[i].name; i++)
		{
			string s=unicodeEntityList[i].name;

			s += ":";

			size_t l=s.size();

			while (l < 16)
			{
				s += " ";
				++l;
			}

			cout << s;

			unicode_char uc[2];

			uc[0]=unicodeEntityList[i].iso10646;
			uc[1]=0;

			char *p=(*unicode_UTF8.u2c)(&unicode_UTF8, uc, NULL);

			if (p)
			{
				cout << p;
				free(p);
			}
			else cout << " ";

			cout << " (" << hex << setw(4) << setfill('0')
			     << unicodeEntityList[i].iso10646
			     << ")" << endl;
		}

		exit(0);
	}

	myParser p;

	char buffer[8192];

	while (cin.read(buffer, sizeof(buffer)).gcount() > 0)
	{
		p.parse(string(buffer, buffer + cin.gcount()));
	}

	p.flush();
	return 0;
}
