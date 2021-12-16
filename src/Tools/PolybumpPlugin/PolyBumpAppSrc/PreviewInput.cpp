#include "stdafx.h"
#include "PolyBumpApplication.h"
#include "PreviewInput.h"
#include "previewinput.h"
#include <limits>									// FLT_MAX
#include "previewinput.h"



// CPreviewInput dialog

IMPLEMENT_DYNAMIC(CPreviewInput, CDialog)

CPreviewInput::CPreviewInput( CWnd* pParent /*=NULL*/)
	: CDialog(CPreviewInput::IDD, pParent), m_vMin(FLT_MAX,FLT_MAX,FLT_MAX), m_vMax(-FLT_MAX,-FLT_MAX,-FLT_MAX),
	m_vOffset(0,0,0), m_vScale(0,0,0), m_bIsResizing(false)
{
}


void CPreviewInput::SetRayLengthInOS( const float fRayLengthInOS )
{
	m_fRayLengthInOS=fRayLengthInOS;
}

CPreviewInput::~CPreviewInput()
{
	{
		std::vector<SMeshEntry *>::iterator it, end=m_MeshEntries.end();

		for(it=m_MeshEntries.begin();it!=end;++it)
		{
			SMeshEntry *pEntry = *it;

			delete pEntry;
		}
		m_MeshEntries.clear();
	}
}

void CPreviewInput::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPreviewInput, CDialog)
	ON_MESSAGE(WM_ENTERSIZEMOVE,OnEnterSizeMove)
	ON_MESSAGE(WM_EXITSIZEMOVE,OnExitSizeMove)
	ON_WM_DRAWITEM()
//	ON_WM_SIZE()
END_MESSAGE_MAP()


// CPreviewInput message handlers

BOOL CPreviewInput::OnInitDialog()
{
	CRect rect;
	GetClientRect(&rect);
	m_canvas.Create("", WS_CHILD|WS_VISIBLE|SS_OWNERDRAW, rect, this, 0);
	m_status.Create(WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_LEFT|ES_READONLY|ES_MULTILINE, rect, this, 0);
	CFont font;
	font.Attach(GetStockObject(SYSTEM_FIXED_FONT));
	m_status.SetFont(&font);
	RecalcLayout();
	return TRUE;
}

struct TextEntry {TextEntry(const string& name, int faces, int verts): name(name), faces(faces), verts(verts) {}
	string name; int faces; int verts;};
void CPreviewInput::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (m_canvas.GetSafeHwnd() && lpDrawItemStruct->hwndItem == m_canvas.GetSafeHwnd())
	{
		if(m_bIsResizing)
			return;

		CDC dc;
		dc.Attach(lpDrawItemStruct->hDC);

		// do this only once
		if(m_vMin==Vec3(FLT_MAX,FLT_MAX,FLT_MAX) && m_vMax==Vec3(-FLT_MAX,-FLT_MAX,-FLT_MAX))
		{
			std::string sWindowTitle;

			std::vector<TextEntry> entries;
			int totalFaces = 0, totalVerts = 0;
			std::vector<SMeshEntry *>::iterator it, end=m_MeshEntries.end();
			for(it=m_MeshEntries.begin();it!=end;++it)
			{
				SMeshEntry &ref = *(*it);

				if(ref.m_Mesh.m_FaceCount)
				{
					entries.push_back(TextEntry(ref.m_sType, ref.m_Mesh.m_FaceCount, ref.m_Mesh.m_VertCount));

					ExtendMinMax(ref.m_Mesh);
				}

				totalFaces += (*it)->m_Mesh.m_FaceCount;
				totalVerts += (*it)->m_Mesh.m_VertCount;
			}
			entries.push_back(TextEntry("TOTAL", totalFaces, totalVerts));

			for (std::vector<TextEntry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				char str[256], namestr[256], tristr[256], vertstr[256];
				sprintf(namestr, "%s:", (*it).name.c_str());
				sprintf(tristr, "%d", (*it).faces);
				sprintf(vertstr, "%d", (*it).verts);
				sprintf(str, "%-12s %12s tri %12s vert\r\n", namestr, tristr, vertstr);

				sWindowTitle += str;
			}

			m_status.SetWindowText(sWindowTitle.c_str());

			RecalcLayout();
		}

		std::vector<SMeshEntry *>::iterator it, end=m_MeshEntries.end();

		for(it=m_MeshEntries.begin();it!=end;++it)
		{
			SMeshEntry &ref = *(*it);

			if(ref.m_bLines)
				DrawMesh_Lines(dc,ref.m_Mesh,ref.m_dwColor,0xff7f7f);
			else
				DrawMesh_Points(dc,ref.m_Mesh,ref.m_dwColor);
		}
	}
}

void CPreviewInput::DrawMesh_Lines( CDC &dc, const CSimpleIndexedMesh &rMesh, const uint32 dwTriColor, const uint32 dwNormalColor )
{
	{
		CPen pen(PS_SOLID,1,dwTriColor);

		CPen *oldpen = dc.SelectObject(&pen);

		for(uint32 dwF=0;dwF<rMesh.m_FaceCount;++dwF)
		{
			uint32 dwI[3];				rMesh.GetPosIndices(dwF,dwI);

			Vec3 v[3] = { rMesh.GetPos(dwI[0]), rMesh.GetPos(dwI[1]), rMesh.GetPos(dwI[2]) };
			
			int x[3],y[3],z[3];

			for(uint32 dwI=0;dwI<3;++dwI)
			{
				x[dwI] = (int)((v[dwI].x+m_vOffset.x)*m_vScale.x);
				y[dwI] = (int)((v[dwI].y+m_vOffset.y)*m_vScale.y);
				z[dwI] = (int)((v[dwI].z+m_vOffset.z)*m_vScale.z);
			}

			// draw triangle
			dc.MoveTo(x[0],y[0]);
			dc.LineTo(x[1],y[1]);
			dc.LineTo(x[2],y[2]);
			dc.LineTo(x[0],y[0]);

			// draw triangle
			dc.MoveTo(z[0],y[0]);
			dc.LineTo(z[1],y[1]);
			dc.LineTo(z[2],y[2]);
			dc.LineTo(z[0],y[0]);
	/*
			int midx = (x[0]+x[1]+x[2])/3;
			int midy = (y[0]+y[1]+y[2])/3;
			int midz = (z[0]+z[1]+z[2])/3;
	*/
		}

		dc.SelectObject(oldpen);
	}

	if(rMesh.m_NormCount)		// normals
	{
		CPen pen(PS_SOLID,1,dwNormalColor);

		CPen *oldpen = dc.SelectObject(&pen);

		for(uint32 dwTri=0;dwTri<rMesh.m_FaceCount;++dwTri)
		{
			uint32 dwNormalIndex[3];
			rMesh.GetNormalIndices(dwTri,dwNormalIndex);

			uint32 dwPosIndex[3];
			rMesh.GetPosIndices(dwTri,dwPosIndex);

			for(uint32 dwI=0;dwI<3;++dwI)
			{
				Vec3 v0 = rMesh.GetPos(dwPosIndex[dwI]);

				Vec3 v1 = v0 + rMesh.GetNormal(dwNormalIndex[dwI]) * m_fRayLengthInOS;		// normal scale

				int x0,y0,z0;

				x0 = (int)((v0.x+m_vOffset.x)*m_vScale.x);
				y0 = (int)((v0.y+m_vOffset.y)*m_vScale.y);
				z0 = (int)((v0.z+m_vOffset.z)*m_vScale.z);

				int x1,y1,z1;

				x1 = (int)((v1.x+m_vOffset.x)*m_vScale.x);
				y1 = (int)((v1.y+m_vOffset.y)*m_vScale.y);
				z1 = (int)((v1.z+m_vOffset.z)*m_vScale.z);

				// normal
				dc.MoveTo(x0,y0);
				dc.LineTo(x1,y1);

				dc.MoveTo(z0,y0);
				dc.LineTo(z1,y1);
			}
		}
		dc.SelectObject(oldpen);
	}
}


void CPreviewInput::DrawMesh_Points( CDC &dc, const CSimpleIndexedMesh &rMesh, const uint32 dwColor )
{
	CPen pen(PS_SOLID,1,0x0000ff00);

	CPen *oldpen = dc.SelectObject(&pen);
/*
	if(rMesh.m_NormCount)		// normals
	{
		for(uint32 dwTri=0;dwTri<rMesh.m_FaceCount;++dwTri)
		{
			uint32 dwNormalIndex[3];
			rMesh.GetNormalIndices(dwTri,dwNormalIndex);

			uint32 dwPosIndex[3];
			rMesh.GetPosIndices(dwTri,dwPosIndex);

			for(uint32 dwI=0;dwI<3;++dwI)
			{
				Vec3 v0 = rMesh.GetPos(dwPosIndex[dwI]);
				Vec3 v1 = v0 + rMesh.GetNormal(dwNormalIndex[dwI]) * 10.0f;		// normal scale
				
				int x0,y0,z0;

				x0 = (int)((v0.x+m_vOffset.x)*m_vScale.x);
				y0 = (int)((v0.y+m_vOffset.y)*m_vScale.y);
				z0 = (int)((v0.z+m_vOffset.z)*m_vScale.z);

				int x1,y1,z1;

				x1 = (int)((v1.x+m_vOffset.x)*m_vScale.x);
				y1 = (int)((v1.y+m_vOffset.y)*m_vScale.y);
				z1 = (int)((v1.z+m_vOffset.z)*m_vScale.z);

				// normal
				dc.MoveTo(x0,y0);
				dc.LineTo(x1,y1);

				dc.MoveTo(z0,y0);
				dc.LineTo(z1,y1);
			}
		}
	}
*/
	for(uint32 dwV=0;dwV<rMesh.m_VertCount;++dwV)
	{
		Vec3 v0 = rMesh.GetPos(dwV);

		int x0,y0,z0;

		x0 = (int)((v0.x+m_vOffset.x)*m_vScale.x);
		y0 = (int)((v0.y+m_vOffset.y)*m_vScale.y);
		z0 = (int)((v0.z+m_vOffset.z)*m_vScale.z);

		dc.SetPixel(x0,y0,dwColor);
		dc.SetPixel(z0,y0,dwColor);
	}

	dc.SelectObject(oldpen);
}


void CPreviewInput::ExtendMinMax( CSimpleIndexedMesh &rMesh )
{
	// go through triangles corners because not all vertices might be used

	for(uint32 dwF=0;dwF<rMesh.m_FaceCount;++dwF)
	{
		uint32 dwI[3];				rMesh.GetPosIndices(dwF,dwI);

		Vec3 v[3] = { rMesh.GetPos(dwI[0]), rMesh.GetPos(dwI[1]), rMesh.GetPos(dwI[2]) };

		for(uint32 dwI=0;dwI<3;++dwI)
		{
			m_vMin.CheckMin(v[dwI]);
			m_vMax.CheckMax(v[dwI]);
		}
	}
}
	
CSimpleIndexedMesh &CPreviewInput::CreateMeshEntry( const uint32 dwColor, const char *szType, const bool bLines )
{
	m_MeshEntries.push_back(new SMeshEntry(dwColor,szType,bLines));

	return m_MeshEntries.back()->m_Mesh;
}



LRESULT	CPreviewInput::OnEnterSizeMove( WPARAM	wparam,	LPARAM lparam	)
{
	m_bIsResizing=true;

	return 0;
}
LRESULT	CPreviewInput::OnExitSizeMove( WPARAM	wparam,	LPARAM lparam	)
{
	m_bIsResizing=false;

	RecalcLayout();

	InvalidateRect(0,true);

	return 0;
}


void CPreviewInput::RecalcLayout()
{
	RECT rect;

	GetClientRect(&rect);

	RECT statusRect = rect;
	rect.bottom = max(rect.bottom - 80, rect.top);
	statusRect.top = rect.bottom;
	m_canvas.MoveWindow(&rect);
	m_status.MoveWindow(&statusRect);

	uint32 dwFullWidth = rect.right-rect.left;
	uint32 dwFullHeight = rect.bottom-rect.top;

	float fNeededWidth = (m_vMax.x-m_vMin.x) + (m_vMax.z-m_vMin.z);
	float fNeededHeight = (m_vMax.y-m_vMin.y);

	float fScale = min(dwFullWidth/fNeededWidth,dwFullHeight/fNeededHeight);

	m_vOffset = Vec3(-m_vMin.x,-m_vMin.y,-m_vMin.z+(m_vMax.x-m_vMin.x)); 
	m_vScale = Vec3(fScale,fScale,fScale);

}
