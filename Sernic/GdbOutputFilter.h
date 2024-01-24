#pragma once

#include "BaseFilter.h"


class GdbOutputFilter : public BaseFilter
{
public:
	GdbOutputFilter(BufferPool& bufferPool);

	uint Process(const Buffer* pBuffer) override;

private:
	size_t PassThrough(std::span<const uint8_t> srcData);
	size_t ParseGdb(std::span<const uint8_t> srcData);

	enum class State
	{
		Pass,
		Plus,
		Dollar,
		Hash,
		Checksum1,
		Checksum2
	} m_state{};

	uint m_packetSize{};
	bool m_isPlus{};
};

