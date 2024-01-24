#pragma once

#include "BaseClient.h"

struct IFilter;


class TcpClient : public BaseClient
{
public:
	explicit TcpClient(
			std::string_view name,
			uint16_t port,
			BufferPool& bufferPool,
			std::unique_ptr<IFilter> pTxFilter = {});
	~TcpClient();

protected:
	int ProcessEvent(uint index, Buffer** ppRxBuffer) override;
	bool Send(const Buffer* pBuffer) override;

private:
	uint Open(std::span<WSAEVENT> events) override;

	bool PrepareAccept();
	bool StartReceiving(DWORD flags);
	int OnConnection();
	int OnDataReceived(Buffer** ppRxBuffer);
	void Cleanup();

	// The value of dwLocalAddressLength and dwRemoteAddressLength
	// for the AcceptEx call (as prescribed in the doc)
	static constexpr size_t cAcceptAddressSize{ sizeof(SOCKADDR_IN) + 16 };

	// The accept buffer must accommodate at least two address structures.
	// We will not receive the initial client data, so this is enough.
	static constexpr size_t cAcceptBufferSize{ 2 * cAcceptAddressSize };

	SOCKET m_socketListen{ INVALID_SOCKET };
	SOCKET m_socketData{ INVALID_SOCKET };
	OVERLAPPED m_ovListen{};
	OVERLAPPED m_ovReceive{};
	OVERLAPPED m_ovSend{};
	std::array<uint8_t, cAcceptBufferSize> m_acceptBuffer{};
	WSABUF m_rxWsaBuf{};
	WSABUF m_txWsaBuf{};

	uint16_t m_port;
	bool m_isConnected{};
};
