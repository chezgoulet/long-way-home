// lwh_textfit.h -- the one wrap: measured, whole-word, never clipped to fit.
//
// The owner's ruling, recorded across the UI: **on-screen text is never clipped to fit -- it
// wraps.** A description in the configurator, the meeting's option pills and the computer's
// transcript have each shipped a line cut off at the frame edge; the transcript was the third. This
// header is the single wrap they all use, so a fourth is a caller that forgets to call it rather
// than a fourth implementation that gets the metric subtly wrong.
//
// It lives in a header with **no engine includes** so the unit test can hold it to its contract
// without a game, and both the UI module and the start screen call the same code.
//
// The wrap does not know a font. It asks the caller to measure a candidate line in whatever metric
// the caller renders in -- the engine passes UI_ProportionalStringWidth, so a proportional font's
// narrow and wide letters are accounted for and a character count is never used as a proxy. **The
// property, and the property the test asserts, is: every line returned measures no wider than
// `width` in that same metric** -- for the real strings, not a short sample. A single word wider
// than the column has no wrap that fits it, so it is returned whole rather than clipped, and the
// caller's column is then the thing to widen.

#ifndef LWH_TEXTFIT_H
#define LWH_TEXTFIT_H

#include <cstring>

// The most lines one wrap will produce, and the longest line. Bounded so the caller owns the
// storage and the header needs neither the engine nor the heap.
enum { LWH_WRAP_MAX_LINES = 32, LWH_WRAP_LINE_LEN = 256 };

// Measure one string in the caller's own rendering metric. `ctx` is opaque to the wrap (the engine
// carries the font style through it).
typedef int ( *LWH_MeasureFn )( const void *ctx, const char *text );

// The lines a wrap would draw.
struct LWH_Wrapped {
	char line[LWH_WRAP_MAX_LINES][LWH_WRAP_LINE_LEN];
	int  count;      // how many lines are filled (0 when the text is empty)
	bool overflow;   // the text needed more than the caller's `maxLines`; text was NOT drawn
};

// Copy `src` into a bounded line buffer. Engine-free: no Q_strncpyz, so the test can build it too.
inline void LWH_CopyLine( char *dst, const char *src )
{
	std::strncpy( dst, src ? src : "", LWH_WRAP_LINE_LEN - 1 );
	dst[LWH_WRAP_LINE_LEN - 1] = 0;
}

// Wrap `text` at spaces so every returned line measures <= `width` in `measure`'s metric. Words are
// taken whole: a word is never split, so nothing is lost mid-word. `maxLines` is the caller's
// vertical budget; when the text needs more, only the first `maxLines` lines are returned and
// `overflow` is set. The caller decides whether to grow the entry, scroll, or report -- but nothing
// is silently clipped.
inline LWH_Wrapped LWH_WrapText( const char *text, int width, int maxLines,
                                 LWH_MeasureFn measure, const void *ctx )
{
	LWH_Wrapped out;
	out.count = 0;
	out.overflow = false;
	for ( int i = 0; i < LWH_WRAP_MAX_LINES; ++i ) out.line[i][0] = 0;
	if ( !text || maxLines <= 0 ) return out;
	if ( maxLines > LWH_WRAP_MAX_LINES ) maxLines = LWH_WRAP_MAX_LINES;

	char cur[LWH_WRAP_LINE_LEN] = "";
	int  n = 0;
	const char *p = text;
	while ( *p )
	{
		while ( *p == ' ' ) ++p;
		if ( !*p ) break;

		char word[LWH_WRAP_LINE_LEN];
		int wl = 0;
		while ( *p && *p != ' ' && wl < LWH_WRAP_LINE_LEN - 1 ) word[wl++] = *p++;
		word[wl] = 0;
		if ( !word[0] ) break;

		// A line is already committed for every slot: any further text is overflow, not a wrap.
		if ( n >= maxLines ) { out.overflow = true; break; }

		char candidate[LWH_WRAP_LINE_LEN];
		const size_t curLen = std::strlen( cur );
		if ( curLen )
		{
			size_t w = curLen;
			if ( w > static_cast<size_t>( LWH_WRAP_LINE_LEN - 2 ) ) w = LWH_WRAP_LINE_LEN - 2;
			std::memcpy( candidate, cur, w );
			candidate[w++] = ' ';
			size_t m = std::strlen( word );
			if ( m > static_cast<size_t>( LWH_WRAP_LINE_LEN - 1 ) - w ) m = LWH_WRAP_LINE_LEN - 1 - w;
			std::memcpy( candidate + w, word, m );
			candidate[w + m] = 0;
		}
		else
		{
			LWH_CopyLine( candidate, word );
		}

		if ( curLen && measure( ctx, candidate ) > width )
		{
			// The word does not fit on this line: flush it, and start the next with the word.
			LWH_CopyLine( out.line[n], cur );
			++n;
			LWH_CopyLine( cur, word );
		}
		else
		{
			LWH_CopyLine( cur, candidate );
		}
	}
	if ( cur[0] )
	{
		if ( n >= maxLines ) out.overflow = true;
		else { LWH_CopyLine( out.line[n], cur ); ++n; }
	}
	out.count = n;
	return out;
}

#endif
