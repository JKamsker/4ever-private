// TTypes.h: CPoint, CSize and CRect without ATL.
//
// The same classes as ATL's atltypes.h: they derive from POINT, SIZE and RECT, so they pass
// where those are expected, and their operations treat a rectangle as Win32 does (right and
// bottom excluded, an empty or inverted one is "empty").
//////////////////////////////////////////////////////////////////////

#if !defined __TTYPES_H
#define __TTYPES_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CSize;
class CRect;

class CPoint : public POINT
{
public:
	CPoint() { x = 0; y = 0; }
	CPoint( int initX, int initY) { x = initX; y = initY; }
	CPoint( POINT initPt) { x = initPt.x; y = initPt.y; }
	CPoint( SIZE initSize) { x = initSize.cx; y = initSize.cy; }
	CPoint( LPARAM dwPoint) { x = (short) LOWORD(dwPoint); y = (short) HIWORD(dwPoint); }

	void Offset( int xOffset, int yOffset) { x += xOffset; y += yOffset; }
	void Offset( POINT point) { x += point.x; y += point.y; }
	void Offset( SIZE size) { x += size.cx; y += size.cy; }
	void SetPoint( int X, int Y) { x = X; y = Y; }

	BOOL operator == ( POINT point) const { return x == point.x && y == point.y; }
	BOOL operator != ( POINT point) const { return x != point.x || y != point.y; }

	void operator += ( SIZE size) { x += size.cx; y += size.cy; }
	void operator -= ( SIZE size) { x -= size.cx; y -= size.cy; }
	void operator += ( POINT point) { x += point.x; y += point.y; }
	void operator -= ( POINT point) { x -= point.x; y -= point.y; }

	inline CPoint operator + ( SIZE size) const;
	inline CPoint operator - ( SIZE size) const;
	CPoint operator - () const { return CPoint( -x, -y); }
	CPoint operator + ( POINT point) const { return CPoint( x + point.x, y + point.y); }
	inline CSize operator - ( POINT point) const;

	inline CRect operator + ( const RECT *lpRect) const;
	inline CRect operator - ( const RECT *lpRect) const;
};

class CSize : public SIZE
{
public:
	CSize() { cx = 0; cy = 0; }
	CSize( int initCX, int initCY) { cx = initCX; cy = initCY; }
	CSize( SIZE initSize) { cx = initSize.cx; cy = initSize.cy; }
	CSize( POINT initPt) { cx = initPt.x; cy = initPt.y; }
	CSize( DWORD dwSize) { cx = (short) LOWORD(dwSize); cy = (short) HIWORD(dwSize); }

	BOOL operator == ( SIZE size) const { return cx == size.cx && cy == size.cy; }
	BOOL operator != ( SIZE size) const { return cx != size.cx || cy != size.cy; }

	void operator += ( SIZE size) { cx += size.cx; cy += size.cy; }
	void operator -= ( SIZE size) { cx -= size.cx; cy -= size.cy; }
	void SetSize( int CX, int CY) { cx = CX; cy = CY; }

	CSize operator + ( SIZE size) const { return CSize( cx + size.cx, cy + size.cy); }
	CSize operator - ( SIZE size) const { return CSize( cx - size.cx, cy - size.cy); }
	CSize operator - () const { return CSize( -cx, -cy); }
	CPoint operator + ( POINT point) const { return CPoint( cx + point.x, cy + point.y); }
	CPoint operator - ( POINT point) const { return CPoint( cx - point.x, cy - point.y); }

	inline CRect operator + ( const RECT *lpRect) const;
	inline CRect operator - ( const RECT *lpRect) const;
};

class CRect : public RECT
{
public:
	CRect() { left = 0; top = 0; right = 0; bottom = 0; }
	CRect( int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
	CRect( const RECT& srcRect) { CopyRect(&srcRect); }
	CRect( LPCRECT lpSrcRect) { CopyRect(lpSrcRect); }
	CRect( POINT point, SIZE size) { right = (left = point.x) + size.cx; bottom = (top = point.y) + size.cy; }
	CRect( POINT topLeft, POINT bottomRight) { left = topLeft.x; top = topLeft.y; right = bottomRight.x; bottom = bottomRight.y; }

	int Width() const { return right - left; }
	int Height() const { return bottom - top; }
	CSize Size() const { return CSize( right - left, bottom - top); }

	CPoint& TopLeft() { return *((CPoint *) this); }
	CPoint& BottomRight() { return *((CPoint *) this + 1); }
	const CPoint& TopLeft() const { return *((const CPoint *) this); }
	const CPoint& BottomRight() const { return *((const CPoint *) this + 1); }
	CPoint CenterPoint() const { return CPoint( (left + right) / 2, (top + bottom) / 2); }

	operator LPRECT() { return this; }
	operator LPCRECT() const { return this; }

	BOOL IsRectEmpty() const { return left >= right || top >= bottom; }
	BOOL IsRectNull() const { return left == 0 && right == 0 && top == 0 && bottom == 0; }
	BOOL PtInRect( POINT point) const { return point.x >= left && point.x < right && point.y >= top && point.y < bottom; }

	void SetRect( int x1, int y1, int x2, int y2) { left = x1; top = y1; right = x2; bottom = y2; }
	void SetRect( POINT topLeft, POINT bottomRight) { SetRect( topLeft.x, topLeft.y, bottomRight.x, bottomRight.y); }
	void SetRectEmpty() { left = top = right = bottom = 0; }
	void CopyRect( LPCRECT lpSrcRect) { left = lpSrcRect->left; top = lpSrcRect->top; right = lpSrcRect->right; bottom = lpSrcRect->bottom; }
	BOOL EqualRect( LPCRECT lpRect) const { return left == lpRect->left && top == lpRect->top && right == lpRect->right && bottom == lpRect->bottom; }

	void InflateRect( int x, int y) { left -= x; top -= y; right += x; bottom += y; }
	void InflateRect( SIZE size) { InflateRect( size.cx, size.cy); }
	void InflateRect( LPCRECT lpRect) { left -= lpRect->left; top -= lpRect->top; right += lpRect->right; bottom += lpRect->bottom; }
	void InflateRect( int l, int t, int r, int b) { left -= l; top -= t; right += r; bottom += b; }
	void DeflateRect( int x, int y) { InflateRect( -x, -y); }
	void DeflateRect( SIZE size) { InflateRect( -size.cx, -size.cy); }
	void DeflateRect( LPCRECT lpRect) { left += lpRect->left; top += lpRect->top; right -= lpRect->right; bottom -= lpRect->bottom; }
	void DeflateRect( int l, int t, int r, int b) { left += l; top += t; right -= r; bottom -= b; }

	void OffsetRect( int x, int y) { left += x; top += y; right += x; bottom += y; }
	void OffsetRect( SIZE size) { OffsetRect( size.cx, size.cy); }
	void OffsetRect( POINT point) { OffsetRect( point.x, point.y); }

	void MoveToY( int y) { bottom = Height() + y; top = y; }
	void MoveToX( int x) { right = Width() + x; left = x; }
	void MoveToXY( int x, int y) { MoveToX(x); MoveToY(y); }
	void MoveToXY( POINT point) { MoveToXY( point.x, point.y); }

	void NormalizeRect()
	{
		int nTemp;

		if( left > right )
		{
			nTemp = left;
			left = right;
			right = nTemp;
		}

		if( top > bottom )
		{
			nTemp = top;
			top = bottom;
			bottom = nTemp;
		}
	}

	// Like Win32: an empty result is all zeros and returns FALSE.
	BOOL IntersectRect( LPCRECT lpRect1, LPCRECT lpRect2)
	{
		CRect rect(
			lpRect1->left > lpRect2->left ? lpRect1->left : lpRect2->left,
			lpRect1->top > lpRect2->top ? lpRect1->top : lpRect2->top,
			lpRect1->right < lpRect2->right ? lpRect1->right : lpRect2->right,
			lpRect1->bottom < lpRect2->bottom ? lpRect1->bottom : lpRect2->bottom);

		if( CRect(lpRect1).IsRectEmpty() || CRect(lpRect2).IsRectEmpty() || rect.IsRectEmpty() )
		{
			SetRectEmpty();
			return FALSE;
		}

		*this = rect;
		return TRUE;
	}

	BOOL UnionRect( LPCRECT lpRect1, LPCRECT lpRect2)
	{
		BOOL bEmpty1 = CRect(lpRect1).IsRectEmpty();
		BOOL bEmpty2 = CRect(lpRect2).IsRectEmpty();

		if( bEmpty1 && bEmpty2 )
		{
			SetRectEmpty();
			return FALSE;
		}

		if(bEmpty1)
			CopyRect(lpRect2);
		else if(bEmpty2)
			CopyRect(lpRect1);
		else
			SetRect(
				lpRect1->left < lpRect2->left ? lpRect1->left : lpRect2->left,
				lpRect1->top < lpRect2->top ? lpRect1->top : lpRect2->top,
				lpRect1->right > lpRect2->right ? lpRect1->right : lpRect2->right,
				lpRect1->bottom > lpRect2->bottom ? lpRect1->bottom : lpRect2->bottom);

		return TRUE;
	}

	BOOL SubtractRect( LPCRECT lpRectSrc1, LPCRECT lpRectSrc2) { return ::SubtractRect( this, lpRectSrc1, lpRectSrc2); }

	void operator = ( const RECT& srcRect) { CopyRect(&srcRect); }
	BOOL operator == ( const RECT& rect) const { return EqualRect(&rect); }
	BOOL operator != ( const RECT& rect) const { return !EqualRect(&rect); }
	void operator += ( POINT point) { OffsetRect( point.x, point.y); }
	void operator += ( SIZE size) { OffsetRect( size.cx, size.cy); }
	void operator += ( LPCRECT lpRect) { InflateRect(lpRect); }
	void operator -= ( POINT point) { OffsetRect( -point.x, -point.y); }
	void operator -= ( SIZE size) { OffsetRect( -size.cx, -size.cy); }
	void operator -= ( LPCRECT lpRect) { DeflateRect(lpRect); }
	void operator &= ( const RECT& rect) { IntersectRect( this, &rect); }
	void operator |= ( const RECT& rect) { UnionRect( this, &rect); }

	CRect operator + ( POINT pt) const { CRect rect(*this); rect.OffsetRect( pt.x, pt.y); return rect; }
	CRect operator - ( POINT pt) const { CRect rect(*this); rect.OffsetRect( -pt.x, -pt.y); return rect; }
	CRect operator + ( SIZE size) const { CRect rect(*this); rect.OffsetRect( size.cx, size.cy); return rect; }
	CRect operator - ( SIZE size) const { CRect rect(*this); rect.OffsetRect( -size.cx, -size.cy); return rect; }
	CRect operator + ( LPCRECT lpRect) const { CRect rect(this); rect.InflateRect(lpRect); return rect; }
	CRect operator - ( LPCRECT lpRect) const { CRect rect(this); rect.DeflateRect(lpRect); return rect; }
	CRect operator & ( const RECT& rect2) const { CRect rect; rect.IntersectRect( this, &rect2); return rect; }
	CRect operator | ( const RECT& rect2) const { CRect rect; rect.UnionRect( this, &rect2); return rect; }

	CRect MulDiv( int nMultiplier, int nDivisor) const
	{
		return CRect(
			::MulDiv( left, nMultiplier, nDivisor),
			::MulDiv( top, nMultiplier, nDivisor),
			::MulDiv( right, nMultiplier, nDivisor),
			::MulDiv( bottom, nMultiplier, nDivisor));
	}
};

inline CPoint CPoint::operator + ( SIZE size) const { return CPoint( x + size.cx, y + size.cy); }
inline CPoint CPoint::operator - ( SIZE size) const { return CPoint( x - size.cx, y - size.cy); }
inline CSize CPoint::operator - ( POINT point) const { return CSize( x - point.x, y - point.y); }
inline CRect CPoint::operator + ( const RECT *lpRect) const { CRect rect(lpRect); rect.OffsetRect( x, y); return rect; }
inline CRect CPoint::operator - ( const RECT *lpRect) const { CRect rect(lpRect); rect.OffsetRect( -x, -y); return rect; }
inline CRect CSize::operator + ( const RECT *lpRect) const { CRect rect(lpRect); rect.OffsetRect( cx, cy); return rect; }
inline CRect CSize::operator - ( const RECT *lpRect) const { CRect rect(lpRect); rect.OffsetRect( -cx, -cy); return rect; }

#endif // !defined __TTYPES_H
