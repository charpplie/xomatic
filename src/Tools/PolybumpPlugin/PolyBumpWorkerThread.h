#pragma once

#include "SimpleIndexedMesh.h"												// CSimpleIndexedMesh
#include "Properties.h"																// CProperties
#include "SRFData.h"																	// CSRFData
#include <string>																			// STL string
#include "AG_Thread.h"																// CAG_Thread
#include "types.h"																		// uint16
#include "PolyBump.h"																	// CPbHighMesh
#include "PolybumpProgress.h"													// CPolybumpProgress

class CPbLowTri;
class CPbLowMesh;

enum {WM_POLYBUMPTHREADFINISHED = WM_USER + 1019};



//! class used for storing all the relevant data for the normal map generation (used by worker thread)
class CPolyBumpWorkerThread :public CAG_Thread
{
public:
	//! constructor
	CPolyBumpWorkerThread();

	//! destructor
	virtual ~CPolyBumpWorkerThread();

	//! call StartComputation
	//! \return true=success, false=in progress
	bool StartComputationSetup( HWND hThreadderParent );

	//!
	//! \return in ms
	uint32 GetComputationTime() const;

	void FreeData();

	//! multiple high poly meshes can be pushed, only during SetupPhase
	//! \return success
	bool PushHighPolyMesh( const CSimpleIndexedMesh &rMesh );

	void CancelComputationSetup();

	//! before call StartComputationSetup()
	void StartComputation();

	//! get the low resolution mesh
	//! /return pointer to the low resolution mesh
	CSimpleIndexedMesh &GetLowMesh() { assert(m_bSetupPhase); return m_LowMesh; }

	//! get the high resolution mesh
	//! /return pointer to the high resolution mesh
//	CSimpleIndexedMesh &GetHighMesh() { assert(m_bSetupPhase);return m_HighMesh; }

	//! get the cage mesh (to limit the ray shooting)
	//! /return pointer to the cage mesh
	CSimpleIndexedMesh &GetCageMesh() { assert(m_bSetupPhase);return m_CageMesh; }

	//! \return 0 if not available
	CSRFData *GetSRFData();

	//! this has to be called from time to time to show the user the current state and to process UI messages
	//! /return true/false: false means the user tries to stop the process with the UI
  //! /inszPassName pass name string
	//! /param iniPercent shows the user how much work is done so far (0..100)
	bool ThreadPollMessages( float infPercent, const char *inszPassName );

	//!
	void DrawTrisPointers( uint16 *pTriIndices, CPbLowMesh *pLowMesh, const int iMaterialId );

	//! only logs (to file PolyBumpDebug.txt) if m_bOutputDebugInfo is true
	void DebugLog( const char *sFormat, ... );

	void SetProgressState( const uint32 dwId, const CPolybumpProgress::EPolybumpProgressState state );

	// Attributes *************************************************************************************

	//! set attribute
	void SetStartTime( DWORD indwStartTime );

	//! get attribute
	DWORD GetStartTime();		

	// **************************************************************************************

	CProperties							m_Properties;						//!< properties chosen by the user at start time
	bool										m_bUserHasStopped;			//!< user has stopped the process

  int											m_nImageSizeX;					//!<
  int											m_nImageSizeY;					//!<

private: // -------------------------------------------------------------------------------

	CSRFData								m_SRFData;							//!< used to store the .SRF file format in memory
	CPolybumpProgress				m_CurrentProgress;			//!< not thread safe but as this is only for the preview it doesn't have to be 100% accurate
	CPolybumpProgress				m_PreviewProgress;			//!< is compared against m_CurrentProgess to detect if there is an update needed
	std::vector<uint32>			m_PreviewBuffer;				//!< required to avoid memory allocations for the preview

	CSimpleIndexedMesh			m_LowMesh;							//!< low resolution mesh
	CSimpleIndexedMesh			m_CageMesh;							//!< cage mesh to limit ray shooting


	bool										m_bSetupPhase;					//!<
	HWND										m_hThreadderWndParent;	//!<
	HWND										m_hThreadderWnd;				//!< handle to the progress window
	uint32									m_dwStartTime;					//!< absolute, for time calculations (in ms)
	uint32									m_dwComputationTime;		//!< relative, set after computation is finished
	bool										m_bComputationFinished;	//!<

	// hidden
  CPbHighMesh							m_HighMeshData;					//!<

	//! paint all triangles into the texture, every triangle with its pointer
  void DrawTriPointer( uint16 *pTriIndices, CPbLowTri &rTri, int color, const bool inbConservative );
	
	void CreateThreadderHWND();

	void DestroyThreadderHWND();

	virtual UINT ThreadMain();

	void Paint( HDC hdc );

	void UpdateProgress();

	void ComputePreviewExtend( uint32 &dwWidth, uint32 &dwHeight );

	friend bool CreateBumpMap( CPolyBumpWorkerThread *inpData );
	friend LRESULT static CALLBACK ThreadderWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
};


