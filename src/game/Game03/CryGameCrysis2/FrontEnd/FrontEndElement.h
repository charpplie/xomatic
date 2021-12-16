#ifndef __FRONTENDELEMENT_H__
#define __FRONTENDELEMENT_H__

//////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"


//////////////////////////////////////////////////////////////////////////


class CFrontEndElement
{
public:

						CFrontEndElement();
						CFrontEndElement(const char* pName, float posX, float posY, float width, float height);
	virtual ~	CFrontEndElement();

	virtual void	Update(float frameTime);
	virtual void	Render();

	virtual bool	IsTabActive() { return false; }
	virtual void	Initialize(XmlNodeRef element);

	void				SetName(const char* name) { m_name = string(name); }
	const char*	GetName() { return m_name.c_str(); }

	void				SetCaption(const char* caption) { m_caption = caption; }
	string			GetCaption() { return m_caption; }

	void				SetAlignment(int alignment) { m_alignment = alignment; }
	int					GetAlignment() { return m_alignment; }

	void				SetPosX(float posX) { m_posX = posX; }
	const float	GetPosX() { return m_posX; }

	void				SetPosY(float posY) { m_posY = posY; }
	const float	GetPosY() { return m_posY; }

	void				SetWidth(float width) { m_width = width; }
	const float	GetWidth() { return m_width; }

	void				SetHeight(float height) { m_height = height; }
	const float	GetHeight() { return m_height; }

	void				SetOpacity(float opacity) { m_opacity = opacity; }
	float				GetOpacity()
	{
		if(m_fadingOut>=0.0f)
			return m_opacity * m_fadingOut;
		else if(m_fadingIn>=0.0f)
			return m_opacity * (1.0f - m_fadingIn);
		else
			return m_opacity;
	}

	void				SetFadeIn(float fade) { m_fadeIn = fade; }
	const float	GetFadeIn() { return m_fadeIn; }

	void				SetFadeOut(float fade) { m_fadeOut = fade; }
	const float	GetFadeOut() { return m_fadeOut; }

	bool				IsFading() { return m_fadingIn>0.0f || m_fadingOut>0.0f; }

	void				Fade(bool fadeOut = true);

protected:

	string	m_name;
	string m_caption;

	int m_alignment;

	float m_posX;
	float m_posY;
	float m_width;
	float	m_height;
	float m_opacity;
	
	float m_fadeIn;
	float m_fadeOut;
	float m_fadingIn;
	float m_fadingOut;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class CFrontEndPane : public CFrontEndElement
{
public:

	CFrontEndPane();
	virtual ~	CFrontEndPane();

	virtual void	Update(float frameTime);
	virtual void	Render();

	virtual void	Initialize(XmlNodeRef element);
	virtual bool	IsTabActive() { return false; }

	void				SetTexture(ITexture* texture) { m_normal = texture; }
	ITexture*		GetTexture() { return m_normal; }

private:

	ITexture* m_normal;

};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class CFrontEndInteractive : public CFrontEndElement
{
public:

						CFrontEndInteractive();
	virtual ~	CFrontEndInteractive();

	virtual void	Update(float frameTime);
	virtual void	Render();

	virtual void	Initialize(XmlNodeRef element);
	virtual bool	IsTabActive() { return true; }

	virtual bool	Input(int input, int state);

	void				SetActive(bool active) { m_active = active; }
	bool				GetActive() { return m_active; }

	void				SetEnabled(bool enabled) { m_enabled = enabled; }
	bool				GetEnabled() { return m_enabled; }

	void				SetCommand(const char* command) { m_command = string(command); }
	string			GetCommand() { return m_command; }

	void				SetParameter(const char* param) { m_param = string(param); }
	string			GetParameter() { return m_param; }

	void				SetNormalTexture(ITexture* texture) { m_normal = texture; }
	ITexture*		GetNormalTexture() { return m_normal; }

	void				SetHoverTexture(ITexture* texture) { m_hover = texture; }
	ITexture*		GetHoverTexture() { return m_hover; }

	void				SetDisabledTexture(ITexture* texture) { m_disabled = texture; }
	ITexture*		GetDisabledTexture() { return m_disabled; }

private:

	bool m_active;
	bool m_enabled;

	string m_command;
	string m_param;

	ITexture* m_normal;
	ITexture* m_hover;
	ITexture* m_disabled;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class CFrontEndButton : public CFrontEndInteractive
{
public:

	CFrontEndButton();
	virtual ~	CFrontEndButton();

	virtual void	Update(float frameTime);
	virtual void	Render();

	virtual void	Initialize(XmlNodeRef element);
	virtual bool	Input(int input, int state);


	void				SetPressed(bool pressed) { m_pressed = pressed; }
	bool				GetPressed() { return m_pressed; }

	void				SetPressTexture(ITexture* texture) { m_press = texture; }
	ITexture*		GetPressTexture() { return m_press; }

	void				SetSoundEnabled(bool enabled) { m_soundEnabled = enabled; }
	const bool	GetSoundEnabled() { return m_soundEnabled; }

private:

	bool m_pressed;
	ITexture* m_press;
	bool m_soundEnabled;

};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class CFrontEndSlider : public CFrontEndInteractive
{
public:

	CFrontEndSlider();
	virtual ~	CFrontEndSlider();

	virtual void	Update(float frameTime);
	virtual void	Render();

	virtual void	Initialize(XmlNodeRef element);
	virtual bool	Input(int input, int state);

	void				SetLowTexture(ITexture* texture) { m_low = texture; }
	ITexture*		GetLowTexture() { return m_low; }

	void				SetHighTexture(ITexture* texture) { m_high = texture; }
	ITexture*		GetHighTexture() { return m_high; }

	void				SetMargin(float margin) { m_margin = margin; }
	float				GetMargin() { return m_margin; }

	void				SetOffset(float offset) { m_offset = offset; }
	float				GetOffset() { return m_offset; }

	void				SetSteps(int steps) { m_steps = steps; }
	int					GetSteps() { return m_steps; }

	void				SetStart(float start) { m_start = start; }
	float				GetStart() { return m_start; }

	void				SetEnd(float end) { m_end = end; }
	float				GetEnd() { return m_end; }

	void				SetCurrentStep(int step) { m_currentStep = step; }
	int					GetCurrentStep() { return m_currentStep; }

private:

	int m_steps;
	int m_currentStep;
	float m_margin;
	float m_offset;
	float m_start;
	float m_end;
	ITexture* m_low;
	ITexture* m_high;

};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class CFrontEndSwitch : public CFrontEndInteractive
{
public:

	CFrontEndSwitch();
	virtual ~	CFrontEndSwitch();

	virtual void		Update(float frameTime);
	virtual void		Render();

	virtual void		Initialize(XmlNodeRef element);
	virtual bool		Input(int input, int state);

	int										NumOptions() { return m_options.size(); }
	void									AddOption(CFrontEndInteractive* pane) {m_options.push_back(pane); }
	CFrontEndInteractive*	GetOptionByIndex(int index);
	void									ClearOptions();

	void						SetCurrentOption(int option) { m_currentOption = option; }
	int							GetCurrentOption() { return m_currentOption; }

private:

	int m_currentOption;
	typedef std::vector<CFrontEndInteractive*> Options;
	Options m_options;

};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CFrontEndProgressBar : public CFrontEndInteractive
{
public:

	CFrontEndProgressBar();
	virtual ~	CFrontEndProgressBar();

	virtual void	Update(float frameTime);
	virtual void	Render();

	virtual void	Initialize(XmlNodeRef element);
	virtual bool	Input(int input, int state);

	void					SetProgress(float progress) { m_progress = progress; }
	const float		GetProgress() { return m_progress; }

	void					SetStartPixel(float start) { m_start = start; }
	const float		GetStartPixel() { return m_start; }

	void					SetEndPixel(float end) { m_end = end; }
	const float		GetEndPixel() { return m_end; }

private:

	float	m_progress;
	float	m_start;
	float	m_end;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CFrontEndRandomLoadingMessage : public CFrontEndInteractive
{
	public:
	CFrontEndRandomLoadingMessage();
	virtual void	Render();
	virtual void	Initialize(XmlNodeRef element);

	private:
	static const unsigned int kMaxLines = 3;
	float m_fontSize;
	string m_message[kMaxLines];

	void PickRandomMessage();
	void AddLineOfText(const string & newText, int numLinesFilledIn);
};

//////////////////////////////////////////////////////////////////////////

#endif
