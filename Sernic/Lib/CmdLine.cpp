#include "CmdLine.h"


namespace
{
	// Returns:
	// 0: success
	// 1: text was empty
	// -1: error (text is not a valid uint)
	// value is only updated on success
	//
	int ToUint(std::string_view text, uint32_t& value, int radix)
	{
		int result{ 1 };

		if (!text.empty())
		{
			result = -1;
			char* pEnd{};
			auto val32{ std::strtoul(text.data(), &pEnd, radix) };

			if (*pEnd == 0)
			{
				value = val32;
				result = 0;
			}
		}

		return result;
	}


	// Returns:
	// 0: success
	// 1: text was empty
	// -1: error (text is not a valid uint)
	// value is only updated on success
	//
	int ToUint(std::string_view text, uint16_t& value, int radix)
	{
		uint32_t val32{};
		int result{ ToUint(text, val32, radix) };

		if (result == 0)
		{
			if (val32 <= (uint32_t)std::numeric_limits<uint16_t>::max)
				value = (uint16_t)val32;
			else
				result = -1;
		}

		return result;
	}


	void OptionError(std::string_view name)
	{
		std::cerr << "Invalid value for option -" << name << std::endl;
	}
}


CmdLine::CmdLine(int argc, char* argv[], std::initializer_list<std::string_view> validOptions)
	: m_validOptions{ validOptions }
{
	m_isOk = Parse(argc, argv);

#ifdef DIAGNOSTIC
	for (auto pair : m_options)
		std::cout << "-" << pair.first << " " << pair.second << "\n";

	for (auto str : m_arguments)
		std::cout << str << "\n";
#endif
}


bool CmdLine::Parse(int argc, char* argv[])
{
	std::string_view optName{};
	bool isOk{ true };

	m_progName = argv[0];

	for (int arg = 1; arg < argc && isOk; ++arg)
	{
		const char* pText = argv[arg];

		if (pText[0] == '-' || pText[0] == '/')
		{
			optName = pText + 1;
			isOk = !optName.empty() && m_validOptions.contains(optName);

			if (isOk)
				m_options[optName] = {};
		}
		else
		{
			if (!optName.empty())
			{
				m_options[optName] = pText;
				optName = {};
			}
			else
				m_arguments.push_back(pText);
		}
	}

	return isOk;
}


bool CmdLine::GetArgument(int index, uint32_t& value, int radix) const
{
	return ToUint(GetArgument(index), value, radix) == 0;
}


bool CmdLine::GetArgument(int index, uint16_t& value, int radix) const
{
	return ToUint(GetArgument(index), value, radix) == 0;
}


std::string_view CmdLine::GetOption(std::string_view name) const
{
	auto iter = m_options.find(name);

	return iter != m_options.end() ? iter->second : ""sv;
}


bool CmdLine::GetOption(std::string_view name, uint32_t& value, uint32_t defValue, int radix) const
{
	uint32_t val32;

	switch (ToUint(GetOption(name), val32, radix))
	{
	case 0:
		value = val32;
		return true;

	case 1:
		value = defValue;
		return true;
	}

	OptionError(name);
	return false;
}


bool CmdLine::GetOption(std::string_view name, uint16_t& value, uint16_t defValue, int radix) const
{
	uint16_t val16;

	switch (ToUint(GetOption(name), val16, radix))
	{
	case 0:
		value = val16;
		return true;

	case 1:
		value = defValue;
		return true;
	}

	OptionError(name);
	return false;
}


bool CmdLine::GetOption(std::string_view name, uint32_t& value, int radix) const
{
	uint32_t val32;

	if (ToUint(GetOption(name), val32, radix) == 0)
	{
		value = val32;
		return true;
	}

	OptionError(name);
	return false;
}


bool CmdLine::GetOption(std::string_view name, uint16_t& value, int radix) const
{
	uint16_t val16;

	if (ToUint(GetOption(name), val16, radix) == 0)
	{
		value = val16;
		return true;
	}

	OptionError(name);
	return false;
}
