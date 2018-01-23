/* $Id: typeahead.C,v 1.1 2003/05/27 14:09:04 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#include <typeahead.H>

Typeahead *Typeahead::typeahead=NULL;

Typeahead::Typeahead()
{
	typeahead=this;
}

Typeahead::~Typeahead()
{
}

