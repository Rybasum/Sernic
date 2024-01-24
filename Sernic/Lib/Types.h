#pragma once

#ifndef UNDER_TEST
import std;
#endif
using std::uint32_t;
using std::uint16_t;
using std::uint8_t;
using uint = unsigned int;
using namespace std::literals;

// Prevents the object being copied or moved
//
struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};
