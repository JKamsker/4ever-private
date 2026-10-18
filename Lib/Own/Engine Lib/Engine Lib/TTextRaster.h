// TTextRaster.h: the game's text measured and drawn into coverage images.
//
// Two rasterizers give GDI+'s results. GDI+ itself draws on Windows (the default). The font files,
// read with stb_truetype, draw on the other systems, and on Windows with TextRenderer=FontFiles
// in config.ini. They lay text out as GDI+ does with its default string format: the advances 3 %
// wider, 1/6 em of padding on each side, the first baseline at the font's Windows ascent, a
// measured height of the Windows ascent and descent plus 1/8 em, and lines wrapped at the layout
// width. The fonts are those GDI picks: a face that is not installed (2002L, the UI's font)
// becomes Arial, or Malgun Gothic for its vertical form (@2002L), else fonts with the same
// metrics (Liberation Sans, bundled in the Fonts folder next to the game).
//////////////////////////////////////////////////////////////////////

#pragma once

// Gdiplus::TextRenderingHintAntiAlias, the hint the UI measures with.
#define TTEXTHINT_ANTIALIAS				4

class CTTextRaster
{
public:
	// Draw from the font files rather than with GDI+ (always so where there is no GDI+).
	static void SetFontFiles( BOOL bFontFiles);
	static BOOL UsesFontFiles();

	// Graphics::MeasureString from (0, 0): the size of the text in pixels. bTrailingSpaces
	// counts the spaces at the end of the lines (StringFormatFlagsMeasureTrailingSpaces).
	static void Measure(
		CFont *pFont,
		LPCWSTR lpszText,
		BOOL bTrailingSpaces,
		int nRenderingHint,
		FLOAT *pWidth,
		FLOAT *pHeight);

	// Graphics::DrawString of white text in the layout rectangle, on a black nWidth x nHeight
	// image: its coverage, one byte (0 to 255) per pixel. nFormat: DT_CENTER or DT_RIGHT, and
	// DT_VCENTER.
	static void Draw(
		CFont *pFont,
		LPCWSTR lpszText,
		int nRenderingHint,
		FLOAT fLeft,
		FLOAT fTop,
		FLOAT fWidth,
		FLOAT fHeight,
		UINT nFormat,
		int nWidth,
		int nHeight,
		LPBYTE pCoverage);
};
