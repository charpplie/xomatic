#include "StdAfx.h"

#include "FrontEndElement.h"
#include "FrontEnd.h"
#include "FrontEnd/ILoadingMessageProvider.h"

#include "GameCVars.h"

#include "IRenderer.h"

//////////////////////////////////////////////////////////////////////////


CFrontEndElement::CFrontEndElement() :
m_name(""),
m_caption(""),
m_alignment(eFEA_Center),
m_posX(0),
m_posY(0),
m_width(100),
m_height(100),
m_opacity(1.0f),
m_fadeIn(0.0f),
m_fadeOut(0.0f),
m_fadingIn(-1.0f),
m_fadingOut(-1.0f)
{
}



CFrontEndElement::CFrontEndElement(const char* pName, float posX, float posY, float width, float height) :
m_name(pName),
m_caption(""),
m_alignment(eFEA_Center),
m_posX(posX),
m_posY(posY),
m_width(width),
m_height(height),
m_opacity(1.0f),
m_fadeIn(0.0f),
m_fadeOut(0.0f),
m_fadingIn(-1.0f),
m_fadingOut(-1.0f)
{
}



CFrontEndElement::~CFrontEndElement()
{
}



void CFrontEndElement::Initialize(XmlNodeRef element)
{
	for(int attr = 0; attr < element->getNumAttributes(); ++attr)		
	{
		const char *key, *value;
		if(element->getAttributeByIndex(attr, &key, &value))
		{
			if(!stricmp(key, "name"))
				SetName(value);
		}
	}

	for(int i = 0; i < element->getChildCount(); ++i)
	{
		XmlNodeRef child = element->getChild(i);
		if(!stricmp(child->getTag(),"Position"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					if(!stricmp(key, "x"))
						SetPosX((float)atof(value));
					else if(!stricmp(key, "y"))
						SetPosY((float)atof(value));
				}
			}
		}
		else if(!stricmp(child->getTag(),"Size"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					if(!stricmp(key, "width"))
						SetWidth((float)atof(value));
					else if(!stricmp(key, "height"))
						SetHeight((float)atof(value));
				}
			}
		}
		if(!stricmp(child->getTag(),"Caption"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					if(!stricmp(key, "text"))
						SetCaption(value);
					else if(!stricmp(key, "align"))
					{
						if(!stricmp(key, "Left"))
							SetAlignment(eFEA_Left);
						else if(!stricmp(key, "Right"))
							SetAlignment(eFEA_Right);
						else
							SetAlignment(eFEA_Center);
					}
				}
			}
		}
		if(!stricmp(child->getTag(),"Effects"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					if(!stricmp(key, "fadein"))
						SetFadeIn((float)atof(value));
					else if(!stricmp(key, "fadeout"))
						SetFadeOut((float)atof(value));
				}
			}
		}
		if(!stricmp(child->getTag(),"Opacity"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					if(!stricmp(key, "value"))
						SetOpacity((float)atof(value));
				}
			}
		}
	}
}



void CFrontEndElement::Fade(bool fadeOut)
{
	if(fadeOut && GetFadeOut()>0.0f)
	{
		if(m_fadingIn>0.0f)
		{
			m_fadingOut = 1.0f - m_fadingIn;
			m_fadingIn = -1.0f;
		}
		else
		{
			m_fadingIn = -1.0f;
			m_fadingOut =	1.0f;
		}
	}
	else if(!fadeOut && GetFadeIn()>0.0f)
	{
		m_fadingOut = -1.0f;
		m_fadingIn =	1.0f;
	}
}



void CFrontEndElement::Update(float frameTime)
{
	if(m_fadingOut>0.0f)
	{
		float current = m_fadingOut * GetFadeOut();
		current = max(0.0f, current - frameTime);
		m_fadingOut = current / max(GetFadeOut(), 0.01f);
	}
	else if(m_fadingIn>0.0f)
	{
		float current = m_fadingIn * GetFadeIn();
		current = max(0.0f, current - frameTime);
		m_fadingIn = current / max(GetFadeIn(), 0.01f);
	}
}



void CFrontEndElement::Render()
{
	if(g_pGameCVars->hud_DebugFrontEnd)
	{
		CFrontEnd::Get()->DrawLine(GetPosX()						, GetPosY()							, GetPosX()+GetWidth()	, GetPosY());
		CFrontEnd::Get()->DrawLine(GetPosX()						, GetPosY()							, GetPosX()							, GetPosY()+GetHeight());
		CFrontEnd::Get()->DrawLine(GetPosX()						, GetPosY()+GetHeight()	, GetPosX()+GetWidth()	, GetPosY()+GetHeight());
		CFrontEnd::Get()->DrawLine(GetPosX()+GetWidth()	, GetPosY()							, GetPosX()+GetWidth()	, GetPosY()+GetHeight());
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CFrontEndPane::CFrontEndPane() :
m_normal(NULL)
{
}



CFrontEndPane::~CFrontEndPane()
{
	if(m_normal)
		gEnv->pRenderer->RemoveTexture(m_normal->GetTextureID());
}



void CFrontEndPane::Initialize(XmlNodeRef element)
{
	CFrontEndElement::Initialize(element);

	for(int attr = 0; attr < element->getNumAttributes(); ++attr)		
	{
		const char *key, *value;
		if(element->getAttributeByIndex(attr, &key, &value))
		{
			//attribs
		}
	}

	for(int i = 0; i < element->getChildCount(); ++i)
	{
		XmlNodeRef child = element->getChild(i);
		if(!stricmp(child->getTag(),"Textures"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					if(!stricmp(key, "normal"))
					{
						string tex(value);
#if defined(PS3)
						tex.replace("%con%", "ps3");
#else
						tex.replace("%con%", "xbox");
#endif

						SetTexture(gEnv->pRenderer->EF_LoadTexture(tex.c_str(), FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D));
					}
				}
			}
		}
	}
}



void CFrontEndPane::Update(float frameTime)
{
	CFrontEndElement::Update(frameTime);
}



void CFrontEndPane::Render()
{
	CFrontEndElement::Render();
	if(m_normal)
		CFrontEnd::Get()->DrawTexture(GetTexture(), GetPosX(), GetPosY(), GetWidth(), GetHeight(), GetOpacity());
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CFrontEndInteractive::CFrontEndInteractive() :
m_active(false),
m_enabled(true),
m_normal(NULL),
m_hover(NULL),
m_disabled(NULL)
{
}



CFrontEndInteractive::~CFrontEndInteractive()
{
	if(m_normal)
		gEnv->pRenderer->RemoveTexture(m_normal->GetTextureID());
	if(m_hover)
		gEnv->pRenderer->RemoveTexture(m_hover->GetTextureID());
	if(m_disabled)
		gEnv->pRenderer->RemoveTexture(m_disabled->GetTextureID());
}



void CFrontEndInteractive::Initialize(XmlNodeRef element)
{
	CFrontEndElement::Initialize(element);

	for(int attr = 0; attr < element->getNumAttributes(); ++attr)		
	{
		const char *key, *value;
		if(element->getAttributeByIndex(attr, &key, &value))
		{
			if(!stricmp(key, "command"))
				SetCommand(value);
			else if(!stricmp(key, "param"))
				SetParameter(value);
			else if(!stricmp(key, "enabled"))
				SetEnabled(stricmp(value, "1")==0);
		}
	}

	for(int i = 0; i < element->getChildCount(); ++i)
	{
		XmlNodeRef child = element->getChild(i);
		if(!stricmp(child->getTag(),"Textures"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					if(!stricmp(key, "normal"))
						SetNormalTexture(gEnv->pRenderer->EF_LoadTexture(value, FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D));
					else if(!stricmp(key, "hover"))
						SetHoverTexture(gEnv->pRenderer->EF_LoadTexture(value, FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D));
					else if(!stricmp(key, "disabled"))
						SetDisabledTexture(gEnv->pRenderer->EF_LoadTexture(value, FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D));
				}
			}
		}
	}
}



bool CFrontEndInteractive::Input(int input, int state)
{
	return false;
}



void CFrontEndInteractive::Update(float frameTime)
{
	CFrontEndElement::Update(frameTime);
	if(!GetEnabled() && GetActive())
		SetActive(false);
}



void CFrontEndInteractive::Render()
{
	CFrontEndElement::Render();

	if(!GetNormalTexture() && !GetHoverTexture() && !GetDisabledTexture())
	{
		ColorF color(0.3f, 0.3f, 0.3f, 0.4f * GetOpacity());
		if(!GetEnabled())
		{
			color = ColorF(0.0f, 0.0f, 0.0f, 0.7f * GetOpacity());
		}
		else if(GetActive())
		{
			color = ColorF(1.0f, 1.0f, 1.0f, GetOpacity());
		}

		CFrontEnd::Get()->DrawFont(GetCaption(), GetPosX(), GetPosY(), GetWidth()*0.5f, GetHeight()*0.5f, eFEA_Center, 8.0f, color);
		return;
	}

	ITexture* pTexture = GetNormalTexture();
	if(!GetEnabled())
	{
		if(GetDisabledTexture())
			pTexture = GetDisabledTexture();
	}
	else if(GetActive())
	{
		if(GetHoverTexture())
			pTexture = GetHoverTexture();
	}
	if(pTexture)
		CFrontEnd::Get()->DrawTexture(pTexture, GetPosX(), GetPosY(), GetWidth(), GetHeight(), GetOpacity());
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CFrontEndButton::CFrontEndButton() :
m_press(NULL),
m_pressed(false),
m_soundEnabled(true)
{
}



CFrontEndButton::~CFrontEndButton()
{
	if(m_press)
		gEnv->pRenderer->RemoveTexture(m_press->GetTextureID());
}



void CFrontEndButton::Initialize(XmlNodeRef element)
{
	CFrontEndInteractive::Initialize(element);

	for(int attr = 0; attr < element->getNumAttributes(); ++attr)		
	{
		const char *key, *value;
		if(element->getAttributeByIndex(attr, &key, &value))
		{
			//attribs
		}
	}

	for(int i = 0; i < element->getChildCount(); ++i)
	{
		XmlNodeRef child = element->getChild(i);
		if(!stricmp(child->getTag(),"Textures"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					if(!stricmp(key, "press"))
						SetPressTexture(gEnv->pRenderer->EF_LoadTexture(value, FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D));
				}
			}
		}
		if(!stricmp(child->getTag(),"Sound"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					SetSoundEnabled(atoi(value)!=0);
				}
			}
		}
	}
}



bool CFrontEndButton::Input(int input, int state)
{
	bool handled = CFrontEndInteractive::Input(input, state);
	if(!handled)
	{
		if(input==eFEIT_Ok)
		{
			if(state==eFEIS_Press)
			{
				SetPressed(true);
				
				if(GetSoundEnabled())
					CFrontEnd::Get()->PlaySound(eFES_Confirm,true);
				
				if(CFrontEnd::Get()->Execute(GetCommand(), GetParameter()))
					SetPressed(false);
				handled = true;
			}
			else if(state==eFEIS_Release)
			{
				SetPressed(false);
				handled = true;
			}
		}
	}
	return handled;
}



void CFrontEndButton::Update(float frameTime)
{
	CFrontEndInteractive::Update(frameTime);
	if(!GetActive() && GetPressed())
		SetPressed(false);
}



void CFrontEndButton::Render()
{
	if(GetEnabled() && GetPressed())
	{
		if(GetPressTexture())
			CFrontEnd::Get()->DrawTexture(GetPressTexture(), GetPosX(), GetPosY(), GetWidth(), GetHeight(), GetOpacity());
	}
	else
	{
		CFrontEndInteractive::Render();
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CFrontEndSlider::CFrontEndSlider() :
m_steps(0),
m_currentStep(0),
m_margin(0),
m_start(0),
m_end(1.0f),
m_low(NULL),
m_high(NULL)
{
}



CFrontEndSlider::~CFrontEndSlider()
{
	if(m_low)
		gEnv->pRenderer->RemoveTexture(m_low->GetTextureID());
	if(m_high)
		gEnv->pRenderer->RemoveTexture(m_high->GetTextureID());
}



void CFrontEndSlider::Initialize(XmlNodeRef element)
{
	CFrontEndInteractive::Initialize(element);

	for(int attr = 0; attr < element->getNumAttributes(); ++attr)		
	{
		const char *key, *value;
		if(element->getAttributeByIndex(attr, &key, &value))
		{
			if(!stricmp(key, "steps"))
				SetSteps(atoi(value));
			else if(!stricmp(key, "default"))
				SetCurrentStep(atoi(value));
			else if(!stricmp(key, "start"))
				SetStart((float)atof(value));
			else if(!stricmp(key, "end"))
				SetEnd((float)atof(value));
		}
	}

	for(int i = 0; i < element->getChildCount(); ++i)
	{
		XmlNodeRef child = element->getChild(i);
		if(!stricmp(child->getTag(),"SliderTextures"))
		{
			for(int attr = 0; attr < child->getNumAttributes(); ++attr)		
			{
				const char *key, *value;
				if(child->getAttributeByIndex(attr, &key, &value))
				{
					if(!stricmp(key, "low"))
						SetLowTexture(gEnv->pRenderer->EF_LoadTexture(value, FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D));
					else if(!stricmp(key, "high"))
						SetHighTexture(gEnv->pRenderer->EF_LoadTexture(value, FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D));
					else if(!stricmp(key, "margin"))
						SetMargin((float)atof(value));
					else if(!stricmp(key, "offset"))
						SetOffset((float)atof(value));
				}
			}
		}
	}

	string cur = CFrontEnd::Get()->GetCurrentValue(GetCommand());

	float range = GetEnd() - GetStart();
	int curStep(0);

	if(range!=0.0f)
	{
		float value = (float)atof(cur);
		float ratio = (value-GetStart()) / range;
		curStep = (int)((ratio) * (float)GetSteps() + 0.5f);
	}

	SetCurrentStep(curStep);
}



bool CFrontEndSlider::Input(int input, int state)
{
	bool handled = CFrontEndInteractive::Input(input, state);
	if(!handled)
	{
		if(state==eFEIS_Press)
		{
			bool submit = false;
			if(input==eFEIT_Left)
			{
				if(GetCurrentStep()>0)
				{
					SetCurrentStep(GetCurrentStep()-1);
					submit = true;
				}
				handled = true;
			}
			else if(input==eFEIT_Right)
			{
				if(GetCurrentStep()<GetSteps())
				{
					SetCurrentStep(GetCurrentStep()+1);
					submit = true;
				}
				handled = true;
			}

			if(submit)
			{
				float value = ((float)GetCurrentStep() / (float)GetSteps());
				float range = GetEnd() - GetStart();

				char param[32];
				sprintf(param,"%f",((value*range) + GetStart()));

				CFrontEnd::Get()->PlaySound(eFES_Move,true);
				CFrontEnd::Get()->Execute(GetCommand(), param);
			}
		}
	}
	return handled;
}



void CFrontEndSlider::Update(float frameTime)
{
	CFrontEndInteractive::Update(frameTime);
}



void CFrontEndSlider::Render()
{
	CFrontEndInteractive::Render();
	if(!GetLowTexture() || !GetHighTexture())
		return;

	float width = (float)GetHighTexture()->GetWidth();
	float height = (float)GetHighTexture()->GetHeight();

	float posY = GetPosY() + ((GetHeight()-height) * 0.5f);
	for(int i=0; i<GetSteps(); ++i)
	{
		ITexture *pTexture = NULL;
		if(GetCurrentStep()>i)
			pTexture = GetHighTexture();
		else
			pTexture = GetLowTexture();

		float posX = GetPosX() + GetOffset() + i*GetMargin();

		CFrontEnd::Get()->DrawTexture(pTexture, posX, posY, width, height, GetOpacity());
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CFrontEndSwitch::CFrontEndSwitch() :
m_currentOption(0)
{
	m_options.resize(0);
}



CFrontEndSwitch::~CFrontEndSwitch()
{
	ClearOptions();
}



void CFrontEndSwitch::Initialize(XmlNodeRef element)
{
	CFrontEndInteractive::Initialize(element);

	for(int attr = 0; attr < element->getNumAttributes(); ++attr)		
	{
		const char *key, *value;
		if(element->getAttributeByIndex(attr, &key, &value))
		{
			//attribs
		}
	}

	for(int i = 0; i < element->getChildCount(); ++i)
	{
		XmlNodeRef child = element->getChild(i);
		if(!stricmp(child->getTag(),"Option"))
		{
			CFrontEndInteractive* pInteractive = new CFrontEndInteractive();
			pInteractive->Initialize(child);
			AddOption(pInteractive);
		}
	}

	string cur = CFrontEnd::Get()->GetCurrentValue(GetCommand());
	for(int i = 0; i < NumOptions(); ++i)
	{
		if(cur == GetOptionByIndex(i)->GetName())
		{
			SetCurrentOption(i);
			break;
		}
	}

}



bool CFrontEndSwitch::Input(int input, int state)
{
	bool handled = CFrontEndInteractive::Input(input, state);

	if(NumOptions()<=0)
		return handled;

	if(!handled)
	{
		if(state==eFEIS_Press)
		{
			bool submit = false;
			if(input==eFEIT_Left)
			{
				if(GetCurrentOption()>0)
				{
					SetCurrentOption(GetCurrentOption()-1);
				}
				else
				{
					SetCurrentOption(NumOptions()-1);
				}
				submit = true;
				handled = true;
			}
			else if(input==eFEIT_Right)
			{
				if(GetCurrentOption()<(NumOptions()-1))
				{
					SetCurrentOption(GetCurrentOption()+1);
				}
				else
				{
					SetCurrentOption(0);
				}
				submit = true;
				handled = true;
			}

			if(submit)
			{
				string option = GetOptionByIndex(GetCurrentOption())->GetName();
				CFrontEnd::Get()->PlaySound(eFES_Move,true);
				CFrontEnd::Get()->Execute(GetCommand(), option.c_str());
			}
		}
	}
	return handled;
}



CFrontEndInteractive*  CFrontEndSwitch::GetOptionByIndex(int index)
{
	if(index<0 || index>=NumOptions())
		return NULL;
	return m_options[index];
}



void CFrontEndSwitch::ClearOptions()
{
	while(m_options.size()>0)
	{
		CFrontEndElement* pElement = m_options.back(); m_options.pop_back();
		SAFE_DELETE(pElement);
	}
}



void CFrontEndSwitch::Update(float frameTime)
{
	CFrontEndInteractive::Update(frameTime);

	CFrontEndInteractive* pCurrent = GetOptionByIndex(GetCurrentOption());
	if(!pCurrent)
		return;
	pCurrent->SetActive(GetActive());
	pCurrent->SetEnabled(GetEnabled());
	pCurrent->Update(frameTime);
	pCurrent->SetOpacity(GetOpacity());
}



void CFrontEndSwitch::Render()
{
	CFrontEndInteractive::Render();

	CFrontEndInteractive* pCurrent = GetOptionByIndex(GetCurrentOption());
	if(!pCurrent)
		return;
	pCurrent->Render();
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CFrontEndProgressBar::CFrontEndProgressBar() :
m_progress(0.0f),
m_start(0.0f),
m_end(100.0f)
{
}



CFrontEndProgressBar::~CFrontEndProgressBar()
{
}



void CFrontEndProgressBar::Initialize(XmlNodeRef element)
{
	CFrontEndInteractive::Initialize(element);

	for(int attr = 0; attr < element->getNumAttributes(); ++attr)		
	{
		const char *key, *value;
		if(element->getAttributeByIndex(attr, &key, &value))
		{
			if(!stricmp(key, "startpixel"))
				SetStartPixel((float)atof(value));
			else if(!stricmp(key, "endpixel"))
				SetEndPixel((float)atof(value));
		}
	}

}



bool CFrontEndProgressBar::Input(int input, int state)
{
	return false;
}



void CFrontEndProgressBar::Update(float frameTime)
{
	CFrontEndInteractive::Update(frameTime);
	SetProgress(frameTime);
}



void CFrontEndProgressBar::Render()
{
	float barWidthPixel = GetEndPixel() - GetStartPixel();
	float barWidth = barWidthPixel * GetProgress();

	float toPixel = min(GetEndPixel(),(GetStartPixel() + barWidth));
	float toRatio = 1.0f;
	if(GetWidth()!=0.0f)
		toRatio = min(1.0f, toPixel / GetWidth());

	if(GetNormalTexture())
		CFrontEnd::Get()->DrawTexture(GetNormalTexture(), GetPosX() + toPixel, GetPosY(), GetWidth() - toPixel, GetHeight(), GetOpacity(), toRatio, 1.0f, 1.0f, 0.0f);

	if(GetHoverTexture())
		CFrontEnd::Get()->DrawTexture(GetHoverTexture(), GetPosX(), GetPosY(), toPixel, GetHeight(), GetOpacity(), 0.0f, 1.0f, toRatio, 0.0f);

}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CFrontEndRandomLoadingMessage::CFrontEndRandomLoadingMessage() :
	m_fontSize(1.f)
{
	PickRandomMessage();
}

void CFrontEndRandomLoadingMessage::PickRandomMessage()
{
	string wholeMessage = CLoadingMessageProviderListNode::GetRandomMessage();

	int len = wholeMessage.size();
	int numberOfTimesToSplit = min(kMaxLines - 1, len / 120);
	int numLinesFilledIn = 0;
	int startFromChar = 0;

	CryLogAlways ("Picked message: '%s' (needs splitting %d times)", wholeMessage.c_str(), numberOfTimesToSplit);

	for (int i = 0; i < numberOfTimesToSplit; ++ i)
	{
		// Try to make final line only 80% of length of other lines, because we search BACKWARDS for spaces. Otherwise, the final line
		// of the split-up message will almost always be the longest, which looks kind of rubbish.

		float splitFraction = (i + 1.0f) / (numberOfTimesToSplit + 0.8f);
		
		for (int here = (int)(len * splitFraction); here; -- here)
		{
			if (wholeMessage[here] == ' ')
			{
				AddLineOfText(wholeMessage.substr(startFromChar, here - startFromChar), numLinesFilledIn ++);
				startFromChar = here + 1;
				break;
			}
		}
	}

	AddLineOfText(wholeMessage.substr(startFromChar), numLinesFilledIn);
}

void CFrontEndRandomLoadingMessage::AddLineOfText(const string & newText, int numLinesFilledIn)
{
	assert (numLinesFilledIn < kMaxLines);
	m_message[numLinesFilledIn] = newText;
	CryLog("  LINE %d: '%s'", numLinesFilledIn, m_message[numLinesFilledIn].c_str());

	// Copy colour command from previous line of text, if there was one...
	if (numLinesFilledIn > 0)
	{
		const char * lastString = m_message[numLinesFilledIn - 1].c_str();
		const char * lastColourCommand = strrchr(lastString, '$');

		if (lastColourCommand)
		{
			// Might run into problems if the string actually contains a printable $ character (i.e. in the string as "$$")...
			// For the time being, just assert rather than dealing with it, workaround can be put into place if required!
			assert (lastColourCommand == lastString || lastColourCommand[-1] != '$');

			// Also, for sanity's sake, we'd better check that the next character is something we'd expect to find after a $...
			assert (strchr("1234567890oO", lastColourCommand[1]));

			if (lastColourCommand[1] >= '0' && lastColourCommand[1] <= '9')
			{
				m_message[numLinesFilledIn].Format("$%c%s", lastColourCommand[1], m_message[numLinesFilledIn].c_str());
			}
		}
	}
}

void CFrontEndRandomLoadingMessage::Initialize(XmlNodeRef element)
{
	CFrontEndInteractive::Initialize(element);
	element->getAttr("fontSize", m_fontSize);
}

void CFrontEndRandomLoadingMessage::Render()
{
	float color[4] = {1,1,1,1};
	float y = GetPosY();

	for (int i = 0; i < kMaxLines; ++ i)
	{
		gEnv->pRenderer->Draw2dLabel(GetPosX(), y, m_fontSize, color, true, "%s", m_message[i].c_str());
		y += m_fontSize * 12.f;
	}
}

//////////////////////////////////////////////////////////////////////////