/* $Id: rfc2047decode.C,v 1.3 2008/05/24 17:57:42 mrsam Exp $
**
** Copyright 2002-2008, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#include "libmail_config.h"
#include "rfc2047decode.H"
#include "rfc822/rfc2047.h"
#include "mail.H"
#include <cstring>

using namespace std;

mail::rfc2047::decoder::decoder()
{
}

mail::rfc2047::decoder::~decoder()
{
}

int mail::rfc2047::decoder::rfc2047_decode_callback(const char *text,
						    int text_len,
						    const char *charset,
						    const char *language,
						    void *voidarg)
{
	return ((mail::rfc2047::decoder *)voidarg)
		->rfc2047_callback(text, text_len, charset, language);
}

int mail::rfc2047::decoder::rfc2047_callback(const char *text, int text_len,
					     const char *charset,
					     const char *language)
{
	if (charset == NULL)
	{
		string s;

		s.insert(s.end(), text, text+text_len);

		unicode_char *u= (*unicode_ISO8859_1.c2u)(&unicode_ISO8859_1,
							  s.c_str(), NULL);

		if (!u)
			LIBMAIL_THROW("Out of memory.");

		size_t n;

		for (n=0; u[n]; n++)
			;
		try {
			unicodes.insert(unicodes.end(), u, u+n);
			free(u);
		} catch (...) {
			free(u);
			LIBMAIL_THROW();
		}
		return (0);
	}

	const struct unicode_info *uc=unicode_find(charset);

	if (strcasecmp(charset, nativeCharset->chset) == 0)
		uc= nativeCharset;

	if (strcasecmp(charset, nativeCharset->chset)
	    && !(nativeCharset->flags & UNICODE_UTF))
		// No need to insert [] for UTF charset
	{
		string s=string(" [") + charset + "]";
		rfc2047_callback(&*s.begin(), s.size(), NULL, NULL);
		// HACK - reuse code above.
	}

	if (uc == NULL)
		uc= &unicode_ISO8859_1;

	string s;

	s.insert(s.end(), text, text+text_len);

	unicode_char *u= (*uc->c2u)(uc, s.c_str(), NULL);

	if (!u)
		LIBMAIL_THROW("Out of memory.");

	size_t n;

	for (n=0; u[n]; n++)
		;
	try {
		unicodes.insert(unicodes.end(), u, u+n);
		free(u);
	} catch (...) {
		free(u);
		LIBMAIL_THROW();
	}
	return (0);
}

const unicode_char *mail::rfc2047::decoder::decode(string text)
{
	nativeCharset= &unicode_ISO8859_1;
	unicodes.clear();

	rfc2047_decode(text.c_str(),
		       &mail::rfc2047::decoder::rfc2047_decode_callback,
		       this);
	unicodes.push_back(0);

	return &*unicodes.begin();
}

string mail::rfc2047::decoder::decode(string text, string charset)
{
	const struct unicode_info *uc=unicode_find(charset.c_str());

	if (!uc)
		uc=&unicode_ISO8859_1;

	return decode(text, *uc);
}

string mail::rfc2047::decoder::decode(string text,
				      const struct unicode_info &uc)
{
	nativeCharset=&uc;
	unicodes.clear();

	rfc2047_decode(text.c_str(),
		       &mail::rfc2047::decoder::rfc2047_decode_callback,
		       this);

	unicodes.push_back(0);


	string s;

	char *p= (uc.u2c)(&uc, &*unicodes.begin(), NULL);

	if (!p)
		LIBMAIL_THROW("Out of memory.");

	try {
		s=p;
		free(p);
	} catch (...) {
		free(p);
		LIBMAIL_THROW();
	}
	return s;
}

void mail::rfc2047::decoder::decode(vector<mail::address> &addr_cpy,
				    const struct unicode_info &unicodeInfo)
{
	vector<mail::address>::iterator b=addr_cpy.begin(), e=addr_cpy.end();

	while (b != e)
	{
		b->setName(decode(b->getName(), unicodeInfo));
		b++;
	}
}

string mail::rfc2047::decoder::decodeSimple(string str)
{
	char *p=rfc2047_decode_simple(str.c_str());

	string s="";

	if (p)
		try {
			s=p;
			free(p);
		} catch (...) {
			free(p);
			LIBMAIL_THROW();
		}

	return s;
}

string mail::rfc2047::decoder::decodeEnhanced(string str,
					      const struct unicode_info
					      &myCharset)
{
	char *p=rfc2047_decode_unicode(str.c_str(), &myCharset,
				       RFC2047_DECODE_NOTAG);

	string s="";

	if (p)
		try {
			s=p;
			free(p);
		} catch (...) {
			free(p);
			LIBMAIL_THROW();
		}

	return s;
}
