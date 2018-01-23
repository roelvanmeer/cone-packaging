/* $Id: colors.H,v 1.2 2003/10/12 01:36:33 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#ifndef colors_H
#define colors_H

#include "config.h"

#include <string>


//
// Custom colors
//

struct CustomColor {
	const char *descr;
	const char *shortname;
	int defaultFcolor;
	int fcolor;
};

struct CustomColorGroup {
	const char *groupDescr;
	struct CustomColor **colors;
};

extern struct CustomColorGroup *getColorGroups();

extern void initColorGroups();

#endif
