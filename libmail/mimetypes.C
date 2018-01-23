/* $Id: mimetypes.C,v 1.1 2003/05/27 14:09:08 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#include "libmail_config.h"
#include "mimetypes.H"
#include "namespace.H"
#include <fstream>
#include <ctype.h>

using namespace std;

LIBMAIL_START

#include "mimetypefiles.h"

LIBMAIL_END

mail::mimetypes::mimetypes(string searchKey)
{
	size_t i;

	for (i=0; mimetypefiles[i]; i++)
	{
		ifstream f( mimetypefiles[i]);

		while (!f.bad() && !f.eof())
		{
			string line;

			getline(f, line);

			size_t p;

			p=line.find('#');

			if (p != line.npos)
				line=line.substr(0, p);

			string::iterator b, e, c;

			vector<string> words;

			b=line.begin();
			e=line.end();

			bool found=false;

			while (b != e)
			{
				if (isspace((int)(unsigned char)*b))
				{
					b++;
					continue;
				}
				c=b;
				while (b != e && !isspace((int)(unsigned char)
							  *b))
					b++;

				string w=string(c, b);

				if (strcasecmp(w.c_str(), searchKey.c_str())
				    == 0)
					found=true;
				words.push_back(w);
			}

			if (found)
			{
				type=words[0];
				extensions.insert(extensions.end(),
						  words.begin()+1,
						  words.end());
				return;
			}
		}
	}
}

mail::mimetypes::~mimetypes()
{
}
