#ifndef __WNDMATH_H__
#define __WNDMATH_H__

#ifdef WIN32

class WinPoint : public tagPOINT
{
public:
	inline WinPoint(int x_ = 0, int y_ = 0) { x = x_; y = y_; }
	inline WinPoint(const POINT& pt) { x = pt.x; y = pt.y; }
};

class WinSize : public tagSIZE
{
public:
	inline WinSize(int cx_ = 0, int cy_ = 0) { cx = cx_; cy = cy_; }
	inline WinSize(const SIZE& sz) { cx = sz.cx; cy = sz.cy; }
};

class WinRect : public tagRECT
{
public:
	inline WinRect(int left_ = 0, int top_ = 0, int right_ = 0, int bottom_ = 0) { left = left_; top = top_; right = right_; bottom = bottom_; }
	inline WinRect(const RECT& rc) { left = rc.left; top = rc.top; right = rc.right; bottom = rc.bottom; }

public:
	inline int Width() const { return right - left; }
	inline int Height() const { return bottom - top; }

public:
	inline void Expand(const int x, const int y) { left -= x; top -= y; right += x; bottom += y; }
	inline void Shrink(const int x, const int y) { left += x; top += y; right -= x; bottom -= y; }
	inline void Expand(const int v) { Expand(v, v); }
	inline void Shrink(const int v) { Shrink(v, v); }
};

#endif

#endif
