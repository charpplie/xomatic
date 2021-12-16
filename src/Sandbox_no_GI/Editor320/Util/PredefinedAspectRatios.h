#ifndef __Predefined_Aspect_Ratios__h__
#define __Predefined_Aspect_Ratios__h__

#include <vector>

class CPredefinedAspectRatios
{
public:
	CPredefinedAspectRatios();
	virtual ~CPredefinedAspectRatios();

	void AddAspectRatio( float x, int y );
	void AddAspectRatio( int x, int y );

	float getCurrentValue() const;

	bool isEmpty() const;
	size_t getCount() const;

	const CString& getName( size_t aspectRatioId ) const;
	float getValue( size_t aspectRatioId ) const;
	bool isCurrent( size_t aspectRatioId ) const;
	
private:
	struct SAspectRatio
	{
		CString name;
		float value;
	};
	std::vector< SAspectRatio > m_aspectRatios;
};

#endif