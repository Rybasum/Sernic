#include "BaseFilter.h"


BaseFilter::BaseFilter(BufferPool& bufferPool, uint maxPassBuffers, size_t maxRejectSize)
	: m_bufferPool{ bufferPool }
	, m_passQueue{ maxPassBuffers }
	, m_rejectBuffer{ maxRejectSize }
{
}


// Returns one filtered buffer or nullptr
//
const Buffer* BaseFilter::GetResult()
{
	if (const Buffer* pResult{}; m_passQueue.Dequeue(&pResult))
		return pResult;

	return {};
}


void BaseFilter::UndoReject()
{
	auto srcData{ m_rejectBuffer.GetDataSpan() };
	auto* pSrc{ srcData.data() };
	size_t chunkSize{};

	for (auto remainingBytes{ srcData.size() }; remainingBytes; remainingBytes -= chunkSize, pSrc += chunkSize)
	{
		auto* pBuffer{ m_bufferPool.GetBuffer() };

		chunkSize = std::min(remainingBytes, Buffer::cSize);
		std::memcpy(pBuffer->GetBufferPtr(), pSrc, chunkSize);
		pBuffer->SetDataSize(chunkSize);

		m_passQueue.Enqueue(pBuffer);
	}

	m_rejectBuffer.Clear();
}
