#include "StdAfx.h"
#include "../../../Layer0/CCryDXPS.hpp"
#include "CCryDXPSRenderTargetView.hpp"
#include "CCryDXPSDepthStencilView.hpp"
//#include <assert.h>

void CCryDXPSRenderTargetView::GetDesc(D3D11_RENDER_TARGET_VIEW_DESC *pDesc)
{
}

void CCryDXPSRenderTargetView::GetResource(ID3D11Resource** pResource)
{
	*pResource	=	m_pTex;
	m_pTex->IncRef();
}


