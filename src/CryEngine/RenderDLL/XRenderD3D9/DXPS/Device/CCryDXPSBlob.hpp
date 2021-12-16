#ifndef __CRYDXPSBLOB__
#define __CRYDXPSBLOB__

#include "Resource/CCryDXPSResource.hpp"
#include "../CCryDXPSMisc.hpp"
#include "../CCryDXPSDummy.hpp"
#include "../Layer0/CCryDXPS.hpp"

enum	CCRYDXPSBlobType
{
	EDXPS_BT_DATA,
	EDXPS_BT_SHADER,
};

class CCryDXPSBlob;
class CCryDXPSBlob	:	public CCryDXPSResource	,	public	CCryRefAndWeak<CCryDXPSBlob>
{
	uint8*												m_pProgram;
public:
																CCryDXPSBlob(const char* pProgram,uint32 Size MMRES_PARAM):
																CCryDXPSResource(EDXPS_RT_BLOB MMRES_PASS),
																m_pProgram(reinterpret_cast<uint8*>(CRY_DXPS_NEWARRAY(uint32,(Size+4+3)>>2))),
																m_Size(Size+4),
																m_Type(EDXPS_BT_SHADER)
																{
																	MMRES_ADDCOUNT();
																	MMRES_ADDMM(sizeof(CCryDXPSBlob));
																	MMRES_ADDMM(((Size+4+3)>>2)*sizeof(uint32));
																	*reinterpret_cast<uint32*>(m_pProgram)	=	Size;
																	for(uint32 a=0;a<Size;a++)
																		m_pProgram[a+4]=pProgram[a];
																}
	uint8*												Program()const
																{
																	return m_pProgram;
																}
	CCRYDXPSBlobType							m_Type;
	CCryAPtrScope<uint8>					m_pData;
	size_t												m_Size;
public:
	inline												CCryDXPSBlob(size_t Size MMRES_PARAM):
																CCryDXPSResource(EDXPS_RT_BLOB MMRES_PASS),
																m_pProgram(0),
																m_Type(EDXPS_BT_DATA),
																m_pData(CRY_DXPS_NEWARRAY(uint8,Size)),
																m_Size(Size)
																{
																	MMRES_ADDCOUNT();
																	MMRES_ADDMM(sizeof(CCryDXPSBlob));
																	MMRES_ADDMM(m_Size);
																}
																~CCryDXPSBlob()
																{
																	MMRES_ADDCOUNT();
																	MMRES_ADDMM(sizeof(CCryDXPSBlob));
																	MMRES_SUBMM(m_Size);
																	CRY_DXPS_DELETEARRAY(m_pProgram);
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
																	if(EDXPS_RT_BLOB==CCryDXPSResource::Type())
																		CCryDXPSResource::Type(EDXPS_RT_NONE);
#endif
																}

	ILINE void										ReleaseResources()
																{
																	m_pData.Release();
																	CRY_DXPS_DELETEARRAY(m_pProgram);
																	m_pProgram	=	0;
																}

	ILINE	void*										GetBufferPointer()
																{
																			if(m_Type==EDXPS_BT_SHADER)
																			return m_pProgram;
																	return m_pData;
																}
	ILINE size_t									GetBufferSize()
																{
																	return m_Size;
																}

	ILINE CCRYDXPSBlobType				Type()const
																{
																	return m_Type;
																}
	ILINE int											Release(){return DecRef();}
};

typedef CCryDXPSBlob ID3D10Blob;
typedef ID3D10Blob* LPD3D10BLOB;

#endif

