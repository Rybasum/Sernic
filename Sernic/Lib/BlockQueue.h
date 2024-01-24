#pragma once

#include <cassert>
#include "Types.h"


namespace Lib
{
	// Simple queue of same-size data blocks.
	// Each block stores L items of type T.
	// Thread-safe when there is one writer thread and one reader thread.
	//
	// Writer:
	// 1. Get the back block and write data to it
	// 2. Push the block into the queue
	//
	// Reader:
	// 1. Get the front block and read data from it
	// 2. Pop the block from the queue
	//
	template<typename T, size_t L>
	class BlockQueue : NonCopyable
	{
	public:
		static constexpr size_t cBlockLength{ L };				// in items of type T

		// Allocates a queue for the given number of blocks
		BlockQueue(const uint numBlocks)
			: c_pOwnedBuffer{ new T[numBlocks * cBlockLength] }
		{
			SetBuffer({ c_pOwnedBuffer, numBlocks * cBlockLength });
		}

		// Uses an external buffer for the queue
		BlockQueue(std::span<T> buffer = {})
		{
			SetBuffer(buffer);
		}

		~BlockQueue()
		{
			delete[] c_pOwnedBuffer;
		}

		void SetBuffer(std::span<T> buffer)
		{
			m_buffer = buffer;
			m_numBlocks = buffer.size() / cBlockLength;

			Clear();
		}

		uint GetNumFreeBlocks() const { return m_numFreeBlocks; }
		uint GetNumUsedBlocks() const { return m_numBlocks - m_numFreeBlocks; }

		// Returns a pointer to the back of the queue, or nullptr if the queue is full.
		// The caller must write data to the buffer and call Push.
		//
		T* Back()
		{
			// Thread safe: the reader side can only increment m_numFreeBlocks
			//
			return m_numFreeBlocks > 0 ? ToPointer(m_writeIndex) : nullptr;
		}

		// Enqueues the block previously obtained by calling Back
		//
		void Push()
		{
			assert(m_numFreeBlocks > 0);
			--m_numFreeBlocks;
			Advance(m_writeIndex);
		}

		// Pushes a copy of an element at the back of the queue.
		// Returns true on success, false if the queue is full.
		//
		bool Enqueue(const T& element)
		{
			if (m_numFreeBlocks > 0)
			{
				*ToPointer(m_writeIndex) = element;
				Push();

				return true;
			}

			return false;
		}

		// Returns a pointer to the front of the queue, or nullptr if the queue is empty.
		// The caller must read the data and call Pop.
		//
		T* Front()
		{
			// Thread safe: the writer size can only decrement m_numFreeBlocks
			//
			return GetNumUsedBlocks() > 0 ? ToPointer(m_readIndex) : nullptr;
		}

		// Pops the block previously obtained by calling Front
		//
		void Pop()
		{
			assert(m_numFreeBlocks < m_numBlocks);
			++m_numFreeBlocks;
			Advance(m_readIndex);
		}

		// Pops a copy of an element from the front of the queue.
		// Returns true on success, false if the queue is empty.
		//
		bool Dequeue(T* pElement)
		{
			if (GetNumUsedBlocks() > 0)
			{
				*pElement = *ToPointer(m_readIndex);
				Pop();

				return true;
			}

			return false;
		}

		// Discards all data from the buffer
		//
		void Clear()
		{
			m_writeIndex = 0;
			m_readIndex = 0;
			m_numFreeBlocks = m_numBlocks;
		}

	private:
		T* ToPointer(const uint index) { return m_buffer.data() + index * cBlockLength; }
		void Advance(uint& index) const { if (++index >= m_numBlocks) index = 0; }

		T* const			c_pOwnedBuffer{};
		std::span<T>		m_buffer;
		uint				m_numBlocks{};
		std::atomic<uint>	m_numFreeBlocks{};
		uint				m_writeIndex{};		// index of the block at the write position
		uint				m_readIndex{};		// index of the block at the read position
	};
}
