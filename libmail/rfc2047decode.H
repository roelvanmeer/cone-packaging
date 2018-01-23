/* $Id: rfc2047decode.H,v 1.2 2004/05/03 00:04:05 mrsam Exp $
**
** Copyright 2002, Double Precision Inc.
**
** See COPYING for distribution information.
*/
#ifndef libmail_rfc2047_decode_h
#define libmail_rfc2047_decode_h

#include "libmail_config.h"
#include "unicode/unicode.h"
#include "rfcaddr.H"

#include <vector>
#include <string>

#include "namespace.H"

LIBMAIL_START

//
// Mail header decoder.  A variety of decoding options are available.
// This is basically a wrapper for librfc822.a's functions, with some
// value-added code.
//

namespace rfc2047 {

	class decoder {

		std::vector<unicode_char> unicodes;

		const struct unicode_info *nativeCharset;

	public:
		decoder();
		~decoder();

	private:
		static int rfc2047_decode_callback(const char *text,
						   int text_len,
						   const char *charset,
						   const char *language,
						   void *voidarg);

		int rfc2047_callback(const char *text, int text_len,
				     const char *charset,
				     const char *language);
	public:

		// Decode to unicode chars.
		const unicode_char *decode(std::string rfc2047_text);

		// Decode to charset 'charset'.  8-bit content in another
		// charset gets prepended with [CHARSET]

		std::string decode(std::string rfc2047_text,
				   std::string charset);

		// Decode to charset 'toCharset'. 8-bit content in a charset
		// other than 'toCharset' gets prepended with [CHARSET]

		std::string decode(std::string rfc2047_text,
				   const struct unicode_info &toCharset);

		// Decode the name portion in a parsed list of addresses.
		// Decode to charset 'nativeInfo', and prepend [CHARSET] to
		// content in any charset other than 'nativeInfo'.

		void decode(std::vector<mail::address> &addr_cpy,
			    const struct unicode_info &nativeInfo);

		// Decode without translating to any charset, and without
		// prepending [CHARSET]

		static std::string decodeSimple(std::string str);

		// Decode and translate, if possible, to charset 'myCharset',
		// but without prepending [CHARSET].

		static std::string decodeEnhanced(std::string str,
						  const struct unicode_info
						  &myCharset);
	};
}

LIBMAIL_END

#endif

