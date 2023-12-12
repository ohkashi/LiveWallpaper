#pragma once

#include "resource.h"
#include "MFPVideoPlayer.h"


inline LONG Width(const RECT& r)
{
	return r.right - r.left;
}

inline LONG Height(const RECT& r)
{
	return r.bottom - r.top;
}

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}
