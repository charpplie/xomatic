#ifndef __STATICASSERT_H__
#define __STATICASSERT_H__

//===================================================================================
// Compile-time assert.
// Syntax: STATIC_ASSERT(BooleanExpression);
//===================================================================================
template <bool b>
struct STATIC_ASSERT_FAILED;

template <>
struct STATIC_ASSERT_FAILED<true>
{
};

template <int i>
struct STATIC_ASSERT_TEST
{
	enum { dummy = i };
};

#define STATIC_ASSERT_BUILD_NAME2(x,y) x##y
#define STATIC_ASSERT_BUILD_NAME1(x,y) STATIC_ASSERT_BUILD_NAME2(x,y)
#define STATIC_ASSERT_BUILD_NAME(x,y)  STATIC_ASSERT_BUILD_NAME1(x,y)

#define STATIC_ASSERT( expr ) \
	typedef STATIC_ASSERT_TEST< sizeof( STATIC_ASSERT_FAILED< (bool)(expr) > ) > \
	STATIC_ASSERT_BUILD_NAME(static_assert_test_,__LINE__)
// note: for MS Visual Studio we could use __COUNTER__ instead of __LINE__
//===================================================================================

#endif
