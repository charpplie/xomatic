
class CDynamicRibbonBar
{
public:
	void Init( CXTPRibbonBar *pRibbonBar );

	void LoadFromFile( const CString &filename );
	void LoadFromXML( XmlNodeRef &node );
	void MakeCreateTab();
	void MakeViewTab();

protected:
	void LoadTab( XmlNodeRef &tabNode );
	void LoadGroup( CXTPRibbonTab* pTab,XmlNodeRef &groupNode );
	void LoadControl( CXTPRibbonGroup *pGroup,XmlNodeRef &controlNode );
	void ParseResourceFile( const char *filename );

	int  MakeCreateCommand( const CString &className );

private:
	CXTPRibbonBar *m_pRibbonBar;
	std::map<CString,int> m_resourceNameToIdMap;
};