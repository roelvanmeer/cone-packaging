/* $Id: rfcaddr.C,v 1.6 2009/06/27 17:12:00 mrsam Exp $
**
** Copyright 2002-2004, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#include "libmail_config.h"
#include <ctype.h>
#include <string.h>
#include "rfcaddr.H"
#include "mail.H"
#include "misc.H"
#include "rfc2047encode.H"
#include "rfc2047decode.H"
#include "rfc822/rfc822.h"
#include "unicode/unicode.h"

using namespace std;

mail::address::address(string n, string a): name(n), addr(a)
{
}

mail::address::~address()
{
}

string mail::address::toString() const
{
	if (addr.length() == 0)
		return name;

	string q=addr;

	// Only add < > if there's a name

	if (name.length() > 0)
	{
		q=" <" + q + ">";

		// If name is all alphabetics, don't bother with quotes.

		string::const_iterator b=name.begin(), e=name.end();

		while (b != e)
		{
			if (!strchr(" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789?*+-/=_",
				    *b))
			{
				string s="\"";

				b=name.begin(), e=name.end();

				while (b != e)
				{
					if (strchr("\\\"()<>", *b))
						s += "\\";
					// These chars must be escaped.

					s.append(&*b, 1);
					b++;
				}
				return s + "\"" + q;
			}
			b++;
		}
		q=name + q;
	}
	return q;
}

template<class T>
string mail::address::toString(string hdr, const vector<T> &h,
			       size_t w)
{
	int len=hdr.length();
	string comma="";
	bool first=true;

	string spc="";

	spc.insert(spc.end(), len < 20 ? len:20, ' ');
	// Leading whitespace for folded lines.

	typename std::vector<T>::const_iterator b=h.begin(), e=h.end();

	while (b != e)
	{
		string s= b->toString();

		if (!first && len + comma.length() + s.length() > w)
		{
			// Line too long, wrap it.

			if (comma.size() > 0)
				comma=",";

			hdr=hdr + comma + "\n" + spc;
			len=spc.size();
			comma="";
		}
		first=false;
		hdr = hdr + comma + s;

		len += comma.length() + s.length();
		comma=", ";

		if (b->addr.length() == 0)
			comma="";
		b++;
	}
	return hdr;
}

template
string mail::address::toString(string hdr, const vector<mail::address> &h,
			       size_t w);

template
string mail::address::toString(string hdr,
			       const vector<mail::emailAddress> &h,
			       size_t w);

string mail::address::getCanonAddress() const
{
	string a=addr;

	size_t n=a.find_last_of('@');

	if (n < a.size())
	{
		char *p=
			(*unicode_ISO8859_1.tolower_func)(&unicode_ISO8859_1,
							  a.c_str() + n, NULL);

		if (!p)
			LIBMAIL_THROW("Out of memory.");

		try {
			a=a.substr(0, n) + p;
			free(p);
		} catch (...) {
			free(p);
			LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
		}
	}

	return a;
}

bool mail::address::operator==(const mail::address &o) const
{
	return getCanonAddress() == o.getCanonAddress();
}

static void parseCallback(const char *str, int pos, void *voidarg)
{
	*(size_t *)voidarg=pos;
}

template<class T> bool mail::address::fromString(string addresses,
						 vector<T> &h,
						 size_t &errIndex)
{
	errIndex=addresses.npos;

	struct rfc822t *t=rfc822t_alloc_new(addresses.c_str(),
					    &parseCallback, &errIndex);
	if (!t)
		return false;

	struct rfc822a *a=rfc822a_alloc(t);

	if (!a)
	{
		rfc822t_free(t);
		return false;
	}

	try {
		int i;

		for (i=0; i<a->naddrs; i++)
		{
			string name;
			string addr;

			char *n=rfc822_getname_orlist(a, i);

			if (!n)
			{
				rfc822a_free(a);
				rfc822t_free(t);
				return false;
			}

			try {
				name=n;
				free(n);
			} catch (...) {
				free(n);
				LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
			}


			n=rfc822_getaddr(a, i);

			if (!n)
			{
				rfc822a_free(a);
				rfc822t_free(t);
				return false;
			}

			try {
				addr=n;
				free(n);
			} catch (...) {
				free(n);
				LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
			}

			if (a->addrs[i].name == 0)
				name="";

			h.push_back( mail::address(name, addr));
		}
		rfc822a_free(a);
		rfc822t_free(t);
	} catch (...) {
		rfc822a_free(a);
		rfc822t_free(t);
		LIBMAIL_THROW(LIBMAIL_THROW_EMPTY);
	}
	return true;
}

template
bool mail::address::fromString(string addresses,
			       vector<mail::address> &h,
			       size_t &errIndex);

template
bool mail::address::fromString(string addresses,
			       vector<mail::emailAddress> &h,
			       size_t &errIndex);


void mail::address::setName(std::string s)
{
	name=s;
}

/////////////////////////////////////////////////////////////////////////

mail::emailAddress::emailAddress(string n, string a)
	: mail::address(n, a)
{
	setAddrName(n);
}

mail::emailAddress::~emailAddress()
{
}

mail::emailAddress::emailAddress(const mail::address &a)
	: mail::address(a.getName(), a.getAddr())
{
	decode();
}

void mail::emailAddress::setAddrName(string s)
{
	encodedName=s;
	if (appcharset == NULL)
		appcharset=unicode_find(unicode_default_chset());

	name=mail::rfc2047::encodeAddrName(s, appcharset->chset);
}

void mail::emailAddress::setName(string s)
{
	mail::address::setName(s);
	decode();
}

void mail::emailAddress::decode()
{
	if (appcharset == NULL)
		appcharset=unicode_find(unicode_default_chset());
	encodedName=mail::rfc2047::decoder::decodeEnhanced(name,
							   *appcharset);
}

