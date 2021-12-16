/************************************************************************/
/* UI element tree for Component HUD, Jan Müller, 2009									*/
/************************************************************************/

#include "StdAfx.h"

#include "HUD/UI/UIElement.h"

#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h"

#include "StringUtils.h"
#include "IItemSystem.h"

#if 0
CSubUIElement::CSubUIElement() //const IItemParamsNode * xmlElement, EUISubElementType eType, IUIElement *in_parent /*= NULL*/) 
: m_parent(NULL)
{
	InitializeMemberData( );


	//Initialize(xmlElement, m_parent);
}
 
CSubUIElement::~CSubUIElement()
{
}

void CSubUIElement::InitializeMemberData( void )
{
	TSubElementParent::InitializeMemberData();

	m_data = "";
	m_haveValue = false;
}

void CSubUIElement::Update(float frameTime)
{
	if(        m_flags & eSEF_VALUE_CONDITION
	    && ( !(m_flags & eSEF_VALUE_FIXED) || !m_haveValue ) )
	{
		CRY_FIXME( 06, 11, 2009, "HUD_Impl: UIElements have no data!");
		m_haveValue = true;
	}

	int numElements = m_subElements.size();
	for(int e = 0; e < numElements; ++e)
		m_subElements[e]->Update(frameTime);

	UpdatePopup();
}

void CSubUIElement::Initialize( const IItemParamsNode* params, IUIElement* parent )
{
	assert(parent);
	m_parent = parent;
	//m_type = eType;
	m_flags = 0;

	m_aspectDisplacement = 0;
	params->GetAttribute("aspectDisplacement", m_aspectDisplacement);

	TSubElementParent::Initialize( params );
	HudLog( 1, "CSubUIElement::Initialize - reading sub-element '%s' (subelement of '%s')", name.c_str(), (parent?parent->GetName():"noparent") );

	const IItemParamsNode * xmlSize = params->GetChild("Size");
	// no size specified but we have a parent, use the parent's size.
	// unless we have an image, in which case draw the image at it's default size.
	if( !xmlSize )
	{
#if 0
		if( m_type == eSET_TEXTURE )
		{
			m_width = 0.0f; // HUP_Imp draw 0x0 images at source size.
			m_height = 0.0f;
		}
		else 
#endif
			if( parent )
		{
			m_width = parent->GetWidth();
			m_height = parent->GetHeight();
		}
	}

	memset (& m_paramsUnion, 0, sizeof(m_paramsUnion));

	const IItemParamsNode * xmlPos = params->GetChild("Position");
	if(xmlPos)
	{
		const char * relationship_vars = xmlPos->GetAttribute( "relation" );
		if( relationship_vars )
		{
			if( CryStringUtils::stristr(relationship_vars, "bottom") != NULL )
			{
				m_flags |= eSEF_ALIGN_BOTTOM; // turn on bottom-relative rendering, hehe I said bottom.
			}

			if( CryStringUtils::stristr(relationship_vars, "right") != NULL )
			{
				m_flags |= eSEF_ALIGN_RIGHT; // turn off left-relative rendering, turning on right-relative.
			}

			if( CryStringUtils::stristr(relationship_vars, "center") != NULL )
			{
				m_flags |= eSEF_ALIGN_CENTER_HORIZ|eSEF_ALIGN_CENTER_VERT; 
			}

			if( CryStringUtils::stristr(relationship_vars, "top") != NULL )
			{
				m_flags |= eSEF_ALIGN_TOP;
			}

			if( CryStringUtils::stristr(relationship_vars, "left") != NULL )
			{
				m_flags |= eSEF_ALIGN_LEFT;
			}
		}
	}

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

void CSubUIElement::Draw( void ) const
{
	if(m_flags & eSEF_NORENDER)
	{
		return;
	}

	CRY_FIXME( 06, 11, 2009, "HUD_Impl: UIElements have no way of being hidden!");

	CHUD_Impl* pHudInterface = g_pGame->GetHUD()->GetHUDImpl();

	float drawPosX = GetPosX();
	float drawPosY = GetPosY();

	float drawHeight = m_height;

	ColorF col(1.0f);
	if(!(m_flags & eSEF_NO_TAINTING))
		col = pHudInterface->GetBaseColor();
	else if (m_useTintColor)
		col = tintColor;

	if(m_flags & eSEF_FADE_OUT)
	{
		CRY_FIXME( 06, 11, 2009, "HUD_Impl: UIElements have no way of fading out!");
	}

	HudLog( 3, "CSubUIElement::Draw() : Drawing element '%s' at (%f, %f), size is (%f,%f)", GetName(), drawPosX, drawPosY, m_width, m_height );
#if 0
	if(m_type == eSET_TEXTURE)
	{
		pHudInterface->DrawTexture(m_data.c_str(), drawPosX, drawPosY, m_width, drawHeight, col );
	}
	else if(m_type == eSET_SLIDER && (m_flags & eSEF_VALUE_CONDITION))
	{
		CRY_FIXME( 06, 11, 2009, "HUD_Impl: UIElements have no support for sliders!");
		float drawWidth = m_width;
		pHudInterface->DrawTexture(m_data.c_str(), drawPosX, drawPosY, drawWidth, drawHeight, col );
	}
	else if(m_type == eSET_TEXTURE_SWITCH && (m_flags & eSEF_VALUE_CONDITION))
	{
		CRY_FIXME( 06, 11, 2009, "HUD_Impl: UIElements have no support for texture switches!");
		assert(false);// shouldn't be called?
	}
	else if(m_type == eSET_TEXT || m_type == eSET_TEXT_WITH_INT_DATA || m_type == eSET_TEXT_WITH_STRING_DATA )
	{
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
			pHudInterface->DrawText(text.c_str(), drawPosX, drawPosY, m_width, drawHeight);

			HudDbgDisplay("UI element [%s] '%s' being rendered at %.2f %.2f (ignore h_align) size=%.1fx%.1f", GetName(), text.c_str(), drawPosX, drawPosY, m_width, drawHeight);
		}
		else
		{
			pHudInterface->DrawText(text.c_str(), drawPosX, drawPosY, m_width, drawHeight, textColor, h_align);

			HudDbgDisplay("UI element [%s] '%s' being rendered at %.2f %.2f (h_align=%s) size=%.1fx%.1f", GetName(), text.c_str(), drawPosX, drawPosY,
				(h_align == UIDRAWHORIZONTAL_LEFT) ? "LEFT" :
				((h_align == UIDRAWHORIZONTAL_RIGHT) ? "RIGHT" : "CENTRE"), m_width, drawHeight);
		}
	}
#endif
	TSubElementParent::Draw();
	return;
}

const float	CSubUIElement::GetPosX() const 
{ 
	if(!m_parent)
	{
		return TSubElementParent::GetPosX();
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

void CSubUIElement::SetData(string newData)
{
	m_data = newData;
}

const float	CSubUIElement::GetPosY() const 
{ 
	if(!m_parent)
	{
		return TSubElementParent::GetPosY();
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
#endif