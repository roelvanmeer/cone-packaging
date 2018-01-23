/* $Id: fdtls.C,v 1.3 2008/05/24 17:57:42 mrsam Exp $
**
** Copyright 2002-2008, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#include "libmail_config.h"
#include "fdtls.H"
#include <cstdlib>

#if HAVE_LIBCOURIERTLS

// libcouriertls.a callback - get a config setting

const char *mail::fdTLS::get_tls_config_var(const char *varname, void *vp)
{
	return ((mail::fdTLS *)vp)->get_tls_config_var(varname);
}

// libcouriertls.a callback - report a tls error msg

void mail::fdTLS::get_tls_err_msg(const char *errmsg, void *vp)
{
	((mail::fdTLS *)vp)->get_tls_err_msg(errmsg);
}

// Get a config setting, for now, use getenv.

const char *mail::fdTLS::get_tls_config_var(const char *varname)
{
	if (strcmp(varname, "TLS_PROTOCOL") == 0 && tlsflag)
		varname="TLS_STARTTLS_PROTOCOL";

	if (strcmp(varname, "TLS_VERIFYPEER") == 0)
	{
		if (domain.size() == 0)
			return "NONE";
	}

	return getenv(varname);
}

// libcouriertls.a callback - report a tls error msg

void mail::fdTLS::get_tls_err_msg(const char *errmsgArg)
{
	errmsg=errmsgArg;
}
#else

mail::fdTLS::fdTLS()
{
}

mail::fdTLS::~fdTLS()
{
}

#endif
