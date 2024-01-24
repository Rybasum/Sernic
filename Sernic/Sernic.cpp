#include <csignal>
#include "Lib/CmdLine.h"
#include "SerialClient.h"
#include "TcpClient.h"
#include "GdbOutputFilter.h"
#include "Runner.h"
#include "Defs.h"


namespace
{
	constexpr auto cLogo{ "Serial-Network Inter-Connector v1.0\n"sv };
	void Usage(std::string_view progName);
}


int main(int argc, char* argv[])
{
	CmdLine cmdLine{ argc, argv, { "h"sv, "c"sv, "g"sv, "r"sv}};

	if (cmdLine.GetNumArguments() == 0 && cmdLine.GetNumOptions() == 0 && cmdLine.HasOption("h"sv))
	{
		Usage(cmdLine.GetProgName());
		return 0;
	}

	if (!cmdLine.IsOk() || cmdLine.GetNumArguments() != 1 || cmdLine.GetNumOptions() < 1)
	{
		Usage(cmdLine.GetProgName());
		return -1;
	}

	const auto comText{ "COM"sv };
	std::string_view comPort{ cmdLine.GetArgument(0) };

	uint32_t baudRate{ 115200 };
	char* pEnd{};
	bool isOk{ comPort.starts_with(comText) };

	if (isOk)
	{
		auto portText{ comPort };
		portText.remove_prefix(comText.size());
		
		auto portNum{ std::strtoul(portText.data(), &pEnd, 10) };

		isOk = (*pEnd == ':' || *pEnd == 0) && portNum > 0 && portNum < 99;
	}

	if (!isOk)
	{
		std::cerr << "Invalid COM port\n";
		return -1;
	}

	if (*pEnd == ':')
	{
		*pEnd = 0;	// null-terminate the COMxx string (HACK: should be const!)
		comPort = comPort.substr(0, pEnd - comPort.data());
		baudRate = std::strtoul(pEnd + 1, &pEnd, 10);

		if (*pEnd != 0 || baudRate < 100 || baudRate > 5000000)
		{
			std::cerr << "Invalid baud rate value\n";
			return -1;
		}
	}

	uint16_t portConsole{};

	if (cmdLine.HasOption("c"sv) && (!cmdLine.GetOption("c"sv, portConsole) || portConsole) == 0)
	{
		std::cerr << "Invalid value for the console port\n";
		return -1;
	}

	uint16_t portGdb{};

	if (cmdLine.HasOption("g"sv) && (!cmdLine.GetOption("g"sv, portGdb) || portGdb) == 0)
	{
		std::cerr << "Invalid value for the gdb port\n";
		return -1;
	}

	uint16_t portRaw{};

	if (cmdLine.HasOption("r"sv) && (!cmdLine.GetOption("r"sv, portRaw) || portRaw) == 0)
	{
		std::cerr << "Invalid value for the raw console port\n";
		return -1;
	}

	std::cout << cLogo;

	BufferPool bufferPool{ cNumBuffers };
	SerialClient serialClient{ comPort, baudRate, bufferPool };

	std::unique_ptr<TcpClient> pConsoleClient{};
	std::unique_ptr<TcpClient> pGdbClient{};
	std::unique_ptr<TcpClient> pRawClient{};

	if (portConsole)
	{
		auto pGdbOutFilter = std::make_unique<GdbOutputFilter>(bufferPool);
		pConsoleClient = std::make_unique<TcpClient>("Console"sv, portConsole, bufferPool, std::move(pGdbOutFilter));
	}

	if (portGdb)
		pGdbClient = std::make_unique<TcpClient>("GDB"sv, portGdb, bufferPool);

	if (portRaw)
		pRawClient = std::make_unique<TcpClient>("Raw console"sv, portRaw, bufferPool);

	Runner runner{ serialClient, pConsoleClient.get(), pGdbClient.get(), pRawClient.get()};
	static Runner* s_pRunner{ &runner };

	std::signal(SIGINT, [](int) { s_pRunner->Close(); });

	runner.Run();

	return 0;
}


namespace
{
	void Usage(const std::string_view progName)
	{
		auto name{ std::filesystem::path{ progName }.stem().string() };

		std::cout << cLogo;
		std::cout << "\nUsage:\n\n";
		std::cout << name << " COMx[:baudrate] [-c portConsole] [-g portGdb] [-r portRaw]\n\n";
		std::cout << "where\n";
		std::cout << "\tCOMx - serial port for kgdb connection\n";
		std::cout << "\tbaudrate - baud rate (default 115200)\n\n";
		std::cout << "\tportConsole - port number on localhost for console (telnet)\n";
		std::cout << "\tportGdb - port number on localhost for gdb\n";
		std::cout << "\tportRaw - port number on localhost for unfiltered console\n";
		std::cout << "Example:\n\n";
		std::cout << name << " COM3:115200 -c 4321 -g 4322 -r 4323\n\n";
	}
}
