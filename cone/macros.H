/* $Id: macros.H,v 1.2 2004/05/01 04:59:52 mrsam Exp $
**
** Copyright 2004, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef macros_H
#define macros_H

#include "config.h"

#include <string>
#include <map>
#include <vector>
#include "unicode/unicode.h"

class Macros {

public:
	class name {
	public:
		unsigned f; // If > 0, function key #+1, else n:
		std::vector<unicode_char> n;

		name(std::vector<unicode_char> &s) : f(0), n(s) {}
		name(int fkey) : f(fkey+1) {}
		~name() {}

		bool operator==(const name &o) const
		{
			return f == o.f && n == o.n;
		}

		bool operator<(const name &o) const
		{
			return f < o.f || (f == o.f && n < o.n);
		}
	};

	std::map<name, std::string> macroList;

	Macros();
	~Macros();

	static Macros *getRuntimeMacros();
};

#endif
