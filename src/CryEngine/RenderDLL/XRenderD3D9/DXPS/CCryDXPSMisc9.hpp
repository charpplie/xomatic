#ifndef __CRYDXPSMISC9__
#define __CRYDXPSMISC9__

#include MATH_H
#include <float.h>

typedef uint8 UINT8;
typedef f32	FLOAT;

#define D3D_OK 0
#define S_OK 0
#define STDMETHODCALLTYPE 
#define STDMETHODVCALLTYPE 
#define THIS void
#define THIS_ 

#define interface struct
#define STDMETHOD(method)       virtual long int STDMETHODCALLTYPE method
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method
#define STDMETHODV(method)       virtual long int STDMETHODVCALLTYPE method
#define STDMETHODV_(type,method) virtual type STDMETHODVCALLTYPE method
#define D3DXToRadian(X)	((X)*(3.14159265f/180.f))
#define TEXT(X) (X)
#define E_INVALIDARG                     0x80070057L
#define D3DGETDATA_FLUSH		(1 << 0)
#define D3DX_PI										3.14159265f
#define CONST											const
#define MAXD3DDECLLENGTH        64

/*
 * Options for clearing
 */
#define D3DCLEAR_TARGET            0x00000001l  /* Clear target surface */
#define D3DCLEAR_ZBUFFER           0x00000002l  /* Clear target z buffer */
#define D3DCLEAR_STENCIL           0x00000004l  /* Clear stencil planes */

#define D3DX_FROM_FILE          ((UINT) -3)

//////////////////////////////////////////////////////////////////////////
//enums
//////////////////////////////////////////////////////////////////////////
#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((unsigned int)(unsigned char)(ch0) | ((unsigned int)(unsigned char)(ch1) << 8) |       \
                ((unsigned int)(unsigned char)(ch2) << 16) | ((unsigned int)(unsigned char)(ch3) << 24 ))
#endif // defined(MAKEFOURCC)
typedef enum _D3DFORMAT
{
    D3DFMT_UNKNOWN              =  0,

    D3DFMT_R8G8B8               = 20,
    D3DFMT_A8R8G8B8             = 21,
    D3DFMT_X8R8G8B8             = 22,
    D3DFMT_R5G6B5               = 23,
    D3DFMT_X1R5G5B5             = 24,
    D3DFMT_A1R5G5B5             = 25,
    D3DFMT_A4R4G4B4             = 26,
    D3DFMT_R3G3B2               = 27,
    D3DFMT_A8                   = 28,
    D3DFMT_A8R3G3B2             = 29,
    D3DFMT_X4R4G4B4             = 30,
    D3DFMT_A2B10G10R10          = 31,
    D3DFMT_A8B8G8R8             = 32,
    D3DFMT_X8B8G8R8             = 33,
    D3DFMT_G16R16               = 34,
    D3DFMT_A2R10G10B10          = 35,
    D3DFMT_A16B16G16R16         = 36,

    D3DFMT_A8P8                 = 40,
    D3DFMT_P8                   = 41,

    D3DFMT_L8                   = 50,
    D3DFMT_A8L8                 = 51,
    D3DFMT_A4L4                 = 52,

    D3DFMT_V8U8                 = 60,
    D3DFMT_L6V5U5               = 61,
    D3DFMT_X8L8V8U8             = 62,
    D3DFMT_Q8W8V8U8             = 63,
    D3DFMT_V16U16               = 64,
    D3DFMT_A2W10V10U10          = 67,

    D3DFMT_UYVY                 = MAKEFOURCC('U', 'Y', 'V', 'Y'),
    D3DFMT_R8G8_B8G8            = MAKEFOURCC('R', 'G', 'B', 'G'),
    D3DFMT_YUY2                 = MAKEFOURCC('Y', 'U', 'Y', '2'),
    D3DFMT_G8R8_G8B8            = MAKEFOURCC('G', 'R', 'G', 'B'),
    D3DFMT_DXT1                 = MAKEFOURCC('D', 'X', 'T', '1'),
    D3DFMT_DXT2                 = MAKEFOURCC('D', 'X', 'T', '2'),
    D3DFMT_DXT3                 = MAKEFOURCC('D', 'X', 'T', '3'),
    D3DFMT_DXT4                 = MAKEFOURCC('D', 'X', 'T', '4'),
    D3DFMT_DXT5                 = MAKEFOURCC('D', 'X', 'T', '5'),

    D3DFMT_D16_LOCKABLE         = 70,
    D3DFMT_D32                  = 71,
    D3DFMT_D15S1                = 73,
    D3DFMT_D24S8                = 75,
    D3DFMT_D24X8                = 77,
    D3DFMT_D24X4S4              = 79,
    D3DFMT_D16                  = 80,

    D3DFMT_D32F_LOCKABLE        = 82,
    D3DFMT_D24FS8               = 83,


    D3DFMT_L16                  = 81,

    D3DFMT_VERTEXDATA           =100,
    D3DFMT_INDEX16              =101,
    D3DFMT_INDEX32              =102,

    D3DFMT_Q16W16V16U16         =110,

    D3DFMT_MULTI2_ARGB8         = MAKEFOURCC('M','E','T','1'),

    // Floating point surface formats

    // s10e5 formats (16-bits per channel)
    D3DFMT_R16F                 = 111,
    D3DFMT_G16R16F              = 112,
    D3DFMT_A16B16G16R16F        = 113,

    // IEEE s23e8 formats (32-bits per channel)
    D3DFMT_R32F                 = 114,
    D3DFMT_G32R32F              = 115,
    D3DFMT_A32B32G32R32F        = 116,

    D3DFMT_CxV8U8               = 117,


    D3DFMT_FORCE_DWORD          =0x7fffffff
} D3DFORMAT;


typedef enum _D3DBLEND
{
	D3DBLEND_ZERO								= 1,
	D3DBLEND_ONE								= 2,
	D3DBLEND_SRCCOLOR						= 3,
	D3DBLEND_INVSRCCOLOR				= 4,
	D3DBLEND_SRCALPHA						= 5,
	D3DBLEND_INVSRCALPHA				= 6,
	D3DBLEND_DESTALPHA					= 7,
	D3DBLEND_INVDESTALPHA				= 8,
	D3DBLEND_DESTCOLOR					= 9,
	D3DBLEND_INVDESTCOLOR				= 10,
	D3DBLEND_SRCALPHASAT				= 11,
	D3DBLEND_BOTHSRCALPHA				= 12,
	D3DBLEND_BOTHINVSRCALPHA		= 13,
	D3DBLEND_BLENDFACTOR				= 14,
	D3DBLEND_INVBLENDFACTOR			= 15,
	D3DBLEND_FORCE_DWORD				= 0x7fffffff
}D3DBLEND;

typedef enum _D3DRESOURCETYPE
{
	D3DRTYPE_SURFACE						= 1,
	D3DRTYPE_VOLUME							= 2,
	D3DRTYPE_TEXTURE						= 3,
	D3DRTYPE_VOLUMETEXTURE			= 4,
	D3DRTYPE_CUBETEXTURE				= 5,
	D3DRTYPE_VERTEXBUFFER				= 6,
	D3DRTYPE_INDEXBUFFER				= 7,
	D3DRTYPE_FORCE_DWORD				= 0x7fffffff
} D3DRESOURCETYPE;

typedef enum _D3DPOOL
{
	D3DPOOL_DEFAULT 						= 0,
	D3DPOOL_MANAGED 						= 1,
	D3DPOOL_SYSTEMMEM						= 2,
	D3DPOOL_SCRATCH							= 3,
	D3DPOOL_FORCE_DWORD 				= 0x7fffffff
} D3DPOOL;


enum FVF
{
	D3DFVF_RESERVED0        =	0x001,
	D3DFVF_XYZ              =	0x002,
	D3DFVF_XYZRHW           =	0x004,
	D3DFVF_XYZB1            =	0x006,
	D3DFVF_XYZB2            =	0x008,
	D3DFVF_XYZB3            =	0x00a,
	D3DFVF_XYZB4            =	0x00c,
	D3DFVF_XYZB5            =	0x00e,
	D3DFVF_XYZW             =	0x4002,

	D3DFVF_NORMAL           =	0x010,
	D3DFVF_PSIZE            =	0x020,
	D3DFVF_DIFFUSE          =	0x040,
	D3DFVF_SPECULAR         =	0x080,

	D3DFVF_TEX0             =	0x000,
	D3DFVF_TEX1             =	0x100,
	D3DFVF_TEX2             =	0x200,
	D3DFVF_TEX3             =	0x300,
	D3DFVF_TEX4             =	0x400,
	D3DFVF_TEX5             =	0x500,
	D3DFVF_TEX6             =	0x600,
	D3DFVF_TEX7             =	0x700,
	D3DFVF_TEX8             =	0x800,
};

enum D3DDEVTYPE
{
	D3DDEVTYPE_HAL					= 1,
	D3DDEVTYPE_REF					= 2,
	D3DDEVTYPE_SW						= 3,
	D3DDEVTYPE_NULLREF			= 4,
	D3DDEVTYPE_FORCE_DWORD  = 0x7fffffff
};

enum D3DPRESENT_INTERVAL
{
	D3DPRESENT_INTERVAL_DEFAULT     =	0x0,
	D3DPRESENT_INTERVAL_ONE         =	0x1,
	D3DPRESENT_INTERVAL_TWO         =	0x2,
	D3DPRESENT_INTERVAL_THREE       =	0x4,
	D3DPRESENT_INTERVAL_FOUR        =	0x8,
	D3DPRESENT_INTERVAL_IMMEDIATE   =	0x80000000,
};

enum D3DCREATE_FLAGS
{
	D3DCREATE_FPU_PRESERVE									=	0x00000002,
	D3DCREATE_MULTITHREADED                 =	0x00000004,

	D3DCREATE_PUREDEVICE                    =	0x00000010,
	D3DCREATE_SOFTWARE_VERTEXPROCESSING     =	0x00000020,
	D3DCREATE_HARDWARE_VERTEXPROCESSING     =	0x00000040,
	D3DCREATE_MIXED_VERTEXPROCESSING        =	0x00000080,

	D3DCREATE_DISABLE_DRIVER_MANAGEMENT     =	0x00000100,
	D3DCREATE_ADAPTERGROUP_DEVICE           =	0x00000200,
	D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX  =	0x00000400,
};



//////////////////////////////////////////////////////////////////////////
//struct
//////////////////////////////////////////////////////////////////////////

typedef uint32 D3DCOLOR; 

typedef struct D3DCOLORVALUE
{
    float r;
    float g;
    float b;
    float a;
};

typedef struct _D3DADAPTER_IDENTIFIER9
{
} D3DADAPTER_IDENTIFIER9;

typedef struct _D3DDISPLAYMODE
{

} D3DDISPLAYMODE;

typedef struct _D3DLOCKED_RECT
{
    int                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;

typedef struct D3DCAPS9
{
};

typedef struct D3DVERTEXELEMENT9 {
	uint16								Stream;
	uint16								Offset;
	uint8									Type;
	uint8									Method;
	uint8									Usage;
	uint8									UsageIndex;
};




typedef struct D3DXFLOAT16
{
protected:
    short value;
public:
    D3DXFLOAT16() {};
		D3DXFLOAT16( float v):value(0xffff){};
		D3DXFLOAT16( const D3DXFLOAT16& v):value(v.value){};

    // casting
		operator float (){return float(value);}

    // binary operators
		bool operator == ( CONST D3DXFLOAT16& rV) const{return rV.value	==	value;}
		bool operator != ( CONST D3DXFLOAT16& rV) const{return !(rV==*this);}

} D3DXFLOAT16, *LPD3DXFLOAT16;

FORCEINLINE
void D3DXFloat32To16Array(D3DXFLOAT16 *pOut,const float *pIn,unsigned int n)
{
	for(uint32 a=0;a<n;a++)
	{
		const	int	Value	=	*reinterpret_cast<const int32*>(&pIn[a]);
		const int Sign	=	(Value&0x80000000) >> 16;
		const int Exp		= ((Value >> 23) & 0x000000ff) - (127 - 15);
		const int Mant	=	Value&0x007fffff;

		*reinterpret_cast<short*>(&pOut[a])	=	Sign|(Exp<<10)|(Mant>>13);
	}
}

typedef struct D3DXVECTOR4
{
	union
	{
		struct
		{
			float x,y,z;
			union
			{
				float w;
				float	d;
			};
		};
		struct
		{
			float r,g,b,a;
		};
	};
    D3DXVECTOR4() {};
		D3DXVECTOR4( const float * pV):x(pV[0]),y(pV[1]),z(pV[2]),w(pV[3]){};
		D3DXVECTOR4( const  D3DXVECTOR4& rV):x(rV.x),y(rV.y),z(rV.z),w(rV.w){};
		D3DXVECTOR4( DWORD dw )
		{
			const float f = 1.0f / 255.0f;
			r = f * (float) (uint8) (dw >> 16);
			g = f * (float) (uint8) (dw >>  8);
			b = f * (float) (uint8) (dw >>  0);
			a = f * (float) (uint8) (dw >> 24);
		}

//    D3DXVECTOR4( CONST D3DXFLOAT16 * );
		D3DXVECTOR4( float X, float Y, float Z, float W ):x(X),y(Y),z(Z),w(W){};

    // casting
		operator float* (){return reinterpret_cast<float*>(this);}
		operator const float* () const{return reinterpret_cast<const float*>(this);}

    // assignment operators
	D3DXVECTOR4& operator += ( const D3DXVECTOR4& rV){x+=rV.x;y+=rV.y;z+=rV.z;w+=rV.w;return *this;}
    D3DXVECTOR4& operator -= ( const D3DXVECTOR4& rV){x-=rV.x;y-=rV.y;z-=rV.z;w-=rV.w;return *this;}
    D3DXVECTOR4& operator *= ( float v){x*=v;y*=v;z*=v;w*=v;return *this;}
    D3DXVECTOR4& operator /= ( float v){x/=v;y/=v;z/=v;w/=v;return *this;}

    // unary operators
    D3DXVECTOR4 operator + () const;
    D3DXVECTOR4 operator - () const;

    // binary operators
		D3DXVECTOR4 operator + ( const D3DXVECTOR4& rV) const{D3DXVECTOR4 T=*this;T+=rV;return T;}
    D3DXVECTOR4 operator - ( const D3DXVECTOR4& rV) const{D3DXVECTOR4 T=*this;T-=rV;return T;}
    D3DXVECTOR4 operator * ( float v) const{D3DXVECTOR4 T=*this;T*=v;return T;}
    D3DXVECTOR4 operator / ( float v) const{D3DXVECTOR4 T=*this;T/=v;return T;}

    friend D3DXVECTOR4 operator * ( float, const struct D3DXVECTOR4& );

		int operator == ( const D3DXVECTOR4& rV) const{return fabsf(x-rV.x)+fabsf(y-rV.y)+fabsf(z-rV.z)+fabsf(w-rV.w)<FLT_EPSILON;}
		int operator != ( const D3DXVECTOR4& rV) const{return !(*this==rV);}
} D3DXVECTOR4, *LPD3DXVECTOR4;

typedef struct D3DXVECTOR3
{
	float x,y,z;
    D3DXVECTOR3() {};
		D3DXVECTOR3( const float * pV):x(pV[0]),y(pV[1]),z(pV[2]){};
//    D3DXVECTOR3( const D3DVECTOR& );
//    D3DXVECTOR3( const D3DXFLOAT16 * );
		D3DXVECTOR3( float X, float Y, float Z ):x(X),y(Y),z(Z){};

    // casting
		operator float* (){return reinterpret_cast<float*>(this);}
		operator const float* () const{return reinterpret_cast<const float*>(this);}

    // assignment operators
		D3DXVECTOR3& operator += ( const D3DXVECTOR3& rV){x+=rV.x;y+=rV.y;z+=rV.z;return *this;}
    D3DXVECTOR3& operator -= ( const D3DXVECTOR3& rV){x-=rV.x;y-=rV.y;z-=rV.z;return *this;}
    D3DXVECTOR3& operator *= ( float v){x*=v;y*=v;z*=v;return *this;}
    D3DXVECTOR3& operator /= ( float v){x/=v;y/=v;z/=v;return *this;}

    // unary operators
    D3DXVECTOR3 operator + () const;
    D3DXVECTOR3 operator - () const;

    // binary operators
		D3DXVECTOR3 operator + ( const D3DXVECTOR3& rV) const{D3DXVECTOR3 T=*this;T+=rV;return T;}
    D3DXVECTOR3 operator - ( const D3DXVECTOR3& rV) const{D3DXVECTOR3 T=*this;T-=rV;return T;}
    D3DXVECTOR3 operator * ( float v) const{D3DXVECTOR3 T=*this;T*=v;return T;}
    D3DXVECTOR3 operator / ( float v) const{D3DXVECTOR3 T=*this;T/=v;return T;}

    friend D3DXVECTOR3 operator * ( float, const struct D3DXVECTOR3& );

		int operator == ( const D3DXVECTOR3& rV) const{return fabsf(x-rV.x)+fabsf(y-rV.y)+fabsf(z-rV.z)<FLT_EPSILON;}
		int operator != ( const D3DXVECTOR3& rV) const{return !(*this==rV);}
} D3DXVECTOR3, *LPD3DXVECTOR3;

typedef struct D3DXVECTOR2
{
	float x,y;
    D3DXVECTOR2() {};
//    D3DXVECTOR3( CONST float * );
//    D3DXVECTOR3( CONST D3DVECTOR& );
//    D3DXVECTOR3( CONST D3DXFLOAT16 * );
		D3DXVECTOR2(const D3DXVECTOR3& rVec3):x(rVec3.x),y(rVec3.y){};
		D3DXVECTOR2(const D3DXVECTOR4& rVec4):x(rVec4.x),y(rVec4.y){};
		D3DXVECTOR2( float X, float Y):x(X),y(Y){};

    // casting
		operator float* (){return reinterpret_cast<float*>(this);}
		operator const float* () const{return reinterpret_cast<const float*>(this);}

    // assignment operators
		D3DXVECTOR2& operator += ( const D3DXVECTOR2& rV){x+=rV.x;y+=rV.y;return *this;}
    D3DXVECTOR2& operator -= ( const D3DXVECTOR2& rV){x-=rV.x;y-=rV.y;return *this;}
    D3DXVECTOR2& operator *= ( float v){x*=v;y*=v;return *this;}
    D3DXVECTOR2& operator /= ( float v){x/=v;y/=v;return *this;}

    // unary operators
    D3DXVECTOR2 operator + () const;
    D3DXVECTOR2 operator - () const;

    // binary operators
		D3DXVECTOR2 operator + ( const D3DXVECTOR2& rV) const{D3DXVECTOR2 T=*this;T+=rV;return T;}
    D3DXVECTOR2 operator - ( const D3DXVECTOR2& rV) const{D3DXVECTOR2 T=*this;T-=rV;return T;}
    D3DXVECTOR2 operator * ( float v) const{D3DXVECTOR2 T=*this;T*=v;return T;}
    D3DXVECTOR2 operator / ( float v) const{D3DXVECTOR2 T=*this;T/=v;return T;}

    friend D3DXVECTOR2 operator * ( float, const struct D3DXVECTOR2& );

		int operator == ( const D3DXVECTOR2& rV) const{return fabsf(x-rV.x)+fabsf(y-rV.y)<FLT_EPSILON;}
		int operator != ( const D3DXVECTOR2& rV) const{return !(*this==rV);}
} D3DXVECTOR2, *LPD3DXVECTOR2;


typedef struct D3DXMATRIX
{
public:
		union
		{
			struct 
			{
				float        _11, _12, _13, _14;
				float        _21, _22, _23, _24;
				float        _31, _32, _33, _34;
				float        _41, _42, _43, _44;
			};
			float m[4][4];
			float Mat[16];
    };

    D3DXMATRIX() {};
		D3DXMATRIX( const float * pMat){for(int a=0;a<16;a++)Mat[a]=pMat[a];}
    D3DXMATRIX( const D3DXMATRIX& rMat){for(int a=0;a<16;a++)Mat[a]=rMat.Mat[a];}
    //D3DXMATRIX( const D3DXFLOAT16 *pMat){for(int a=0;a<16;a++)Mat[a]=pMat->Mat[a];}
    D3DXMATRIX( float _m11, float _m12, float _m13, float _m14,
                float _m21, float _m22, float _m23, float _m24,
                float _m31, float _m32, float _m33, float _m34,
								float _m41, float _m42, float _m43, float _m44 ):
		_11(_m11),_12(_m12),_13(_m13),_14(_m14),
		_21(_m11),_22(_m12),_23(_m13),_24(_m14),
		_31(_m11),_32(_m12),_33(_m13),_34(_m14),
		_41(_m11),_42(_m12),_43(_m13),_44(_m14)
		{};


    // access grants
		float& operator () (unsigned int Row,unsigned int Col ){return m[Row][Col];}
		float operator () (unsigned int Row,unsigned int Col ) const{return m[Row][Col];};

    // casting operators
		operator float* (){return &Mat[0];}
    operator const float* () const{return &Mat[0];}

    // assignment operators
		D3DXMATRIX& operator *= ( const D3DXMATRIX& ){CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator*=(const D3DXMATRIX&) not implemented yet");return *this;}
    D3DXMATRIX& operator += ( const D3DXMATRIX& ){CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator+=(const D3DXMATRIX&) not implemented yet");return *this;}
    D3DXMATRIX& operator -= ( const D3DXMATRIX& ){CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator-=(const D3DXMATRIX&) not implemented yet");return *this;}
    D3DXMATRIX& operator *= ( float ){CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator*=(const D3DXMATRIX&) not implemented yet");return *this;}
    D3DXMATRIX& operator /= ( float ){CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator/=(const D3DXMATRIX&) not implemented yet");return *this;}

    // unary operators
    D3DXMATRIX operator + () const{CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator+() not implemented yet");return D3DXMATRIX();}
    D3DXMATRIX operator - () const{CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator-() not implemented yet");return D3DXMATRIX();}

    // binary operators
    D3DXMATRIX operator * ( const D3DXMATRIX& ) const{CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator*(const D3DXMATRIX&) not implemented yet");return D3DXMATRIX();}
    D3DXMATRIX operator + ( const D3DXMATRIX& ) const{CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator+(const D3DXMATRIX&) not implemented yet");return D3DXMATRIX();}
    D3DXMATRIX operator - ( const D3DXMATRIX& ) const{CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator-(const D3DXMATRIX&) not implemented yet");return D3DXMATRIX();}
    D3DXMATRIX operator * ( float ) const{CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator*(float) not implemented yet");return D3DXMATRIX();}
    D3DXMATRIX operator / ( float ) const{CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::operator/(float) not implemented yet");return D3DXMATRIX();}

    friend D3DXMATRIX operator * ( float, const D3DXMATRIX& ){CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::not implemented yet");return D3DXMATRIX();}

    int operator == ( const D3DXMATRIX& rMat) const{CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::not implemented yet");return false;}
    int operator != ( const D3DXMATRIX& rMat) const{CRY_ASSERT_MESSAGE(0,"D3DXMATRIX::not implemented yet");return !(*this==rMat);};

} D3DXMATRIX, *LPD3DXMATRIX;

typedef D3DXMATRIX D3DMATRIX16;
typedef D3DXMATRIX D3DXMATRIXA16;
FORCEINLINE
void D3DXMatrixIdentity(D3DXMATRIX* pMat)
{
	pMat->Mat[0]	=	pMat->Mat[5]	=	pMat->Mat[10]	=	pMat->Mat[15]	=	1.f;
	pMat->Mat[1]	=	pMat->Mat[2]	=	pMat->Mat[3]		=
	pMat->Mat[4]	=	pMat->Mat[6]	=	pMat->Mat[7]		=
	pMat->Mat[8]	=	pMat->Mat[9]	=	pMat->Mat[11]	=
	pMat->Mat[12]	=	pMat->Mat[13]	=	pMat->Mat[14]	=	0.f;
}

typedef struct _D3DSURFACE_DESC
{
	D3DFORMAT						Format;
	D3DRESOURCETYPE			Type;
	uint32							Usage;
	D3DPOOL							Pool;
	D3DMULTISAMPLE_TYPE MultiSampleType;
	uint32 							MultiSampleQuality;
	uint32 							Width;
	uint32 							Height;
} D3DSURFACE_DESC;

typedef D3DXVECTOR4 D3DXCOLOR;
typedef D3DXVECTOR4 D3DXPLANE;

FORCEINLINE 
float D3DXPlaneDotNormal(CONST D3DXPLANE * pP,CONST D3DXVECTOR3 * pV)
{
	return pP->x*pV->x+pP->y*pV->y+pP->z*pV->z;
}

FORCEINLINE 
void D3DXColorLerp(D3DXCOLOR * pOut,const D3DXCOLOR * pC1,const D3DXCOLOR * pC2,FLOAT s)
{
	*pOut	=	*pC1+(*pC2-*pC1)*s;
}


typedef struct IDirect3DCubeTexture9 IDirect3DCubeTexture9;
typedef struct IDirect3DCubeTexture9*	LPDIRECT3DCUBETEXTURE9, *LPDIRECT3DCUBETEXTURE9;

typedef struct IDirect3DTexture9	IDirect3DTexture9;
typedef struct IDirect3DTexture9*	LPDIRECT3DTEXTURE9, *PDIRECT3DTEXTURE9;

typedef struct IDirect3DSurface9	IDirect3DSurface9;
typedef struct IDirect3DSurface9*	LPDIRECT3DSURFACE9, *PDIRECT3DSURFACE9;

typedef struct IDirect3DVertexBuffer9		IDirect3DVertexBuffer9;
typedef struct IDirect3DVertexBuffer9*	LPDIRECT3DVERTEXBUFFER9, *PDIRECT3DVERTEXBUFFER9;

typedef struct IDirect3DIndexBuffer9	IDirect3DIndexBuffer9;
typedef struct IDirect3DIndexBuffer9*	LPDIRECT3DINDEXBUFFER9, *PDIRECT3DINDEXBUFFER9;

typedef struct IDirect3DDevice9		IDirect3DDevice9;
typedef struct IDirect3DDevice9*	LPDIRECT3DDEVICE9, *PDIRECT3DDEVICE9;

typedef ID3D10Blob		ID3DXBuffer;
typedef ID3DXBuffer*	LPD3DXBUFFER;

typedef interface ID3DXConstTable		ID3DXConstTable;
typedef interface ID3DXConstTable*	LPD3DXCONSTANTTABLE;

typedef class CCryDXPSResource IDirect3DBaseTexture9;

class IDirect3D9;


#endif

