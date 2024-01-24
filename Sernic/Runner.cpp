#include "Runner.h"


Runner::Runner(
		IClient& serialClient,
		IClient* pConsoleClient,
		IClient* pGdbClient,
		IClient* pRawClient)
	: m_serialClient{ serialClient }
	, m_pConsoleClient{ pConsoleClient  }
	, m_pGdbClient{ pGdbClient }
	, m_pRawClient{ pRawClient }
{
}


void Runner::Run()
{
	WSADATA wsaData;

	if (WSAStartup(0x0202, &wsaData))
	{
		std::cerr << "WSAStartup failed\n";
		return;
	}

	WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS]{};
	std::span<WSAEVENT> dstEvents{ events };
	uint firstConsoleEventIndex{};
	uint firstGdbEventIndex{};
	uint firstRawEventIndex{};
	uint firstSerialEventIndex{};
	DWORD numEvents{};
	int numTelnetClients{};

	m_cancelEvent = WSACreateEvent();
	bool isOk{ m_cancelEvent != WSA_INVALID_EVENT };

	if (isOk)
	{
		dstEvents[numEvents++] = m_cancelEvent;
		dstEvents = dstEvents.subspan(numEvents);
	}
	else
		std::cerr << "Failed to create WSA event\n";

	if (isOk && m_pConsoleClient)
	{
		++numTelnetClients;
		firstConsoleEventIndex = numEvents;
		auto eventCount = m_pConsoleClient->Open(dstEvents);

		isOk = eventCount > 0;

		if (isOk)
		{
			numEvents += eventCount;
			dstEvents = dstEvents.subspan(eventCount);
		}
	}

	if (isOk && m_pGdbClient)
	{
		++numTelnetClients;
		firstGdbEventIndex = numEvents;
		auto eventCount = m_pGdbClient->Open(dstEvents);

		isOk = eventCount > 0;

		if (isOk)
		{
			numEvents += eventCount;
			dstEvents = dstEvents.subspan(eventCount);
		}
	}

	if (isOk && m_pRawClient)
	{
		++numTelnetClients;
		firstRawEventIndex = numEvents;
		auto eventCount = m_pRawClient->Open(dstEvents);

		isOk = eventCount > 0;

		if (isOk)
		{
			numEvents += eventCount;
			dstEvents = dstEvents.subspan(eventCount);
		}
	}

	if (isOk)
	{
		firstSerialEventIndex = numEvents;
		auto eventCount = m_serialClient.Open(dstEvents);

		isOk = eventCount > 0;

		if (isOk)
			numEvents += eventCount;
	}

	bool running{ isOk };

	while (running)
	{
		auto result
		{
			WSAWaitForMultipleEvents(
					numEvents,
					events,
					FALSE,		// wait for any event set
					1000,		// timeout in ms
					FALSE)		// not alertable
		};

		if (result != WSA_WAIT_TIMEOUT)
		{
			auto index{ result - WSA_WAIT_EVENT_0 };

			if (index == 0)
				running = false;
			else if (index >= firstSerialEventIndex)
			{
				// If data was received on serial port forward it to all the remaining ports

				Buffer* pBuffer{};
				auto bytesReceived{ m_serialClient.ProcessEvent(index - firstSerialEventIndex, &pBuffer) };

				if (bytesReceived > 0)
				{
					pBuffer->SetRefCount(numTelnetClients);

					if (m_pGdbClient && !m_pGdbClient->Send(pBuffer))
						running = false;

					if (m_pConsoleClient && !m_pConsoleClient->Send(pBuffer))
						running = false;

					if (m_pRawClient && !m_pRawClient->Send(pBuffer))
						running = false;
				}
				else if (bytesReceived < 0)
					running = false;
			}
			else if (m_pRawClient && index >= firstRawEventIndex)
			{
				// If data was received from raw console forward the data to serial port

				Buffer* pBuffer{};
				auto bytesReceived{ m_pRawClient->ProcessEvent(index - firstRawEventIndex, &pBuffer) };

				if (bytesReceived > 0)
				{
					if (!m_serialClient.Send(pBuffer))
						running = false;
				}
				else if (bytesReceived < 0)
					running = false;
			}
			else if (m_pGdbClient && index >= firstGdbEventIndex)
			{
				// If data was received from gdb forward the data to serial port

				Buffer* pBuffer{};
				auto bytesReceived{ m_pGdbClient->ProcessEvent(index - firstGdbEventIndex, &pBuffer) };

				if (bytesReceived > 0)
				{
					if (!m_serialClient.Send(pBuffer))
						running = false;
				}
				else if (bytesReceived < 0)
					running = false;
			}
			else if (m_pConsoleClient && index >= firstConsoleEventIndex)
			{
				// If data was received from console forward the data to serial port

				Buffer* pBuffer{};
				auto bytesReceived{ m_pConsoleClient->ProcessEvent(index - firstConsoleEventIndex, &pBuffer) };

				if (bytesReceived > 0)
				{
					if (!m_serialClient.Send(pBuffer))
						running = false;
				}
				else if (bytesReceived < 0)
					running = false;
			}
		}
	}

	WSACloseEvent(m_cancelEvent);
	m_cancelEvent = WSA_INVALID_EVENT;

	WSACleanup();
}


void Runner::Close()
{
	WSASetEvent(m_cancelEvent);
	std::cout << "Closing...\n";
}
