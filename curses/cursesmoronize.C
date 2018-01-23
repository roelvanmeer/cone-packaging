/* $Id: cursesmoronize.C,v 1.4 2004/05/01 02:28:21 mrsam Exp $
**
** Copyright 2003-2004, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "curses_config.h"
#include "cursesmoronize.H"

using namespace std;

bool CursesMoronize::enabled=false;

CursesMoronize::Entry CursesMoronize::moronizationList[] = {
	{ ")C(", 3, 169},
	{ ")R(", 3, 174},
	{ "-/+", 3, 177},
	{ " 4/1", 4, 188},
	{ " 2/1", 4, 189},
	{ " 4/3", 4, 190},
	{ "]mt[", 4, 8482},
	{ "-<", 2, 8592},
	{ ">-", 2, 8594},
	{ ">-<", 3, 8596},
	{ "``", 2, 8220},
	{ "''", 2, 8221},
	{ " * ", 3, 8226},
	{ "...", 3, 8230},
	{ NULL, 0, 0}};

size_t CursesMoronize::moronize(const char *buf, wchar_t &nreplaced)
{
	Entry *e=moronizationList;

	while (e->keycode)
	{
		if (strncmp(e->keycode, buf, e->keycodeLen) == 0 &&
		    e->unicode_char)
		{
			nreplaced=e->unicode_char;
			return e->keycodeLen;
		}
		++e;
	}
	return 0;
}
