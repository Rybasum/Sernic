#pragma once

#include "Types.h"
#include <cassert>


namespace Lib
{
	template <typename T>
	class Buffer
	{
	public:
		Buffer(size_t length)
			: m_buffer{ new T[length], length }
		{
		}

		~Buffer()
		{
			delete[] m_buffer.data();
		}

		// Clear data from the buffer
		void Clear() { m_dataLength = 0; }

		bool IsEmpty() const { return m_dataLength == 0; }

		// Returns the length of the free space (in items)
		auto GetFreeLength() const { return m_buffer.size() - m_dataLength; }

		// Returns a span containing data
		std::span<const T> GetDataSpan() const { return m_buffer.first(m_dataLength); }

		// Returns a span of the free space
		auto GetFreeSpan() const { return m_buffer.subspan(m_dataLength); }

		// Decreases free space by the given length
		void Consume(size_t length) { m_dataLength += length; }

		// Copies a data item at the end of the buffer
		void Append(const T& data)
		{
			assert(GetFreeLength() >= sizeof(T));
			m_buffer[m_dataLength++] = data;
		}

	private:
		const std::span<T> m_buffer;
		size_t m_dataLength{};
	};
}
