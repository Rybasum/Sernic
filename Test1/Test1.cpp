#include "pch.h"
#include "CppUnitTest.h"

// NOTE: CppUnitTest is incompatible with C++ Modules
// so we have to include the std headers in pch.h

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include "GdbOutputFilter.h"


namespace
{
	constexpr char cData1[]{ "Legia+++\n++$blabla#aa+$hehe#bbWarszawa" };
	constexpr char cData2[]{ "+$to nasza dupa i chala#aa chyba" };

	constexpr char cData3[]{ "Legia+++\n++$blabla#aaWarszawa" };
	constexpr char cData4[]{ "+$to nasza dupa i chala#aa chyba" };

	constexpr char cData5[]{ "Legia+++\n++$blabla#aaWarszawa+" };
	constexpr char cData6[]{ "$to nasza dupa i chala#aa chyba" };

	constexpr char cData7[]{ "Legia+++\n++$blabla#aaWarszawa+$" };
	constexpr char cData8[]{ "to nasza dupa i chala#aa chyba" };

	constexpr char cData9[]{ "Legia+++\n++$blabla#aaWarszawa+$t" };
	constexpr char cData10[]{ "o nasza dupa i chala#aa chyba" };

	constexpr char cData11[]{ "Legia+++\n++$blabla#aaWarszawa+$to nasza dupa i chal" };
	constexpr char cData12[]{ "a#aa chyba" };

	constexpr char cData13[]{ "Legia+++\n++$blabla#aaWarszawa+$to nasza dupa i chala" };
	constexpr char cData14[]{ "#aa chyba" };

	constexpr char cData15[]{ "Legia+++\n++$blabla#aaWarszawa+$to nasza dupa i chala#" };
	constexpr char cData16[]{ "aa chyba" };

	constexpr char cData17[]{ "Legia+++\n++$blabla#aaWarszawa+$to nasza dupa i chala#a" };
	constexpr char cData18[]{ "a chyba" };

	constexpr char cData19[]{ "Legia+++\n++$blabla#aaWarszawa+$to nasza dupa i chala#aa" };
	constexpr char cData20[]{ " chyba" };
}


namespace Test1
{
	TEST_CLASS(Test1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			BufferPool bufferPool{ 2048 };
			GdbOutputFilter filter{ bufferPool };

			Test(L"Test 1", &bufferPool, &filter, cData1, cData2);
			Test(L"Test 2", &bufferPool, &filter, cData3, cData4);
			Test(L"Test 3", &bufferPool, &filter, cData5, cData6);
			Test(L"Test 4", &bufferPool, &filter, cData7, cData8);
			Test(L"Test 5", &bufferPool, &filter, cData9, cData10);
			Test(L"Test 6", &bufferPool, &filter, cData11, cData12);
			Test(L"Test 7", &bufferPool, &filter, cData13, cData14);
			Test(L"Test 8", &bufferPool, &filter, cData15, cData16);
			Test(L"Test 9", &bufferPool, &filter, cData17, cData18);
			Test(L"Test 10", &bufferPool, &filter, cData19, cData20);
		}

	private:

		void Test(const wchar_t* id, BufferPool* pBufferPool, GdbOutputFilter* pFilter, std::span<const char> data1, std::span<const char> data2)
		{
			// Buffer 1

			auto* pBuffer1{ pBufferPool->GetBuffer() };
			std::memcpy(pBuffer1->GetBufferPtr(), data1.data(), data1.size() - 1);
			pBuffer1->SetDataSize(data1.size() - 1);

			auto numBuffers{ pFilter->Process(pBuffer1) };

			Assert::AreEqual(numBuffers, 2U, id);

			auto* pResult{ pFilter->GetResult() };
			auto result{ pResult->GetData() };

			Assert::IsTrue(std::strncmp("Legia+++\n+", (const char*)result.data(), result.size()) == 0, id);
			pBufferPool->PutBuffer(pResult);

			pResult = pFilter->GetResult();
			result = pResult->GetData();

			Assert::IsTrue(std::strncmp("Warszawa", (const char*)result.data(), result.size()) == 0, id);
			pBufferPool->PutBuffer(pResult);

			// Buffer 2

			auto* pBuffer2{ pBufferPool->GetBuffer() };
			std::memcpy(pBuffer2->GetBufferPtr(), data2.data(), data2.size() - 1);
			pBuffer2->SetDataSize(data2.size() - 1);

			numBuffers = pFilter->Process(pBuffer2);

			Assert::AreEqual(numBuffers, 1U, id);

			pResult = pFilter->GetResult();
			result = pResult->GetData();

			Assert::IsTrue(std::strncmp(" chyba", (const char*)result.data(), result.size()) == 0, id);
			pBufferPool->PutBuffer(pResult);
		}
	};
}
