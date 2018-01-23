/* $Id: tags.H,v 1.3 2003/09/01 21:47:03 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef tags_H
#define tags_H

#include "config.h"

#include <string>
#include <vector>

class Tags {

public:
	Tags();

	std::vector<std::string> names;

	~Tags();

	std::string getTagName(size_t n) const;
	bool isTagName(std::string, size_t &) const;
	static Tags tags;
};

#endif
