#pragma once

#include "Lib/ByteBufferPool.h"
#include "Defs.h"


using BufferPool = Lib::ByteBufferPool<cBufferSize>;
using Buffer = BufferPool::Buffer;
