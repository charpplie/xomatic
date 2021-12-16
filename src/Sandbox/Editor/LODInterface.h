/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2013.
*************************************************************************/

#ifndef __LODINTERFACE_H__
#define __LODINTERFACE_H__

//TODOJM: fix this
class CPreviewModelCtrl;
class CLODGeneratorErrorGraphRamp;

#include "Util/LODGenerator.h"

class CLodGeneratorInteractionManager
{
public:
	enum eTextureType
	{
		eTextureType_Diffuse = 0,
		eTextureType_Normal = 1,
		eTextureType_Spec = 2,
		eTextureType_Max = 3,
	};

private:
	CLodGeneratorInteractionManager();
	virtual ~CLodGeneratorInteractionManager();

	static CLodGeneratorInteractionManager * m_pInstance;

public:
	static CLodGeneratorInteractionManager* Instance();
	static void DestroyInstance();

	const CString GetSelectedBrushFilepath();
	const CString GetSelectedBrushMaterial();
	const CString GetDefaultBrushMaterial(const CString& filepath);

	const CString GetParameterFilePath() { return m_parameterFilePath; };
	void SetParameterFilePath(const CString& filePath) { m_parameterFilePath = filePath; };
	
	void OpenMaterialEditor();

	bool LoadStatObj(const CString& filepath);
	bool LoadMaterial(CString strMaterial);
	CMaterial* LoadSpecificMaterial(CString strMaterial);

	IStatObj* GetLoadedModel(int nLod = 0);
	CMaterial* GetLoadedMaterial();
	CMaterial* GetLODMaterial();
	void SetLODMaterial(CMaterial *pMat);
	const CString GetLoadedFilename();
	const CString GetLoadedMaterialFilename();
	void ReloadModel();

	const IStatObj::SStatistics GetLoadedStatistics();
	float GetRadius(const int nSourceLod);
	int NumberOfLods();
	int GetSubMatId(const int nLodId);
	int GetSubMatCount(const int nLodId);
	int GetHighestLod();
	int GetMaxNumLods();
	float GetLodsPercentage(const int nLodIdx);
	int GetNumMoves();
	float GetErrorAtMove(const int nIndex);

	bool LodGenGenerate();
	bool LodGenGenerateTick(float* fProgress);
	void LogGenCancel();

	IStatObj* TryLoadCageObject(const int nLodId);
	bool UsingLodCage(const int nLodId);
	IStatObj* CreateLODStatObj(const int nLodId,const float fPercentage);

	bool GenerateTemporaryLod(const float fPercentage, CPreviewModelCtrl* ctrl, CLODGeneratorErrorGraphRamp* ramp);
	void GenerateLod(const int nLodId, const float fPercentage);
	void ClearUnusedLods(const int nNewLods);
	bool GenerateLodFromCageMesh(IStatObj **pLodStatObj);

	bool RunProcess(const int nLod, int nWidth, const int nHeight);
	void SaveTextures(const int nLod);
	CString GetPreset(const int texturetype);
	const SMeshBakingOutput* GetResults(const int nLod);
	void ClearResults();
	
	void SaveSettings();
	void LoadSettings();
	XmlNodeRef GetSettings(const CString& settings);
	void ResetSettings();

	template <class T> 
	T GetGeometryOption(CString paramName)
	{
		T value;
		if (IVariable* pVar = m_pGeometryVarBlock->FindVariable(paramName))
			pVar->Get(value);
		return value;
	}

	template <class T> 
	void SetGeometryOption(CString paramName, T paramValue)
	{
		if (IVariable* pVar = m_pGeometryVarBlock->FindVariable(paramName))
			pVar->Set(paramValue);
	}

	template <class T> 
	T GetMaterialOption(CString paramName)
	{
		T value;
		if (IVariable* pVar = m_pMaterialVarBlock->FindVariable(paramName))
			pVar->Get(value);
		return value;
	}

	template <class T> 
	void SetMaterialOption(CString paramName, T paramValue)
	{
		if (IVariable* pVar = m_pMaterialVarBlock->FindVariable(paramName))
			pVar->Set(paramValue);
	}
	
	CString ExportPath();
	void OpenExportPath();
	void ShowLodInExplorer(const int nLodId);
	void UpdateFilename(CString filename);
	
	bool CreateChangelist();
	bool CheckoutInSourceControl(const CString& filename);
	bool CheckoutOrExtract(const char * filename);
	
	CVarBlock* GetGeometryVarBlock() { return m_pGeometryVarBlock; }
	CVarBlock* GetMaterialVarBlock() { return m_pMaterialVarBlock; }

protected:
	
	bool PrepareMaterial(const CString& materialName, bool bAddToMultiMaterial);
	int FindExistingLODMaterial(const CString& matName);
	CMaterial * CreateMaterialFromTemplate(const CString& matName);
	int CreateLODMaterial(const CString& submatName);
	void SetLODMaterialId(IStatObj * pStatObj, const int matId);

	CString GetDefaultTextureName(const int i, const int nLod, const bool bAlpha, const CString& exportPath, const CString& fileNameTemplate);
	bool SaveTexture(ITexture* pTex, const bool bAlpha, const CString& fileName, const CString& texturePreset);
	void AssignToMaterial(const int nLod, const bool bAlpha, const bool bSaveSpec, const CString& exportPath, const CString& fileNameTemplate);
	bool VerifyNoOverlap(const int nSubMatIdx);
	bool RasteriseTriangle(Vec2 *tri, byte *buffer, const int width, const int height);
	
	void CreateSettings();
		
private:

	_smart_ptr<IStatObj> m_pLoadedStatObj;
	_smart_ptr<CMaterial> m_pLoadedMaterial;
	_smart_ptr<CMaterial> m_pLODMaterial;

	std::map<int,SMeshBakingOutput> m_resultsMap;
	CLODGenerator::SLODSequenceGenerationOutput m_results;
	CString m_parameterFilePath;
	bool m_bAwaitingResults;
	CString m_changeId;
	XmlNodeRef m_xmlSettings;
	
	_smart_ptr<CVarBlock>		m_pGeometryVarBlock;
	CSmartVariable<int>			m_nSourceLod;
	CSmartVariable<bool>		m_bObjectHasBase;
	CSmartVariable<int>			m_nViewsAround;
	CSmartVariable<int>			m_nViewElevations;
	CSmartVariable<float>		m_fSilhouetteWeight;
	CSmartVariable<float>		m_fViewResolution;
	CSmartVariable<float>		m_fVertexWelding;
	CSmartVariable<bool>		m_bCheckTopology;
	CSmartVariable<bool>		m_bWireframe;
	CSmartVariable<bool>		m_bExportObj;
	CSmartVariable<bool>		m_bAddToParentMaterial;
	CSmartVariable<bool>		m_bUseCageMesh;
	CSmartVariable<bool>		m_bPreviewSourceLod;
	
	_smart_ptr<CVarBlock>		m_pMaterialVarBlock;
	CSmartVariable<float>		m_fRayLength;
	CSmartVariable<float>		m_fRayStartDist;
	CSmartVariable<bool>		m_bBakeAlpha;
	CSmartVariable<bool>		m_bBakeSpec;
	CSmartVariable<bool>		m_bSmoothCage;
	CSmartVariable<bool>		m_bDilationPass;
	CSmartVariable<int>			m_cBackgroundColour;
	CSmartVariable<CString>		m_strExportPath;
	CSmartVariable<CString>		m_strFilename;
	CSmartVariable<CString>		m_strPresetDiffuse;
	CSmartVariable<CString>		m_strPresetNormal;
	CSmartVariable<CString>		m_strPresetSpecular;
	CSmartVariable<bool>		m_useAutoTextureSize;
	CSmartVariable<float>		m_autoTextureRadius1;
	CSmartVariable<int>			m_autoTextureSize1;
	CSmartVariable<float>		m_autoTextureRadius2;
	CSmartVariable<int>			m_autoTextureSize2;
	CSmartVariable<float>		m_autoTextureRadius3;
	CSmartVariable<int>			m_autoTextureSize3;
};

#endif