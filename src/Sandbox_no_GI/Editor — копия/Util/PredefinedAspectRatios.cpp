#include "StdAfx.h"
#include "PredefinedAspectRatios.h"

CPredefinedAspectRatios::CPredefinedAspectRatios()
{
	m_aspectRatios.reserve( 10 );

	AddAspectRatio( 5, 4 );
	AddAspectRatio( 4, 3 );
	AddAspectRatio( 3, 2 );
	AddAspectRatio( 16, 10 );
	AddAspectRatio( 16, 9 );
	AddAspectRatio( 1.85f, 1 );
	AddAspectRatio( 2.39f, 1 );
}

CPredefinedAspectRatios::~CPredefinedAspectRatios()
{

}

void CPredefinedAspectRatios::AddAspectRatio( float x, int y )
{
	if ( y == 0 )
	{
		return;
	}

	SAspectRatio aspectRatio;
	aspectRatio.name.Format( "%.2f:%d", x, y );
	aspectRatio.value = x / y;

	m_aspectRatios.push_back( aspectRatio );
}

void CPredefinedAspectRatios::AddAspectRatio( int x, int y )
{
	if ( y == 0 )
	{
		return;
	}

	SAspectRatio aspectRatio;
	aspectRatio.name.Format( "%d:%d", x, y );
	aspectRatio.value = float( x ) / y;

	m_aspectRatios.push_back( aspectRatio );
}


float CPredefinedAspectRatios::getCurrentValue() const
{
	return gSettings.viewports.fDefaultAspectRatio;
}

bool CPredefinedAspectRatios::isEmpty() const
{
	return m_aspectRatios.empty();
}

size_t CPredefinedAspectRatios::getCount() const
{
	return m_aspectRatios.size();
}


const CString& CPredefinedAspectRatios::getName( size_t aspectRatioId ) const
{
	bool validAspectRatioId = ( aspectRatioId < getCount() );
	assert( validAspectRatioId );
	if ( ! validAspectRatioId )
	{
		static CString dummyAspectRatioName( "1:1" );
		return dummyAspectRatioName;
	}

	const SAspectRatio& aspectRatio = m_aspectRatios[ aspectRatioId ];
	
	return aspectRatio.name;
}

float CPredefinedAspectRatios::getValue( size_t aspectRatioId ) const
{
	bool validAspectRatioId = ( aspectRatioId < getCount() );
	assert( validAspectRatioId );
	if ( ! validAspectRatioId )
	{
		return 1;
	}

	const SAspectRatio& aspectRatio = m_aspectRatios[ aspectRatioId ];

	return aspectRatio.value;
}

bool CPredefinedAspectRatios::isCurrent( size_t aspectRatioId ) const
{
	float selectedValue = getValue( aspectRatioId );
	float currentValue = getCurrentValue();

	const float THRESHOLD = 0.01f;

	bool valuesCloseEnough = ( fabs( selectedValue - currentValue ) <= THRESHOLD );

	return valuesCloseEnough;
}
