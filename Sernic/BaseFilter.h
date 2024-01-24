#pragma once

#include "Lib/BlockQueue.h"
#include "Lib/Buffer.h"
#include "IFilter.h"


class BaseFilter : public IFilter
{
public:
	explicit BaseFilter(BufferPool& bufferPool, uint maxPassBuffers, size_t maxRejectSize);
	virtual ~BaseFilter() {}

	const Buffer* GetResult() override;

protected:
	// Packs all the data from the reject buffer into buffers and adds
	// them to the pass queue, then clears the reject buffer.
	void UndoReject();

	BufferPool& m_bufferPool;
	Lib::BlockQueue<const Buffer*, 1> m_passQueue;	// to be sent to output
	Lib::Buffer<uint8_t> m_rejectBuffer;
};
