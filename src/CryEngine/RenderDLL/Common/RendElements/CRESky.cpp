#include "StdAfx.h"
#include "RendElement.h"
#include "CRESky.h"
#include "Stars.h"
#include "I3DEngine.h"


void CRESky::mfPrepare()
{
  gRenDev->FX_CheckOverflow(0, 0, this);

  gRenDev->m_RP.m_pRE = this;
  gRenDev->m_RP.m_RendNumIndices = 0;
  gRenDev->m_RP.m_RendNumVerts = 0;
}

float CRESky::mfDistanceToCameraSquared(Matrix34& matInst)
{
  return 999999.0f;
}

bool CRESky::DrawFogLayer()
{
  return true;
}

CRESky::~CRESky()
{
}

// render black occlusion volumes mostly to hide seams in indoors
bool CRESky::DrawBlackPortal()
{
  assert(0);
  return true;
}

void CRESky::DrawSkySphere(float fHeight)
{
  assert(0);
}


//////////////////////////////////////////////////////////////////////////
// HDR Sky
//////////////////////////////////////////////////////////////////////////

void CREHDRSky::mfInit()
{
  if (!m_pStars)
    m_pStars = new CStars;
}

void CREHDRSky::mfPrepare()
{
	gRenDev->FX_CheckOverflow( 0, 0, this );
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}


float CREHDRSky::mfDistanceToCameraSquared( Matrix34& matInst )
{
	return 999999.0f;
}


CREHDRSky::~CREHDRSky()
{
	SAFE_DELETE(m_pStars);
	SAFE_DELETE_ARRAY(m_pTmpStorage);

	SAFE_DELETE(m_pSkyDomeTextureMie);
	SAFE_DELETE(m_pSkyDomeTextureRayleigh);
}


//////////////////////////////////////////////////////////////////////////
// Stars
//////////////////////////////////////////////////////////////////////////


CStars::CStars()
: m_numStars(0)
//, m_pStarVB(0)
, m_pStarMesh(0)
, m_pShader(0)
, m_shaderTech()
, m_vspnStarSize()
, m_pspnStarIntensity()
{
	if (LoadData())
	{
		m_pShader = gRenDev->m_cEF.mfForName("Stars", EF_SYSTEM);
		m_shaderTech = "Stars";
		m_vspnStarSize = "StarSize";
		m_pspnStarIntensity = "StarIntensity";		
	}
}


CStars::~CStars()
{
	//if (m_pStarVB)
	//{
	//	gRenDev->ReleaseBuffer(m_pStarVB);
	//	m_pStarVB = 0;
	//}
	SAFE_RELEASE(m_pStarMesh);
	SAFE_RELEASE(m_pShader);
}


bool CStars::LoadData()
{
	const uint32 c_fileTag(0x52415453);				// "STAR"
	const uint32 c_fileVersion(0x00010001);
	const char c_fileName[] = "Libs/Sky/stars.dat";

	ICryPak* pPak(gEnv->pCryPak);
	if (pPak)
	{
		FILE* f(pPak->FOpen(c_fileName, "rb"));
		if (f)
		{
			// read and validate header
			size_t itemsRead(0);
			uint32 fileTag(0);
			itemsRead = pPak->FRead(&fileTag, 1, f);
			if (itemsRead != 1 || fileTag != c_fileTag)
			{
				pPak->FClose(f);
				return false;
			}

			uint32 fileVersion(0);
			itemsRead = pPak->FRead(&fileVersion, 1, f);
			if (itemsRead != 1 || fileVersion != c_fileVersion)
			{
				pPak->FClose(f);
				return false;
			}

			// read in stars
			pPak->FRead(&m_numStars, 1, f);

			SVF_P3S_C4B_T2S* pData(new SVF_P3S_C4B_T2S[m_numStars]);

			for (unsigned int i(0); i<m_numStars; ++i)
			{
				float ra(0);
				pPak->FRead(&ra, 1, f);

				float dec(0);
				pPak->FRead(&dec, 1, f);

				uint8 r(0);
				pPak->FRead(&r, 1, f);

				uint8 g(0);
				pPak->FRead(&g, 1, f);

				uint8 b(0);
				pPak->FRead(&b, 1, f);

				uint8 mag(0);
				pPak->FRead(&mag, 1, f);

				Vec3 v;
				v.x = -cosf(DEG2RAD(dec)) * sinf(DEG2RAD(ra * 15.0f));
				v.y = cosf(DEG2RAD(dec)) * cosf(DEG2RAD(ra * 15.0f));
				v.z = sinf(DEG2RAD(dec));
				pData[i].xyz = v;

#if defined(DIRECT3D10)
				pData[i].color.dcolor = (mag << 24) + (b << 16) + (g << 8) + r;
#else
				pData[i].color.dcolor = (mag << 24) + (r << 16) + (g << 8) + b;
#endif
			}

			m_pStarMesh = gRenDev->CreateRenderMeshInitialized(pData, m_numStars, eVF_P3S_C4B_T2S, 0, 0, R_PRIMV_TRIANGLES, "Stars", "Stars");

			delete [] pData;

			// check if we read entire file
			long curPos(pPak->FTell(f));
			pPak->FSeek(f, 0, SEEK_END);
			long endPos(pPak->FTell(f));
			if (curPos != endPos)
			{
				pPak->FClose(f);
				return false;
			}

			pPak->FClose(f);
			return true;
		}
	}

	return false;
}