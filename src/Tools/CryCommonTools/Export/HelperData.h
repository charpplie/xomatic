#ifndef __HELPERDATA_H__
#define __HELPERDATA_H__

struct SHelperData
{
public:
	enum EHelperType
	{
		eHelperType_UNKNOWN,
		eHelperType_Point,
		eHelperType_Dummy
	};

public:
	SHelperData()
		: m_eHelperType(eHelperType_UNKNOWN)
	{
	}

public:
	EHelperType m_eHelperType;
	float m_boundBoxMin[3];	    // used for eHelperType_Dummy only
	float m_boundBoxMax[3];     // used for eHelperType_Dummy only
};

#endif
