#ifndef htmlentity_h
#define htmlentity_h

#include "config.h"
#include "unicode/unicode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct unicode_info;

struct unicodeEntity {
	const char *name;
	unicode_char iso10646;
	const char *alt;	/* Not null - plaintext alternative */
};

extern const struct unicodeEntity unicodeEntityList[];

/* Last list terminated by a NULL name */

/*
** Rich text demoronization: replace unicode characters not displayable
** in the current charset with their US-ASCII equivalents (where possible).
**
** Step 1: Build a hash table of replacable unicode characters
*/

struct unicodeEntityHashBucket {
	struct unicodeEntityHashBucket *next;
	const struct unicodeEntity *entity;
};

typedef struct unicodeEntityHashBucket *unicodeEntityAltList[99];

extern unicodeEntityAltList
*unicode_createAltList(const struct unicode_info *);

extern void unicode_destroyAltList(unicodeEntityAltList *);

extern const char *unicode_searchAltList(unicodeEntityAltList *,
					 unicode_char iso10646);

/* Step 2: Replace */

extern char *unicode_fromCharset(const struct unicode_info *chset,
				 unicode_char *uc,
				 unicodeEntityAltList *altMap);

#ifdef __cplusplus
};
#endif

#endif
