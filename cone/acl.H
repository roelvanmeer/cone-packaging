#ifndef acl_h
#define acl_h

/* $Id: acl.H,v 1.1 2004/03/21 05:30:01 mrsam Exp $
**
** Copyright 2004, Double Precision Inc.
**
** See COPYING for distribution information.
*/

/////////////////////////////////////////////////////////////////////////////
//
// Useful things to do with access control lists.

#include <string>
#include <vector>

//
// An ACL

class Acl {

	std::string code;
	std::string descr;

public:
	Acl(std::string code);
	~Acl();

	std::string getDescription() const { return descr; }
	static void editScreen(void *);

	static std::string getDescription(std::string);

	static std::string getErrorRightsDescription(std::string,
						     std::vector<std::string>
						     &);
	class EditScreen;
	class RightsScreen;
};

#endif
