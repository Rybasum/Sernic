#pragma once

#include "ByteBuffer.h"
#include "Pool.h"


namespace Lib
{
	// A pool of ManagedByteBuffer objects
	//
	template <size_t BufSize>
	class ByteBufferPool : Lib::Pool<Lib::ManagedByteBuffer<BufSize>>
	{
		using Base = Lib::Pool<Lib::ManagedByteBuffer<BufSize>>;

	public:
		using Buffer = Base::ItemType;

		ByteBufferPool(uint count)
			: Base{ count }
		{
		}

		// Gets a new buffer from the pool, or nullptr if the pool was empty.
		// The buffer's ref count is 1.
		//
		Buffer* GetBuffer()
		{
			Buffer* pBuffer{ Base::Get() };

			if (pBuffer)
			{
				pBuffer->m_dataSize = 0;
				pBuffer->m_refCount = 1;
			}

			return pBuffer;
		}

		// Returns a buffer obtained from GetBuffer() to the pool.
		// It decrements the buffer's ref count and if it becomes
		// zero the buffer is put back into the pool.
		//
		void PutBuffer(const Buffer* pBuffer)
		{
			auto* pBuf{ const_cast<Buffer*>(pBuffer) };

			assert(pBuf->m_refCount > 0);

			if (--pBuf->m_refCount == 0)
				Base::Put(pBuf);
		}
	};
}
