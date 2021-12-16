#include "StdAfx.h"
#include "CubemapUtils.h"
#include "NumberDlg.h"
#include "Objects\EntityObject.h"
#include "Objects\EnvironmentProbeObject.h"
#include "Util\ImageTIF.h"

void CubemapSizeUpdateCallback(CNumberCtrl* pControl)
{
	static const int nAcceptableValues[6] = { 32, 64, 128, 256, 512, 1024 };
	static int nOldValue = 256;
	static int nOldIndex = 4;
	static bool bRecursive = false;

	if(bRecursive || (int)pControl->GetValue() ==  nAcceptableValues[nOldIndex])
		return;

	bRecursive = true;

	const int nNextIndex = nOldIndex + ((int)pControl->GetValue() > nAcceptableValues[nOldIndex] ? 1 : -1);

	nOldIndex = clamp_tpl(nNextIndex, 0, 6);

	pControl->SetValue(nAcceptableValues[nOldIndex]);

	bRecursive = false;
};

bool CubemapUtils::GenCubemap(CString & texturename, CWnd* pParent)
{
	CString filename;
  if (!CFileUtil::SelectSaveFile( "*.tif","tif","Textures",filename ))
	{
    return false;
	}

  filename = Path::GetRelativePath(filename);
  if (filename.IsEmpty())
  {
    AfxMessageBox( _T("Texture Must be inside MasterCD folder!"),MB_OK|MB_APPLMODAL|MB_ICONWARNING );
    return false;
  }

  CNumberDlg dlg( pParent,256,"Enter Cubemap Resolution" );
  dlg.SetInteger( true );
	dlg.SetRange(32.f, 1024.f);

	dlg.GetNumControl().SetStep(32.f);
	dlg.GetNumControl().SetUpdateCallback(functor(&CubemapSizeUpdateCallback));

  if (dlg.DoModal() != IDOK)
	{
    return false;
	}

	if ( !GenCubemapWithPathAndSize(filename, dlg.GetValue()) )
	{
		return false;
	}

	texturename = filename;
	return true; 
}

///////////////////////////////////////////////////////////////////////////////////
bool CubemapUtils::GenCubemapWithPathAndSize(CString &filename, const int size, const bool dds)
{
	CBaseObject *pObject = GetIEditor()->GetSelectedObject();
	return GenCubemapWithObjectPathAndSize(filename, pObject, size, dds);
}

///////////////////////////////////////////////////////////////////////////////////
bool CubemapUtils::GenCubemapWithObjectPathAndSize(CString &filename, CBaseObject * pObject, const int size, const bool dds)
{
	if (!pObject)
  {
    Warning("Select One Entity/Brush to Generate Cubemap");
    return false; 
  }
	
	if (pObject->GetType() != OBJTYPE_ENTITY && pObject->GetType() != OBJTYPE_BRUSH)
	{
		Warning("Only Entities and Brushes are allowed as a selected object. Please Select Entity or Brush objects");
		return false; 
	}

	int res = 1;
  // Make size power of 2.
  for (int i = 0; i < 16; i++)
  {
    if (res*2 > size)
      break;
    res *= 2;
  }
  if (res > 4096)
  {
    Warning( "Bad texture resolution.\nMust be power of 2 and less or equal to 4096");
    return false;
  }
	
	IRenderNode* pRenderNode = pObject->GetEngineNode();
	IEntity * pIEnt = NULL;
	bool bIsHidden = false;
	// Hide object before Cubemap generation.
	if(pRenderNode)
	{
		bIsHidden = (pRenderNode->GetRndFlags() & ERF_HIDDEN) != 0;
		pRenderNode->SetRndFlags(ERF_HIDDEN, true);
	}
	else if(pObject->GetType() == OBJTYPE_ENTITY)
	{
		CEntityObject * pEnt = (CEntityObject *)pObject;
		pIEnt = pEnt->GetIEntity();
		if(pIEnt)
		{
			bIsHidden = pIEnt->IsHidden();
			pIEnt->Hide(true);
		}
	}

  CString texname = Path::GetFileName(filename);
  CString path = Path::GetPath(filename);
		
	// Add _CM suffix if missing 
	int32 nCMSufixCheck = texname.Find( "_cm" ); 
	texname = Path::Make( path,texname + ( (nCMSufixCheck == -1)? "_cm.tif" : ".tif" ) );

  // Assign this texname to current material.
  texname = Path::ToUnixPath(texname);

	// Temporary solution to save both dds and tiff hdr cubemap
	AABB pObjAABB;
	pObject->GetBoundBox(pObjAABB);

	Vec3 pObjCenter = pObjAABB.GetCenter();

	GenHDRCubemapTiff(texname,res,pObjCenter);//pObject->GetWorldPos());

	// restore object's visibility
	if(pRenderNode)
	{
		pRenderNode->SetRndFlags(ERF_HIDDEN, bIsHidden);
	}
	else if(pIEnt)
	{
		pIEnt->Hide(bIsHidden);
	}
	filename = Path::ToUnixPath(texname);
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CubemapUtils::GenHDRCubemapTiff( const CString& fileName, int nDstSize, Vec3& pos )
{
	const int xSsaa = 1, channels = 4;
	int nSrcSize = nDstSize*xSsaa; // Render xSsaa^2 bigger cubemap - SSAA

	TArray<unsigned short> vecData;
	vecData.Reserve(nSrcSize*nSrcSize*6*channels);
	vecData.SetUse(0);

	if(!GetIEditor()->GetRenderer()->EF_RenderEnvironmentCubeHDR(nSrcSize, pos, vecData ))
	{
		assert(0);
		return;
	} 

	assert(vecData.size() == nSrcSize*nSrcSize*6*channels);

	// todo: such big downsampling should be on gpu

	// save data to tiff
	// resample the image at the original size
	CWordImage img;
	img.Allocate(nDstSize * channels * 6, nDstSize);

	size_t srcPitch = nSrcSize*channels;
	size_t srcSlideSize = nSrcSize*srcPitch;

	size_t dstPitch = nDstSize*channels;
	for(int side = 0;side < 6;++side)
	{
		for(uint32 y = 0; y < nDstSize; ++y)
		{
			CryHalf4 *pSrcSide = (CryHalf4 *)&vecData[side*srcSlideSize];
			CryHalf4 *pDst = (CryHalf4 *)&img.ValueAt(side*dstPitch, y);
			for(uint32 x=0;x<nDstSize;++x)
			{
				Vec4 cResampledColor(0.f, 0.f, 0.f, 0.f);

				// resample the image at the original size
				for(uint32 yres = 0; yres < xSsaa; ++yres)
				{
					for(uint32 xres=0;xres<xSsaa;++xres)
					{ const CryHalf4& pSrc = pSrcSide[(y*xSsaa + yres)*nSrcSize + (x*xSsaa+xres)]; cResampledColor += Vec4(CryConvertHalfToFloat(pSrc.x), CryConvertHalfToFloat(pSrc.y), CryConvertHalfToFloat(pSrc.z), CryConvertHalfToFloat(pSrc.w)); }
				}

				cResampledColor /= float(xSsaa*xSsaa);

				*pDst++=CryHalf4(cResampledColor.x, cResampledColor.y, cResampledColor.z, cResampledColor.w);
			}
		}
	}

	assert(IsHeapValid());

	CImageTIF tif;
	const bool res = tif.SaveRAW(fileName, img.GetData(), nDstSize*6, nDstSize, 2, "HDRCubemapRGBK_highQ");
	assert(res);
}


//function will recurse all probes and generate a cubemap for each
void CubemapUtils::RegenerateAllEnvironmentProbeCubemaps()
{
	std::vector<CBaseObject*> results;
	GetIEditor()->GetObjectManager()->FindObjectsOfType(RUNTIME_CLASS(CEnvironementProbeObject), results);
	for ( std::vector<CBaseObject*>::iterator end = results.end(),item = results.begin();item != end; ++item )
	{
		((CEnvironementProbeObject*)(*item))->GenerateCubemap();
	}
}