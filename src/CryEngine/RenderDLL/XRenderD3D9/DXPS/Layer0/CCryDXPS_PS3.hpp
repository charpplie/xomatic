//needed for printf debug output trace
#include <stdio.h>



//values
#define GL_FRAMEBUFFER_CRY				GL_FRAMEBUFFER_OES
#define GL_COLOR_ATTACHMENT0_CRY	GL_COLOR_ATTACHMENT0_EXT

//functions
#define glGenFramebuffersCRY			glGenFramebuffersOES
#define glBindFramebufferCRY			glBindFramebufferOES
#define glBindFramebufferCRY			glBindFramebufferOES
#define glFramebufferTexture2DCRY	glFramebufferTexture2DOES

//extras
//#define HDC PSGLcontext*
//#define HGLRC PSGLdevice*
typedef void *HWND;
typedef int HINSTANCE;
typedef long HRESULT;
//typedef long *LRESULT;
typedef uint32 UINT;

//win replacement
struct CryRect
{
    int    left;
    int    top;
    int    right;
    int    bottom;
};


inline void CryZeroMemory(void* pDst,unsigned  int Len)
{
	for(uint32 a=0;a<Len;a++)
		reinterpret_cast<uint8*>(pDst)[a]=0;
}

inline bool CryFailed(int Value)
{
	return Value<0;
}

#define CryPS3Warning(X) CRY_DEBUGOUT(X)

#define CryMakeCurrent(x,y)		(1,psglMakeCurrent(x,y),1)
#define	CryDeleteContext(...)	
#define CryResetContext		psglResetCurrentContext
#define CryReleaseDC(...)
#define wglSwapIntervalEXT(...)
#define Cry_strcpy_s(X,Y,Z)	strcpy(X,Z)
#define Cry_strcat_s(X,Y,Z)	strcat(X,Z)
#if defined(CRY_MM_DEBUG)
	#define CRY_DEBUGOUT(...)	printf(__VA_ARGS__)
#else
	#define CRY_DEBUGOUT(...)
#endif

#define CRY_DEBUGOUT_ALWAYS(...)	printf(__VA_ARGS__)



#ifdef CRY_USE_GCM
#define CryGetClientRect(X,Y) {(Y)->left=(Y)->top=0;	tdLayer0::ScreenSize(((CryRect*)Y)->right,((CryRect*)Y)->bottom);}
#else
inline void CryGetClientRect(HWND,CryRect* pRect)
{
	pRect->left		=	0;
	pRect->top		=	0;
	pRect->right	=	1280;
	pRect->bottom	=	720;
}
#endif


