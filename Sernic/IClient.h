#pragma once

#include <WinSock2.h>
#include "Buffers.h"


struct IClient : NonCopyable
{
	// Opens the client and adds events to the given span.
	// Returns the number of events added, or 0 on error.
	virtual uint Open(std::span<WSAEVENT> events) = 0;

	// Processes the indexed event.
	// If data was received it sets ppRxBuffer and returns > 0.
	// The caller must later return the buffer to the buffer pool.
	// Returns 0 if no data was received or -1 on error.
	virtual int ProcessEvent(uint index, Buffer** ppRxBuffer) = 0;

	// Starts sending the data and returns true on success.
	// When the buffer is finished it will be returned to the buffer pool.
	virtual bool Send(const Buffer* pBuffer) = 0;
};
