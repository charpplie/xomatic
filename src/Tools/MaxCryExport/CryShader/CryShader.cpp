//////////////////////////////////////////////////////////////////////////////
//
//		Shader plug-ins, implementation
//
//		Created: 8/18/98 Kells Elmquist
//
#include "StdAfx.h"
#include "gport.h"
#include "macrorec.h"
#include "iColorMan.h"
#include "ResourceCompilerHelper.h"

#include "CryShader.h"

#include "..\CGF\resource.h"

#include <map>


#include "../xml/xml.h"

#include "MaterialSender.h"

//#define WM_MATEDITSEND (WM_USER+315)
#define WM_MATEDITPICK (WM_USER+316)



#define CRYSHADER_ROLLUP_NAME _T("Crytek Shader Basic Paramaters")

#define CRYSHADER_PARAMS (\
	STD_PARAM_AMBIENT_CLR+\
	STD_PARAM_DIFFUSE_CLR+\
	STD_PARAM_SPECULAR_CLR+\
	STD_PARAM_LOCKDS+\
	STD_PARAM_LOCKAD+\
	STD_PARAM_SELFILLUM+\
	STD_PARAM_GLOSSINESS+\
	STD_PARAM_SPECULAR_LEV)

// including self-illum color but not filter
#define NCOLBOX 3
static int colID[NCOLBOX] = { IDC_STD_COLOR1, IDC_STD_COLOR2, IDC_STD_COLOR3 };
static int colParamID[NCOLBOX] = { shdr_ambient, shdr_diffuse, shdr_specular };

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//
//	Shader Channel Descriptions 
//
#define NMBUTS 4

static int texMButtonsIDC[NMBUTS] = {
	IDC_MAPON_DI,	IDC_MAPON_SP, IDC_MAPON_SH,	IDC_MAPON_TR,
};

// This array gives the texture map number for given MButton number
// static int texmapFromMBut[NMBUTS] = { 0, 1, 2, 7, 3, 4, 5, 6 };
// these ID_s are from stdmat.h
//static int texmapFromMBut[NMBUTS] = { ID_AM, ID_DI, ID_SP, ID_SS, ID_SH, ID_OP };
static int texmapFromMBut[NMBUTS] = { 0,1,2,3 };

// internal non-local parsable channel map names
static TCHAR* texNames[STD2_NMAX_TEXMAPS] =
{
	_T("Diffuse Color"),
	_T("Specular Color"),
	_T("Glossiness"),
	_T("Opacity (NE)"),
	_T("Detail"),
	_T("Bump Normals"),
	_T("Procedural"),
	_T("Decal"),
	_T("SubSurface"),
	_T("Reserved1"),
	_T("Reserved2"),
	_T("Reserved3"),
	_T(""),
	_T(""),
  _T(""),
	_T(""), _T(""), _T(""), _T(""),	
	_T(""), _T(""), _T(""), _T("") };

// internal non-local parsable channel map names
static TCHAR* texInternalNames[STD2_NMAX_TEXMAPS] = {
	_T("diffuseMap"),
	_T("specularMap"),
	_T("glossinessMap"),
	_T("opacityMap"),
	_T("detailMap"),
	_T("normalMap"),
	_T("proceduralMap"),
	_T("decalMap"),
	_T("subsurfaceMap"),
	_T("reserved1"),
	_T("reserved2"),
	_T("reserved3"),
	_T(""),
	_T(""),
	_T(""),
	_T(""), _T(""), _T(""), _T(""),	
	_T(""), _T(""), _T(""), _T("") };	

static int channelType[] = {
	CLR_CHANNEL,	// Diffuse Color.
	CLR_CHANNEL,	// Specular Color.
	UNSUPPORTED_CHANNEL,	// Glosiness.
	MONO_CHANNEL,	// Opacity
	CLR_CHANNEL,	// Detail.
	CLR_CHANNEL,	// Normal Map.
	CLR_CHANNEL,	// Procedural Map.
	CLR_CHANNEL,	// Decal Map.
  CLR_CHANNEL,  // Subsurface Map.
	UNSUPPORTED_CHANNEL, // Reserved1
	UNSUPPORTED_CHANNEL, // Reserved2
	UNSUPPORTED_CHANNEL, // Reserved3
	UNSUPPORTED_CHANNEL, // Bump Map.
	UNSUPPORTED_CHANNEL, // Reflection Map.
	UNSUPPORTED_CHANNEL, // Refraction Map.
	UNSUPPORTED_CHANNEL|ELIMINATE_CHANNEL,	 // Displacement Map.
	UNSUPPORTED_CHANNEL,
	UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL,
	UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL,
	UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL,
};

// what channel corresponds to the stdMat ID's
static int stdIDToChannel[N_ID_CHANNELS] =
{
	6, // Ambient.
	0,	// Diffuse.
	1,	// Specular.
	2,	// Glossines.
	-1,	// Shininess strength.
	-1,	// Self-illumination.
	3,	// Opacity.
	4,	// Filter Color.
	-1,	// Bump.
	-1,	// Reflection.
	8,	// Refraction.
	-1	// Displacement.
};

static const char* channelIDToName[STD2_NMAX_TEXMAPS] = 
{
	_T("DI"),
	_T("SP"),
	_T("GL"),
	_T("OP"),
	_T("DT"),
	_T("BN"),
	_T("PR"),
	_T("DC"),
	_T("SSS"),
	_T("RES2"),
	_T("RES3"),
	_T("RES4"),
	_T("BU"),
	_T("RL"),
	_T("RR"),
	_T("") 
	_T(""), _T(""), _T(""),	
	_T(""), _T(""), _T(""), _T(""),
};	

//////////////////////////////////////////////////////////////////////////

#define CrytekShaderParamDlgClassID Class_ID(0x64c23312, 0x2f1a7b08);

extern HINSTANCE hInstance;

static LRESULT CALLBACK HiliteWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static void phongIllum(ShadeContext &sc, IllumParams &ip, float softThresh, BOOL clrSelfIllum );

//////////////////////////////////////////////////////////////////////////
// Gets alpha from givven channel. (1 if no alpha).
inline float GetTexmapAlpha( ShadeContext &sc, IllumParams& ip,int channel )
{
	float a = 1.0f;
	// Opacity.
	Texmap *tex = ip.pMtl->GetSubTexmap(channel);
	if(tex && tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
	{
		BitmapTex *bmt = (BitmapTex*)tex;
		Bitmap *bmp = bmt->GetBitmap(0);
		if (bmp && bmp->HasAlpha())
		{
			a = tex->EvalMono(sc);
		}
	}
	return a;
}

//////////////////////////////////////////////////////////////////////////
// Check if texmap on specified channel have been enabled.
inline bool IsTexmapEnabled( ShadeContext &sc, IllumParams& ip,int channel )
{
	if (((StdMat2*)ip.pMtl)->GetMapState(channel) == 2)
		return true;
	else
		return false;
}

//////////////////////////////////////////////////////////
static HIMAGELIST hLockButtons = NULL;

// mjm - begin - 5.28.99
class StdShaderResourceDelete
{
public:
	StdShaderResourceDelete() {}
	~StdShaderResourceDelete() { if (hLockButtons) ImageList_Destroy(hLockButtons); }
};

static StdShaderResourceDelete theResourceDelete;
// mjm - end

static TCHAR *GetString(int id)
{
	static TCHAR buf[256];
	if(hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
	return NULL;
}


static BOOL IsButtonChecked(HWND hWnd,int id)
	{
	ICustButton *iBut;
	BOOL res;
	iBut = GetICustButton(GetDlgItem(hWnd,id));
	res = iBut->IsChecked();
	ReleaseICustButton(iBut);
	return res;
	}

static void CheckButton(HWND hWnd,int id, BOOL check) {
	ICustButton *iBut;
	iBut = GetICustButton(GetDlgItem(hWnd,id));
	iBut->SetCheck(check);
	ReleaseICustButton(iBut);
	}

static void SetupLockButton(HWND hWnd,int id, BOOL check)
	{
	ICustButton *iBut;
	iBut = GetICustButton(GetDlgItem(hWnd,id));
	iBut->SetImage(hLockButtons,0,1,0,1,16,15);
	iBut->SetType(CBT_CHECK);
	ReleaseICustButton(iBut);
	}

static void SetupPadLockButton(HWND hWnd,int id, BOOL check) {
	ICustButton *iBut;
	iBut = GetICustButton(GetDlgItem(hWnd,id));
	iBut->SetImage(hLockButtons,2,2,2,2,16,15);
	iBut->SetType(CBT_CHECK);
	ReleaseICustButton(iBut);
	}
 
static void LoadStdShaderResources()
	{
	static BOOL loaded=FALSE;
	if (loaded) return;
	loaded = TRUE;	
	HBITMAP hBitmap, hMask;

	hLockButtons = ImageList_Create(16, 15, TRUE, 2, 0);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DMTL_BUTTONS));
	hMask   = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DMTL_MASKBUTTONS));
	ImageList_Add(hLockButtons,hBitmap,hMask);
	DeleteObject(hBitmap);
	DeleteObject(hMask);
	}


inline float PcToFrac(int pc) { return (float)pc/100.0f; }

inline int FracToPc(float f) {
	if (f<0.0) return (int)(100.0f*f - .5f);
	else return (int) (100.0f*f + .5f);
}

// Quadratic
inline float Soften(float r) {
	return r*(2.0f-r);
}
 
inline float Abs( float a ) { return (a < 0.0f) ? -a : a; }
inline float LBound( float x, float min = 0.0f ){ return x < min ? min : x; }
inline float UBound( float x, float max = 1.0f ){ return x > max ? max : x; }

static void Tokenize( const std::string& str,std::vector<std::string>& tokens,const std::string& delimiters = " ")
{
	// Skip delimiters at beginning.
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Global array of known shaders.
typedef std::map<std::string,ShaderInfo> ShaderInfoMap;
static ShaderInfoMap gShaders;

//! Global array of known physics materials.
static std::vector<std::string> gPhysMaterials;
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//	generic standard shader dlg panel
//
class CrytekShaderDlg : public ShaderParamDlg
{
public:
	CrytekShader*	pShader;
	StdMat2*	pMtl;
	HPALETTE	hOldPal;
	HWND		hwmEdit;	 // window handle of the materials editor dialog
	IMtlParams*	pMtlPar;
	HWND		hwHilite;    // the hilite window
	HWND		hRollup; // Rollup panel
	TimeValue	curTime;
	DWORD		curRGB;
	BOOL		valid;
	BOOL		isActive;

	IColorSwatch *cs[NCOLBOX];
	//ISpinnerControl *softSpin;
	ISpinnerControl *shSpin, *ssSpin, *siSpin, *trSpin, *shOpSpin;
	ISpinnerControl *atSpin;
	ICustButton** texMBut;

	TexDADMgr dadMgr;
	
	CrytekShaderDlg(HWND hwMtlEdit, IMtlParams *pParams, int nButtons); 
	~CrytekShaderDlg(); 

	// required for correctly operating map buttons
	int FindSubTexFromHWND(HWND hw);

	// Methods
	INT_PTR PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam ); 
	Class_ID ClassID() { return CrytekShaderParamDlgClassID; }
	void SetThing(ReferenceTarget *m)
	{
		// Is m is Material or shader??
		pMtl = (StdMat2*)m;
	}
	void SetThings( StdMat2* theMtl, Shader* theShader ){
		if (pShader) 
			pShader->SetParamDlg(NULL,0);   // DS 3/11/99
		
		pShader = (CrytekShader*)theShader; 
		pMtl = theMtl;
		if( pShader )
		{
			pShader->SetParamDlg(this,0);
			// Force material transparency type to our additive var.
			// Changed.
			if (pShader->AssignMaterial(theMtl))
			{
				pMtlPar->MtlChanged();	
			}
		}
	}

	ReferenceTarget* GetThing(){ return (ReferenceTarget*)pMtl; }
	Shader* GetShader(){ return pShader; }
	void SetTime(TimeValue t) {
		//DS 2/26/99: added interval test to prevent redrawing when not necessary
		curTime = t; 
		if (!pShader->ivalid.InInterval(t)) {
			Interval v;
			pShader->Update(t,v);
			LoadDialog(TRUE); 
		} else
			UpdateOpacity();  // always update opacity since it's not in validity computations
	}		
	BOOL KeyAtCurTime(int id) { return pShader->KeyAtTime(id,curTime); } 
	void DeleteThis() { delete this; }
	void ActivateDlg( BOOL dlgOn ){ isActive = dlgOn; }
	HWND GetHWnd(){ return hRollup; }

	// ***** required code
	void UpdateOpacity();
	// ***** 

	void LoadDialog(BOOL draw);
	void ReloadDialog();
	void UpdateDialog( ParamID paramId ){ ReloadDialog(); }
	void NotifyChanged();

	// the rest is all specific to standard shaders
	void UpdateMtlDisplay();
	void UpdateHilite( );
	void UpdateColSwatches();
	void UpdateLockADTex(BOOL passOn);
	void UpdateMapButtons();
	void DrawHilite( HDC hdc, Rect& rect );
	void DisableNonMetalControls();

	void SetLockDS(BOOL lock);
	void SetLockAD(BOOL lock);
	void SetLockADTex(BOOL lock);

	void   SetMtlColor( int i, Color c );
	Color  GetMtlColor( int i );
	TCHAR* GetColorName( int i );

	void SelectEditColor(int i) ;
	void SetCurRGB(DWORD rgb) { curRGB = rgb; }

	//////////////////////////////////////////////////////////////////////////
	// Shader info.
	//////////////////////////////////////////////////////////////////////////
	void InitShaders();
	void ShowShaderInfo( const char *shaderName = NULL );

private:
	CMaterialSender m_MatSender;
	std::string m_EditorPath;
};

long CrytekShader::StdIDToChannel( long stdID ){ return stdIDToChannel[stdID]; }

////////////////////////////////////////////////////////////////////////////////////////
long  CrytekShader::nTexChannelsSupported(){ return N_CRYSHADER_CHANNELS; }
TSTR  CrytekShader::GetTexChannelName( long nTex )
{
	return texNames[nTex];
}
TSTR  CrytekShader::GetTexChannelInternalName( long nTex ){ return texInternalNames[ nTex ]; }
long  CrytekShader::ChannelType( long nTex )
{
	return channelType[ nTex ];
}

Color CrytekShader::GetAmbientClr(TimeValue t)  { return pblock->GetColor(shdr_ambient,t); }		
Color CrytekShader::GetDiffuseClr(TimeValue t)  { return pblock->GetColor(shdr_diffuse,t); }		
Color CrytekShader::GetSpecularClr(TimeValue t) { return pblock->GetColor(shdr_specular,t);	}
float CrytekShader::GetGlossiness( TimeValue t) {return pblock->GetFloat(shdr_glossiness,t);  }		
float CrytekShader::GetSpecularLevel(TimeValue t)
{
	return  pblock->GetFloat(shdr_spec_lvl,t);
}
float CrytekShader::GetSoftenLevel(TimeValue t) { return  0; }
float CrytekShader::GetSelfIllum(TimeValue t){ return  pblock->GetFloat(shdr_self_illum_amnt,t); }		
Color CrytekShader::GetSelfIllumClr(TimeValue t){ return Color(0,0,0); }		

//CrytekShader::CrytekShader(ClassDesc2* pParentCD) 
CrytekShader::CrytekShader() 
{ 
//	lockDS = lockAD = lockADTex = selfIllumClrOn = 0;
	lockDS = selfIllumClrOn = 0;
	lockAD = lockADTex = TRUE;
	pblock = NULL; 
	paramDlg = NULL; 
	Color black(0,0,0);
	ambient = diffuse = specular = selfIllumClr = black;
	glossiness = specularLevel = selfIllum = softThresh = 0.0f;
	shOpacity = 1.f;
	ivalid.SetEmpty(); 
	softThresh = 0;
	
	m_bDiffuseAlpha = false;

	EnumShaders();
}

ULONG CrytekShader::SupportStdParams()
{
	return CRYSHADER_PARAMS/*STD_BASIC+STD_EXTRA*/;
}

//////////////////////////////////////////////////////////////////////////
const TCHAR* CrytekShader::GetShaderName()
{
	const char *str = pblock->GetStr(shdr_shader_name,0);
	if (str)
		return str;
	return "";
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::SetShaderName( TCHAR *sName )
{
	pblock->SetValue(shdr_shader_name,0,sName);
	Interval v;
	Update(0,v);
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::SetSurfaceName( TCHAR *sName )
{
	pblock->SetValue(shdr_surface_name,0,sName);
	Interval v;
	Update(0,v);
}

//////////////////////////////////////////////////////////////////////////
const TCHAR* CrytekShader::GetSurfaceName()
{
	const char *str = pblock->GetStr(shdr_surface_name,0);
	if (str)
		return str;
	return "";
}

//////////////////////////////////////////////////////////////////////////
bool CrytekShader::IsAdditive()
{
	return pblock->GetInt(shdr_additive,0) != 0;
}

//////////////////////////////////////////////////////////////////////////
bool CrytekShader::IsAdditiveDecal()
{
	return pblock->GetInt(shdr_additive_decal,0) != 0;
}

bool CrytekShader::IsUseGlossiness()
{
  return pblock->GetInt(shdr_use_glossiness,0) != 0;
}

//////////////////////////////////////////////////////////////////////////
bool CrytekShader::IsAlphaBlend()
{
	return pblock->GetInt(shdr_alpha_blend,0) != 0;
}

//////////////////////////////////////////////////////////////////////////
bool CrytekShader::IsNoShadow()
{
	return pblock->GetInt(shdr_no_shadow,0) != 0;
}

//////////////////////////////////////////////////////////////////////////
bool CrytekShader::IsPhysicalize()
{
	return pblock->GetInt(shdr_physicalize,0) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::SetPhysicalize(bool isPhys)
{
	pblock->SetValue( shdr_physicalize,0,isPhys ? 1:0);
	Interval v;
	Update(0,v);
}

//////////////////////////////////////////////////////////////////////////
bool CrytekShader::IsSH()
{
	return pblock->GetInt(shdr_compute_sh,0) != 0;
}

bool CrytekShader::HasAmbientSH()
{
	return pblock->GetInt(shdr_sh_ambient,0) != 0;
}

//////////////////////////////////////////////////////////////////////////
bool CrytekShader::IsSH2Sided()
{
	return pblock->GetInt(shdr_sh_2sided,0) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::SetIsSH(bool val)
{
	pblock->SetValue( shdr_compute_sh,0,val ? 1:0);
	Interval v;
	Update(0,v);
}

void CrytekShader::SetHasAmbientSH(bool val)
{
	pblock->SetValue( shdr_sh_ambient,0,val ? 1:0);
	Interval v;
	Update(0,v);
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::SetIsSH2Sided(bool val)
{
	pblock->SetValue( shdr_sh_2sided,0,val ? 1:0);
	Interval v;
	Update(0,v);
}

//////////////////////////////////////////////////////////////////////////
float CrytekShader::GetSHOpacity(TimeValue t)
{
	float b = pblock->GetFloat(shdr_sh_opacity,t);
	return b;
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::SetAlphaTest( float fAlphaTest )
{
	pblock->SetValue( shdr_alpha_test,0,fAlphaTest );
	Interval v;
	Update(0,v);
}

//////////////////////////////////////////////////////////////////////////
float CrytekShader::GetAlphaTest()
{
	return pblock->GetFloat(shdr_alpha_test,0);
}

//////////////////////////////////////////////////////////////////////////
bool CrytekShader::AssignMaterial( StdMat2 *mtl )
{
	bool bChanged = false;
	/*
	for (int i = 0; i < mtl->NumSubTexmaps(); i++)
	{
		Texmap *tex = ip.pMtl->GetSubTexmap(channel);
		ICustAttribContainer *cc = tex->GetCustAttribContainer();
		if (!cc)
		{
			tex->AllocCustAttribContainer();
			cc = tex->GetCustAttribContainer();

			ICustAttribContainer
			ca->AppendCustAttrib(attr);
		}
	}
	*/
	return bChanged;
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::EnumShaders()
{
	/*
	if (gShaders.empty())
	{
		char szFilename[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		//char fname[_MAX_FNAME];
		//char ext[_MAX_EXT];

		GetModuleFileName( hInstance,szFilename,sizeof(szFilename) );
		_splitpath( szFilename,drive,dir,NULL,NULL );
		_makepath( szFilename,drive,dir,"CryExport","ini" );

		CfgFile cfgFile;
		if (cfgFile.Load( szFilename ))
		{
			unsigned int i,j;
			std::vector<CfgFile::Entry> entries;
			cfgFile.GetSection( "Shaders",entries );
			for (i = 0; i < entries.size(); i++)
			{	
				std::vector<std::string> tokens;
				Tokenize( entries[i].value,tokens,", " );
				if (tokens.empty())
					continue;
				ShaderInfo si;
				si.name = tokens[0].c_str();
				for (j = 1; j < tokens.size(); j++)
				{
					const char *s = tokens[j].c_str();
					if (stricmp(s,"BN") == 0)
						si.usedchannels |= 1 << CH_BN;
					else if (stricmp(s,"SP") == 0)
						si.usedchannels |= 1 << CH_SP;
					else if (stricmp(s,"GL") == 0)
						si.usedchannels |= 1 << CH_GL;
					else if (stricmp(s,"DI") == 0)
						si.usedchannels |= 1 << CH_DI;
					else if (stricmp(s,"OP") == 0)
						si.usedchannels |= 1 << CH_OP;
					else if (stricmp(s,"DT") == 0)
						si.usedchannels |= 1 << CH_DT;
					else if (stricmp(s,"PR") == 0)
						si.usedchannels |= 1 << CH_PR;
					else if (stricmp(s,"DC") == 0)
						si.usedchannels |= 1 << CH_DC;
          else if (stricmp(s,"SSS") == 0)
            si.usedchannels |= 1 << CH_SSS;
				}
				gShaders[si.name.data()] = si;
			}

			entries.clear();
			cfgFile.GetSection( "Descriptions",entries );
			for (i = 0; i < entries.size(); i++)
			{
				ShaderInfo *sh = FindShaderInfo( entries[i].key.c_str() );
				if (sh)
				{
					sh->desc = entries[i].value.c_str();
				}
			}

			gPhysMaterials.clear();
			entries.clear();
			cfgFile.GetSection( "Materials",entries );
			for (i = 0; i < entries.size(); i++)
			{
				gPhysMaterials.push_back( entries[i].value );
			}
		}
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
ShaderInfo* CrytekShader::FindShaderInfo( const char *shaderName )
{
	std::string name;
	if (shaderName != NULL)
	{
		name = shaderName;
	}
	else
	{
		const char *str = pblock->GetStr(shdr_shader_name,0);
		if (str)
			name = str;
	}
	ShaderInfoMap::iterator it = gShaders.find(name);
	if (it != gShaders.end())
	{
		return &it->second;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::CopyStdParams( Shader* pFrom )
{
	macroRecorder->Disable();  // don't want to see this parameter copying in macrorecorder
		SetLockDS( pFrom->GetLockDS() );
		SetLockAD( pFrom->GetLockAD() );
		SetLockADTex( pFrom->GetLockADTex() );
		SetSelfIllumClrOn( pFrom->IsSelfIllumClrOn() );

		SetAmbientClr( pFrom->GetAmbientClr(0,0), curTime );
		SetDiffuseClr( pFrom->GetDiffuseClr(0,0), curTime );
		SetSpecularClr( pFrom->GetSpecularClr(0,0), curTime );
		SetSelfIllumClr( pFrom->GetSelfIllumClr(0,0), curTime );

		SetSelfIllum( pFrom->GetSelfIllum(0,0), curTime );
		SetSpecularLevel( pFrom->GetSpecularLevel(0,0), curTime );
		SetGlossiness( pFrom->GetGlossiness(0,0), curTime );
		SetSoftenLevel( pFrom->GetSoftenLevel(0,0), curTime );
	macroRecorder->Enable();

	ivalid.SetEmpty();	
}

void CrytekShader::ConvertParamBlk( ParamBlockDescID *oldPBDesc, int oldCount, IParamBlock *oldPB )
{
	// old Standrard material loaded, transfer any shader-related parameters from old Mtl PB to shader PB2
	UpdateParameterBlock2(oldPBDesc, oldCount, oldPB, pblock->GetDesc(), pblock);
}


BOOL CrytekShader::KeyAtTime(int id,TimeValue t) { return pblock->KeyFrameAtTime((ParamID)id,t); }

#define LIMIT0_1(x) if (x < 0.0f) x = 0.0f; else if (x > 1.0f) x = 1.0f;
#define LIMITMINMAX(x, min, max) if (x < min) x = min; else if (x > max) x = max;

static Color LimitColor(Color c) {
	LIMIT0_1(c.r);
	LIMIT0_1(c.g);
	LIMIT0_1(c.b);
	return c;
}

RefTargetHandle CrytekShader::Clone( RemapDir &remap, CrytekShader* mnew )
{
	if (mnew == NULL ){
		assert( 0 );
		mnew = new CrytekShader();
	}

	mnew->ReplaceReference(0,remap.CloneRef(pblock));
	mnew->ivalid.SetEmpty();	

	mnew->ambient = ambient;
	mnew->diffuse = diffuse;
	mnew->specular = specular;
	mnew->glossiness = glossiness;
	mnew->specularLevel = specularLevel;
	mnew->softThresh = softThresh;
	mnew->selfIllum = selfIllum;
	mnew->selfIllumClr = selfIllumClr;
	mnew->selfIllumClrOn = selfIllumClrOn;
	mnew->lockDS = lockDS;
	mnew->lockAD = lockAD;
	mnew->lockADTex = lockADTex;
	
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
}

//////////////////////////////////////////////////////////////////////////
ULONG CrytekShader::GetRequirements( int subMtlNum )
{
	int flags = MTLREQ_PHONG;
	if (IsAdditive())
		flags |= MTLREQ_ADDITIVE_TRANSP;
	return flags;
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::GetIllumParams( ShadeContext &sc, IllumParams& ip )
{
	ip.stdParams = SupportStdParams();
	ip.channels[CH_AM] = ambient;
	ip.channels[CH_DI] = diffuse;
	ip.channels[CH_SP] = lockDS? diffuse : specular;
	ip.channels[CH_GL].r = glossiness;
	ip.channels[CH_SS].r = specularLevel;
	ip.channels[CH_SI].r = ip.channels[CH_SI].g = ip.channels[CH_SI].b = selfIllum;
	ip.channels[CH_BN].r = ip.channels[CH_BN].g = ip.channels[CH_BN].b = 0;
	ip.channels[CH_DC] = Color(0,0,0);
  ip.channels[CH_SS] = Color(0,0,0);

	// Initialize boolean.
	m_bAdditiveDecal = IsAdditiveDecal();
  m_bUseGlossiness = IsUseGlossiness();
	
	/*
	if (IsAdditive())
	{
		// Set material as additive.
		
	}
	*/
}

static BOOL inUpdate = FALSE;

void CrytekShader::Update(TimeValue t, Interval &valid)
{
	Point3 p,p2;
	if( inUpdate )
		return;
	inUpdate = TRUE;
	if (!ivalid.InInterval(t)) {
		ivalid.SetInfinite();

		// DS 10/12/00: added tests to avoid calling SetValue when not necessary, as this causing
		// unnecessary mtl editor re-renders.
		pblock->GetValue( shdr_diffuse, t, p, ivalid );
		diffuse= LimitColor(Color(p.x,p.y,p.z));
		pblock->GetValue( shdr_ambient, t, p2, ivalid );
		if( lockAD && !(p==p2)){
			//pblock->SetValue( shdr_ambient, t, diffuse);		  // DS 11/6/00 -- removed this
			ambient = diffuse;
		} else {
			//pblock->GetValue( shdr_ambient, t, p, ivalid );   
			ambient = LimitColor(Color(p2.x,p2.y,p2.z));
		}

		pblock->GetValue( shdr_specular, t, p2, ivalid );
		if( lockDS && !(p==p2)){
			//pblock->SetValue( shdr_specular, t, diffuse);     // DS 11/6/00 -- removed this
			specular = diffuse;
		} else {
			//pblock->GetValue( shdr_specular, t, p, ivalid );
			specular = LimitColor(Color(p2.x,p2.y,p2.z));
		}
		pblock->GetValue( shdr_glossiness, t, glossiness, ivalid );
		LIMIT0_1(glossiness);
		
		pblock->GetValue( shdr_spec_lvl, t, specularLevel, ivalid );
		LIMITMINMAX(specularLevel,0.0f,9.99f);

		pblock->GetValue( shdr_sh_opacity, t, shOpacity, ivalid );
		LIMITMINMAX(shOpacity,0.f,100.f);
		

		//pblock->GetValue( shdr_soften, t, softThresh, ivalid); 
		//LIMIT0_1(softThresh);

		pblock->GetValue( shdr_self_illum_amnt, t, selfIllum, ivalid );
		LIMIT0_1(selfIllum);
		
		//pblock->GetValue( shdr_self_illum_color, t, p, ivalid );
		//selfIllumClr = LimitColor(Color(p.x,p.y,p.z));

		// also get the non-animatables in case changed from scripter or other pblock accessors
		pblock->GetValue(shdr_ds_lock, t, lockDS, ivalid);
		pblock->GetValue(shdr_ad_lock, t, lockAD, ivalid);
		pblock->GetValue(shdr_ad_texlock, t, lockADTex, ivalid);
		
		//pblock->GetValue(shdr_use_self_illum_color, t, selfIllumClrOn, ivalid);

		curTime = t;
	}
	valid &= ivalid;
	inUpdate = FALSE;
}

void CrytekShader::Reset()
{
	if ( pblock==NULL )
		GetCD()->MakeAutoParamBlocks(this);	// make and intialize paramblock2
	
	ivalid.SetEmpty();
	macroRecorder->Disable();  // don't want to see this parameter reset in macrorecorder
		SetSoftenLevel(0.1f,0);
//		SetAmbientClr(Color(0.0f,0.0f,0.0f),0);
		SetDiffuseClr(Color(0.588f,0.588f,0.588f),0);
		SetSpecularClr(Color(0.9f,0.9f,0.9f),0);
		SetGlossiness(.10f,0);   // change from .25, 11/6/00
		SetSpecularLevel(.0f,0);   
		SetSHOpacityLevel(1.f,0);   
		SetSelfIllum(.0f,0);
		SetSelfIllumClr( Color(.0f, .0f, .0f), 0 );
		SetSelfIllumClrOn( FALSE );
		SetLockADTex( TRUE );
		SetLockAD( TRUE ); // DS 10/26/00: changed to TRUE
		SetLockDS( FALSE );
	macroRecorder->Enable(); 
}

void CrytekShader::NotifyChanged() {
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

RefResult CrytekShader::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
									  PartID& partID, RefMessage message ) 
{
	switch (message) {
		case REFMSG_CHANGE:
			ivalid.SetEmpty();
			if (hTarget==pblock)
			{
				// update UI if paramblock changed, possibly from scripter
				ParamID changingParam = pblock->LastNotifyParamID();
				// reload the dialog if present
				if (paramDlg)
					paramDlg->UpdateDialog( changingParam );


			}
			break;
	}
	return(REF_SUCCEED);
}


TSTR CrytekShader::SubAnimName(int i)
{ 
	return TSTR(GetString(IDS_DS_PARAMETERS));
}		

Animatable* CrytekShader::SubAnim(int i) {
	switch(i) {
		case 0: return pblock; 
		default: assert(0); return NULL;
	}
}

RefTargetHandle CrytekShader::GetReference(int i) {
	switch(i) {
		case 0: return pblock;
		default: assert(0);	 return NULL;
	}
}

void CrytekShader::SetReference(int i, RefTargetHandle rtarg) {
	switch(i) {
		case 0:	pblock = (IParamBlock2*)rtarg; return;
		default: assert(0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//	IO Routines
//
#define SHADER_HDR_CHUNK 0x4000
#define SHADER_SELFILLUM_CLR_ON_CHUNK 0x5000
#define SHADER_LOCKDS_ON_CHUNK 0x5001
#define SHADER_LOCKAD_ON_CHUNK 0x5002
#define SHADER_LOCKADTEX_ON_CHUNK 0x5003
#define SHADER_MAPSON_CHUNK 0x5004
#define SHADER_VERS_CHUNK 0x5300

#define SHADER_VERSION  1

// IO
IOResult CrytekShader::Save(ISave *isave) 
{ 
ULONG nb;

	isave->BeginChunk(SHADER_VERS_CHUNK);
	int version = SHADER_VERSION;
	isave->Write(&version,sizeof(version),&nb);			
	isave->EndChunk();


	return IO_OK;
}		

IOResult CrytekShader::Load(ILoad *iload) { 
	ULONG nb;
	int id;
	int version = 0;

	selfIllumClrOn = lockAD = lockADTex = lockDS = 0;

	IOResult res;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(id = iload->CurChunkID())  {
			case SHADER_VERS_CHUNK:
				res = iload->Read(&version,sizeof(version), &nb);
				break;
			case SHADER_SELFILLUM_CLR_ON_CHUNK:
				selfIllumClrOn = TRUE;
				break;
			case SHADER_LOCKDS_ON_CHUNK:
				lockDS = TRUE;
				break;
			case SHADER_LOCKAD_ON_CHUNK:
				lockAD = TRUE;
				break;
			case SHADER_LOCKADTEX_ON_CHUNK:
				lockADTex = TRUE;
				break;
		}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
	}

	return IO_OK;

}



void CrytekShader::SetAmbientClr(Color c, TimeValue t) 
{
	ambient =c;
	pblock->SetValue( shdr_ambient, t, Point3(c.r,c.g,c.b));
}
			
void CrytekShader::SetDiffuseClr(Color c, TimeValue t) 
{
	Point3 p;
	Interval iv;
	pblock->SetValue( shdr_diffuse, t, Point3(c.r,c.g,c.b));

	pblock->GetValue( shdr_diffuse, t, p, iv );
	diffuse = LimitColor(Color(p.x,p.y,p.z));

}
			
void CrytekShader::SetSpecularClr(Color c, TimeValue t) 
{
    specular = c;
	pblock->SetValue( shdr_specular, t, Point3(c.r,c.g,c.b));
}
			
			
void CrytekShader::SetGlossiness(float v, TimeValue t) 
{
	glossiness = v;
	pblock->SetValue( shdr_glossiness, t, v);
}
			
void CrytekShader::SetSpecularLevel(float v, TimeValue t) 
{
	specularLevel = v;
	pblock->SetValue( shdr_spec_lvl, t, v);
}

void CrytekShader::SetSHOpacityLevel(float v, TimeValue t) 
{
	shOpacity = v;
	pblock->SetValue( shdr_sh_opacity, t, v);
}

void CrytekShader::SetSoftenLevel(float v, TimeValue t) 
{
	softThresh = v;
	//pblock->SetValue( shdr_soften, t, v);
}

void CrytekShader::SetSelfIllum(float v, TimeValue t) 
{
	selfIllum =v;
	pblock->SetValue( shdr_self_illum_amnt, t, v);
}

void CrytekShader::SetSelfIllumClr(Color c, TimeValue t) 
{
	selfIllumClr = c;
	//pblock->SetValue( shdr_self_illum_color, t, Point3(c.r,c.g,c.b) );
}

void CrytekShader::CombineComponents( ShadeContext &sc, IllumParams& ip )
{
	ip.finalC = ip.finalAttenuation * (ip.ambIllumOut + ip.diffIllumOut  + ip.selfIllumOut)
			+ ip.specIllumOut + ip.reflIllumOut + ip.transIllumOut;

	/*
	if (IsTexmapEnabled(sc,ip,CH_DC))
	{
		float decalAlpha = GetTexmapAlpha(sc,ip,CH_DC);
		// Add decal texture.
		if (m_bAdditiveDecal)
		{
			// Additive blend decal.
			ip.finalC += ip.channels[CH_DC]*decalAlpha;
		}
		else
		{
			// Alpha blend decal.
			ip.finalC = ip.finalC*(1.0f - decalAlpha) + ip.channels[CH_DC]*decalAlpha;
		}
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void CrytekShader::Illum(ShadeContext &sc, IllumParams &ip)
{
	phongIllum( sc, ip, softThresh, selfIllumClrOn ); 
	ShadeTransmission(sc, ip, ip.channels[CH_SSS], ip.refractAmt);
	ShadeReflection( sc, ip, ip.channels[CH_RL] ); 
	CombineComponents( sc, ip );
}

//////////////////////////////////////////////////////////////////////////
static INT_PTR CALLBACK  CrytekShaderDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	CrytekShaderDlg *theDlg;
	if (msg == WM_INITDIALOG) {
		theDlg = (CrytekShaderDlg*)lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
	} else {
	    if ( (theDlg = (CrytekShaderDlg *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA) ) == NULL )
			return FALSE; 
	}
	theDlg->isActive = 1;
	BOOL res = theDlg->PanelProc(hwndDlg, msg, wParam, lParam);
	theDlg->isActive = 0;
	return res;
}



ShaderParamDlg* CrytekShader::CreateParamDialog(HWND hOldRollup, HWND hwMtlEdit, IMtlParams *imp, StdMat2* theMtl, int rollupOpen, int ) {
	Interval v;
	Update(imp->GetTime(),v);
	CrytekShaderDlg *pDlg = new CrytekShaderDlg(hwMtlEdit, imp, NMBUTS );
	pDlg->SetThings( theMtl, theMtl->GetShader() );
	LoadStdShaderResources();
	if ( hOldRollup )
		pDlg->hRollup = imp->ReplaceRollupPage( 
			hOldRollup,
			hInstance,
			MAKEINTRESOURCE(IDD_CRY_SHADER),
			CrytekShaderDlgProc, 
			CRYSHADER_ROLLUP_NAME,
			(LPARAM)pDlg , 
			// NS: Bugfix 263414 keep the old category and store it for the current rollup
			rollupOpen|ROLLUP_SAVECAT|ROLLUP_USEREPLACEDCAT
			);		
	else 
		pDlg->hRollup = imp->AddRollupPage( 
			hInstance,
			MAKEINTRESOURCE(IDD_CRY_SHADER),
			CrytekShaderDlgProc, 
			CRYSHADER_ROLLUP_NAME,
			(LPARAM)pDlg , 
			rollupOpen
			);	
	return pDlg;	
}


///////////////////////////////////////////////////////////////////
//
//	generic standard shader dlg panel
//
CrytekShaderDlg::CrytekShaderDlg( HWND hwMtlEdit, IMtlParams *pParams, int nMapButtons ) : m_MatSender(false)
{
	pShader = NULL;
	pMtl = NULL;
	hwmEdit = hwMtlEdit;
	pMtlPar = pParams;

	dadMgr.Init(this);
	
	//softSpin = NULL;
	shSpin = ssSpin = siSpin = trSpin = atSpin = shOpSpin = NULL;
	for( long i = 0; i < NCOLBOX; ++i )
		cs[ i ] = NULL;

	texMBut = new ICustButton*[ NMBUTS ];
	for( i = 0; i < NMBUTS; ++i )
		texMBut[ i ] = NULL;

	hRollup = hwHilite = NULL;
	curTime = pMtlPar->GetTime();
	curRGB = 0;
	isActive = valid = FALSE;
}

CrytekShaderDlg::~CrytekShaderDlg()
{
	HDC hdc = GetDC(hRollup);
	GetGPort()->RestorePalette(hdc, hOldPal);
	ReleaseDC(hRollup, hdc);

	if ( pShader ) pShader->SetParamDlg(NULL,0);

	for (long i=0; i < NMBUTS; i++ )
	{
		ReleaseICustButton( texMBut[i] );
		texMBut[i] = NULL; 
	}
// mjm - begin - 5.10.99
	delete[] texMBut;

	for (i=0; i<NCOLBOX; i++)
		if (cs[i]) ReleaseIColorSwatch(cs[i]);
// mjm - end

	ReleaseISpinner(shSpin);
	if(shOpSpin)
		ReleaseISpinner(shOpSpin);
	ReleaseISpinner(ssSpin);
	ReleaseISpinner(siSpin);
	ReleaseISpinner(trSpin);
	ReleaseISpinner(atSpin);

	SetWindowLongPtr(hRollup, GWLP_USERDATA, NULL);
	SetWindowLongPtr(hwHilite, GWLP_USERDATA, NULL);
	hwHilite = hRollup = NULL;
}

int CrytekShaderDlg::FindSubTexFromHWND(HWND hw)
{
	for (long i=0; i<NMBUTS; i++) {
		if (hw == texMBut[i]->GetHwnd()) 
			return texmapFromMBut[i];
	}	
	return -1;
}

void  CrytekShaderDlg::LoadDialog(BOOL draw) 
{
	if (pShader && hRollup)
	{
		Interval v;
		
		IRollupWindow* pRollup = pMtlPar->GetMtlEditorRollup();
		pRollup->SetPanelTitle( pRollup->GetPanelIndex(hRollup), CRYSHADER_ROLLUP_NAME );

		shSpin->SetValue(FracToPc(pShader->GetGlossiness(0,0)),FALSE);
		shSpin->SetKeyBrackets(KeyAtCurTime(shdr_glossiness));

		ssSpin->SetValue(FracToPc(pShader->GetSpecularLevel(0,0)),FALSE);
		ssSpin->SetKeyBrackets(KeyAtCurTime(shdr_spec_lvl));

		shOpSpin->SetValue(FracToPc(pShader->GetSHOpacityLevel(0,0)),FALSE);
		shOpSpin->SetKeyBrackets(KeyAtCurTime(shdr_sh_opacity));

		trSpin->SetValue(FracToPc(pMtl->GetOpacity( curTime )),FALSE);
		trSpin->SetKeyBrackets(pMtl->KeyAtTime(OPACITY_PARAM, curTime));

		//softSpin->Enable( ! (pShader->SupportStdParams() & STD_PARAM_METAL) );

		CheckButton(hRollup, IDC_LOCK_AD, pShader->GetLockAD() );
		CheckButton(hRollup, IDC_LOCK_DS, pShader->GetLockDS() );
	 	UpdateLockADTex( FALSE ); //don't send to mtl
		DisableNonMetalControls();

		// >>>> color selfIllum
		// show self-illum slider
//			ShowWindow( siSpin->GetHwnd(), SW_SHOW );
		ShowWindow( GetDlgItem(hRollup, IDC_SI_EDIT), SW_SHOW );
		ShowWindow( GetDlgItem(hRollup, IDC_SI_SPIN), SW_SHOW );
		siSpin->SetValue(FracToPc(pShader->GetSelfIllum(0,0)), FALSE);
		siSpin->SetKeyBrackets(KeyAtCurTime(shdr_self_illum_amnt));

		UpdateColSwatches();
		UpdateHilite();


		//////////////////////////////////////////////////////////////////////////
		// Show CryShader params.
		//////////////////////////////////////////////////////////////////////////
		InitShaders();

		atSpin->SetValue( FracToPc(pShader->pblock->GetFloat(shdr_alpha_test)),FALSE );

		HWND hWnd = GetHWnd();

    CheckDlgButton( hWnd,IDC_USEGLOSSINESS,pShader->IsUseGlossiness() );
		CheckDlgButton( hWnd,IDC_ADDITIVE,pShader->IsAdditive() );
		CheckDlgButton( hWnd,IDC_ADDITIVE_DECAL,pShader->IsAdditiveDecal() );
		CheckDlgButton( hWnd,IDC_PHYSICALIZE,pShader->IsPhysicalize() );
		CheckDlgButton( hWnd,IDC_SH,pShader->IsSH() );
		CheckDlgButton( hWnd,IDC_SH_AMBIENT,pShader->HasAmbientSH() );
		CheckDlgButton( hWnd,IDC_SH_2,pShader->IsSH2Sided() );
		CheckDlgButton( hWnd,IDC_ALPHA_BLEND,pShader->IsAlphaBlend() );
		CheckDlgButton( hWnd,IDC_NO_SHADOWS,pShader->IsNoShadow() );

		const char *str = pShader->pblock->GetStr(shdr_shader_name,0);
		if (str)
			SetDlgItemText( hWnd,IDC_SHADER,str );
		else
			SetDlgItemText( hWnd,IDC_SHADER,"" );

		str = pShader->pblock->GetStr(shdr_surface_name,0);
		HWND hCombo = GetDlgItem(GetHWnd(),IDC_SURFACE);
		if (hCombo != 0 && str != 0)
		{
			SendMessage( hCombo,CB_SELECTSTRING,-1,(LPARAM)str );
		}
		/*
		if (str)
			SetDlgItemText( hWnd,IDC_SURFACE,str );
		else
			SetDlgItemText( hWnd,IDC_SURFACE,"" );
		*/

		if (!pShader->IsAlphaBlend())
			trSpin->Disable();
		else
			trSpin->Enable(TRUE);

		ShowShaderInfo();
	}
}

void CrytekShaderDlg::NotifyChanged() 
{
	pShader->NotifyChanged();
}

void CrytekShaderDlg::ReloadDialog() 
{
	Interval v;
	pShader->Update(pMtlPar->GetTime(), v);
	LoadDialog(FALSE);
}


void CrytekShaderDlg::UpdateMtlDisplay()
{
	if (pMtlPar)
		pMtlPar->MtlChanged(); // redraw viewports
}


static TCHAR* mapStates[] = { _T(" "), _T("m"),  _T("M") };

void CrytekShaderDlg::UpdateMapButtons() 
{
	for ( long i = 0; i < NMBUTS; ++i )
	{
		if (!texMBut[i])
			continue;
		int nMap = texmapFromMBut[ i ];
		int state = pMtl->GetMapState( nMap );
		texMBut[i]->SetText( mapStates[ state ] );

		TSTR nm	 = pMtl->GetMapName( nMap );
		texMBut[i]->SetTooltip(TRUE,nm);
	}
}


void CrytekShaderDlg::UpdateOpacity() 
{
	trSpin->SetValue(FracToPc(pMtl->GetOpacity(curTime)),FALSE);
	trSpin->SetKeyBrackets(pMtl->KeyAtTime(OPACITY_PARAM, curTime));
}

Color CrytekShaderDlg::GetMtlColor(int i) 
{
	switch(i) {
		case 0:  return pShader->GetAmbientClr(0,0); 
		case 1:  return pShader->GetDiffuseClr(0,0);
		case 2:  return pShader->GetSpecularClr(0,0);
		case 3:  return pShader->GetSelfIllumClr(0,0);
		default: return Color(0,0,0);
	}
}

TCHAR *CrytekShaderDlg::GetColorName(int i)
{
	switch(i)
	{
		case 0:  return GetString(IDS_DS_AMBIENT);
		case 1:  return GetString(IDS_DS_DIFFUSE);
		case 2:  return GetString(IDS_DS_SPECULAR);
		default: return GetString(IDS_DS_NOSUCH_CLR);
	}
}

void CrytekShaderDlg::SetMtlColor(int i, Color c) {
	switch(i) {
		case 0: //ambient
			pShader->SetAmbientClr(c,curTime); 
			if ( pShader->GetLockAD() ){
				pShader->SetDiffuseClr(c, curTime);
				cs[1]->SetColor( c );
				if (pShader->GetLockDS() ){
					pShader->SetSpecularClr(c,curTime);
					cs[2]->SetColor(c);
				}
			}
			break;
		case 1: //diffuse
			pShader->SetDiffuseClr(c,curTime); 
			if (pShader->GetLockAD() ){
				pShader->SetAmbientClr(c,curTime);
				cs[0]->SetColor(c);
			}
			if ( pShader->GetLockDS() ){
				pShader->SetSpecularClr(c,curTime);
				cs[2]->SetColor(c);
				}
			break;
		case 2: // specular
			pShader->SetSpecularClr(c,curTime); 
			if (pShader->GetLockDS() ){
				pShader->SetDiffuseClr(c,curTime);
				cs[1]->SetColor(c);
				if (pShader->GetLockAD() ){
					pShader->SetAmbientClr(c,curTime);
					cs[0]->SetColor(c);
					}
				}
			break;
		case 3: 
			pShader->SetSelfIllumClr(c,curTime); 
			break;
	}
}

void CrytekShaderDlg::SelectEditColor(int i) {
	cs[ i ]->EditThis(FALSE);
	curRGB = GetMtlColor(i).toRGB();
}

void CrytekShaderDlg::SetLockAD(BOOL lock)
{
	if (lock) {
		if (IDYES!=MessageBox(hwmEdit, GetString(IDS_DS_LOCKAD), GetString(IDS_DS_LOCKCOL), MB_YESNO)) {
			CheckButton(hRollup, IDC_LOCK_AD, FALSE);	
			return;	
		}
		// set ambient color to diffuse
		pShader->SetAmbientClr( pShader->GetDiffuseClr(0,0), 0 );
		UpdateColSwatches();
	}
	pShader->SetLockAD(lock);
}


void CrytekShaderDlg::UpdateColSwatches() 
{
	for(int i=0; i < NCOLBOX; i++) {
		if ( cs[ i ] ) {
			cs[i]->SetKeyBrackets( pShader->KeyAtTime(colParamID[i],curTime) );
			cs[i]->SetColor( GetMtlColor(i) );
		}
	}
}

//-HiLite Curve Control------------------------------------------------------

static LRESULT CALLBACK HiliteWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) 
{
	int id = GetWindowLongPtr(hwnd,GWL_ID);
	HWND hwParent = GetParent(hwnd);
	CrytekShaderDlg *theDlg = (CrytekShaderDlg *)GetWindowLongPtr(hwParent, GWLP_USERDATA);
	if (theDlg==NULL) return FALSE;

    switch (msg) {
		case WM_COMMAND: 	
		case WM_MOUSEMOVE: 	
		case WM_LBUTTONUP: 
		case WM_CREATE:
		case WM_DESTROY: 
		break;

		case WM_PAINT: 	
		{
			PAINTSTRUCT ps;
			Rect rect;
			HDC hdc = BeginPaint( hwnd, &ps );
			if (!IsRectEmpty(&ps.rcPaint)) {
				GetClientRect( hwnd, &rect );
				theDlg->DrawHilite(hdc, rect);
			}
			EndPaint( hwnd, &ps );
		}													
		break;
	}
	return DefWindowProc(hwnd,msg,wParam,lParam);
} 


static void VertLine(HDC hdc,int x, int ystart, int yend) 
{
	MoveToEx(hdc, x, ystart, NULL); 
	if (ystart <= yend)
		LineTo(hdc, x, yend+1);
	else 
		LineTo(hdc, x, yend-1);
}

void CrytekShaderDlg::DrawHilite(HDC hdc, Rect& rect)
{
	int w,h,npts,xcen,ybot,ytop,ylast,i,iy;

	HPEN linePen = (HPEN)GetStockObject(WHITE_PEN);
	HPEN fgPen = CreatePen(PS_SOLID,0,GetCustSysColor(COLOR_BTNFACE));
	HPEN bgPen = CreatePen(PS_SOLID,0,GetCustSysColor(COLOR_BTNSHADOW));

	w = rect.w();
	h = rect.h()-3;
	npts = (w-2)/2;
	xcen = rect.left+npts;
	ybot = rect.top+h;
	ytop = rect.top+2;
	ylast = -1;
	for (i=0; i<npts; i++) {
		float v = pShader->EvalHiliteCurve( (float)i/((float)npts*2.0f) );
		if (v>2.0f) v = 2.0f; // keep iy from wrapping
		iy = ybot-(int)(v*((float)h-2.0f));

		if (iy<ytop) iy = ytop;

		SelectPen(hdc, fgPen);
		VertLine(hdc,xcen+i,ybot,iy);
		VertLine(hdc,xcen-i,ybot,iy);

		if (iy-1>ytop) {
			// Fill in above curve
			SelectPen(hdc,bgPen);
			VertLine(hdc,xcen+i, ytop, iy-1);
			VertLine(hdc,xcen-i, ytop, iy-1);
			}
		if (ylast>=0) {
			SelectPen(hdc,linePen);
			VertLine(hdc,xcen+i-1,iy-1,ylast);
			VertLine(hdc,xcen-i+1,iy-1,ylast);
			}

		ylast = iy;
	}

	SelectObject( hdc, linePen );
	DeleteObject(fgPen);
	DeleteObject(bgPen);
	WhiteRect3D(hdc, rect, 1);
}


void CrytekShaderDlg::DisableNonMetalControls() 
{
	BOOL b = (pShader->SupportStdParams() & STD_PARAM_METAL) ? 0 : 1;
//	EnableWindow( GetDlgItem(hRollup,  IDC_SOFT_EDIT), b);
//	EnableWindow( GetDlgItem(hRollup,  IDC_SPEC), b);
//	EnableWindow( GetDlgItem(hRollup,  IDC_MAPON_SP), b);
//	EnableWindow( GetDlgItem(hRollup,  IDC_LOCK_DS), b);

	//ShowWindow( GetDlgItem(hRollup,  IDC_SOFT_EDIT), b);
	//ShowWindow( GetDlgItem(hRollup,  IDC_SOFT_SPIN), b);
	//ShowWindow( GetDlgItem(hRollup,  IDC_MAPON_SP), b);
	ShowWindow( GetDlgItem(hRollup,  IDC_LOCK_DS), b);
	ShowWindow( GetDlgItem(hRollup,  IDC_SPEC_TEXT), b);
	//ShowWindow( GetDlgItem(hRollup,  IDC_SOFTEN_TEXT), b);
	ShowWindow( cs[2]->GetHwnd(), b );
}

void CrytekShaderDlg::UpdateHilite()
{
	HDC hdc = GetDC(hwHilite);
	Rect r;
	GetClientRect(hwHilite,&r);
	DrawHilite(hdc, r);
	ReleaseDC(hwHilite,hdc);
}

void CrytekShaderDlg::UpdateLockADTex( BOOL passOn) {
	int lock = 	pShader->GetLockADTex();
	CheckButton(hRollup, IDC_LOCK_ADTEX, lock);

	//ShowWindow(GetDlgItem(hRollup, IDC_MAPON_AM), !lock);
	texMBut[ 0 ]->Enable(!lock);

	if ( passOn ) {
		pMtl->SyncADTexLock( lock );
	}
//	UpdateMtlDisplay();
}


void CrytekShaderDlg::SetLockADTex(BOOL lock) {
	pShader->SetLockADTex( lock );
	UpdateLockADTex(TRUE); // passon to mtl
	NotifyChanged();
	UpdateMtlDisplay();
	}

void CrytekShaderDlg::SetLockDS(BOOL lock) 
{
	if (lock) {
		if (IDYES!=MessageBox(hwmEdit, GetString(IDS_DS_LOCK_DS),GetString(IDS_DS_LOCKCOL), MB_YESNO)) {
			CheckButton(hRollup, IDC_LOCK_DS, FALSE);	
			return;	
		}
		pShader->SetSpecularClr( pShader->GetDiffuseClr(0,0), 0 );
		UpdateColSwatches();
	}
	pShader->SetLockDS( lock );
}

int _ColIDCToIndex(int id) {
	switch (id) {
		case IDC_STD_COLOR1: return 0;
		case IDC_STD_COLOR2: return 1;
		case IDC_STD_COLOR3: return 2;
		//case IDC_SI_COLOR: return 3;
		default: return 0;
		}
	}


INT_PTR CrytekShaderDlg::PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam ) 
{
	int id = LOWORD(wParam);
	int code = HIWORD(wParam);
	switch (msg)
	{
		case WM_INITDIALOG:
			{
				int i;

				HDC theHDC = GetDC(hwndDlg);
				hOldPal = GetGPort()->PlugPalette(theHDC);
				ReleaseDC(hwndDlg,theHDC);

				for (i=0; i<NCOLBOX; i++) {
					cs[i] = GetIColorSwatch(GetDlgItem(hwndDlg, colID[i]),
						GetMtlColor(i), GetColorName(i));
				}

				hwHilite = GetDlgItem(hwndDlg, IDC_HIGHLIGHT);
				SetWindowLongPtr( hwHilite, GWLP_WNDPROC, (LONG_PTR)HiliteWndProc);

				shSpin = SetupIntSpinner(hwndDlg, IDC_SH_SPIN, IDC_SH_EDIT, 0,100, 0);
				ssSpin = SetupIntSpinner(hwndDlg, IDC_SS_SPIN, IDC_SS_EDIT, 0,999, 0);
				trSpin = SetupIntSpinner(hwndDlg, IDC_TR_SPIN, IDC_TR_EDIT, 0,100, 0);
				shOpSpin = SetupIntSpinner(hwndDlg, IDC_SHOP_SPIN, IDC_SHOP_EDIT, 0,100, 0);

				for (int j=0; j<NMBUTS; j++)
				{
					texMBut[j] = GetICustButton(GetDlgItem(hwndDlg,texMButtonsIDC[j]));
					assert( texMBut[j] );
					texMBut[j]->SetRightClickNotify(TRUE);
					texMBut[j]->SetDADMgr(&dadMgr);
				}

				SetupLockButton(hwndDlg,IDC_LOCK_AD,FALSE);
				SetupLockButton(hwndDlg,IDC_LOCK_DS,FALSE);
				SetupPadLockButton(hwndDlg,IDC_LOCK_ADTEX, TRUE);

				/* // mjm - 5.10.99 - isn't this already created above when i == N_SI_CLR?
				// create both a self-illum color as well as a spinner
				cs[N_SI_CLR] = GetIColorSwatch(GetDlgItem(hwndDlg, colID[N_SI_CLR] ),
				GetMtlColor(N_SI_CLR), GetColorName(N_SI_CLR));
				*/
				// self-illum spinner
				siSpin = SetupIntSpinner(hwndDlg, IDC_SI_SPIN, IDC_SI_EDIT, 0,100, 0);
				atSpin = SetupIntSpinner(hwndDlg, IDC_ALPHATEST_SPIN, IDC_ALPHATEST_EDIT, 0,100, 0);

				/*
				if( pShader->IsSelfIllumClrOn() ) {
				// enable the color swatch, disable the spinner
				ShowWindow( GetDlgItem(hwndDlg, IDC_SI_EDIT), SW_HIDE );
				ShowWindow( GetDlgItem(hwndDlg, IDC_SI_SPIN), SW_HIDE );
				} else {
				// disable the color swatch
				ShowWindow( cs[N_SI_CLR]->GetHwnd(), SW_HIDE );
				}
				*/
				LoadDialog(TRUE);
			}
			break;

		case WM_MATEDITSEND:
			//MessageBox(NULL, "received!", "", MB_OK);
			{			
				HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS,	FALSE, "EditMatMappingObject");
				if(hMapFile)
				{
					void * pMes = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

					if(pMes)
					{
						//MessageBox(NULL, (const char *)pMes, "", MB_OK);
						XmlParser parser;
						XmlNodeRef data = parser.parseBuffer((const char *)pMes);
						
						char pName[256];
						strcpy(pName, data->getAttr("Name"));

						float r,g,b,a;

						data->getAttr("dif_r", r);
						data->getAttr("dif_g", g);
						data->getAttr("dif_b", b);
						data->getAttr("dif_a", a);

						pMtl->SetName(_T(pName));

						Color diffuseColor(r, g, b);
						pShader->SetDiffuseClr(diffuseColor, curTime);

						HWND hWnd ;
						hWnd = GetDlgItem(hwmEdit, 0x400);
						if(hWnd)
							SetWindowText(hWnd, pName);

						hWnd = GetDlgItem(hwmEdit, 0x3E9);
						if(hWnd)
							SetWindowText(hWnd, pName);


						if(data->getAttr("dif_tex"))
						{
							strcpy(pName, data->getAttr("dif_tex"));
							BitmapTex * pTex = NewDefaultBitmapTex();

							pTex->SetMapName(pName);
							pTex->SetName(pName);
							pTex->ReloadBitmapAndUpdate();
							pMtl->SetSubTexmap(CH_DI, pTex);
						}


						NotifyChanged();
						UpdateMtlDisplay();

						ReloadDialog();

						ShowWindow(hwmEdit, SW_HIDE);
						RedrawWindow(hwmEdit, 0, 0, RDW_ALLCHILDREN);
						ShowWindow(hwmEdit, SW_SHOW);

						UnmapViewOfFile(pMes);
					}
					CloseHandle(hMapFile);
				}
			}
			break;

		case WM_COMMAND: 
			{
				for ( int i=0; i<NMBUTS; i++) {
					if (id == texMButtonsIDC[i]) {
						PostMessage(hwmEdit,WM_TEXMAP_BUTTON, texmapFromMBut[i],(LPARAM)pMtl );
						UpdateMapButtons();
						goto exit;
					}
				}
			}

			switch (id)
			{
			case IDC_MATEDIT:
			case IDC_CREATEMATERIAL:
			case IDC_PICKSELECTEDMATERIAL:
				{
					CResourceCompilerHelper rch;
					HWND hWnd = (HWND)rch.CallEditor( 0, "Material Editor (Sandbox)", "-MatEdit" );
					if(!hWnd)
						break;

					m_MatSender.SetupWindows(hwndDlg, hWnd);

					XmlNodeRef data = new CXmlNode("Material");

					if(id == IDC_CREATEMATERIAL)
					{
						::SetForegroundWindow(hWnd);
						::SetFocus(hWnd);

						data->setAttr("Name", pMtl->GetName());
						Color diffuseColor = pShader->GetDiffuseClr(curTime);
						int nFlags = 0x0010; // MF_LIGHTING
						data->setAttr( "MtlFlags", nFlags);
						data->setAttr( "Diffuse", (float *)&diffuseColor.r);

						Texmap * txmp = pMtl->GetSubTexmap(CH_DI);
						if(txmp)
						{
							if( txmp->ClassID()==Class_ID(BMTEX_CLASS_ID,0) )
							{
								BitmapTex * pTex = (BitmapTex *) txmp;
								data->setAttr( "DiffuseTexture", pTex->GetMapName());
								//MessageBox (NULL, pTex->GetMapName(), "", MB_OK);
							}
						}

						m_MatSender.SendMessage(eMSM_Create, data);
					}
					if(id == IDC_PICKSELECTEDMATERIAL)
					{
						m_MatSender.SendMessage(eMSM_GetSelectedMaterial, data);
					}
				}
				break;

			case IDC_LOCK_AD:
				SetLockAD(IsButtonChecked(hwndDlg, IDC_LOCK_AD));
				UpdateMtlDisplay();
				break;

			case IDC_LOCK_DS:
				SetLockDS(IsButtonChecked(hwndDlg, IDC_LOCK_DS));
				UpdateMtlDisplay();
				break;

			case IDC_LOCK_ADTEX:
				{
					BOOL on = IsButtonChecked(hwndDlg, IDC_LOCK_ADTEX);
					SetLockADTex(on);
					UpdateMtlDisplay();
				}
				break;

			case IDC_ADDITIVE:
				{
					BOOL bOn = IsDlgButtonChecked( hwndDlg,IDC_ADDITIVE );
					pShader->pblock->SetValue( shdr_additive,0,bOn );
					/*
					if (pShader->IsAdditive())
						pMtl->SetTransparencyType( TRANSP_ADDITIVE );
					else
						pMtl->SetTransparencyType( TRANSP_FILTER );
						*/
					UpdateMtlDisplay();
				}
				break;

      case IDC_USEGLOSSINESS:
        {
          BOOL bOn = IsDlgButtonChecked( hwndDlg,IDC_USEGLOSSINESS );
          pShader->pblock->SetValue( shdr_use_glossiness,0,bOn );
          UpdateMtlDisplay();
        }
        break;

			case IDC_NO_SHADOWS:
				{
					BOOL bOn = IsDlgButtonChecked( hwndDlg,IDC_NO_SHADOWS );
					pShader->pblock->SetValue( shdr_no_shadow,0,bOn );
					UpdateMtlDisplay();
				}
				break;

			case IDC_ADDITIVE_DECAL:
				{
					BOOL bOn = IsDlgButtonChecked( hwndDlg,IDC_ADDITIVE_DECAL );
					pShader->pblock->SetValue( shdr_additive_decal,0,bOn );
					UpdateMtlDisplay();
				}
				break;

			case IDC_ALPHA_BLEND:
				{
					BOOL bOn = IsDlgButtonChecked( hwndDlg,IDC_ALPHA_BLEND );
					pShader->pblock->SetValue( shdr_alpha_blend,0,bOn );
					UpdateMtlDisplay();
				}
				break;

			case IDC_PHYSICALIZE:
				{
					BOOL bOn = IsDlgButtonChecked( hwndDlg,IDC_PHYSICALIZE );
					pShader->pblock->SetValue( shdr_physicalize,0,bOn );
				}
				break;

			case IDC_SH:
				{
					BOOL bOn = IsDlgButtonChecked( hwndDlg,IDC_SH );
					pShader->pblock->SetValue( shdr_compute_sh,0,bOn );
				}
				break;

			case IDC_SH_2:
				{
					BOOL bOn = IsDlgButtonChecked( hwndDlg,IDC_SH_2 );
					pShader->pblock->SetValue( shdr_sh_2sided,0,bOn );
				}
				break;
			case IDC_SH_AMBIENT:
				{
					BOOL bOn = IsDlgButtonChecked( hwndDlg,IDC_SH_AMBIENT );
					pShader->pblock->SetValue( shdr_sh_ambient,0,bOn );
				}
				break;
			case IDC_SHADER:
				{
					static bool ignoreKillFocus=false;
					if (code == CBN_SELENDOK)
					{
						char str[1024];
						strcpy( str,"" );
						int curSel = SendMessage( (HWND)lParam,CB_GETCURSEL,0,0 );
						if (curSel != CB_ERR)
						{
							SendMessage( (HWND)lParam,CB_GETLBTEXT,(WPARAM)curSel,(LPARAM)str );
						}
						ShowShaderInfo( str );
					}
					else if (code == CBN_SETFOCUS)
					{
						DisableAccelerators();
					}
					else if (code == CBN_KILLFOCUS)
					{
						if (!ignoreKillFocus)
						{
							char str[1024];
							GetDlgItemText( hwndDlg,IDC_SHADER,str,sizeof(str) );
							pShader->pblock->SetValue( shdr_shader_name,0,str );
							EnableAccelerators();
						}
					}
				}
				break;

			case IDC_SURFACE:
				{
					if (code == CBN_SETFOCUS)
					{
						DisableAccelerators();
					}
					else if (code == CBN_KILLFOCUS || code == CBN_SELENDOK)
					{
						char str[1024];
						strcpy( str,"" );
						HWND hCombo = GetDlgItem(GetHWnd(),IDC_SURFACE);
						int nSel = SendMessage( hCombo,CB_GETCURSEL,0,0 );
						if (nSel != CB_ERR)
						{
							SendMessage( hCombo,CB_GETLBTEXT,nSel,(LPARAM)str );
						}
						//GetDlgItemText( hwndDlg,IDC_SURFACE,str,sizeof(str) );
						pShader->pblock->SetValue( shdr_surface_name,0,str );
						EnableAccelerators();
					}
				}
				break;

				/*
				case IDC_SI_COLORON:{
				int isOn = GetCheckBox(hwndDlg, IDC_SI_COLORON );
				pShader->SetSelfIllumClrOn( isOn );			
				if ( isOn ) {
				// enable the color swatch, disable the spinner
				ShowWindow( GetDlgItem(hwndDlg, IDC_SI_EDIT), SW_HIDE );
				ShowWindow( GetDlgItem(hwndDlg, IDC_SI_SPIN), SW_HIDE );
				ShowWindow( cs[N_SI_CLR]->GetHwnd(), SW_SHOW );
				} else {
				// disable the color swatch
				ShowWindow( cs[N_SI_CLR]->GetHwnd(), SW_HIDE );
				ShowWindow( GetDlgItem(hwndDlg, IDC_SI_EDIT), SW_SHOW );
				ShowWindow( GetDlgItem(hwndDlg, IDC_SI_SPIN), SW_SHOW );
				}
				NotifyChanged();
				UpdateMtlDisplay();
				}
				*/
				break;
			}
			break;

		case CC_COLOR_SEL:
			{
				int id = LOWORD(wParam);
				SelectEditColor(_ColIDCToIndex(id));
			}			
			break;
		case CC_COLOR_DROP:
			{
				int id = LOWORD(wParam);
				SelectEditColor(_ColIDCToIndex(id));
				UpdateMtlDisplay();				
			}			
			break;
		case CC_COLOR_BUTTONDOWN:
			theHold.Begin();
			break;
		case CC_COLOR_BUTTONUP:
			if (HIWORD(wParam)) theHold.Accept(GetString(IDS_DS_PARAMCHG));
			else theHold.Cancel();
			UpdateMtlDisplay();				
			break;
		case CC_COLOR_CHANGE:
			{			
				int id = LOWORD(wParam);
				int buttonUp = HIWORD(wParam); 
				int n = _ColIDCToIndex(id);
				if (buttonUp) theHold.Begin();
				curRGB = cs[n]->GetColor();
				SetMtlColor(n, Color(curRGB));
				if (buttonUp) {
					theHold.Accept(GetString(IDS_DS_PARAMCHG));
					// DS: 5/3/99-  this was commented out. I put it back in, because
					// it is necessary for the Reset button in the color picker to 
					// update the viewport.				
					UpdateMtlDisplay();  
				}
			}			
			break;
		case WM_PAINT: 
			if (!valid)
			{
				valid = TRUE;
				ReloadDialog();
			}
			return FALSE;
		case WM_CLOSE:
			break;       
		case WM_DESTROY:
			break;
		case CC_SPINNER_CHANGE: 
			if (!theHold.Holding()) theHold.Begin();
			switch (id)
			{
			case IDC_SH_SPIN: 
				pShader->SetGlossiness(PcToFrac(shSpin->GetIVal()), curTime); 
				UpdateHilite();
				break;
			case IDC_SS_SPIN: 
				pShader->SetSpecularLevel(PcToFrac(ssSpin->GetIVal()),curTime); 
				UpdateHilite();
				break;
			case IDC_SHOP_SPIN: 
				pShader->SetSHOpacityLevel(PcToFrac(shOpSpin->GetIVal()),curTime); 
				UpdateHilite();
				break;
			case IDC_SI_SPIN: 
				pShader->SetSelfIllum(PcToFrac(siSpin->GetIVal()),curTime); 
				break;
			case IDC_ALPHATEST_SPIN: 
				pShader->pblock->SetValue( shdr_alpha_test,0,PcToFrac(atSpin->GetIVal()) ); 
				break;
				//******** >>>><<<< required handling for opacity....must be present in all dialogs
			case IDC_TR_SPIN: 
				pMtl->SetOpacity(PcToFrac( trSpin->GetIVal()),curTime); 
				break;
			}
			//			UpdateMtlDisplay();
			break;

		case CC_SPINNER_BUTTONDOWN:
			theHold.Begin();
			break;

		case WM_CUSTEDIT_ENTER:
		case CC_SPINNER_BUTTONUP: 
			if (HIWORD(wParam) || msg==WM_CUSTEDIT_ENTER) 
				theHold.Accept(GetString(IDS_DS_PARAMCHG));
			else 
				theHold.Cancel();
			UpdateMtlDisplay();
	}
	exit:
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CrytekShaderDlg::InitShaders()
{
	HWND hCombo = GetDlgItem(GetHWnd(),IDC_SHADER);
	SendMessage( hCombo,CB_RESETCONTENT,0,0 );

	for (ShaderInfoMap::iterator it = gShaders.begin(); it != gShaders.end(); ++it)
	{
		ShaderInfo &sh = it->second;
		const char *sName = sh.name;
		SendMessage( hCombo,CB_ADDSTRING,0,(LPARAM)sName );
	}

	hCombo = GetDlgItem(GetHWnd(),IDC_SURFACE);
	SendMessage( hCombo,CB_RESETCONTENT,0,0 );
	/*
	for (i = 0; i < (int)gPhysMaterials.size(); i++)
	{
		const char *sName = gPhysMaterials[i].c_str();
		SendMessage( hCombo,CB_ADDSTRING,0,(LPARAM)sName );
	}
	*/
	SendMessage( hCombo,CB_ADDSTRING,0,(LPARAM)"Default" );
	SendMessage( hCombo,CB_ADDSTRING,0,(LPARAM)"Physical Proxy (NoDraw)" );
	SendMessage( hCombo,CB_ADDSTRING,0,(LPARAM)"No Collide" );
	SendMessage( hCombo,CB_ADDSTRING,0,(LPARAM)"Obstruct" );
}

//////////////////////////////////////////////////////////////////////////
void CrytekShaderDlg::ShowShaderInfo( const char *shaderName )
{
	HWND hWnd = GetHWnd();
	ShaderInfo *sh = pShader->FindShaderInfo( shaderName );
	if (sh)
	{
		SetDlgItemText( hWnd,IDC_SHADER_DESC,(const char*)sh->desc.data() );

		char str[1024];
		strcpy( str,"" );
		for (int i = 0; i < STD2_NMAX_TEXMAPS; i++)
		{
			if (sh->usedchannels & (1<<i))
			{
				if (strlen(channelIDToName[i]) > 0)
				{
					strcat( str,channelIDToName[i] );
					strcat( str,"," );
				}
			}
		}
		int len = strlen(str)-1;
		if (len > 0 && str[len] == ',')
			str[len] = 0;
		SetDlgItemText( hWnd,IDC_SHADER_REQUIREMENTS,str );
	}
}

//----------------------------------------------------------------------------------------
//- Constant & Phong Shader  -------------------------------------------------------------
//----------------------------------------------------------------------------------------
#define	SELFILLUM_FRAC		1.0f

//////////////////////////////////////////////////////////////////////////
static void phongIllum( ShadeContext &sc, IllumParams &ip, float softThresh, BOOL clrSelfIllum ) 
{
	LightDesc *l;
	Color lightCol;
	BOOL isShiny;
	Point3 R;

	if (isShiny = (ip.channels[CH_SS].r > 0.0f) ) 
		R = sc.ReflectVector(); // no use of ip.N?, no, it's in sc too

	double phExp = 0.0;
	if (isShiny)
	{
		phExp = pow(2.0, ip.channels[CH_GL].r * 10.0); // expensive.!!	TBD
	}

	Point3 normal = sc.Normal();
	
	if (ip.channels[CH_BN].r != 0 || ip.channels[CH_BN].g != 0 || ip.channels[CH_BN].b != 0)
	{
		Point3  dp[3];
		sc.DPdUVW( dp,CH_BN );
		Matrix3 mtx( dp[0],dp[1],dp[2],Point3(0,0,0) );
		Point3 n = ip.channels[CH_BN];//ip.channels[CH_BN].r,ip.channels[CH_BN].g,ip.channels[CH_BN].b );
		// Make normal in -1 to 1 range.
		n = Point3(1,1,1) - n*2;
		//Point3 n(0,0,1);
		n = n.Normalize();
		normal = mtx.VectorTransform(n);
	}

	for (int i = 0; i < sc.nLights; i++)
	{
		l = sc.Light(i);
		register float NL, kL;
		Point3 L;
		if (l->Illuminate(sc, normal, lightCol, L, NL, kL) )
		{
			if (l->ambientOnly)
			{
				ip.ambIllumOut += lightCol;
				continue;
			}
			if (NL<=0.0f) 
				continue;
			// diffuse
			if (l->affectDiffuse)
				ip.diffIllumOut += kL * lightCol;

			// Specular
			if (isShiny && l->affectSpecular)
			{
				// specular (CrytekShader) 
				// Blinn Half-Angle.
				Point3 H = Normalize( L-sc.V() );
				float c = DotProd(sc.Normal(),H);	 
				//PHONG float c = DotProd(L,R);
				if (c > 0.0f) {
					if (softThresh != 0.0 && kL < softThresh )
					{
						float r = kL/softThresh;
						c *= Soften(r);
					}
					c = (float)pow((double)c, (double)phExp); // could use table lookup for speed
					ip.specIllumOut += c * ip.channels[CH_SS].r * lightCol;
				}
			}
		}
	}
	
	// Apply mono self illumination
//	BOOL clrSelfIllum = (ip.shFlags & SELFILLUM_CLR_ON) ? 1 : 0;
	if ( ! clrSelfIllum ){
		// changed back, fixed in getIllumParams, KE 4/27
		float si = 0.3333333f * (ip.channels[CH_SI].r + ip.channels[CH_SI].g + ip.channels[CH_SI].b);
		//		float si = ip.channels[CH_SI].r; // DS: 4/23/99
//		ip.diffIllumOut = (si>=1.0f) ?  Color(1.0f,1.0f,1.0f) 
//			: ip.diffIllumOut * (1.0f-si) + si;
		if ( si > 0.0f ) {
			si = UBound( si );
			ip.selfIllumOut = si * ip.channels[CH_DI];
			ip.diffIllumOut *= (1.0f-si);
			// fade the ambient down on si: 5/27/99 ke
			ip.ambIllumOut *= 1.0f-si;
		}
	}
	else {
		// colored self-illum
		ip.selfIllumOut += ip.channels[CH_SI];
		}

	// now we can multiply by the clrs,
	ip.ambIllumOut *= ip.channels[CH_AM]; 
	ip.diffIllumIntens = Intens(ip.diffIllumOut);
	ip.diffIllumOut *= ip.channels[CH_DI]; 
	ip.specIllumOut *= ip.channels[CH_SP];

	/*
	// Opacity.
	float diffuseAlpha = GetTexmapAlpha(sc,ip,CH_DI);
	if (diffuseAlpha < 1.0f)
	{
		ip.finalOpac = diffuseAlpha * ip.channels[CH_OP].r;
		ip.finalT = 1.0f - ip.finalC;
		if( ip.finalOpac < 1.0f )
			ip.hasComponents |= HAS_OPACITY;
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CrytekShaderClassDesc : public ClassDesc2
{
public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading) { 	return new CrytekShader(); }
	const TCHAR *	ClassName() { return _T("Crytek Shader"); }
	SClass_ID		SuperClassID() { return SHADER_CLASS_ID; }
	Class_ID 		ClassID() { return CrytekShaderClassID; }
	const TCHAR* 	Category() { return _T("");  }
	const TCHAR*	InternalName() { return _T("CrytekShader"); }		// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

CrytekShaderClassDesc cryShaderCD;
ClassDesc * GetCryShaderCD(){ return &cryShaderCD; }
ClassDesc2* CrytekShader::GetCD() { return &cryShaderCD; }

// shader parameters
static ParamBlockDesc2 crytek_shader_param_blk( shdr_params, _T("shaderParameters"),  0, &cryShaderCD, P_AUTO_CONSTRUCT+P_AUTO_UI, 0, 
	IDD_CRY_SHADER, _T("My Params"), 0, 0, NULL, 
	// params
	shdr_ambient,	 _T("ambient"),	TYPE_RGBA,	P_ANIMATABLE,	IDS_DS_AMBIENT,	
		p_default,		Color(1, 1, 1), 
//		p_accessor,		&stdShadersPBAccessor,
		end,
	shdr_diffuse,	 _T("diffuse"),	TYPE_RGBA,	P_ANIMATABLE,	IDS_DS_DIFFUSE,	
		p_default,		Color(0.5f, 0.5f, 0.5f), 
//		p_accessor,		&stdShadersPBAccessor,
		end,
	shdr_specular,	 _T("specular"), TYPE_RGBA,				P_ANIMATABLE,	IDS_DS_SPECULAR,	
		p_default,		Color(1.0f, 1.0f, 1.0f), 
//		p_accessor,		&stdShadersPBAccessor,
		end,
	shdr_ad_texlock, _T("adTextureLock"),	TYPE_BOOL,		0,				IDS_JW_ADTEXLOCK,	
		p_default,		TRUE, 
		end,
	shdr_ad_lock,	_T("adLock"),	TYPE_BOOL,				0,				IDS_JW_ADLOCK,	
		p_default,		FALSE, 
		end,
	shdr_ds_lock,	_T("dsLock"),	TYPE_BOOL,				0,				IDS_JW_DSLOCK,	
		p_default,		FALSE, 
		end,
		/*
	shdr_use_self_illum_color, _T("useSelfIllumColor"), TYPE_BOOL, 0,		IDS_JW_SELFILLUMCOLORON,	
		p_default,		FALSE, 
		end,
		*/
	shdr_self_illum_amnt, _T("selfIllumAmount"), TYPE_PCNT_FRAC,	P_ANIMATABLE, IDS_KE_SELFILLUM,
		p_default,		0.0,
		p_range,		0.0, 100.0,
		end,
		/*
	shdr_self_illum_color, _T("selfIllumColor"), TYPE_RGBA, P_ANIMATABLE,	IDS_KE_SELFILLUM_CLR,	
		p_default,		Color(0, 0, 0), 
		end,
		*/
	shdr_spec_lvl,	_T("specularLevel"),TYPE_PCNT_FRAC,		P_ANIMATABLE,	IDS_KE_SPEC_LEVEL,
		p_default,	 	0.0,
		p_range,		0.0, 999.0,
		end,
	shdr_glossiness, _T("glossiness"),	TYPE_PCNT_FRAC,		P_ANIMATABLE,	IDS_KE_GLOSSINESS,
		p_default,		0.0,
		p_range,		0.0, 100.0,
		end,
		/*
	shdr_soften,		_T("soften"),		TYPE_FLOAT,			P_ANIMATABLE,	IDS_DS_SOFTEN,
		p_default,		0.0,
		p_range,		0.0, 1.0,
		end,
		*/
	shdr_alpha_test, _T("alphaTest"), TYPE_PCNT_FRAC,	0, 0,
		p_default,		0.0,
		p_range,		0.0, 100.0,
		end,

	shdr_additive, _T("additiveBlend"), TYPE_BOOL, 0, 0,
		p_default, FALSE,
		end,

  shdr_use_glossiness, _T("useGlossiness"), TYPE_BOOL, 0, 0,
    p_default, FALSE,
    end,

	shdr_shader_name, _T("shaderName"), TYPE_STRING,	0, 0,
		end,

	shdr_surface_name, _T("surfaceName"), TYPE_STRING,	0, 0,
		p_default, "Default",
		end,

	shdr_physicalize, _T("physicalizeMaterial"), TYPE_BOOL, 0, 0,
		p_default, FALSE,
		end,

	shdr_additive_decal, _T("additiveDecal"), TYPE_BOOL, 0, 0,
		p_default, FALSE,
		end,

	shdr_alpha_blend, _T("alphaBlend"), TYPE_BOOL, 0, 0,
		p_default, FALSE,
		end,

	shdr_no_shadow, _T("noShadow"), TYPE_BOOL, 0, 0,
		p_default, FALSE,
		end,
	shdr_compute_sh, _T("SH"), TYPE_BOOL, 0, 0,
		p_default, FALSE,
		end,
	shdr_sh_2sided, _T("2SIDED"), TYPE_BOOL, 0, 0,
		p_default, FALSE,
		end,
	shdr_sh_ambient, _T("SH_AMBIENT"), TYPE_BOOL, 0, 0,
		p_default, TRUE,
		end,
	shdr_sh_opacity,	_T("shOpacity"),TYPE_PCNT_FRAC,		P_ANIMATABLE,	IDS_KE_SH_OPACITY_LEVEL,
		p_default,	 	100.0,
		p_range,		0.0, 100.0,
		end,

	end
	);