#include "SerialClient.h"
#include <cassert>
#include "IFilter.h"


namespace
{
	enum class EventType
	{
		Send,
		Receive,
		_NumEvents
	};

	constexpr uint cNumTxBuffers{ 256 };
}


SerialClient::SerialClient(std::string_view name, uint baudrate, BufferPool& bufferPool)
	: BaseClient{ name, bufferPool, cNumTxBuffers, {} }
	, m_baudrate{ baudrate }
{
}


SerialClient::~SerialClient()
{
	Cleanup();
}


void SerialClient::Cleanup()
{
	if (m_handle != INVALID_HANDLE_VALUE)
	{
		CancelIo(m_handle);
		CloseHandle(m_handle);

		m_handle = INVALID_HANDLE_VALUE;
	}

	if (m_ovSend.hEvent)
	{
		CloseHandle(m_ovSend.hEvent);
		m_ovSend.hEvent = NULL;
	}

	if (m_ovReceive.hEvent)
	{
		CloseHandle(m_ovReceive.hEvent);
		m_ovReceive.hEvent = NULL;
	}
}


uint SerialClient::Open(std::span<WSAEVENT> events)
{
	m_handle = CreateFileA(
			m_name.data(),
			GENERIC_READ | GENERIC_WRITE,
			0,			// exclusive access
			NULL,		// security attributes
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL);		// hTemplate

	if (m_handle == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Failed to open " << m_name;

		switch (GetLastError())
		{
		case ERROR_FILE_NOT_FOUND:
			std::cerr << " - port not found.";
			break;

		case ERROR_ACCESS_DENIED:
			std::cerr << " - port is in use.";
			break;
		}

		std::cerr << std::endl;

		return 0;
	}

	DCB dcb
	{
		.DCBlength = sizeof(DCB)
	};

	if (!GetCommState(m_handle, &dcb))
	{
		std::cerr << "Failed to query " << m_name << std::endl;
		return 0;
	}

	dcb.BaudRate = m_baudrate;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	if (!SetCommState(m_handle, &dcb))
	{
		std::cerr << "Failed to set baud rate for " << m_name << std::endl;
		return 0;
	}

	// Timeout behaviuor:
	// - Wait indefinitely for the first byte to arrive (total timeout disabled)
	// - When the first byte arrives, keep on receiving and measure inter-byte interval
	// - Finish when the buffer is full or there was no data for more than the interval
	//
	COMMTIMEOUTS timeouts
	{
		.ReadIntervalTimeout = 5,
		.ReadTotalTimeoutMultiplier = 0,
		.ReadTotalTimeoutConstant = 0,
		.WriteTotalTimeoutMultiplier = 0,
		.WriteTotalTimeoutConstant = 0
	};

	if (!SetCommTimeouts(m_handle, &timeouts))
	{
		std::cerr << "Failed to configure " << m_name << std::endl;
		return 0;
	}

	m_ovSend.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	events[(int)EventType::Send] = m_ovSend.hEvent;

	m_ovReceive.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	events[(int)EventType::Receive] = m_ovReceive.hEvent;

	std::cout << m_name << " port open\n";

	if (!StartReceiving())
		return 0;

	return (uint)EventType::_NumEvents;
}


bool SerialClient::StartReceiving()
{
	m_pRxBuffer = m_bufferPool.GetBuffer();
	bool isOk{ !!m_pRxBuffer };

	if (isOk)
	{
		// NOTE: Because we set lpNumberOfBytesRead to NULL, if the
		// call finishes synchronously it will still fire the event.
		// This function is only valid in Windows 10 or above.
		//
		if (!ReadFile(
			m_handle,
			m_pRxBuffer->GetBufferPtr(),
			m_pRxBuffer->GetBufferSize(),
			NULL,		// lpNumberOfBytesRead
			&m_ovReceive))
		{
			isOk = GetLastError() == ERROR_IO_PENDING;
		}
	}

	if (!isOk)
		std::cerr << "Failed to receive from " << m_name << std::endl;

	return isOk;
}


bool SerialClient::Send(const Buffer* pBuffer)
{
	assert(pBuffer);
	bool isOk{ true };

	if (BaseClient::PrepareSend(pBuffer))
	{
		// Sender is ready, so send the buffer now

		if (!WriteFile(
				m_handle,
				m_pTxBuffer->GetBufferPtr(),
				m_pTxBuffer->GetDataSize(),
				NULL,		// lpNumberOfBytesWritten
				&m_ovSend))
		{
			isOk = GetLastError() == ERROR_IO_PENDING;

			if (!isOk)
				std::cerr << "Failed to send to " << m_name << std::endl;
		}
	}

	return isOk;
}


int SerialClient::ProcessEvent(uint index, Buffer** ppRxBuffer)
{
	switch ((EventType)index)
	{
	case EventType::Send:		// finished sending a buffer
		ResetEvent(m_ovSend.hEvent);
		return OnDataSent();

	case EventType::Receive:	// finished receiving
		ResetEvent(m_ovReceive.hEvent);
		return OnDataReceived(ppRxBuffer);

	default:
		std::cerr << "BUG: SerialClient::ProcessEvent\n";
	}

	return -1;
}


int SerialClient::OnDataReceived(Buffer** ppRxBuffer)
{
	assert(m_pRxBuffer);

	int bytesReceived{ -1 };

	if (GetOverlappedResult(
			m_handle,
			&m_ovReceive,
			(DWORD*)&bytesReceived,
			FALSE))			// don't wait
	{
		if (bytesReceived > 0)
		{
			m_pRxBuffer->SetDataSize((size_t)bytesReceived);
			*ppRxBuffer = m_pRxBuffer;
			m_pRxBuffer = {};	// the caller now owns the buffer
		}
		else
		{
			// Weird...
			m_bufferPool.PutBuffer(m_pRxBuffer);
		}

		if (!StartReceiving())
			bytesReceived = -1;
	}

	return bytesReceived;
}
