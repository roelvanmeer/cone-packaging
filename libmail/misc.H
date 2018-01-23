/* $Id: misc.H,v 1.2 2004/06/14 00:18:42 mrsam Exp $
**
** Copyright 2003-2004, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#ifndef libmail_misc_H
#define libmail_misc_H

#include "libmail_config.h"
#include "namespace.H"

#include <string>

struct unicode_info;

LIBMAIL_START

class loginInfo;

std::string toutf8(std::string);
std::string fromutf8(std::string);

std::string hostname();
	// Return this machine's name

extern const struct unicode_info *appcharset;


enum readMode {readHeadersFolded, readContents, readBoth, readHeaders};


bool setAppCharset(const char *charsetName);
	//
	// The application character set.  Folder names are automatically
	// translated to the specified character set.  Should be initialized
	// by the application at startup.  Defaults to iso-8859-1 mapping.

void upper(std::string &w);
	//
	// Convert the string to uppercase characters using the ISO-8859-1
	// character set

std::string homedir();
	//
	// The home directory of the userid running this application.

std::string loginUrlEncode(std::string method,
			   std::string server, std::string uid,
			   std::string password);
	// Create a new mail account URL.
	// method - the support method (imap, pop3Account, etc...)
	// server - the name of the mail account's server
	// uid - mail account ID
	// pwd - mail account password

bool loginUrlDecode(std::string url, class loginInfo &info);
	// Decode a mail account URL
	//
	// url - the URL.
	//
	// info - login information


LIBMAIL_END

#endif
