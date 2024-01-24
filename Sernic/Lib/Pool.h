#pragma once

#include <cassert>
#include "Types.h"


namespace Lib
{
	// Pool of POD items. Not thread-safe.
	//
	template <typename T>
	class Pool : NonCopyable
	{
	public:
		using ItemType = T;

		Pool(size_t count)
			: c_capacity{ count }
		{
			m_pBuffer = new T[count];
			m_pointers.reserve(count);

			for (size_t i{}; i < count; ++i)
				m_pointers.push_back(m_pBuffer + i);
		}

		~Pool()
		{
			delete[] m_pBuffer;
		}

		// Retrieves an item from the pool and returns a pointer to it.
		// Returns nullptr if the pool was empty.
		//
		T* Get()
		{
			T* pElement{};

			if (!m_pointers.empty())
			{
				pElement = m_pointers.back();
				m_pointers.pop_back();
			}

			return pElement;
		}

		// Returns the item previously obtained by Get() to the pool
		//
		void Put(T* pElement)
		{
			assert(m_pointers.size() < c_capacity);

			m_pointers.push_back(pElement);
		}

	private:
		const size_t c_capacity;
		T* m_pBuffer;
		std::vector<T*> m_pointers;
	};
}
