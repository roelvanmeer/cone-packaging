#include "htmlentity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unicode/unicode.h"

const struct unicodeEntity unicodeEntityList[] = {
	{"nbsp", 160},	/* no-break space = non-breaking space, */
	{"iexcl", 161},	/* inverted exclamation mark */
	{"cent", 162, "[cent]"},	/* cent sign */
	{"pound", 163, "[lb]"},	/* pound sign */
	{"curren", 164},	/* currency sign */
	{"yen", 165, "[yen]"},	/* yen sign = yuan sign */
	{"brvbar", 166, "|"},	/* broken bar = broken vertical bar, */
	{"sect", 167},	/* section sign */
	{"uml", 168},	/* diaeresis = spacing diaeresis, */
	{"copy", 169, "(C)"},	/* copyright sign */
	{"ordf", 170},	/* feminine ordinal indicator */
	{"laquo", 171, "<<"},	/* left-pointing double angle quotation mark */
	{"not", 172},	/* not sign */
	{"shy", 173},	/* soft hyphen = discretionary hyphen, */
	{"reg", 174, "(R)"},	/* registered sign = registered trade mark sign, */
	{"macr", 175},	/* macron = spacing macron = overline */
	{"deg", 176},	/* degree sign */
	{"plusmn", 177, "+/-"},	/* plus-minus sign = plus-or-minus sign, */
	{"sup2", 178},	/* superscript two = superscript digit two */
	{"sup3", 179},	/* superscript three = superscript digit three */
	{"acute", 180, "'"},	/* acute accent = spacing acute, */
	{"micro", 181},	/* micro sign */
	{"para", 182},	/* pilcrow sign = paragraph sign, */
	{"middot", 183},	/* middle dot = Georgian comma */
	{"cedil", 184},	/* cedilla = spacing cedilla */
	{"sup1", 185},	/* superscript one = superscript digit one, */
	{"ordm", 186},	/* masculine ordinal indicator, */
	{"raquo", 187, ">>"},	/* right-pointing double angle quotation mark */
	{"frac14", 188, "1/4"},	/* vulgar fraction one quarter */
	{"frac12", 189, "1/2"},	/* vulgar fraction one half */
	{"frac34", 190, "3/4"},	/* vulgar fraction three quarters */
	{"iquest", 191},	/* inverted question mark */
	{"Agrave", 192},	/* latin capital letter A with grave */
	{"Aacute", 193},	/* latin capital letter A with acute, */
	{"Acirc", 194},	/* latin capital letter A with circumflex, */
	{"Atilde", 195},	/* latin capital letter A with tilde, */
	{"Auml", 196},	/* latin capital letter A with diaeresis, */
	{"Aring", 197},	/* latin capital letter A with ring above */
	{"AElig", 198},	/* latin capital letter AE */
	{"Ccedil", 199},	/* latin capital letter C with cedilla, */
	{"Egrave", 200},	/* latin capital letter E with grave, */
	{"Eacute", 201},	/* latin capital letter E with acute, */
	{"Ecirc", 202},	/* latin capital letter E with circumflex, */
	{"Euml", 203},	/* latin capital letter E with diaeresis, */
	{"Igrave", 204},	/* latin capital letter I with grave, */
	{"Iacute", 205},	/* latin capital letter I with acute, */
	{"Icirc", 206},	/* latin capital letter I with circumflex, */
	{"Iuml", 207},	/* latin capital letter I with diaeresis, */
	{"ETH", 208},	/* latin capital letter ETH */
	{"Ntilde", 209},	/* latin capital letter N with tilde, */
	{"Ograve", 210},	/* latin capital letter O with grave, */
	{"Oacute", 211},	/* latin capital letter O with acute, */
	{"Ocirc", 212},	/* latin capital letter O with circumflex, */
	{"Otilde", 213},	/* latin capital letter O with tilde, */
	{"Ouml", 214},	/* latin capital letter O with diaeresis, */
	{"times", 215, "X"},	/* multiplication sign */
	{"Oslash", 216},	/* latin capital letter O with stroke */
	{"Ugrave", 217},	/* latin capital letter U with grave, */
	{"Uacute", 218},	/* latin capital letter U with acute, */
	{"Ucirc", 219},	/* latin capital letter U with circumflex, */
	{"Uuml", 220},	/* latin capital letter U with diaeresis, */
	{"Yacute", 221},	/* latin capital letter Y with acute, */
	{"THORN", 222},	/* latin capital letter THORN, */
	{"szlig", 223},	/* latin small letter sharp s = ess-zed, */
	{"agrave", 224},	/* latin small letter a with grave */
	{"aacute", 225},	/* latin small letter a with acute, */
	{"acirc", 226},	/* latin small letter a with circumflex, */
	{"atilde", 227},	/* latin small letter a with tilde, */
	{"auml", 228},	/* latin small letter a with diaeresis, */
	{"aring", 229},	/* latin small letter a with ring above */
	{"aelig", 230},	/* latin small letter ae */
	{"ccedil", 231},	/* latin small letter c with cedilla, */
	{"egrave", 232},	/* latin small letter e with grave, */
	{"eacute", 233},	/* latin small letter e with acute, */
	{"ecirc", 234},	/* latin small letter e with circumflex, */
	{"euml", 235},	/* latin small letter e with diaeresis, */
	{"igrave", 236},	/* latin small letter i with grave, */
	{"iacute", 237},	/* latin small letter i with acute, */
	{"icirc", 238},	/* latin small letter i with circumflex, */
	{"iuml", 239},	/* latin small letter i with diaeresis, */
	{"eth", 240},	/* latin small letter eth */
	{"ntilde", 241},	/* latin small letter n with tilde, */
	{"ograve", 242},	/* latin small letter o with grave, */
	{"oacute", 243},	/* latin small letter o with acute, */
	{"ocirc", 244},	/* latin small letter o with circumflex, */
	{"otilde", 245},	/* latin small letter o with tilde, */
	{"ouml", 246},	/* latin small letter o with diaeresis, */
	{"divide", 247, "/"},	/* division sign */
	{"oslash", 248},	/* latin small letter o with stroke, */
	{"ugrave", 249},	/* latin small letter u with grave, */
	{"uacute", 250},	/* latin small letter u with acute, */
	{"ucirc", 251},	/* latin small letter u with circumflex, */
	{"uuml", 252},	/* latin small letter u with diaeresis, */
	{"yacute", 253},	/* latin small letter y with acute, */
	{"thorn", 254},	/* latin small letter thorn, */
	{"yuml", 255},	/* latin small letter y with diaeresis, */
	{"fnof", 402},	/* latin small f with hook = function */

	{"Alpha", 913},	/* greek capital letter alpha */
	{"Beta", 914},	/* greek capital letter beta */
	{"Gamma", 915},	/* greek capital letter gamma, */
	{"Delta", 916},	/* greek capital letter delta, */
	{"Epsilon", 917},	/* greek capital letter epsilon */
	{"Zeta", 918},	/* greek capital letter zeta */
	{"Eta", 919},	/* greek capital letter eta */
	{"Theta", 920},	/* greek capital letter theta, */
	{"Iota", 921},	/* greek capital letter iota */
	{"Kappa", 922},	/* greek capital letter kappa */
	{"Lambda", 923},	/* greek capital letter lambda, */
	{"Mu", 924},	/* greek capital letter mu */
	{"Nu", 925},	/* greek capital letter nu */
	{"Xi", 926},	/* greek capital letter xi */
	{"Omicron", 927},	/* greek capital letter omicron */
	{"Pi", 928},	/* greek capital letter pi */
	{"Rho", 929},	/* greek capital letter rho */

	{"Sigma", 931},	/* greek capital letter sigma, */
	{"Tau", 932},	/* greek capital letter tau */
	{"Upsilon", 933},	/* greek capital letter upsilon, */
	{"Phi", 934},	/* greek capital letter phi, */
	{"Chi", 935},	/* greek capital letter chi */
	{"Psi", 936},	/* greek capital letter psi, */
	{"Omega", 937},	/* greek capital letter omega, */
	{"alpha", 945},	/* greek small letter alpha, */
	{"beta", 946},	/* greek small letter beta */
	{"gamma", 947},	/* greek small letter gamma, */
	{"delta", 948},	/* greek small letter delta, */
	{"epsilon", 949},	/* greek small letter epsilon, */
	{"zeta", 950},	/* greek small letter zeta */
	{"eta", 951},	/* greek small letter eta */
	{"theta", 952},	/* greek small letter theta, */
	{"iota", 953},	/* greek small letter iota */
	{"kappa", 954},	/* greek small letter kappa, */
	{"lambda", 955},	/* greek small letter lambda, */
	{"mu", 956},	/* greek small letter mu */
	{"nu", 957},	/* greek small letter nu */
	{"xi", 958},	/* greek small letter xi */
	{"omicron", 959},	/* greek small letter omicron */
	{"pi", 960},	/* greek small letter pi */
	{"rho", 961},	/* greek small letter rho */
	{"sigmaf", 962},	/* greek small letter final sigma, */
	{"sigma", 963},	/* greek small letter sigma, */
	{"tau", 964},	/* greek small letter tau */
	{"upsilon", 965},	/* greek small letter upsilon, */
	{"phi", 966},	/* greek small letter phi */
	{"chi", 967},	/* greek small letter chi */
	{"psi", 968},	/* greek small letter psi */
	{"omega", 969},	/* greek small letter omega, */
	{"thetasym", 977},	/* greek small letter theta symbol, */
	{"upsih", 978},	/* greek upsilon with hook symbol, */
	{"piv", 982},	/* greek pi symbol */

	{"bull", 8226, "*"},	/* bullet = black small circle, */

	{"hellip", 8230, "..."},	/* horizontal ellipsis = three dot leader, */
	{"prime", 8242, "'"},	/* prime = minutes = feet */
	{"Prime", 8243, "''"},	/* double prime = seconds = inches, */
	{"oline", 8254},	/* overline = spacing overscore, */
	{"frasl", 8260, "/"},	/* fraction slash */

	{"weierp", 8472},	/* script capital P = power set */
	{"image", 8465},	/* blackletter capital I = imaginary part, */
	{"real", 8476},	/* blackletter capital R = real part symbol, */
	{"trade", 8482, "[tm]"},	/* trade mark sign */
	{"alefsym", 8501},	/* alef symbol = first transfinite cardinal, */

	{"larr", 8592, "<-"},	/* leftwards arrow */
	{"uarr", 8593},	/* upwards arrow */
	{"rarr", 8594, "->"},	/* rightwards arrow */
	{"darr", 8595},	/* downwards arrow */
	{"harr", 8596, "<->"},	/* left right arrow */
	{"crarr", 8629},	/* downwards arrow with corner leftwards */
	{"lArr", 8656, "<="},	/* leftwards double arrow */
	{"uArr", 8657},	/* upwards double arrow */
	{"rArr", 8658, "=>"},	/* rightwards double arrow, */
	{"dArr", 8659},	/* downwards double arrow */
	{"hArr", 8660, "<=>"},	/* left right double arrow, */

	{"forall", 8704},	/* for all */
	{"part", 8706},	/* partial differential */
	{"exist", 8707},	/* there exists */
	{"empty", 8709},	/* empty set = null set = diameter, */
	{"nabla", 8711},	/* nabla = backward difference, */
	{"isin", 8712},	/* element of */
	{"notin", 8713},	/* not an element of */
	{"ni", 8715},	/* contains as member */
	{"prod", 8719},	/* n-ary product = product sign, */
	{"sum", 8721},	/* n-ary sumation */
	{"minus", 8722, "-"},	/* minus sign */
	{"lowast", 8727, "*"},	/* asterisk operator */
	{"radic", 8730},	/* square root = radical sign, */
	{"prop", 8733},	/* proportional to */
	{"infin", 8734},	/* infinity */
	{"ang", 8736},	/* angle */
	{"and", 8743},	/* logical and = wedge */
	{"or", 8744},	/* logical or = vee */
	{"cap", 8745},	/* intersection = cap */
	{"cup", 8746},	/* union = cup */
	{"int", 8747},	/* integral */
	{"there4", 8756},	/* therefore */
	{"sim", 8764},	/* tilde operator = varies with = similar to, */

	{"cong", 8773},	/* approximately equal to */
	{"asymp", 8776},	/* almost equal to = asymptotic to, */
	{"ne", 8800, "<>"},	/* not equal to */
	{"equiv", 8801, "=="},	/* identical to */
	{"le", 8804, "<="},	/* less-than or equal to */
	{"ge", 8805, ">="},	/* greater-than or equal to, */
	{"sub", 8834},	/* subset of */
	{"sup", 8835},	/* superset of */

	{"nsub", 8836},	/* not a subset of */
	{"sube", 8838},	/* subset of or equal to */
	{"supe", 8839},	/* superset of or equal to, */
	{"oplus", 8853, "(+)"},	/* circled plus = direct sum, */
	{"otimes", 8855, "(x)"},	/* circled times = vector product, */
	{"perp", 8869},	/* up tack = orthogonal to = perpendicular, */
	{"sdot", 8901, "."},	/* dot operator */

	{"lceil", 8968},	/* left ceiling = apl upstile, */
	{"rceil", 8969},	/* right ceiling */
	{"lfloor", 8970},	/* left floor = apl downstile, */
	{"rfloor", 8971},	/* right floor */
	{"lang", 9001},	/* left-pointing angle bracket = bra, */
	{"rang", 9002},	/* right-pointing angle bracket = ket, */

	{"loz", 9674},	/* lozenge */

	{"spades", 9824},	/* black spade suit */
	{"clubs", 9827},	/* black club suit = shamrock, */
	{"hearts", 9829},	/* black heart suit = valentine, */
	{"diams", 9830},	/* black diamond suit */

	{"quot", 34},	/* quotation mark = APL quote, */
	{"amp", 38},	/* ampersand */
	{"lt", 60},	/* less-than sign */
	{"gt", 62},	/* greater-than sign */

	{"OElig", 338},	/* latin capital ligature OE, */
	{"oelig", 339},	/* latin small ligature oe */

	{"Scaron", 352},	/* latin capital letter S with caron, */
	{"scaron", 353},	/* latin small letter s with caron, */
	{"Yuml", 376},	/* latin capital letter Y with diaeresis, */

	{"circ", 710, "^"},	/* modifier letter circumflex accent, */
	{"tilde", 732, "~"},	/* small tilde */

	{"ensp", 8194},	/* en space */
	{"emsp", 8195},	/* em space */
	{"thinsp", 8201},	/* thin space */
	{"zwnj", 8204},	/* zero width non-joiner, */
	{"zwj", 8205},	/* zero width joiner */
	{"lrm", 8206},	/* left-to-right mark */
	{"rlm", 8207},	/* right-to-left mark */
	{"ndash", 8211, "-"},	/* en dash */
	{"mdash", 8212, "--"},	/* em dash */
	{"lsquo", 8216, "`"},	/* left single quotation mark, */
	{"rsquo", 8217, "'"},	/* right single quotation mark, */
	{"sbquo", 8218, ","},	/* single low-9 quotation mark */
	{"ldquo", 8220, "``"},	/* left double quotation mark, */
	{"rdquo", 8221, "''"},	/* right double quotation mark, */
	{"bdquo", 8222, ",,"},	/* double low-9 quotation mark */
	{"dagger", 8224, "+"},	/* dagger */
	{"Dagger", 8225, "++"},	/* double dagger */
	{"permil", 8240, "0/00"},	/* per mille sign */
	{"lsaquo", 8249, "<"},	/* single left-pointing angle quotation mark, */
	{"rsaquo", 8250, ">"},	/* single right-pointing angle quotation mark, */
	{"euro", 8364},	/* euro sign */

	{0, 0}
};

unicodeEntityAltList *unicode_createAltList(const struct unicode_info *chset)
{
	int i;

	unicodeEntityAltList *p=malloc(sizeof(unicodeEntityAltList));

	if (!p)
		return NULL;

	memset(p, 0, sizeof(*p));

	for (i=0; unicodeEntityList[i].name; i++)
	{
		struct unicodeEntityHashBucket *h;
		int bucket;

		if (unicodeEntityList[i].alt == NULL)
			continue;

		if (chset)
		{
			unicode_char uc[2];
			int errflag;
			char *p;

			uc[0]=unicodeEntityList[i].iso10646;
			uc[1]=0;

			p=(*chset->u2c)(chset, uc, &errflag);
			if (p)
			{
				free(p);
				continue;
			}
		}

		h=malloc(sizeof(struct unicodeEntityHashBucket));

		if (!h)
		{
			unicode_destroyAltList(p);
			return NULL;
		}

		h->entity=unicodeEntityList+i;

		bucket=unicodeEntityList[i].iso10646
			% (sizeof(*p)/sizeof( (*p)[0] ));

		h->next= (*p)[bucket];

		(*p)[bucket]=h;
	}

	return p;
}

void unicode_destroyAltList(unicodeEntityAltList *p)
{
	int i;

	for (i=0; i < sizeof(*p)/sizeof( (*p)[0] ); i++)
	{
		struct unicodeEntityHashBucket *h;

		while ((h= (*p)[i]) != NULL)
		{
			(*p)[i]=h->next;
			free(h);
		}
	}
}

const char *unicode_searchAltList(unicodeEntityAltList *p,
				  unicode_char iso10646)
{
	struct unicodeEntityHashBucket *h
		=(*p)[iso10646 % (sizeof(*p)/sizeof((*p)[0]))];

	while (h)
	{
		if (h->entity->iso10646 == iso10646)
			return h->entity->alt;
		h=h->next;
	}
	return NULL;
}

char *unicode_fromCharset(const struct unicode_info *chset,
			  unicode_char *uc,
			  unicodeEntityAltList *altMap)
{
	size_t cnt=0;
	size_t i;
	const char *a;
	char *p;

	int found=0;
	unicode_char *ucMapped;

	for (i=0; uc[i]; i++)
	{
		if (altMap && (a=unicode_searchAltList(altMap, uc[i])) != NULL)
		{
			cnt += strlen(a)-1;
			found=1;
		}
	}

	if (!found)
		return (*chset->u2c)(chset, uc, NULL);

	ucMapped=malloc((i + 1 + cnt) * sizeof(unicode_char));
	if (!ucMapped)
		return NULL;

	cnt=0;
	for (i=0; uc[i]; i++)
	{
		if (altMap && (a=unicode_searchAltList(altMap, uc[i])) != NULL)
		{
			while (*a)
			{
				ucMapped[cnt]= *a;
				cnt++;
				a++;
			}
		}
		else
		{
			ucMapped[cnt]=uc[i];
			cnt++;
		}
	}
	ucMapped[cnt]=0;

	p=(*chset->u2c)(chset, ucMapped, NULL);
	free(ucMapped);
	return p;
}

	
