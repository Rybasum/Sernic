#pragma once

#include "Types.h"

// Command line structure
// ----------------------
// 
// Any arguments must come first, followed by options.
// An option is a string preceded by '-' or '/'.
// The option string may be optionally followed by a value.
//
// progname arg1 arg2 ... argN -opt1 -opt2 value2 -opt3 /opt4 value4

class CmdLine
{
public:
	CmdLine(int argc, char* argv[], std::initializer_list<std::string_view> validOptions);

	bool IsOk() const { return m_isOk; }
	const std::string& GetProgName() const { return m_progName; }

	// Returns the number of arguments
	int GetNumArguments() const { return (int)m_arguments.size(); }

	// Returns the indexed argument as text
	std::string_view GetArgument(int index) const { return m_arguments[index]; }

	// Converts the indexed argument to uint32_t and returns:
	// true - on success
	// false - the value can't be converted to uint32_t
	bool GetArgument(int index, uint32_t& value, int radix = 0) const;

	// Converts the indexed argument to uint32_t and returns:
	// true - on success
	// false - the value can't be converted to uint32_t
	bool GetArgument(int index, uint16_t& value, int radix = 0) const;

	// Returns all the arguments as a vector of strings
	const auto& GetArguments() const { return m_arguments; }

	// Returns the number of options
	int GetNumOptions() const { return (int)m_options.size(); }

	// Returns true if the given option is present
	bool HasOption(std::string_view name) const { return m_options.contains(name); }

	// Returns the value of an option (empty if the option is not present or has no value)
	std::string_view GetOption(std::string_view name) const;

	// Converts the option's value to uint32_t and returns:
	// true - on success or when the option is not present (then the default value is used)
	// false - option is present but its value is invalid or missing
	bool GetOption(std::string_view name, uint32_t& value, uint32_t defValue, int radix = 0) const;

	// Converts the option's value to uint16_t and returns:
	// true - on success or when the option is not present (then the default value is used)
	// false - option is present but its value is invalid or missing
	bool GetOption(std::string_view name, uint16_t& value, uint16_t defValue, int radix = 0) const;

	// Converts the option's value to uint32_t and returns:
	// true - on success
	// false - the option is not present or its value is invalid or missing
	bool GetOption(std::string_view name, uint32_t& value, int radix = 0) const;

	// Converts the option's value to uint16_t and returns:
	// true - on success
	// false - the option is not present or its value is invalid or missing
	bool GetOption(std::string_view name, uint16_t& value, int radix = 0) const;

private:
	bool Parse(int argc, char* argv[]);

	std::string								m_progName;
	std::map<std::string_view, std::string>	m_options;
	std::vector<std::string>				m_arguments;
	const std::set<std::string_view>		m_validOptions;
	bool									m_isOk;
};

