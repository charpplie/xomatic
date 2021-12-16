/************************************************************************/
/* UI element tree for Component HUD, Jan Müller, 2009									*/
/************************************************************************/

#include "StdAfx.h"

#if UI_ENABLE_UI_TEXTELEMENT

#include "HUD/UI/UITextElement.h"

#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h"

#include "Graphics/2DRenderUtils.h"

#include "PerkDbgDisplay.h"
#include "IItemSystem.h"

CTextUIElement::CTextUIElement() 
: m_parent(NULL)
{
	InitializeMemberData( );
}
 
CTextUIElement::~CTextUIElement()
{
}

void CTextUIElement::InitializeMemberData( void )
{
	TTextElementParent::InitializeMemberData();

	m_data = "";
	m_haveValue = false;
}

void CTextUIElement::Update(float frameTime)
{
	TTextElementParent::Update(frameTime);
}

void CTextUIElement::Initialize( const IItemParamsNode* params, IUIElement* parent )
{
	// need to assert?
	assert(parent);
	m_parent = parent;
	//m_type = eType;
	m_flags = 0;

	m_aspectDisplacement = 0;
	params->GetAttribute("aspectDisplacement", m_aspectDisplacement);

	TTextElementParent::Initialize( params, parent );
	HudLog( 1, "CTextUIElement::Initialize - reading sub-element '%s' (subelement of '%s')", m_name.c_str(), (parent?parent->GetName():"noparent") );


	const char * dataStr = params->GetAttribute("data");

	m_data = dataStr;
	if (m_data.length() && (m_data[0] == '@'))
	{
		m_data = g_pGame->GetHUD()->LocalizeString(m_data.c_str(), 0, 0);
	}

	const IItemParamsNode * xmlVis = params->GetChild("visible");
	if( xmlVis )
	{
		if( xmlVis->GetAttributeType( "fixed" ) != eIPT_None )
			m_flags |= eSEF_VALUE_FIXED;
	}

	const IItemParamsNode * xmlVal = params->GetChild("value");
	if( xmlVal )
	{
		if( xmlVal->GetAttributeType( "fixed" ) != eIPT_None )
			m_flags |= eSEF_VALUE_FIXED;
	}

	int noTainting = 0;
	params->GetAttribute("noTainting", noTainting);
	if(noTainting)
		m_flags |= eSEF_NO_TAINTING;

	int fadeOut = 0;
	params->GetAttribute("fadeOut", fadeOut);
	if(fadeOut)
		m_flags |= eSEF_FADE_OUT;
}

void CTextUIElement::Draw( void ) const
{
	if(m_flags & eSEF_NORENDER)
	{
		return;
	}

	CRY_FIXME( 06, 11, 2009, "HUD_Impl: UIElements have no way of being hidden!");

	C2DRenderUtils* pRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();

	float drawPosX = GetPosX();
	float drawPosY = GetPosY();

	float drawHeight = m_height;

	ColorF col(1.0f);
	if(!(m_flags & eSEF_NO_TAINTING))
		col = g_pGame->GetHUD()->GetHUDImpl()->GetBaseColor();
	else if (m_useTintColor)
		col = tintColor;

	if(m_flags & eSEF_FADE_OUT)
	{
		CRY_FIXME( 06, 11, 2009, "HUD_Impl: UIElements have no way of fading out!");
	}

	HudLog( 3, "CTextUIElement::Draw() : Drawing element '%s' at (%f, %f), size is (%f,%f)", GetName(), drawPosX, drawPosY, m_width, m_height );

	static CryFixedStringT<32> text;
	text = m_data.c_str();
	if(m_flags & eSEF_VALUE_CONDITION)
	{
		CRY_FIXME( 06, 11, 2009, "HUD_Impl: UIElements have no support for texture switches!");
		assert(false);		
		text.Format("REPLACEME!");
	}

	EUIDRAWHORIZONTAL h_align = UIDRAWHORIZONTAL_LEFT;
	if( m_flags &  eSEF_ALIGN_CENTER_HORIZ )
	{
		h_align = UIDRAWHORIZONTAL_CENTER;
	}
	else if ( m_flags & eSEF_ALIGN_RIGHT )
	{
		h_align = UIDRAWHORIZONTAL_RIGHT;
	}

	if (!m_useTextColor)
	{
		pRenderUtils->DrawText(drawPosX, drawPosY, m_width, drawHeight, text.c_str(), ColorF(1.0f,1.0f,1.0f,1.0f));

		HudDbgDisplay("UI element [%s] '%s' being rendered at %.2f %.2f (ignore h_align) size=%.1fx%.1f", GetName(), text.c_str(), drawPosX, drawPosY, m_width, drawHeight);
	}
	else
	{
		pRenderUtils->DrawText(drawPosX, drawPosY, m_width, drawHeight, text.c_str(), textColor, h_align);

		HudDbgDisplay("UI element [%s] '%s' being rendered at %.2f %.2f (h_align=%s) size=%.1fx%.1f", GetName(), text.c_str(), drawPosX, drawPosY,
			(h_align == UIDRAWHORIZONTAL_LEFT) ? "LEFT" :
			((h_align == UIDRAWHORIZONTAL_RIGHT) ? "RIGHT" : "CENTRE"), m_width, drawHeight);
	}

	TTextElementParent::Draw();
	return;
}

const float	CTextUIElement::GetPosX() const 
{ 
	if(!m_parent)
	{
		return TTextElementParent::GetPosX();
	}

	// Positioning, relative to parent
	if( !(m_flags & (eSEF_ALIGN_RIGHT|eSEF_ALIGN_CENTER_HORIZ|eSEF_ALIGN_LEFT) ) )
	{
		// Standard (left) align
		return m_posX + m_parent->GetPosX();
	}

	if( m_flags & eSEF_ALIGN_CENTER_HORIZ)
	{
		// centered
		return m_parent->GetPosX() + (m_parent->GetWidth()/2) + m_posX - (m_width/2);
	}

	// right aligned to parent
	return (m_parent->GetPosX() + m_parent->GetWidth()) - m_posX - m_width;

}

void CTextUIElement::SetData(string newData)
{
	m_data = newData;
}

const float	CTextUIElement::GetPosY() const 
{ 
	if(!m_parent)
	{
		return TTextElementParent::GetPosY();
	}

	if( !(m_flags & (eSEF_ALIGN_BOTTOM|eSEF_ALIGN_CENTER_VERT|eSEF_ALIGN_TOP) ) )
	{
		return m_posY + m_parent->GetPosY();
	}
	else if (m_flags & eSEF_ALIGN_TOP)
	{
		return m_parent->GetPosY() + m_posY - m_height;
	}
	else if( m_flags & eSEF_ALIGN_CENTER_HORIZ )
	{
		return m_parent->GetPosY() + (m_parent->GetHeight()/2) + m_posY - (m_height/2);
	}

	return m_parent->GetPosY() + m_parent->GetHeight() - m_posY - m_height;
}

#endif // UI_ENABLE_UI_TEXTELEMENT
