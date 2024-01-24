#pragma once

#include "Lib/BlockQueue.h"
#include "Buffers.h"
#include "IClient.h"

struct IFilter;


class BaseClient : public IClient
{
protected:
	BaseClient(
			std::string_view name,
			BufferPool& bufferPool,
			uint numTxBuffers,
			std::unique_ptr<IFilter> pTxFilter);

	// If a TX filter is present it filters the buffer.
	// Returns true if m_pTxBuffer has to be sent now.
	bool PrepareSend(const Buffer* pBuffer);

	// Returns zero on success
	int OnDataSent();

	std::string_view m_name;
	BufferPool& m_bufferPool;
	Lib::BlockQueue<const Buffer*, 1> m_txQueue;
	Buffer* m_pRxBuffer{};
	const Buffer* m_pTxBuffer{};
	std::unique_ptr<IFilter> m_pTxFilter;
};
