/* $Id: fdtls.H,v 1.3 2003/06/19 01:01:41 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#ifndef libmail_fdtls_h
#define libmail_fdtls_h

#include "libmail_config.h"

#include	"libcouriertls.h"

#if HAVE_LIBCOURIERTLS
#include	"../tcpd/libcouriertls.h"
#endif

#include <string>

#include	"namespace.H"

LIBMAIL_START

class callback;

////////////////////////////////////////////////////////////////////////////
//
// Additional SSL metadata for SSL-enabled server connections.

class fdTLS {

public:

#if HAVE_LIBCOURIERTLS

	// Our read/write buffers

	char readBuffer[BUFSIZ];
	std::string writebuffer;

	// Metadata tls_transfer() needs.

	struct tls_transfer_info tls_transfer;

	// Original login or STARTTLS callback.

	//mail::callback *callback;

	// Callback data for libcouriertls.a
	struct tls_info tls_info;

	// Whether we're supposed to use STARTTLS
	bool tlsflag;

	// OpenSSL stuff:
	SSL_CTX *ctx;
	SSL *ssl;
	int fd;
	bool tlsShutdownSent;

	std::string domain;  // Server's known hostname (for cert checking)
	std::string errmsg;  // Most recent error message.

	fdTLS(bool tlsflagArg) : tls_info( *tls_get_default_info() ),
				 tlsflag(tlsflagArg),
				 ctx(NULL),
				 ssl(NULL),
				 fd(-1), tlsShutdownSent(false)
	{
		errmsg="";
	}

	~fdTLS()
	{
		close();
	}

	void close()
	{
		if (ssl)
		{
			tls_disconnect(ssl, fd);
			ssl=NULL;
		}

		if (ctx)
		{
			tls_destroy(ctx);
			ctx=NULL;
		}
	}

	static const char *get_tls_config_var(const char *, void *);
	static void get_tls_err_msg(const char *, void *);

private:
	const char *get_tls_config_var(const char *);
	void get_tls_err_msg(const char *);


#else
public:
	fdTLS();
	~fdTLS();
	int dummy;
#endif
};

LIBMAIL_END

#endif
