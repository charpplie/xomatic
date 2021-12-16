
/************************************************************************/
/*      Represents the Dialog for setting up asset randomization.       */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_RANDOMIZER_PANEL_H__
#define __ANIMATIONGRAPH2_RANDOMIZER_PANEL_H__

#pragma once

#include "Resource.h"
#include "AnimationGraph/AnimationRandomizer.h"


class CRandomAnimationSlot;
class CRandomAnim;

//! This keeps all the data for one AnimationRandomizer Set
class CRandomizerData
{
public:
	CRandomizerData();
	CRandomizerData(const char* firstAnimName);
	CRandomizerData(CRandomizerData& copyClass);

	~CRandomizerData();

	CRandomizerData& operator= (const CRandomizerData& copyClass);

	bool operator== (const CRandomizerData& b) const;

	//! Saving
	XmlNodeRef ToXml();
	
	//! Loading, returns true if successful
	bool LoadFromXml(XmlNodeRef node);

	//! Returns true if the given animation is not the only animation asset in the list, or there are
	//! other parameters (like a delay) that make this a valid randomizer set
	bool IsDifferent(CString& animName);

	//! Contains at least one animation
	bool IsValid();

public:
	CString										m_name;
	int												m_id;
	std::vector<CRandomAnim*>	m_randomAnims;
	CRandomGeneralParams			m_randomParams;
};


//! Implementation of the Editor interface for the Randomizer
class CAnimationGraph_RandomizerDialog : public CDialog
{

public:
	CAnimationGraph_RandomizerDialog();
	virtual ~CAnimationGraph_RandomizerDialog();

	BOOL OnInitDialog();

	void DoDataExchange(CDataExchange* pDX);
	void OnOK();

	void SetRandomizerData( CRandomizerData* randomizerData );
//	CRandomizerData* GetRandomizerData();

	void RemoveSlot( int slotID );

	//! Calculates a percentage (100% reference) for each animation based on the set probability and displays it.
	//! This should make it easier to judge which animation is being played how often and what effect the probabilities have.
	void CalculateAnimationProbabilities();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	afx_msg void OnAddSlot();
	afx_msg void OnGeneralParamsChanged();

private:

	void GrayOutItems();
	void PositionSlotPanel(CRandomAnimationSlot* pSlot, int index);
	void AdjustScroll();
	int	 GetScrollingPos();

	//! Will remove all slots that are invalid (have no animation name)
	void RemoveEmtpySlots();

	void CreateFromRandomizerData();
	void ReadBackToRandomizerData();

	int				m_slotsCreated;

	CButton		m_addSlot;
	CButton		m_switchAssetsOverTime;
	CButton		m_timeAlignBetweenAssets;
	CButton		m_switchOnNonLoopFinish;
	CButton		m_useInitialDelay;
	CEdit			m_switchProbabilitySeconds;
	CEdit			m_initialDelaySeconds;
	CEdit			m_randomizerName;
	CRandomizerData*	m_randomizerData;
	std::vector<CRandomAnimationSlot*> m_animSlots;
};


//////////////////////////////////////////////////////////////////////////

//! Single Animation Slot for Randomizer with all parameters
class CRandomAnimationSlot
	: public CXTResizeDialog
{
	DECLARE_DYNAMIC( CRandomAnimationSlot )

public:

	CRandomAnimationSlot( CWnd* pParent, int slotId );
	virtual ~CRandomAnimationSlot();

	enum { IDD = IDD_AG2_RANDOMIZER_SLOT };

	ILINE void SetRandomizerDialog(CAnimationGraph_RandomizerDialog* parent) { m_pParent = parent; }
	
	void SetData(CRandomAnim* data);
	void GetData(CRandomAnim& data);

	const int GetSlotID() const { return m_slotID; }

	// Retrieves the number in the probability edit field
	int GetProbability();

	// Sets the relative percentage probability for the static text field
	// which is displayed as a convenience.
	void SetPercentageProbability(float probability);

	//! returns true if the slot is valid (has an animation name)
	bool IsValidSlot();


protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
//	virtual BOOL PreTranslateMessage( MSG* pMsg );

	afx_msg void OnAssetDataUpdated();
	afx_msg void OnRemoveSlot();
	DECLARE_MESSAGE_MAP()

private:

	int			m_slotID;
	CButton	m_loop;
	CButton	m_preventCutoff;
	CButton	m_allowReselection;
	CEdit		m_probability;
	CEdit		m_animationName;
	CEdit		m_transitionTime;
	CAnimationGraph_RandomizerDialog* m_pParent;

/*
	CStatic m_statusImage;

	CToolTipCtrl m_tooltip;

	ISlotControlAnimationNameListener* m_animationNameChangeListener;

	CString m_oldAnimationName;
*/
};


//////////////////////////////////////////////////////////////////////////

//! Random Animation Set Manager
class CRandomAnimationSetManager
{
public:
	CRandomAnimationSetManager();
	~CRandomAnimationSetManager();

	void Reset();

	// Saving and Loading
	void SaveToXML(XmlNodeRef node);
	void LoadFromXML(XmlNodeRef node);

	//! This function retrieves or creates (if it doesn't exist) the randomizer data set with the given name.
	CRandomizerData* GetRandomizerData(const char* name);

	CRandomizerData* DoesRandomizerDataExist( const char* name );

	//! Remove and deletes the randomizer data set with the given name
	//! Make sure it isn't referenced anywhere anymore
	void RemoveRandomizerData(const char* name);

private:

	//! Variable used for automatic consecutive naming of newly generated sets
	uint	m_nextNumber;

	typedef std::vector<CRandomizerData*> TRandomizerByName;
	TRandomizerByName m_randomAnimSets;
};

//////////////////////////////////////////////////////////////////////////

class CAnimationGraph2;

//! Edit box used for Animation Name Input.
class CAnimNameEdit : public CEdit
{
public:

	CAnimNameEdit() : CEdit()
	{	}
	void OpenRandomizer(CAnimationGraph2* pGraph);	
};


#endif // __ANIMATIONGRAPH2_RANDOMIZER_PANEL_H__
