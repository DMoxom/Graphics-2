#pragma once

// Various helper functions

using namespace std;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw exception();
	}
}
