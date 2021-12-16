
//values
#define GL_FRAMEBUFFER_CRY				GL_FRAMEBUFFER_EXT
#define GL_COLOR_ATTACHMENT0_CRY	GL_COLOR_ATTACHMENT0_EXT


//functions
#define glGenFramebuffersCRY	glGenFramebuffersEXT
#define glBindFramebufferCRY	glBindFramebufferEXT
#define glBindFramebufferCRY	glBindFramebufferEXT
#define glFramebufferTexture2DCRY	glFramebufferTexture2DEXT


//win replacement
#define CryRect	RECT
#define CryGetClientRect GetClientRect
#define CryZeroMemory ZeroMemory
#define CryFailed FAILED
#define CryPS3Warning(X) MessageBox( NULL, X, "Error", MB_OK );
#define CryMakeCurrent		wglMakeCurrent
#define CryDeleteContext	wglDeleteContext
#define CryResetContext()
#define CryReleaseDC			ReleaseDC
#define Cry_strcpy_s			strcpy_s
#define Cry_strcat_s			strcat_s
#define CRY_DEBUGOUT(X)


