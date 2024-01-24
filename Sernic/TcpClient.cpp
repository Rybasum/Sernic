#include <cassert>
#include "TcpClient.h"
#include <MSWSock.h>	// after TcpClient.h because it includes WinSock2.h
#include "IFilter.h"


namespace
{
	constexpr uint cNumTxBuffers{ 128 };

	enum class EventType
	{
		Connection,
		DataReceived,
		DataSent,
		_NumEvents
	};
}


TcpClient::TcpClient(
		std::string_view name,
		uint16_t port,
		BufferPool& bufferPool,
		std::unique_ptr<IFilter> pTxFilter)
	: BaseClient{ name, bufferPool, cNumTxBuffers, std::move(pTxFilter) }
	, m_port{ port }
{
}


TcpClient::~TcpClient()
{
	// TODO: CancelIo

	Cleanup();
}


void TcpClient::Cleanup()
{
	if (m_socketListen != INVALID_SOCKET)
	{
		closesocket(m_socketListen);
		m_socketListen = INVALID_SOCKET;
	}

	if (m_socketData != INVALID_SOCKET)
	{
		closesocket(m_socketData);
		m_socketData = INVALID_SOCKET;
	}

	if (m_ovListen.hEvent)
	{
		WSACloseEvent(m_ovListen.hEvent);
		m_ovListen.hEvent = NULL;
	}

	if (m_ovReceive.hEvent)
	{
		WSACloseEvent(m_ovReceive.hEvent);
		m_ovReceive.hEvent = NULL;
	}

	if (m_ovSend.hEvent)
	{
		WSACloseEvent(m_ovSend.hEvent);
		m_ovSend.hEvent = NULL;
	}
}


uint TcpClient::Open(std::span<WSAEVENT> events)
{
	m_socketListen = WSASocketW(
			AF_INET,
			SOCK_STREAM,
			IPPROTO_TCP,
			NULL,	// lpProtocolInfo
			0,		// group
			WSA_FLAG_OVERLAPPED);

	if (m_socketListen == INVALID_SOCKET)
	{
		std::cerr << "Failed to create listening socket\n";
		return 0;
	}

	int value{ 1 };
	setsockopt(m_socketListen, SOL_SOCKET, SO_REUSEADDR, (const char*)&value, sizeof value);
	setsockopt(m_socketListen, IPPROTO_TCP, TCP_NODELAY, (const char*)&value, sizeof value);

	value = 0;
	setsockopt(m_socketListen, SOL_SOCKET, SO_LINGER, (const char*)&value, sizeof value);

	// Bind the listen socket to localhost

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(m_port);

	if (bind(m_socketListen, (const sockaddr*)&addr, sizeof addr))
	{
		std::cerr << "Failed to bind to port " << m_port << std::endl;
		return 0;
	}

	// Allow one connection only
	//
	if (listen(m_socketListen, 1))
	{
		std::cerr << "Failed to listen on port " << m_port << std::endl;
		return 0;
	}

	m_ovListen.hEvent = WSACreateEvent();
	events[(int)EventType::Connection] = m_ovListen.hEvent;

	m_ovReceive.hEvent = WSACreateEvent();
	events[(int)EventType::DataReceived] = m_ovReceive.hEvent;

	m_ovSend.hEvent = WSACreateEvent();
	events[(int)EventType::DataSent] = m_ovSend.hEvent;

	if (!PrepareAccept())
		return 0;

	return (uint)EventType::_NumEvents;
}


bool TcpClient::PrepareAccept()
{
	m_socketData = WSASocketW(
			AF_INET,
			SOCK_STREAM,
			IPPROTO_TCP,
			NULL,	// lpProtocolInfo
			0,		// group
			WSA_FLAG_OVERLAPPED);

	if (m_socketData == INVALID_SOCKET)
	{
		std::cerr << "Failed to create accepting socket\n";
		return false;
	}

	// --- Start listening

	DWORD bytesReceived;

	// We set dwReceiveDataLength to disable reception of the initial
	// data from the remote client. If the call finishes synchronously
	// it returns TRUE and sets bytesReceived. Otherwise it returns FALSE
	// and sets the last error to ERROR_IO_PENDING (bytesReceived is not used).

	if (AcceptEx(
			m_socketListen,
			m_socketData,
			m_acceptBuffer.data(),
			0,		// dwReceiveDataLength
			cAcceptAddressSize,
			cAcceptAddressSize,
			&bytesReceived,
			&m_ovListen) == FALSE)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			std::cerr << "AcceptEx failed\n";
			return false;
		}
	}

	std::cout << m_name << " listening on port " << m_port << std::endl;

	return true;
}


bool TcpClient::StartReceiving(DWORD flags)
{
	m_pRxBuffer = m_bufferPool.GetBuffer();
	bool isOk{ !!m_pRxBuffer };

	if (isOk)
	{
		m_rxWsaBuf.buf = (char*)m_pRxBuffer->GetBufferPtr();
		m_rxWsaBuf.len = m_pRxBuffer->GetBufferSize();

		DWORD bytesReceived;

		if (WSARecv(
				m_socketData,
				&m_rxWsaBuf,
				1,		// WSA buffer count
				&bytesReceived,
				&flags,
				&m_ovReceive,
				NULL	// completion routine
				) == SOCKET_ERROR)
		{
			isOk = WSAGetLastError() == ERROR_IO_PENDING;
		}
	}

	if (!isOk)
		std::cerr << "Failed to receive on socket\n";

	return isOk;
}


bool TcpClient::Send(const Buffer* pBuffer)
{
	assert(pBuffer);
	bool isOk{ true };

	if (m_isConnected)
	{
		if (BaseClient::PrepareSend(pBuffer))
		{
			// Sender is ready, so send the buffer now

			m_txWsaBuf.buf = (char*)m_pTxBuffer->GetBufferPtr();
			m_txWsaBuf.len = m_pTxBuffer->GetDataSize();

			DWORD bytesSent;

			if (WSASend(
					m_socketData,
					&m_txWsaBuf,
					1,		// WSA buffer count
					&bytesSent,
					0,		// flags
					&m_ovSend,
					NULL	// completion routine
				) == SOCKET_ERROR)
			{
				isOk = WSAGetLastError() == ERROR_IO_PENDING;

				if (!isOk)
					std::cerr << "Failed to send to " << m_name << std::endl;
			}
		}
	}
	else
	{
		// Just free the buffer, data is lost

		m_bufferPool.PutBuffer(pBuffer);
	}

	return isOk;
}


int TcpClient::ProcessEvent(uint index, Buffer** ppRxBuffer)
{
	switch ((EventType)index)
	{
	case EventType::Connection:
		return OnConnection();

	case EventType::DataReceived:
		return OnDataReceived(ppRxBuffer);

	case EventType::DataSent:
		WSAResetEvent(m_ovSend.hEvent);
		return OnDataSent();
	}

	assert(false);
	return -1;
}


// Returns 0 on success, -1 on error.
//
int TcpClient::OnConnection()
{
	WSAResetEvent(m_ovListen.hEvent);

	DWORD bytesReceived;
	DWORD flags;

	if (!WSAGetOverlappedResult(
			m_socketListen,
			&m_ovListen,
			&bytesReceived,
			FALSE,		// don't wait
			&flags))
	{
		std::cerr << "WSAGetOverlappedResult failed for listening socket\n";
		return -1;
	}

	// We're connected
	//
	m_isConnected = true;
	std::cout << m_name << " port " << m_port << " connected.\n";

	return StartReceiving(flags) ? 0 : -1;
}


int TcpClient::OnDataReceived(Buffer** ppRxBuffer)
{
	assert(m_pRxBuffer);

	WSAResetEvent(m_ovReceive.hEvent);

	int bytesReceived{ -1 };
	DWORD flags;

	if (!WSAGetOverlappedResult(
			m_socketData,
			&m_ovReceive,
			(DWORD*)&bytesReceived,
			FALSE,		// don't wait
			&flags))
	{
		std::cerr << "WSAGetOverlappedResult failed for data socket\n";
		return -1;
	}

	if (bytesReceived > 0)
	{
		m_pRxBuffer->SetDataSize(bytesReceived);
		*ppRxBuffer = m_pRxBuffer;
		m_pRxBuffer = {};	// the caller now owns the buffer

		if (m_isConnected && !StartReceiving(0))
			bytesReceived = -1;
	}
	else
	{
		// Client has closed the connection

		closesocket(m_socketData);
		m_bufferPool.PutBuffer(m_pRxBuffer);

		m_isConnected = false;
		std::cout << m_name << " port disconnected.\n";

		// Make a new accept socket and issue AcceptEx
		//
		bytesReceived = PrepareAccept() ? 0 : -1;
	}

	return bytesReceived;
}
