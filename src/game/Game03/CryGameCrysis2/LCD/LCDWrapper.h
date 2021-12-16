#ifndef __LCDWRAPPER_H__
#define __LCDWRAPPER_H__

#ifdef USE_G15_LCD

class CEzLcd;

class CLCDWrapper
{
public:
	CLCDWrapper();
	virtual ~CLCDWrapper();

	bool	IsConnected();
	void	Update(float frameTime);

	void*		AddBitmap();
	bool		SetBitmap(void* handle, const char* name);

private:
	CEzLcd*	m_pImpl;
	int			m_currentPage;
	float		m_time;
};

#endif //USE_G15_LCD

#endif