#pragma once

//////////////////////////////////////////////////////////////////////////
class CPVertex;
class CPhotoFrame;

//////////////////////////////////////////////////////////////////////////
class CPQuadtree
{
public:
	CPQuadtree(CPQuadtree *pParent);
	virtual ~CPQuadtree(void);
	
	CPQuadtree *GetParent() { return (m_pParent); }

	void	Draw(CPhotoFrame	*pFrame);
	void	DrawRecursive(CPhotoFrame	*pFrame);

	void	BuildRecursive(lstPhotoVertices *pList,lstFrames *pListFrames,CPhotoFrame *pFrame);
	bool	Split(CPVertex *pCenter,lstPhotoVertices *pList,lstFrames *pListFrames,CPhotoFrame *pFrame);

	bool	IsInside(const CPVertex &pVert,CPhotoFrame *pFrame);

	CPQuadtree	*m_Child[4];
	CPVertex		*m_Corner[4];	
	ftype				m_fError;

protected:

	CPQuadtree	*m_pParent;
};
