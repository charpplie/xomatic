#ifndef __WeaponEditorSettingsDialog_h__
#define __WeaponEditorSettingsDialog_h__

#include "ToolbarDialog.h"
#include "UIPanels/TaskPanelDlg.h"

struct IItem;
class CWeaponEditorDialog;

class CWeaponEditorSettingsDialog : public CToolbarDialog
{
	DECLARE_DYNAMIC(CWeaponEditorSettingsDialog)
	struct SEditHelper
	{
		SEditHelper()
		{}
		SEditHelper(int i, int si)
			:pin(false)
			,idx(i)
			,sub_idx(si)
		{}

		SEditHelper(int i)
			:pin(true)
			,idx(i)
		{}
		bool		pin;
		int			idx;
		int			sub_idx;
	};
public:
	static void RegisterViewClass();

	CWeaponEditorSettingsDialog();
	~CWeaponEditorSettingsDialog();

	void SetEditorDialog(CWeaponEditorDialog* editor);
	void OnLoad();

	bool GetSelectedHelperPos(Matrix34& m, bool local = false) const;
	bool SetSelectedHelperPos(const Matrix34& m);
	void MoveSelectedHelper(const Vec3& v);
	void RotateSelectedHelper(const Quat& q);
	bool GetHelperPos(const char* name, Matrix34& m);

	void OnSave();

	bool HasChanges() const { return m_hasChanges; }

	enum { IDD = IDD_WEAPON_SETTINGS };

	const char* GetFilePath() const;

	const IItem* GetAttachedItem() const { return m_pAttachedItem; }
	const Matrix34& GetAttachedItemTM() const;
protected:
	virtual void OnOK() {}
	virtual void OnCancel() {}

	void RecalcLayout();

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void UpdateAttachments();
	void UpdateHelpers();
	void SaveHelpers();

	void FillHelpersBox();
	void FillBoneHelpersBox();
	void FillBoneHelpersList();
	void FillGeometry();

	bool GetSelectedHelper(int& idx, int& subidx)const;
	bool GetSelectedPin(int &idx)const;

	void BrowseForSlot(int slot);
	void ClearSlot(int slot);

	void CleanUpAttachedEntity();

	void CheckViewMode();

	void OnChange();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClicked_FPCamera();
	afx_msg void OnClicked_Zoom();
	afx_msg void OnClicked_Reload();
	afx_msg void OnClicked_BrowseFP();
	afx_msg void OnClicked_BrowseTP();
	afx_msg void OnClicked_BrowseAUX();
	afx_msg void OnClicked_ClearFP();
	afx_msg void OnClicked_ClearTP();
	afx_msg void OnClicked_ClearAUX();

	afx_msg void OnOffsetChange();

	afx_msg void UpdateHelperPos();
	afx_msg void OnHelperSelectionChanged();
	
	afx_msg void SetHelperPos();
	afx_msg void OnBoneHelperSelected();
	afx_msg void SetBoneHelperBone();

	afx_msg void OnAddAttachment();
	afx_msg void OnRemoveAttachment();

	afx_msg void OnBnClickedWeaponFp();
	afx_msg void OnBnClickedWeaponTp();
private:
	CXTPTaskPanel m_wndTaskPanel;
  CXTPTaskPanelGroup* m_OffsetGroup;
	CXTPTaskPanelGroup* m_HelpersGroup;
	CTaskPanelDlg m_dlgOffset;
	CNumberCtrl		m_pos_x;
	CNumberCtrl		m_pos_y;
	CNumberCtrl		m_pos_z;
	CNumberCtrl		m_rot_yw;
	CNumberCtrl		m_rot_pt;
	CNumberCtrl		m_rot_rl;
	CNumberCtrl		m_fov;

	CNumberCtrl		m_helperPos[6];
	CComboBox			m_subObjects;

	CTaskPanelDlg m_dlgAttachmens;
	CListBox m_attachmentsList;

	CTaskPanelDlg m_dlgHelpers;
	CListBox m_helpersList;

	CListBox m_boneHelpersList;
	CComboBox m_boneHelpersBone;

	CEdit m_geometryFP;
	CEdit m_geometryTP;
	CEdit m_geometryAUX;

	IItem* m_pItem;
	CWeaponEditorDialog* m_editor;

	IItem* m_pAttachedItem;
	int m_attachedId;
	int m_attachedSocketId;
	int m_attachedSocketTypeId;

	bool m_hasChanges;
	std::vector<SEditHelper> m_editHelpers;
};

#endif __WeaponEditorSettingsDialog_h__