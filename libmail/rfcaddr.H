/* $Id: rfcaddr.H,v 1.5 2004/05/30 02:43:00 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#ifndef libmail_rfcaddr_h
#define libmail_rfcaddr_h

#include "libmail_config.h"
#include "namespace.H"

#include <stdlib.h>
#include <vector>
#include <string>

//
// An RFC 2822 address, a name and user@domain.
//

LIBMAIL_START

class address {

protected:
	std::string name;
	std::string addr;
public:

	std::string getName() const { return name; }
	std::string getAddr() const { return addr; }

	virtual void setName(std::string s);
	void setAddr(std::string a) { addr=a; }

	// If addr.size() == 0 this is the obsolete rfc 822
	// group list notation, and name is either ";" or "name:"

	address(std::string n, std::string a);

	virtual ~address();

	// Convert myself to "name" <addr> format.

	std::string toString() const;

	//
	// Create an RFC 2822 address header, from a vector of addresss.
	//

	template<class T>
	static std::string toString(std::string hdr, // Name:
				    const std::vector<T> &h,
				    size_t w=76); // Max length of lines.

	//
	// Create a vector of addresses from a string
	//

	template<class T> static bool fromString(std::string addresses,
						 std::vector<T> &h,
						 size_t &errIndex);

	// Addresses are appended to h
	// Returns false if memory allocation failed or addresses is
	// syntactically invalid.  errIndex points where in addresses the
	// syntax error is (or is string::npos if memory allocation failed)

	std::string getCanonAddress() const;
	// Return addr with domain portion converted to lowercase

	bool operator==(const address &) const;
	// Compare the addr portions only, ignoring domain case

	bool operator!=(const address &a) const
	{
		return !operator==(a);
	}

};

// INPROGRESS: slowly migrate to automatic MIME encoding of names.
//
// Subclass address, override getName() and setName() to use a MIME-encoded
// name, using mail::appcharset.
//
// emailAddress transparently casts to mail::address, importing/exporting
// the name in its encoded form.

class emailAddress : public address {

	std::string encodedName;

	void decode();
public:
	emailAddress(std::string n, std::string a);
	emailAddress(const address &);
	virtual ~emailAddress();

	std::string getAddrName() const { return encodedName; }

	void setAddrName(std::string s);
	virtual void setName(std::string s);

	bool operator==(const address &a) const
	{
		return mail::address::operator==(a);
	}

	bool operator!=(const address &a) const
	{
		return !operator==(a);
	}

};

LIBMAIL_END

#endif
