#ifndef __CRYDXPSSHADERREFLECTION__
#define __CRYDXPSSHADERREFLECTION__

#include "../CCryDXPSResource.hpp"
#include "../../../CCryDXPSMisc.hpp"
#include "../../../Layer0/CCryDXPS.hpp"
#include "CCryDXPSShaderDesc.hpp"


class CCryShaderReflectionType;
class CCryShaderReflectionType
{
private:
	friend class CCryDXPSShaderReflectionConstBuffer;
	D3D10_SHADER_VARIABLE_CLASS	m_TypeClass;
public:
    void GetDesc(D3D11_SHADER_TYPE_DESC *pDesc);
    CCryShaderReflectionType* GetMemberTypeByIndex(uint32 Index);
    CCryShaderReflectionType* GetMemberTypeByName(char* Name);
    const char* GetMemberTypeName(uint32 Index);
};

typedef CCryShaderReflectionType ID3D11ShaderReflectionType;

class CCryShaderReflectionVariable
{
private:
	friend class CCryDXPSShaderReflectionConstBuffer;
	CCryShaderReflectionType		m_Type;
	D3D11_SHADER_VARIABLE_DESC	m_VarDesc;
public:
    void GetDesc(D3D11_SHADER_VARIABLE_DESC *pDesc);
    CCryShaderReflectionType* GetType();
};

typedef CCryShaderReflectionVariable ID3D11ShaderReflectionVariable;

class CCryDXPSShaderReflectionConstBuffer
{
private:
	friend class CCryDXPSShaderReflection;
	CCryShaderReflectionVariable				m_Var;
//	CGprogram														m_Program;
	const CDXPSShaderDesc*							m_pShaderDesc;

	//CCryShaderReflectionVariable* GetVariableByParam(CGparameter Param);
public:
    void GetDesc(D3D11_SHADER_BUFFER_DESC *pDesc);
    CCryShaderReflectionVariable* GetVariableByIndex(uint32 Index);
    CCryShaderReflectionVariable* GetVariableByName(const char* Name);
};

typedef CCryDXPSShaderReflectionConstBuffer ID3D11ShaderReflectionConstantBuffer;


class CCryDXPSShaderReflection;
class CCryDXPSShaderReflection	:	public CCryDXPSResource	,	public CCryRefAndWeak<CCryDXPSShaderReflection>
{
private:
	CCryDXPSShaderReflectionConstBuffer m_ConstBuffer;
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	uint32															m_BufferSize;
#endif


public:
									CCryDXPSShaderReflection(const char* pProgram2 MMRES_PARAM):
									CCryDXPSResource(EDXPS_RT_SHADERREFLECTION MMRES_PASS)
									{
										MMRES_ADDCOUNT();
										MMRES_ADDMM(sizeof(CCryDXPSShaderReflection));

#if defined(DXPS_LZSS_COMPRESS)
										uint8 pProgram3[LOCAL_VS_BUFFER_SIZE] _ALIGN(128);
										uint8* pProgram = &pProgram3[128];
										uint32 ByteSize	=	CDXPSShaderDesc::Decode(pProgram,reinterpret_cast<const uint8*>(pProgram2)+4);
										pProgram-=4;
#else
										const uint32 ByteSize	=	reinterpret_cast<const uint32*>(pProgram2)[0];
										const void* pProgram	=	pProgram2;
#endif
#if defined(_DEBUG)
										if (!ByteSize)
											snPause();
#endif
										uint8* pData	=	reinterpret_cast<uint8*>(CRY_DXPS_NEWARRAY(uint8,ByteSize));
										MMRES_ADDMM(ByteSize);
										for(uint32 a=0;a<ByteSize;a++)
											pData[a]	=	reinterpret_cast<const uint8*>(pProgram)[a+4];

										m_ConstBuffer.m_pShaderDesc	=	(const CDXPSShaderDesc*)pData;
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
										m_BufferSize	=	ByteSize;
#endif
									}
									~CCryDXPSShaderReflection()
									{
										MMRES_SUBCOUNT();
										MMRES_SUBMM(sizeof(CCryDXPSShaderReflection));
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
										if(EDXPS_RT_SHADERREFLECTION==Type())
											Type(EDXPS_RT_NONE);
#endif
									}

	inline	void		ReleaseResources()
									{
										CRY_DXPS_DELETEARRAY(reinterpret_cast<const uint8*>(m_ConstBuffer.m_pShaderDesc));
										m_ConstBuffer.m_pShaderDesc	=	0;
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
										MMRES_SUBMM(m_BufferSize);
#endif									
									}

	void GetDesc(D3D11_SHADER_DESC *pDesc);
    
	CCryDXPSShaderReflectionConstBuffer* GetConstantBufferByIndex(uint32 Index);
	CCryDXPSShaderReflectionConstBuffer* GetConstantBufferByName(char* Name);
    
  void GetResourceBindingDesc(uint32 ResourceIndex, D3D11_SHADER_INPUT_BIND_DESC *pDesc);
    
  void GetInputParameterDesc(uint32 ParameterIndex, D3D11_SIGNATURE_PARAMETER_DESC *pDesc);
  void GetOutputParameterDesc(uint32 ParameterIndex, D3D11_SIGNATURE_PARAMETER_DESC *pDesc);

	unsigned long		AddRef(){return IncRef();}
	unsigned long		Release(){return DecRef();}

};

typedef CCryDXPSShaderReflection ID3D11ShaderReflection;

#endif

