#include "GdbOutputFilter.h"
#include <cassert>

// Filters the data to remove the remote GDB protocol packets
// so they don't clutter the console output.
// The filter only sees the messages sent from a remote stub
// (gdbserver or kgdb) to gdb, so it can't follow the context.
// Hence, it may occasionally let through some data meant for
// the gdb, like '+' characters.
//
// Remote GDB packet:
// $packet-data#xx (xx is checksum in two hex digits)
// The packet-data does not contain '$' or '#'.
// A packet is acknowledged by the receiver sending back '+', or '-'.
//
// Notification packet:
// %packet-data#xx
// The packet-data does not contain '$', '%' or '#'.
// This packet is not acknowledged.
// Currently, we don't filter out these messages.
//
// Breakpoint hit:
// $T05thread:id;#xx
// example: $T05thread:ce;#6e
// The stub sends this packet after stopping the debuggee. For our
// filter this appears asynchronous (it is actually a response to
// a previous gdb command to run the debuggee). We could try to
// catch and filter it out, but we're too lazy, so it will pass...

// GDB remote protocol:
// https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Protocol.html
// https://www.embecosm.com/appnotes/ean4/embecosm-howto-rsp-server-ean4-issue-2.html


namespace
{
	constexpr size_t cMaxPacketSize{ 4096 };	// of a GDB packet including the $ and checksum
	constexpr uint cMaxPassBuffers{ 128 };
}


GdbOutputFilter::GdbOutputFilter(BufferPool& bufferPool)
	: BaseFilter{ bufferPool, cMaxPassBuffers, cMaxPacketSize }
{
}


// Returns the number of accumulated result buffers
// that are ready to be sent (may be zero).
//
uint GdbOutputFilter::Process(const Buffer* pBuffer)
{
	auto srcData{ pBuffer->GetData() };

	while (!srcData.empty())
	{
		size_t bytesRead;

		if (m_state == State::Pass)
			bytesRead = PassThrough(srcData);
		else
			bytesRead = ParseGdb(srcData);

		srcData = srcData.subspan(bytesRead);
	}

	m_bufferPool.PutBuffer(pBuffer);

	return m_passQueue.GetNumUsedBlocks();
}


// Returns the number of bytes read from srcData
//
size_t GdbOutputFilter::PassThrough(std::span<const uint8_t> srcData)
{
	auto* pBuffer = m_bufferPool.GetBuffer();		// get a new buffer
	assert(pBuffer);
	auto dstData{ pBuffer->GetBuffer() };
	size_t bytesRead{};
	size_t bytesCopied{};

	// Don't worry about dst buffer overflow as src and dst have the same size

	while (bytesRead < srcData.size())
	{
		if (!m_isPlus)
		{
			do
			{
				auto data{ srcData[bytesRead++] };

				if (data != '+')
					dstData[bytesCopied++] = data;
				else
				{
					m_isPlus = true;
					break;
				}
			} while (bytesRead < srcData.size());
		}
		else
		{
			auto data{ srcData[bytesRead++] };

			if (data != '$')
			{
				dstData[bytesCopied++] = '+';

				if (data != '+')
				{
					--bytesRead;	// undo consume
					m_isPlus = false;
				}
			}
			else
			{
				m_rejectBuffer.Clear();
				m_rejectBuffer.Append('$');
				m_isPlus = false;
				m_state = State::Dollar;
				break;
			}
		}
	}

	if (bytesCopied > 0)
	{
		pBuffer->SetDataSize(bytesCopied);
		m_passQueue.Enqueue(pBuffer);
	}
	else
		m_bufferPool.PutBuffer(pBuffer);

	return bytesRead;
}


size_t GdbOutputFilter::ParseGdb(std::span<const uint8_t> srcData)
{
	size_t bytesRead{};
	auto dstData{ m_rejectBuffer.GetFreeSpan() };

	if (dstData.size() < srcData.size())
	{
		// Reject buffer nearly full - possibly we mis-interpreted
		// that data as GDB packet. Pass the reject buffer through.

		UndoReject();
		m_state = State::Pass;
	}
	else
	{
		while (bytesRead < srcData.size() && m_state != State::Pass)
		{
			switch (m_state)
			{
			case State::Dollar:
				do
				{
					auto data{ srcData[bytesRead] };
					dstData[bytesRead++] = data;

					if (data == '#')
					{
						m_state = State::Hash;
						break;
					}

					if (data == '$')
					{
						m_rejectBuffer.Consume(bytesRead);
						UndoReject();
						m_state = State::Pass;
						break;
					}
				} while (bytesRead < srcData.size());

				break;

			case State::Hash:
				dstData[bytesRead] = srcData[bytesRead];
				++bytesRead;
				m_state = State::Checksum1;
				break;

			case State::Checksum1:
				dstData[bytesRead] = srcData[bytesRead];
				++bytesRead;
				m_state = State::Checksum2;
				break;

			case State::Checksum2:
				m_rejectBuffer.Clear();
				m_state = State::Pass;
				break;

			default:
				assert(false);
			}
		}
	}

	return bytesRead;
}
