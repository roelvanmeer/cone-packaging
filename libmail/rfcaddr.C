/* $Id: rfcaddr.C,v 1.12 2010/05/02 12:03:22 mrsam Exp $
**
** Copyright 2002-2009, Double Precision Inc.
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

#include <iostream>

#if LIBIDN
#include <idna.h>
#include <stringprep.h>

namespace mail {
	class libidn_init {

	public:
		libidn_init();
		~libidn_init();
	};

	libidn_init libidn_init_instance;
}

mail::libidn_init::libidn_init()
{
	if (!stringprep_check_version(STRINGPREP_VERSION))
	{
		std::cerr << "Required stringprep version "
			  << STRINGPREP_VERSION << " is not installed"
			  << std::endl;
		abort();
	}
}

mail::libidn_init::~libidn_init()
{
}
#endif

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
	errIndex=std::string::npos;

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

			char *n=rfc822_display_name_tobuf(a, i, NULL);

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

void mail::address::setAddr(std::string s)
{
	addr=s;
}

/////////////////////////////////////////////////////////////////////////

mail::emailAddress::emailAddress() : mail::address("", "")
{
}

mail::emailAddress::~emailAddress()
{
}

mail::emailAddress::emailAddress(const mail::address &a)
	: mail::address(a.getName(), a.getAddr())
{
	decode();
}

std::string mail::emailAddress::setDisplayName(string s)
{
	decodedName=s;
	if (appcharset == NULL)
		appcharset=unicode_find(unicode_default_chset());

	name=mail::rfc2047::encodeAddrName(s, appcharset->chset);

	return ""; // TODO
}

std::string mail::emailAddress::setDisplayAddr(string s)
{
	std::string errcode;

	decodedAddr=s;

#if LIBIDN
	char *ascii_ptr;

	size_t at=s.find('@');

	if (at != std::string::npos)
	{
		int err=idna_to_ascii_lz(s.c_str()+at+1, &ascii_ptr, 0);

		if (err != IDNA_SUCCESS)
		{
			errcode=idna_strerror((Idna_rc)err);
		}
		else
		{
			try {
				s=s.substr(0, at+1) + ascii_ptr;
				free(ascii_ptr);
			} catch (...)
			{
				free(ascii_ptr);
				throw;
			}
		}
	}
#endif

	addr=s;

	return errcode;
}

void mail::emailAddress::setName(string s)
{
	mail::address::setName(s);
	decode();
}

void mail::emailAddress::setAddr(string s)
{
	mail::address::setAddr(s);
	decode();
}

void mail::emailAddress::decode()
{
	if (appcharset == NULL)
		appcharset=unicode_find(unicode_default_chset());
	decodedName=mail::rfc2047::decoder().decode(name, *appcharset);

#if LIBIDN
	char *encoded_str;

	size_t at=addr.find('@');

	if (at != std::string::npos)
	{
		int rc=idna_to_unicode_lzlz(addr.c_str()+at+1, &encoded_str, 0);

		if (rc == IDNA_SUCCESS)
		{
			try {
				decodedAddr=addr.substr(0, at+1) + encoded_str;
				free(encoded_str);
			} catch (...)
			{
				free(encoded_str);
				throw;
			}
			return;
		}
	}

#endif
	decodedAddr=addr;
}

