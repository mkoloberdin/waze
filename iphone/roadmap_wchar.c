/* roadmap_wchar.c - Wide char support functions
 *
 * LICENSE:
 *
  *   Copyright 2008 Alex Agranovich
 *
 *   This file is part of RoadMap.
 *
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   See roadmap_wchar.h.
 */

#include "roadmap_wchar.h"
#include "roadmap.h"


static int utf8_to_utf32 (	const char *aUtf8, RM_WCHAR_T *aUtf32Out );


/*************************************************************************************************
 * mbstowc()
 * Converts the multibyte char to the wide char
 * Receives the pointer to the MB character. Returns number of bytes converted
 *  If error in size returns -1
 */
int mbtowc( RM_WCHAR_T *pwc, const char *s, RM_SIZE_T n )
{
	if (s != NULL)
	{
		if ( n < 1 )
		{
			return -1;
		}
		if ( *s == '\0' )
		{
			*pwc = 0;
			return 1;
		}

		if ( pwc != NULL )
			return utf8_to_utf32( s, pwc );
		else
			return -1;
	}
	else
		return -1;
}

/*************************************************************************************************
 * mbstowcs()
 * Our implementation for the multi-byte to wide char conversion utility.
 * Input - utf-8 char
 * Output - 32 bit unicode
 */
RM_SIZE_T mbstowcs( RM_WCHAR_T *pwcs, const char *s, RM_SIZE_T wchar_max_n )
{
	int	wchar_n = 0;
	int mb_char_bytes_n;
	while ( wchar_max_n > 0 )
	{
		if ( ( mb_char_bytes_n = mbtowc( pwcs, s, wchar_max_n ) ) < 0 )
			return ( RM_SIZE_T )-1;
		if ( !*pwcs )	// End of string
		{
			break;
		}

		wchar_n++;				// Number of wide character
		s += mb_char_bytes_n;	// Move forward the source buffer pointer
		pwcs++;					// Move forward the destination buffer pointer
		wchar_max_n--;
		// roadmap_log( ROADMAP_INFO, "MBSTOWCS END %d %d %d %d", wchar_max_n, wchar_n, mb_char_bytes_n, *pwcs );
	}
	return wchar_n;
}


/*************************************************************************************************
 * wcslen()
 * Wide character string length
 *
 *
 */
RM_SIZE_T wcslen( const RM_WCHAR_T *ws )
{
	RM_SIZE_T	n;

	for (n = 0; *ws; n++)
		ws++;
	return n;
}

/*************************************************************************************************
 * utf8_to_utf32()
 * Converts the utf8 string to the array of the 32 bit unicode characters
 *
 *
 */
static int utf8_to_utf32 (	const char *aUtf8, RM_WCHAR_T *aUtf32Out )
{
	RM_WCHAR_T ret;
	int ch;
	int octet_count = 1;
	int bits_count;
	int i;

	ret = 0;

	ch = ( unsigned char ) *aUtf8++;

		if ( ( ch & 0x80 ) == 0x00 ) // Standard ASCI character
		{
			if (aUtf32Out)
				*aUtf32Out = ch;
		}
		else
		{
			if ((ch & 0xE0) == 0xC0)
			{
				octet_count = 2;
				ch &= ~0xE0;
			}
			else if ((ch & 0xF0) == 0xE0)
			{
				octet_count = 3;
				ch &= ~0xF0;
			}
			else if ((ch & 0xF8) == 0xF0)
			{
				octet_count = 4;
				ch &= ~0xF8;
			}
			else if ((ch & 0xFC) == 0xF8)
			{
				octet_count = 5;
				ch &= ~0xFC;
			}
			else if ((ch & 0xFE) == 0xFC)
			{
				octet_count = 6;
				ch &= ~0xFE;
			}
			else
			{
				return -1;
			}

			bits_count = (octet_count-1) * 6;
			ret |= (ch << bits_count);
			for (i=1; i < octet_count; ++i)
			{
				bits_count -= 6;

				ch = (unsigned char) *aUtf8++;
				if ((ch & 0xC0) != 0x80)
				{
					return -1;
				}

				ret |= ((ch & 0x3F) << bits_count);
			}

			if (aUtf32Out)
				*aUtf32Out = ret;
		}
		return octet_count;
}



