#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "CCryDXPSShaderReflection.hpp"
//#include <assert.h>
#include <algorithm>

//////////////////////////////////////////////////////////////
void CCryShaderReflectionType::GetDesc(D3D11_SHADER_TYPE_DESC *pDesc)
{
	pDesc->Class	=	m_TypeClass;
}

ID3D11ShaderReflectionType* CCryShaderReflectionType::GetMemberTypeByIndex(uint32 Index)
{
	CRY_ASSERT_MESSAGE(0,"CCryShaderReflectionType::GetMemberTypeByIndex not implemented yet");
	return 0;
}

ID3D11ShaderReflectionType* CCryShaderReflectionType::GetMemberTypeByName(char* Name)
{
	CRY_ASSERT_MESSAGE(0,"CCryShaderReflectionType::GetMemberTypeByName not implemented yet");
	return 0;
}

const char* CCryShaderReflectionType::GetMemberTypeName(uint32 Index)
{
	CRY_ASSERT_MESSAGE(0,"CCryShaderReflectionType::GetMemberTypeName not implemented yet");
	return "";
}

//////////////////////////////////////////////////////////////
void CCryShaderReflectionVariable::GetDesc(D3D11_SHADER_VARIABLE_DESC *pDesc)
{
	*pDesc	=	m_VarDesc;
//	memcpy(pDsec,&m_VarDesc,sizeof(D3D11_SHADER_VARIABLE_DESC));
}

ID3D11ShaderReflectionType* CCryShaderReflectionVariable::GetType()
{
	return &m_Type;
}

//////////////////////////////////////////////////////////////
void CCryDXPSShaderReflectionConstBuffer::GetDesc(D3D11_SHADER_BUFFER_DESC *pDesc)
{
//	pDesc->Variables	=	cellGcmCgGetCountParameter(m_Program);
	pDesc->Variables	=	m_pShaderDesc->ConstantCount();
}



CCryShaderReflectionVariable* CCryDXPSShaderReflectionConstBuffer::GetVariableByIndex(uint32 Index)
{
#ifdef _DEBUG
	if(Index>=m_pShaderDesc->ConstantCount())
	{
		CRY_ASSERT_MESSAGE(0,"ERROR: index out of bound ->GetVariableByIndex!");
		return 0;
	}
#endif
	const SRefConstant& rConst	=	m_pShaderDesc->Constant()[Index];

	m_Var.m_Type.m_TypeClass			=	D3D10_SVC_SCALAR;
	m_Var.m_VarDesc.uFlags				= D3D10_SVF_USED;
	m_Var.m_VarDesc.Size					= rConst.m_Size<<4;
	m_Var.m_VarDesc.StartOffset		=	rConst.m_ConstRegister<<4;
	m_Var.m_VarDesc.CBufferIndex	=	rConst.m_ConstSlot;
	m_Var.m_VarDesc.Name					=	&m_pShaderDesc->NameTable()[rConst.m_NameIndex];

	return &m_Var;
}

CCryShaderReflectionVariable* CCryDXPSShaderReflectionConstBuffer::GetVariableByName(const char* Name)
{
	return 0;//GetVariableByParam(cellGcmCgGetNamedParameter(m_Program,Name));
}


//////////////////////////////////////////////////////////////


void CCryDXPSShaderReflection::GetDesc(D3D11_SHADER_DESC *pDesc)
{
	pDesc->ConstantBuffers	=	1;
	pDesc->BoundResources		=	m_ConstBuffer.m_pShaderDesc->SamplerCount();
	pDesc->InputParameters  = 0;
	pDesc->InputParameters	=	m_ConstBuffer.m_pShaderDesc->VertexAttributeCount();
}
 
CCryDXPSShaderReflectionConstBuffer* CCryDXPSShaderReflection::GetConstantBufferByIndex(uint32 Index)
{
	return &m_ConstBuffer;
}

CCryDXPSShaderReflectionConstBuffer* CCryDXPSShaderReflection::GetConstantBufferByName(char* Name)
{
	return &m_ConstBuffer;
}

void CCryDXPSShaderReflection::GetResourceBindingDesc(uint32 ResourceIndex, D3D11_SHADER_INPUT_BIND_DESC *pDesc)
{
	const SRefSampler& rSampler	=	m_ConstBuffer.m_pShaderDesc->Sampler()[ResourceIndex];
	pDesc->Type				=	D3D10_SIT_TEXTURE;
	pDesc->BindCount	=	1;
	pDesc->BindPoint	=	rSampler.m_SamplerIndex;
	pDesc->Name				=	&m_ConstBuffer.m_pShaderDesc->NameTable()[rSampler.m_NameIndex];

}

void CCryDXPSShaderReflection::GetInputParameterDesc(uint32 ParameterIndex, D3D11_SIGNATURE_PARAMETER_DESC *pDesc)
{
	const SRefVertexAttribute& rAttribute	=	m_ConstBuffer.m_pShaderDesc->VertexAttribute()[ParameterIndex];
	pDesc->SemanticName		=	&m_ConstBuffer.m_pShaderDesc->NameTable()[rAttribute.m_SemanticNameIndex];
	pDesc->SemanticIndex	=	SEMANTICINDEX[rAttribute.m_InputOffset];//cellGcmCgGetParameterOrdinalNumber(m_ConstBuffer.m_Program,Param);
	pDesc->ReadWriteMask	=	1;


    if (!strnicmp(pDesc->SemanticName, "POSITION", 8))
    {
			if(pDesc->SemanticName[8]!=0)
				pDesc->SemanticIndex	=	pDesc->SemanticName[8]-'0';
    }
    else
    if (!strnicmp(pDesc->SemanticName, "COLOR", 5))
		{
			if(pDesc->SemanticName[5]!=0)
				pDesc->SemanticIndex	=	pDesc->SemanticName[5]-'0';
		}

//	CRY_ASSERT_MESSAGE(0,"CCryShaderReflectionType::GetInputParameterDesc not implemented yet");
//	pDesc->SemanticName = 0;
	//count input parameters
/*	unsigned int Counter=0;
	CGparameter Param=cellGcmCgGetFirstLeafParameter(m_ConstBuffer.m_Program,CG_PROGRAM);
	while(Param)
	{
		const	uint32 InputOffset	=	cellGcmCgGetParameterResource(m_ConstBuffer.m_Program,Param) - CG_ATTR0;
		if(InputOffset<16)
		{
			if(ParameterIndex==Counter)
			{
				pDesc->SemanticName		=	(char*)cellGcmCgGetParameterSemantic(m_ConstBuffer.m_Program,Param);
				pDesc->SemanticIndex	=	cellGcmCgGetParameterOrdinalNumber(m_ConstBuffer.m_Program,Param);
				pDesc->ReadWriteMask	=	1;
				return;
			}
			Counter++;
		}
		Param	=	cellGcmCgGetNextLeafParameter(m_ConstBuffer.m_Program,Param);
	}
	*/
}

void CCryDXPSShaderReflection::GetOutputParameterDesc(uint32 ParameterIndex, D3D11_SIGNATURE_PARAMETER_DESC *pDesc)
{
	CRY_ASSERT_MESSAGE(0,"CCryShaderReflectionType::GetOutputParameterDesc not implemented yet");
	pDesc->SemanticName = 0;
}


