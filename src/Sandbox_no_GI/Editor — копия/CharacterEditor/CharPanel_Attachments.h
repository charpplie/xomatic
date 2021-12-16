#pragma once

class CBoneComboBoxManager;


struct SJPVariable : IVariable
{
	SJPVariable(struct SJointProperty *pjp) { m_pjp=pjp; }
	virtual	const char*	GetName() const;
	virtual	EType	GetType() const;
	virtual void Set( bool value );
	virtual void Set( float value );
	virtual void Get( bool &value ) const;
	virtual void Get( float &value ) const;
	virtual void GetLimits( float& fMin, float& fMax, bool& bHardMin, bool& bHardMax );

	struct SJointProperty *m_pjp;

	virtual	unsigned char	GetDataType() const { return DT_SIMPLE; }
	virtual void SetName( const CString &name ) {}
	virtual	const char*	GetHumanName() const { return GetName(); }
	virtual void SetHumanName( const CString &name ) {}
	virtual const char*	GetDescription() const { return 0; }
	virtual void SetDescription( const char *desc ) {}
	virtual	int	GetSize() const { return 4; }
	virtual void SetDataType( unsigned char dataType ) {}
	virtual void SetUserData( void* pData ) {}
	virtual void* GetUserData() { return 0; }
	virtual	void SetFlags( int flags ) {}
	virtual	int  GetFlags() const { return 0; }
	virtual void Set( int value ) {}
	virtual void Set( const Vec3& value ) {}
	virtual void Set( const Ang3& value ) {}
	virtual void Set( const Quat& value ) {}
	virtual void Set( const CString &value ) {}
	virtual void Set( const char* value ) {}
	virtual void SetDisplayValue( const CString &value );
	virtual void OnSetValue(bool bRecursive) {}
	virtual void Get( int &value ) const {}
	virtual void Get( Vec3& value ) const {}
	virtual void Get( Ang3& value ) const {}
	virtual void Get( Quat &value ) const {}
	virtual void Get( CString &value ) const {}
	virtual CString GetDisplayValue() const;
	virtual int NumChildVars() const { return 0; }
	virtual IVariable* GetChildVar( int index ) const { return 0; }
	virtual void AddChildVar( IVariable *var ) {}
	virtual bool DeleteChildVar( IVariable* var, bool recursive = false ) { return false; }
	virtual void DeleteAllChilds() {}
	virtual IVariable* Clone( bool bRecursive ) const;
	virtual void CopyValue( IVariable *fromVar ) {}
	virtual void SetLimits( float fMin, float fMax, bool bHardMin = true, bool bHardMax = true ) {}
	virtual void Wire( IVariable *targetVar ) {}
	virtual void Unwire( IVariable *targetVar ) {}
	virtual void AddOnSetCallback( OnSetCallback func ) {}
	virtual void RemoveOnSetCallback( OnSetCallback func ) {}
	virtual IVarEnumList* GetEnumList() const { return 0; }
	virtual ISplineInterpolator* GetSpline(bool bCreate) const { return 0; }
	virtual void Serialize( XmlNodeRef node,bool load ) {}
	virtual void EnableUpdateCallbacks(bool boEnable) {}
};

class CBonePropsDlg : public CDialog
{
public:
	CBonePropsDlg(CVarBlock *pVarBlock) : CDialog(IDD_BONE_PROPS,AfxGetMainWnd()) { m_pVB = pVarBlock; }
	virtual BOOL OnInitDialog();
	CVarBlock *m_pVB;
};


class CAttachmentsDlg : public CDialog
{

public:
	DECLARE_DYNAMIC(CAttachmentsDlg)

	CAttachmentsDlg( CWnd* pParent = NULL )	: CDialog(CAttachmentsDlg::IDD, pParent)	
	{
		m_AttachmentType=0;
		CharacterChanged=0;
		m_pBoneComboBoxManager=0;
	}

	~CAttachmentsDlg();

	// Dialog Data
	enum { IDD = IDD_CHARACTER_EDITOR_ATTACHMENTS };

	void ClearBones();
	void AddBone( const CString &bone );
	void SelectBone( const CString &bone );
	void UpdateList();
	void ReloadAttachment();
	void OnOK() {};
	void OnCancel() {};
	CString GetBonenameFromWindow();
	void UpdateAttachmentCharacterMotions();

	afx_msg void OnBnClicked_NEW();
	afx_msg void OnBnClicked_RENAME();
	afx_msg void OnBnClicked_REMOVE();
	afx_msg void OnBnClicked_IMPORT();
	afx_msg void OnBnClicked_EXPORT();

	afx_msg void OnAttachmentSelect();
	afx_msg void OnClicked_FaceAttach();
	afx_msg void OnClicked_BoneAttach();
	afx_msg void OnClicked_SkinAttach();
	afx_msg void OnClicked_HideAttachment();
	afx_msg void OnClicked_PhysAttachment();
	afx_msg void OnClicked_AlignBoneAttachment();

	afx_msg void OnBnClicked_FILEBROWSE();
	afx_msg void OnBnClicked_CLEAR();
	afx_msg void OnBnClicked_APPLY();
	afx_msg void OnBnClicked_MATERIALBROWSE();
	afx_msg void OnBnClicked_DFLTMATERIAL();
	afx_msg void OnBnClicked_PhysProps();
	afx_msg void OnBnClicked_PhysProps1();
	void OnBnClicked_PhysPropsIdx(int);

	afx_msg void OnHingeSelect();
	afx_msg void OnLimitOrDampingChange();
	afx_msg void OnBoneSelect();

	class CModelViewportCE *m_pModelViewportCE;
	CButton m_ButtonCLEAR;

	CButton m_ButtonRENAME;
	CButton m_ButtonREMOVE;
	CButton m_ButtonEXPORT;
	
	CButton m_browseObjectBtn;
	CComboBox m_boneName;
	CEdit m_objectName;
	CEdit m_materialName;
	CListBox m_attachmentsList;
	int m_AttachmentType;
	int m_AlignBoneAttachment;
	uint32 CharacterChanged;
	CComboBox m_hingeIndex;
	CNumberCtrl m_hingeLimit;
	CNumberCtrl m_hingeDamping;
	DynArray<struct SJointProperty> m_physProps[2];
	CVarBlock m_vbPhysProps[2];

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	struct IAttachment* GetSelected();
	CBoneComboBoxManager* m_pBoneComboBoxManager;

	DECLARE_MESSAGE_MAP()
};

