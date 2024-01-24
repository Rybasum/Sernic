#pragma once

#include "Types.h"


namespace Lib
{
	// Buffer for N bytes
	//
	template <size_t N>
	class ByteBuffer : NonCopyable
	{
	public:
		static constexpr size_t cSize{ N };

		auto* GetBufferPtr() const { return m_buffer.data(); }
		auto* GetBufferPtr() { return m_buffer.data(); }
		auto GetBufferSize() const { return m_buffer.size(); }
		auto GetBuffer() { return std::span{ m_buffer }; }
		auto GetData() const { return std::span<const uint8_t>{ m_buffer.data(), m_dataSize }; }
		auto GetData() { return std::span<uint8_t>{ m_buffer.data(), m_dataSize }; }
		auto GetDataSize() const { return m_dataSize; }
		void SetDataSize(size_t size) { m_dataSize = size; }
		auto GetFree() { return std::span<uint8_t>{ m_buffer.data() + m_dataSize, cSize - m_dataSize }; }

	protected:
		size_t m_dataSize;
		std::array<uint8_t, N> m_buffer;
	};


	template <size_t BufSize>
	class ByteBufferPool;


	// Buffer for N bytes, with reference counter
	//
	template <size_t N>
	class ManagedByteBuffer : public ByteBuffer<N>
	{
	public:
		void SetRefCount(int refCount) { m_refCount = refCount; }

	private:
		int m_refCount;

		friend ByteBufferPool<N>;
	};
}
