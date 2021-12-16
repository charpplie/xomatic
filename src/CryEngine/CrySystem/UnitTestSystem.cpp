////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   UnitTestSystem.cpp
//  Created:     19/03/2008 by Timur.
//  Description: Implementation of the CryEngine Unit Testing framework
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UnitTestSystem.h"

#include "ISystem.h"
#include "ITimer.h"
#include "IConsole.h"
#include "UnitTestExcelReporter.h"

using namespace CryUnitTest;


struct SAutoTestsContext
{
	float fStartTime;
	float fCurrTime;
	string sSuiteName;
	string sTestName;
	int waitAfterMSec;
	int iter;
	int runCount;
  CryUnitTest::UnitTestRunContext context;

	SAutoTestsContext(): fStartTime(0), fCurrTime(0), waitAfterMSec(0), iter(-1), runCount(1) { memset(&context, 0, sizeof(context)); }
};


CUnitTestManager::CUnitTestManager()
{
	m_pAutoTestsContext = new SAutoTestsContext;
	assert(m_pAutoTestsContext);
}

CUnitTestManager::~CUnitTestManager()
{
	delete m_pAutoTestsContext;
	m_pAutoTestsContext = 0;

	RemoveTests();
}

void CUnitTestManager::RemoveTests()
{
	for (int i = 0; i < m_tests.size(); ++i)
		delete m_tests[i];

	m_tests.clear();
}

IUnitTest* CUnitTestManager::CreateTest( const UnitTestInfo &info )
{
	for (size_t i = 0; i < m_tests.size(); ++i)
	{
		CUnitTest* pt = m_tests[i];

		if (pt->m_info.pTestImpl == info.pTestImpl)
			return pt;
	}

	CUnitTest *pTest = new CUnitTest( info );
	m_tests.push_back( pTest );
	return pTest;
}

void CUnitTestManager::RunAllTests( UnitTestRunContext &context )
{

	StartTesting( context );

	for (uint32 i = 0; i < m_tests.size(); i++)
	{
		m_tests[i]->Init();
		RunTest( m_tests[i],context );
		m_tests[i]->Done();
	}

	EndTesting( context );
}

void CUnitTestManager::RunMatchingTests( const char *sName,UnitTestRunContext &context )
{
	StartTesting( context );
	
	for (uint32 i = 0; i < m_tests.size(); i++)
	{
		if (strstr(m_tests[i]->m_info.name,sName) != 0 || strcmp(m_tests[i]->m_info.suite,sName) == 0 || strcmp(m_tests[i]->m_info.module,sName) == 0)
		{
			m_tests[i]->Init();
			RunTest( m_tests[i],context );
			m_tests[i]->Done();
		}
	}

	EndTesting( context );
}


void CUnitTestManager::RunAutoTests(const char* sSuiteName, const char* sTestName)
{
	// prepare auto tests context
	// tests actually will be called during Update call
	m_pAutoTestsContext->fStartTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
	m_pAutoTestsContext->fCurrTime = m_pAutoTestsContext->fStartTime;
	m_pAutoTestsContext->sSuiteName = sSuiteName;
	m_pAutoTestsContext->sTestName = sTestName;
	m_pAutoTestsContext->waitAfterMSec = 0;
	m_pAutoTestsContext->iter = 0;
	m_pAutoTestsContext->runCount = max(gEnv->pConsole->GetCVar("ats_loop")->GetIVal(), 1);

	StartTesting(m_pAutoTestsContext->context);
}

void CUnitTestManager::Update()
{
	if (m_pAutoTestsContext->iter != -1)
	{
		m_pAutoTestsContext->fCurrTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();

		if ((m_pAutoTestsContext->fCurrTime - m_pAutoTestsContext->fStartTime) > m_pAutoTestsContext->waitAfterMSec)
		{
			bool wasFound = false;

			for (int i = m_pAutoTestsContext->iter; i < m_tests.size(); i++)
			{
				if (IsTestMatch(m_tests[i], m_pAutoTestsContext->sSuiteName, m_pAutoTestsContext->sTestName))
				{
					m_tests[i]->Init();
					RunTest(m_tests[i], m_pAutoTestsContext->context);
					m_tests[i]->Done();

					AutoTestInfo info;
					m_tests[i]->GetAutoTestInfo(info);
					m_pAutoTestsContext->waitAfterMSec = info.waitMSec;
					m_pAutoTestsContext->iter = i;

					if (info.runNextTest)
						m_pAutoTestsContext->iter++;

					wasFound = true;
					break;
				}
			}
			if (!wasFound)
			{
				// no tests were found so stop testing
				m_pAutoTestsContext->runCount--;

				if (0 == m_pAutoTestsContext->runCount)
				{
					EndTesting(m_pAutoTestsContext->context);
					m_pAutoTestsContext->iter = -1;

					if (gEnv->pConsole->GetCVar("ats_exit")->GetIVal())
						exit(0);
				}
				else
				{
					m_pAutoTestsContext->waitAfterMSec = 0;
					m_pAutoTestsContext->iter = 0;
				}
			}
			m_pAutoTestsContext->fStartTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
			m_pAutoTestsContext->fCurrTime = m_pAutoTestsContext->fStartTime;
		}
	}
}


void CUnitTestManager::StartTesting( UnitTestRunContext &context )
{
	context.testCount = 0;
  context.failedTestCount = 0;
	context.succedTestCount = 0;

	context.pReporter = new CUnitTestExcelReporter;

	if (context.pReporter)
		context.pReporter->OnStartTesting( context );
}

void CUnitTestManager::EndTesting( UnitTestRunContext &context )
{
	if (context.pReporter)
		context.pReporter->OnFinishTesting( context );
}

void CUnitTestManager::RunTest( IUnitTest *pTest,UnitTestRunContext &context )
{

	m_failureMsg[0] = 0;

	bool bFail = false;

	if (context.pReporter)
		context.pReporter->OnTestStart( pTest );

	context.testCount++;

	CTimeValue t0 = gEnv->pTimer->GetAsyncTime();

#if !defined(PS3) && defined(CRY_UNIT_TESTING)
	try
	{
		pTest->Run( context );
		context.succedTestCount++;
	}

	catch (assert_exception const& e)
	{
		context.failedTestCount++;
		bFail = true;
		strcpy_s( m_failureMsg,e.what() );

		// copy filename and line number of unit test assert
		// will be used later for test reporting
		CUnitTest* pT = static_cast<CUnitTest*>(pTest);
		pT->m_info.sFilename = e.m_filename;
		pT->m_info.filename = pT->m_info.sFilename.c_str();
		pT->m_info.lineNumber = e.m_lineNumber;
	}
	catch (std::exception const& e)
	{
		context.failedTestCount++;

		bFail = true;
		strcpy_s( m_failureMsg,"Unhandled exception: " );
		strcat_s( m_failureMsg,e.what() );
	}
	catch (...)
	{
		context.failedTestCount++;

		bFail = true;
		strcpy_s( m_failureMsg,"Crash" );
	}
#endif
	CTimeValue t1 = gEnv->pTimer->GetAsyncTime();

	float fRunTimeInMs = (t1 - t0).GetMilliSeconds();

	if (context.pReporter)
		context.pReporter->OnTestFinish( pTest,fRunTimeInMs,!bFail,m_failureMsg );
}

bool CUnitTestManager::IsTestMatch(CUnitTest* pTest, const string& sSuiteName, const string& sTestName)
{
	assert(pTest);

	bool isMatch = true; // by default test is match

	if (sSuiteName != "" && sSuiteName != "*")
	{
		isMatch &= (strcmp(sSuiteName.c_str(), pTest->m_info.suite) == 0);
	}
	if (sTestName != "" && sTestName != "*")
	{
		isMatch &= (strcmp(sTestName.c_str(), pTest->m_info.name) == 0);
	}
	return isMatch;
}

void CLogUnitTestReporter::OnStartTesting( UnitTestRunContext &context )
{
	CryLog( "UnitTesting Started" );
}

void CLogUnitTestReporter::OnFinishTesting( UnitTestRunContext &context )
{
	CryLog( "UnitTesting Finished" );
}

void CLogUnitTestReporter::OnTestStart( IUnitTest *pTest )
{
	UnitTestInfo info;
	pTest->GetInfo( info );
	CryLog( "UnitTestStart:  [%s]%s:%s",info.module,info.suite,info.name );
}

void CLogUnitTestReporter::OnTestFinish( IUnitTest *pTest,float fRunTimeInMs,bool bSuccess,char const* failureDescription )
{
	UnitTestInfo info;
	pTest->GetInfo( info );
	if (bSuccess)
		CryLog( "UnitTestFinish: [%s]%s:%s | OK (%3.2fms)",info.module,info.suite,info.name,fRunTimeInMs );
	else
		CryLog( "UnitTestFinish: [%s]%s:%s | FAIL (%s)",info.module,info.suite,info.name,failureDescription );
}


#if defined(CRY_UNIT_TESTING)
/*
class CUT_TestString : public CryUnitTest::Test
{
	virtual void Run()
	{
	}
};
*/

/*
CRY_UNIT_TEST( CUT_TestString )
{
	int a = 10;
	int b = 11;
	CRY_UNIT_TEST_ASSERT( a == b );
}

CRY_UNIT_TEST( CUT_TestString2 )
{
	int a = 10;
	int b = 10;
	CRY_UNIT_TEST_ASSERT( a == b );
}

CRY_UNIT_TEST( CUT_TestString3 )
{
	int aa = 10;
	int bb = 11;
	CRY_UNIT_TEST_ASSERT( aa == bb );
}
*/

CRY_UNIT_TEST_SUITE( CryString )
{
	class CUT_CryString : public CryUnitTest::Test
	{
		bool UnitAssert(const char* message, const char* value, const char* refValue)
		{
			int res = strcmp(value, refValue);
			if (res != 0)
				throw CryUnitTest::assert_exception(message,__FILE__,__LINE__);
			return res == 0;
		}

		bool UnitAssert(const char* message, const wchar_t* value, const wchar_t* refValue)
		{
			int res = wcscmp(value, refValue);
			if (res != 0)
				throw CryUnitTest::assert_exception(message,__FILE__,__LINE__);
			return res == 0;
		}

		bool UnitAssert(const char* message, bool cond)
		{
			CRY_ASSERT_MESSAGE(cond, message);
			if (!cond)
				throw CryUnitTest::assert_exception(message,__FILE__,__LINE__);
			return cond;
		}

		virtual void Run()
		{
			//////////////////////////////////////////////////////////////////////////
			// Based on MS documentation of find_last_of
			string strTestFindLastOfOverload1( "abcd-1234-abcd-1234" );
			string strTestFindLastOfOverload2( "ABCD-1234-ABCD-1234" );
			string strTestFindLastOfOverload3( "456-EFG-456-EFG" );
			string strTestFindLastOfOverload4 ( "12-ab-12-ab" );

			const char *cstr2 = "B1";
			const char *cstr2b = "D2";
			const char *cstr3a = "5E";
			string str4a ( "ba3" );
			string str4b ( "a2" );

			size_t nPosition(string::npos);

			nPosition = strTestFindLastOfOverload1.find_last_of ( 'd' , 14 );
			UnitAssert("find_last_of(char,size_type)",(nPosition==13));
			
			
			nPosition=strTestFindLastOfOverload2.find_last_of  ( cstr2 , 12 );
			UnitAssert("find_last_of(char*,size_type)",(nPosition==11));

				
			nPosition = strTestFindLastOfOverload2.find_last_of  ( cstr2b );
			UnitAssert("find_last_of(char*)",(nPosition==16));

			
			nPosition = strTestFindLastOfOverload3.find_last_of ( cstr3a , 8 , 8 );
			UnitAssert("find_last_of(char*,size_type,szie_type)",(nPosition==4));

			
			nPosition = strTestFindLastOfOverload4.find_last_of  ( str4a , 8 );
			UnitAssert("find_last_of(string,size_type)",(nPosition==4));

			
			nPosition = strTestFindLastOfOverload4.find_last_of ( str4b  );
			UnitAssert("find_last_of(string)",(nPosition==9));
			//////////////////////////////////////////////////////////////////////////
			// Based on MS documentation of find_last_not_of
			string strTestFindLastNotOfOverload1 ( "dddd-1dd4-abdd" );
			string strTestFindLastNotOfOverload2 ( "BBB-1111" );
			string strTestFindLastNotOfOverload3 ( "444-555-GGG" );
			string strTestFindLastNotOfOverload4 ( "12-ab-12-ab" );

			const char *cstr2NF = "B1";
			const char *cstr3aNF = "45G";
			const char *cstr3bNF = "45G";

			string str4aNF( "b-a" );
			string str4bNF ( "12" );

			size_t	nPosition3A(string::npos);

			nPosition = strTestFindLastNotOfOverload1.find_last_not_of ( 'd' , 7 );
			UnitAssert("find_last_not_of(char,size_type)",(nPosition==5));

			nPosition  = strTestFindLastNotOfOverload1.find_last_not_of  ( "d" );
			UnitAssert("find_last_not_of(char*,size_type)",(nPosition==11));

			nPosition = strTestFindLastNotOfOverload2.find_last_not_of  ( cstr2NF , 6 );
			UnitAssert("find_last_not_of(char*,size_type)",(nPosition==3));

			nPosition3A=strTestFindLastNotOfOverload3.find_last_not_of ( cstr3aNF );
			UnitAssert("find_last_not_of(char*,size_type)",(nPosition==7));

			nPosition = strTestFindLastNotOfOverload3.find_last_not_of ( cstr3bNF , 6 , nPosition3A - 1 );
			UnitAssert("find_last_not_of(char*,size_type,size_type)",(nPosition==3));
			
			nPosition = strTestFindLastNotOfOverload4.find_last_not_of ( str4aNF,5  );
			UnitAssert("find_last_not_of(string,size_type)",(nPosition==1));

			nPosition = strTestFindLastNotOfOverload4.find_last_not_of ( str4bNF);
			UnitAssert("find_last_not_of(string)",(nPosition==10));
			//////////////////////////////////////////////////////////////////////////
		}
	};

	CRY_UNIT_TEST_REGISTER(CUT_CryString)
} // StringTesting suite


CRY_UNIT_TEST_SUITE(FixedString)
{
	class CUT_FixedString : public CryUnitTest::Test
	{
		bool UnitAssert(const char* message, const char* value, const char* refValue)
		{
			int res = strcmp(value, refValue);
			if (res != 0)
				throw CryUnitTest::assert_exception(message,__FILE__,__LINE__);
			return res == 0;
		}

		bool UnitAssert(const char* message, const wchar_t* value, const wchar_t* refValue)
		{
			int res = wcscmp(value, refValue);
			if (res != 0)
				throw CryUnitTest::assert_exception(message,__FILE__,__LINE__);
			return res == 0;
		}

		bool UnitAssert(const char* message, bool cond)
		{
			CRY_ASSERT_MESSAGE(cond, message);
			if (!cond)
				throw CryUnitTest::assert_exception(message,__FILE__,__LINE__);
			return cond;
		}

		virtual void Run()
		{
			CryStackStringT<char, 10> str1;
			CryStackStringT<char, 10> str2;
			CryStackStringT<char, 4> str3;
			CryStackStringT<char, 10> str4;
			CryStackStringT<char, 5+1> str5;
			CryStackStringT<wchar_t, 16> wstr1;
			CryStackStringT<wchar_t, 255> wstr2;
			CryFixedStringT<100> fixedString100;
			CryFixedStringT<200> fixedString200;

			typedef CryStackStringT<char, 10> T;
			T* pStr = new T;
			*pStr = "adads";
			delete pStr;

			str1 = "abcd";
			UnitAssert ("Assignment1-EnoughSpace", str1, "abcd");

			str2 = "efg";
			UnitAssert ("Assignment2-EnoughSpace", str2, "efg");

			str2 = str1;
			UnitAssert ("Assignment3-EnoughSpace", str2, "abcd");

			str3 = str1;
			UnitAssert ("Assignment4-NotEnoughSpace", str3, "abcd");

			str1 += "XY";
			UnitAssert ("Concatenate-EnoughSpace", str1, "abcdXY");

			str2 += "efghijk";
			UnitAssert ("Concatenate-NotEnoughSpace", str2, "abcdefghijk");

			str1.replace("bc", "");
			UnitAssert ("Replace-Shrink-EnoughSpace", str1, "adXY");

			str1.replace("XY", "1234");
			UnitAssert ("Replace-Grow-EnoughSpace", str1, "ad1234");

			str1.replace("1234", "1234567890");
			UnitAssert ("Replace-Grow-NotEnoughSpace", str1, "ad1234567890");

			str1.reserve(200);
			UnitAssert ("Reserve200-SameString", str1, "ad1234567890");
			UnitAssert ("Reserve200-Capacity", str1.capacity() == 200);

			str1.reserve(0);
			UnitAssert ("Reserve0-SameString", str1, "ad1234567890");
			UnitAssert ("Reserve0-Capacity==Length", str1.capacity() == str1.length());

			str1.erase(7); // doesn't change capacity
			UnitAssert ("Erase-SameString", str1, "ad12345");

			str4.assign("abc");
			UnitAssert ("Str4 Assignment", str4, "abc");
			str4.reserve(9);
			UnitAssert ("Str4", str4.capacity() >= 9); // capacity is always >= MAX_SIZE-1
			str4.reserve(0);
			UnitAssert ("Str4-Shrink", str4.capacity() >= 9); // capacity is always >= MAX_SIZE-1

			size_t idx = str1.find("123");
			UnitAssert ("Str1-Find", idx == 2); 

			idx = str1.find("123", 3);
			UnitAssert ("Str1-Find", idx == str1.npos);

			wstr1 = L"abc";
			UnitAssert ("WStr1-Assign", wstr1, L"abc");
			UnitAssert ("WStr1-CompareCaseGT", wstr1.compare(L"aBc") > 0);
			UnitAssert ("WStr1-CompareCaseLT", wstr1.compare(L"babc") < 0);
			UnitAssert ("WStr1-CompareNoCase", wstr1.compareNoCase(L"aBc") == 0);

			str1.Format("This is a %s %S with %d params", "mixed", L"string", 3);
			str2.Format("This is a %S %s with %d params", L"mixed", "string", 3);
			UnitAssert ("Str1-Format1", str1, "This is a mixed string with 3 params");
			UnitAssert ("Str1-Format2", str1, str2);

			wstr1.Format(L"This is a %s %S with %d params", L"mixed", "string", 3);
			wstr2.Format(L"This is a %S %s with %d params", "mixed", L"string", 3);
			UnitAssert ("WStr1-Format1", wstr1, L"This is a mixed string with 3 params");
			UnitAssert ("WStr1-Format2", wstr1, wstr2);

			str5.FormatFast("%s", "12345");
			UnitAssert ("Str5-FormatFast", str5, "12345");

			str5.FormatFast("%s", "012345");
			UnitAssert ("Str5-FormatFast-Truncate", str5, "01234");

			fixedString100 = str5;
			str2 = fixedString200;
			fixedString200 = fixedString100;
			UnitAssert ("FixedString-Test2", fixedString100, "01234");
			UnitAssert ("FixedString-Test1", fixedString100, fixedString200);

			CryStackStringT<char, 10> testStr;
			CryFixedStringT<100> testStr2;
			CryFixedWStringT<100> testWStr1;
			string normalString;
			wstring normalWString;
			normalString = string(testStr);
			normalString = string(testStr2);
			normalString.assign(testStr2.c_str());
			// normalString = testStr;  // <- must NOT compile, as we don't allow it!
			// normalWString = testWStr1;  // <- must NOT compile, as we don't allow it!
			normalWString = wstring(testWStr1);
		}
	};
	CRY_UNIT_TEST_REGISTER(CUT_FixedString)
}
/*
#include "HeapAllocator.h"

// Base class for multi-threaded testing.
// Increments a shared thread-count, so client can determine when all threads are finished.
struct TestThread: CrySimpleThread<>
{
	TestThread(volatile int& nFlag)
		: _nFlag(nFlag)
	{
		CryInterlockedAdd(&_nFlag, 1);
		Start();
	}

protected:

	virtual void Terminate()
	{
		CrySimpleThread<>::Terminate();
		CryInterlockedAdd(&_nFlag, -1);
	}

	volatile int& _nFlag;
};

// Thread for testing a shared HeapAllocator.
struct HeapThread: TestThread
{
	HeapThread(volatile int& nFlag, stl::HeapAllocator<>& heap, int nKey)
		: TestThread(nFlag), _Heap(heap), _nKey(nKey)
	{
	}

	virtual void Run()
	{
		size_t nCheck = 0;
		for (int i = 0; i < _nKey; i++)
		{
			const int nInts = i * 99;
			int* aInts = _Heap.NewArray<int>(nInts);
			nCheck += nInts * sizeof(int);
			for (int n = 0; n < nInts; n += _nKey)
				aInts[n] = n;
			Matrix34* pMat = _Heap.New<Matrix34>(16);
			CRY_UNIT_TEST_ASSERT(((size_t)pMat & 15) == 0);
			nCheck += sizeof(Matrix34);

			stl::SMemoryUsage Mem = _Heap.GetTotalMemory();
			CRY_UNIT_TEST_ASSERT(Mem.nUsed <= Mem.nAlloc);
		}

		_Heap.Reset();
	}

	stl::HeapAllocator<>&	_Heap;
	int _nKey;
};

CRY_UNIT_TEST(CUT_HeapAllocator)
{
	// Normally, you shoudn't Clear a heap during multi-threading, 
	// as you'll kill other threads' objects.
	// But this is just to test the heap coherency,
	// and we're not using any allocated objects.

	stl::HeapAllocator<> Heap(0x4000);
	volatile int nRunning = 0;

	for (int t = 4; t < 8; t++)
	{
		new HeapThread(nRunning, Heap, t);
	}

	while (nRunning)
	{
		stl::SMemoryUsage Mem = Heap.GetTotalMemory();
		CRY_UNIT_TEST_ASSERT(Mem.nUsed <= Mem.nAlloc);
	}
}

#include "PoolAllocator.h"

struct SPoolStruct
{
	Matrix34			mat;
	char					name[11];
	SPoolStruct*	pNext;

	SPoolStruct()
		: mat(IDENTITY), pNext(0)
	{
		name[0] = 0;
	}
};

// Thread for testing a shared PoolAllocator.
struct PoolThread: TestThread
{
	typedef stl::TPoolAllocator<SPoolStruct,stl::PSyncMultiThread,16>	PoolAllocator;

	PoolThread(volatile int& nFlag, PoolAllocator& pool, int nKey)
		: TestThread(nFlag), _Pool(pool), _nKey(nKey)
	{
	}

	virtual void Run()
	{
		DynArray<SPoolStruct*> aArray;

		for (int n = 0; n < _nKey; ++n)
		{
			for (int i = 0; i <= n; i++)
				aArray.push_back(_Pool.New());
			for (int i = 0; i < aArray.size(); i+= n)
			{
				_Pool.Delete(aArray[i]);
				aArray.erase(i);
			}

			ptrdiff_t nAlloc = _Pool.GetTotalAllocatedMemory();
			ptrdiff_t nUsed = _Pool.GetTotalAllocatedNodeSize();
		}

		for (int i = 0; i < aArray.size(); i++)
			_Pool.Delete(aArray[i]);
	}

	PoolAllocator& _Pool;
	int _nKey;
};

CRY_UNIT_TEST(CUT_PoolAllocator)
{
	stl::TPoolAllocator<SPoolStruct,stl::PSyncMultiThread,16> Pool(256);
	volatile int nRunning = 0;

	for (int t = 24; t < 74; t++)
	{
		new PoolThread(nRunning, Pool, t);
	}

	while (nRunning)
	{
		stl::SMemoryUsage Mem = Pool.GetTotalMemory();
		CRY_UNIT_TEST_ASSERT(Mem.nUsed <= Mem.nAlloc);
	}
}
*/
#endif //CRY_UNIT_TESTING
