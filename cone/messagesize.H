/* $Id: messagesize.H,v 1.1 2003/05/27 14:09:03 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef messagesize_H
#define messagesize_H

///////////////////////////////////////////////////////////////////
//
// Convert byte count to a meaningful string

#include "config.h"

#include <string>

class MessageSize {
	std::string buffer;
public:
	MessageSize(unsigned long bytes, bool showBytes=false);
	~MessageSize();
	operator std::string() const { return buffer; }
};

#endif
