/* $Id: rfc2047encode.C,v 1.3 2004/05/30 02:43:00 mrsam Exp $
**
** Copyright 2002-2004, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#include "libmail_config.h"
#include "rfc2047encode.H"
#include "rfc822/rfc2047.h"
#include "mail.H"

mail::rfc2047::encode::encode(std::string txt, std::string charset)
{
	char *p=rfc2047_encode_str(txt.c_str(), charset.c_str(),
				   rfc2047_qp_allow_any);

	if (!p)
		LIBMAIL_THROW("Out of memory.");

	try {
		encodedString=p;
	} catch (...) {
		free(p);
		LIBMAIL_THROW();
	}

	free(p);
}

mail::rfc2047::encode::~encode()
{
}

mail::rfc2047::encodeAddrName::encodeAddrName(std::string txt,
					      std::string charset)
	: encodedString(txt)
{
	char *p=rfc2047_encode_str(txt.c_str(),
				   charset.c_str(),
				   rfc2047_qp_allow_word);

	if (!p)
		LIBMAIL_THROW("Out of memory.");

	try {
		encodedString=p;
	} catch (...) {
		free(p);
		LIBMAIL_THROW();
	}
	free(p);
}

mail::rfc2047::encodeAddrName::~encodeAddrName()
{
}
