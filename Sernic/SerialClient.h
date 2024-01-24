#pragma once

#include "BaseClient.h"


class SerialClient : public BaseClient
{
public:
	SerialClient(std::string_view name, uint baudrate, BufferPool& bufferPool);
	~SerialClient();

private:
	uint Open(std::span<WSAEVENT> events) override;
	int ProcessEvent(uint index, Buffer** ppRxBuffer) override;
	bool Send(const Buffer* pBuffer) override;

	void Cleanup();
	bool StartReceiving();
	int OnDataReceived(Buffer** ppRxBuffer);

	uint m_baudrate;
	HANDLE m_handle{ INVALID_HANDLE_VALUE };
	OVERLAPPED m_ovSend{};
	OVERLAPPED m_ovReceive{};
};
