#pragma once

#include "Buffers.h"


struct IFilter : NonCopyable
{
	// Submits a data buffer for filtering.
	// Returns the number of accumulated result buffers
	// that are ready to be sent (may be zero).
	virtual uint Process(const Buffer* pBuffer) = 0;

	// Gets a buffer with filtered data
	virtual const Buffer* GetResult() = 0;
};
