#pragma once

#include "IClient.h"


class Runner : NonCopyable
{
public:
	explicit Runner(
			IClient& serialClient,
			IClient* pConsoleClient,
			IClient* pGdbClient,
			IClient* pRawClient);

	void Run();
	void Close();

private:
	IClient& m_serialClient;
	IClient* m_pConsoleClient;
	IClient* m_pGdbClient;
	IClient* m_pRawClient;
	WSAEVENT m_cancelEvent{ WSA_INVALID_EVENT };
};

