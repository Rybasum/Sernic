#include <cassert>
#include "IFilter.h"
#include "BaseClient.h"


BaseClient::BaseClient(
		const std::string_view name,
		BufferPool& bufferPool,
		const uint numTxBuffers,
		std::unique_ptr<IFilter> pTxFilter)
	: m_name{ name }
	, m_bufferPool{ bufferPool }
	, m_txQueue{ numTxBuffers }
	, m_pTxFilter{ std::move(pTxFilter) }
{
}


// If a TX filter is present it filters the buffer.
// Returns true if m_pTxBuffer has to be sent now.
//
bool BaseClient::PrepareSend(const Buffer* pBuffer)
{
	assert(pBuffer);
	bool shouldSendNow{ !m_pTxBuffer };	// don't send if sender is busy

	if (m_pTxFilter)
	{
		uint numFilteredBuffers{ m_pTxFilter->Process(pBuffer) };
		shouldSendNow &= numFilteredBuffers > 0;

		if (shouldSendNow)
		{
			m_pTxBuffer = m_pTxFilter->GetResult();
			--numFilteredBuffers;
		}

		// Any remaining filtered buffers are queued for later
		//
		while (numFilteredBuffers--)
			m_txQueue.Enqueue(m_pTxFilter->GetResult());
	}
	else
	{
		if (shouldSendNow)
			m_pTxBuffer = pBuffer;
		else
			m_txQueue.Enqueue(pBuffer);
	}

	return shouldSendNow;
}


// Returns zero on success
//
int BaseClient::OnDataSent()
{
	assert(m_pTxBuffer);

	m_bufferPool.PutBuffer(m_pTxBuffer);
	m_pTxBuffer = {};

	// Send the next queued buffer (if any)

	if (const Buffer* pBuffer; m_txQueue.Dequeue(&pBuffer))
		return Send(pBuffer) ? 0 : -1;

	return 0;
}
