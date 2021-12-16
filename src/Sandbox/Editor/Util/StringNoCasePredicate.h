/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Utility class to help in STL algorithms on containers with
	CStrings when intending to use case insensitive searches or sorting for
	example.

-------------------------------------------------------------------------
History:
- 05:03:2009   : Created by Pau Novau
*************************************************************************/


#ifndef __StringNoCasePredicate_h__
#define __StringNoCasePredicate_h__
#pragma once

/*
Utility class to help in STL algorithms on containers with CStrings
when intending to use case insensitive searches or sorting for example.

e.g.
std::vector< CString > v;
...
std::sort( v.begin(), v.end(), CStringNoCasePredicate::LessThan() );
std::find_if( v.begin(), v.end(), CStringNoCasePredicate::Equal( stringImLookingFor ) );
*/
class CStringNoCasePredicate
{
public:
//////////////////////////////////////////////////////////////////////////
	struct Equal
	{
		Equal( const CString& referenceString )
			: m_referenceString( referenceString )
		{
		}

		bool operator() ( const CString& arg ) const
		{
			return ( m_referenceString.CompareNoCase( arg ) == 0 );
		}

	private:
		const CString& m_referenceString;
	};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
	struct LessThan
	{
		bool operator() ( const CString& arg1, const CString& arg2 ) const
		{
			return ( arg1.CompareNoCase( arg2 ) < 0 );
		}
	};
//////////////////////////////////////////////////////////////////////////
};


#endif