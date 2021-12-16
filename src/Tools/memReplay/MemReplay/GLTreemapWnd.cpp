#include "stdafx.h"
#include "treemap.h"
#include "GLTreemapWnd.h"
#include "MemReplay.h"
#include "MemReplayDoc.h"
#include "MemReplayView.h"
#include "lists.h"

#define FADE_SCALE			SQR(0.5f)
#define LINEFADE_SCALE	SQR(0.2f)
#define TEXT_CUTOFF			0.01f

#define INTERFACE_TYPE	1
#define RENDER_3D				0

#if INTERFACE_TYPE==0
#define TOP_BORDER			0.015f
#define BOTTOM_BORDER		0.005f
#elif INTERFACE_TYPE==1
#define TOP_BORDER			(0.05f*m_popupScale*collapse)
#define SIDE_BORDER			(0.01f*m_popupScale*collapse)
#define TITLE_HEIGHT		(0.04f*m_popupScale*collapse)
#endif

static PoolAllocator<treemapItem, 4096> s_nodePool;

void* treemapItem::operator new (size_t sz)
{
	return s_nodePool.Allocate();
}

void treemapItem::operator delete (void* p, size_t sz)
{
	s_nodePool.Free(p);
}

CGLTreemapWnd::CGLTreemapWnd()
{
	m_x=0.0f;
	m_y=0.0f;
	m_zoom=1.0f;
	m_findZoomPoint=FALSE;
	m_expand=FALSE;
	m_zooming=FALSE;
	m_firstRender=1;
	m_liveNode=NULL;
	m_popupScale=1.0f;
	m_rebuildNeeded=TRUE;
	m_collapsing=FALSE;
	m_growing=FALSE;
	m_popupFade=0.01f;
	m_moved=FALSE;
	m_collapsedAmount=1.0f;
	m_treeRoot = NULL;
	m_zoomTargetItem = NULL;
	m_style = NULL;
	m_3dOffset = 0.0f;
	m_groupFilter = 1.0f;
	m_groupRatio = 1.0f;

	theApp.AddIdleHandler(this);
}

CGLTreemapWnd::~CGLTreemapWnd(void)
{
	theApp.RemoveIdleHandler(this);

	if (m_treeRoot != NULL)
	{
		freeTree(m_treeRoot);
	}
}

BEGIN_MESSAGE_MAP(CGLTreemapWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

afx_msg void CGLTreemapWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDown(nFlags, point);

	RaiseActiveItemSelected();
	
	m_mouseX=point.x;
	m_mouseY=point.y;

	m_expand=TRUE;
}

afx_msg void CGLTreemapWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDblClk(nFlags, point);

	m_findZoomPoint=TRUE;
}

afx_msg void CGLTreemapWnd::OnMButtonDown(UINT nFlags, CPoint point)
{
	m_collapsing=TRUE;
	m_growing=FALSE;
	m_zooming=FALSE;
}

afx_msg void CGLTreemapWnd::OnMButtonUp(UINT nFlags, CPoint point)
{
	m_collapsing=FALSE;
	m_growing=TRUE;
}

afx_msg void CGLTreemapWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	SetFocus();

	if (nFlags&MK_RBUTTON)
	{
		m_x+=point.x-m_mouseX;
		m_y-=point.y-m_mouseY;
		m_zooming=FALSE;
	}
	if (nFlags&MK_MBUTTON)
	{
		m_zooming=FALSE;
		m_collapsing=TRUE;
		m_growing=FALSE;
	}
	else
	{
		m_collapsing=FALSE;
		m_growing=TRUE;
	}
	m_mouseX=point.x;
	m_mouseY=point.y;
	m_moved=TRUE;

	TRACKMOUSEEVENT track = {0};
	track.cbSize = sizeof(track);
	track.dwFlags = TME_LEAVE;
	track.hwndTrack = m_hWnd;
	TrackMouseEvent(&track);

	Invalidate(FALSE);
}

afx_msg BOOL CGLTreemapWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	float factor=(zDelta>=0)?1.0f/0.8f:0.8f;
	ScreenToClient(&pt);
	float xf=(m_x-pt.x)/m_zoom;
	float yf=(m_y-(m_height-pt.y))/m_zoom;
	while (abs(zDelta)>=WHEEL_DELTA)
	{
		m_zoom*=factor;
		zDelta=abs(zDelta)-WHEEL_DELTA;
	}
	m_zoom=MAX(m_zoom, 0.01f);
	m_x=xf*m_zoom+pt.x;
	m_y=yf*m_zoom+(m_height-pt.y);
	m_zooming=FALSE;
	m_rebuildNeeded=TRUE;

	Invalidate(FALSE);

	return TRUE;
}

void CGLTreemapWnd::OnMouseLeave()
{
	CWnd::OnMouseLeave();

	m_mouseY = m_mouseX = -10000;
}

void CGLTreemapWnd::OnPaint()
{
	CPaintDC dc(this);

	oglDrawScene();
	//ValidateRect(NULL);
}

void CGLTreemapWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	if (0 >= cx || 0 >= cy || nType == SIZE_MINIMIZED)
		return;
	/*
	if (IsWindow(m_hWnd))
		MoveWindow(0,0,cx,cy);
	glViewport(0, 0, cx, cy);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, cx, 0, cy);
	glMatrixMode(GL_MODELVIEW );
	glLoadIdentity();
	*/
	m_width=(float)cx;
	m_height=(float)cy;

	Invalidate();
}

int CGLTreemapWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1) return -1;
	oglInitialize();
	return 0;
}

void CGLTreemapWnd::DefaultColour(const treemapItem* node, int depth, float* col)
{
	float s=0.5f-depth/256.0f;
	float h=depth/12.0f;
	float v=1.0f;
	h=h-floorf(h);
	int hi=(int)(h*6.0f);
	float f=h*6.0f-hi;
	float p=v*(1.0f-s);
	float q=v*(1.0f-f*s);
	float t=v*(1.0f-(1.0f-f)*s);
	switch (hi)
	{
	case 0:
		col[0]=v; col[1]=t; col[2]=p; break;
	case 1:
		col[0]=q; col[1]=v; col[2]=p; break;
	case 2:
		col[0]=p; col[1]=v; col[2]=t; break;
	case 3:
		col[0]=p; col[1]=q; col[2]=v; break;
	case 4:
		col[0]=t; col[1]=p; col[2]=v; break;
	case 5:
		col[0]=v; col[1]=p; col[2]=q; break;
	}
	col[3] = 0.0f;
}

static char s_dummy[4096];

float CGLTreemapWnd::GetSize(const treemapItem *item)
{
	if (m_groupFilter==1.0f)
	{
		return item->size;
	}
	else
	{
		float k=(1.0f-m_groupFilter)/m_groupFilter;
		float f=m_groupRatio/(k+m_groupRatio);
		return f*item->size+(1.0f-f)*item->oldSize;
	}
}

void CGLTreemapWnd::drawTreeFromControl(treemapItem *item, float x, float y, float w, float h, int depth, int parentHadChildren, float collapse)
{
	float barHeight=0.0f;
	int childCount=item->children.size();
	float total=0;
	int selected=FALSE;
	float threedOffset=m_3dOffset*depth;

#if RENDER_3D
	glPushMatrix();
	glTranslatef(threedOffset, 0.0f, 0.0f);
#endif

	if (childCount!=1)
	{
		collapse=1.0f;
	}
					
	item->childrenCollapse += item->collapseDirection*0.05f;
	item->childrenCollapse=CLAMP(item->childrenCollapse,0.0f,1.0f);









	for (int i=0; i<childCount; i++)
	{
		total+=GetSize(item->children[i]);
	}

	int oldX=static_cast<int>(x),oldY=static_cast<int>(y),oldW=static_cast<int>(ceilf(w)),oldH=static_cast<int>(ceilf(h));
#if INTERFACE_TYPE==0
	if (depth>0)
#endif
		float textHeight=m_font.getHeight()*1.15f;
	float scaleY=(TOP_BORDER*h)/textHeight;

	const char* itemTitle = m_style->GetNodeTitle(item);

	{
		float col[4];
		float fade=CLAMP(SQR(4.0f)*w*h/(m_width*m_height),0.0f,1.0f);
#if INTERFACE_TYPE==0
		x+=w*TOP_BORDER;
		y+=h*BOTTOM_BORDER;
		w*=1.0f-TOP_BORDER-BOTTOM_BORDER;
		h*=1.0f-TOP_BORDER-BOTTOM_BORDER;
#endif
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glScissor(static_cast<int>(x+threedOffset-1),static_cast<int>(y-1),static_cast<int>(ceilf(w))+2,static_cast<int>(ceilf(h))+2);
		if (m_style)
			m_style->GetNodeColour(item, depth, col);
		else
			DefaultColour(item, depth, col);
#if !RENDER_3D
		glColorMask(1,1,1,0);
#endif
		glColor4f(col[0], col[1], col[2], lerp(fade, 1.0f, col[3]));
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x+w, y);
		glVertex2f(x+w, y+h);
		glVertex2f(x, y+h);
		glEnd();
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x+w, y);
		glVertex2f(x+w, y+h);
		glVertex2f(x, y+h);
		glEnd();
#if INTERFACE_TYPE==1
		float newX, newY, newW, newH;
		float mouseY=m_height-m_mouseY;
		if ((m_mouseX>=x && m_mouseX<x+w && mouseY>=y && mouseY<y+h) || (m_liveNode == item))
		{
			selected=TRUE;
		}

		char wrappedTitle[512];
		int numTitleLines = m_font.wordWrap(wrappedTitle, 512, itemTitle, (unsigned int) (w / scaleY));
		numTitleLines = std::min(3, numTitleLines);

		barHeight=(TITLE_HEIGHT * numTitleLines + SIDE_BORDER) *h;
		newX=x+(w*SIDE_BORDER);
		newW=w*(1.0f-2.0f*SIDE_BORDER);
		newY=y+(h*SIDE_BORDER);
		newH=h*(1.0f-SIDE_BORDER-TITLE_HEIGHT * numTitleLines - SIDE_BORDER);
		if (childCount!=0 && m_mouseX>=newX && m_mouseX<newX+newW && mouseY>=newY && mouseY<newY+newH)
		{
			selected=FALSE;
		}
		if (selected)
		{
			if (m_liveNode != item)
			{
				m_liveNode = item;
				RaiseActiveItemChanged();
			}

#if !RENDER_3D
			glColorMask(1,1,1,0);
#endif
			glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x+w, y);
			glVertex2f(x+w, y+h);
			glVertex2f(x, y+h);
			glEnd();
			m_popupScale=m_collapsedAmount;
			if (m_findZoomPoint)
			{
				m_zoomTargetItem = item;
			}
			if (m_expand)
			{
				if ((m_mouseX>=newX && m_mouseX<newX+scaleY*textHeight && mouseY>=y+h-scaleY*textHeight && mouseY<y+h))
				{
					item->collapseDirection = -item->collapseDirection;
				}
				m_expand=FALSE;
			}
		}
		if (m_zoomTargetItem == item)
		{
			m_zoomTargetX=-(x-m_x);
			m_zoomTargetY=-(y+h-m_y);
			m_zoomTargetX/=m_zoom;
			m_zoomTargetY/=m_zoom;
			m_zoomTarget=m_zoom*MIN(m_width/w, m_height/h);
			m_zoomTargetX*=m_zoomTarget;
			m_zoomTargetY*=m_zoomTarget;
			m_zoomTargetY+=m_height;
			m_findZoomPoint=FALSE;
			m_zooming = TRUE;
			m_zoomTargetItem = NULL;
		}
		x=newX;
		y=newY;
		w=newW;
		h=newH;
#endif
	}
	if (w*h>150.0f && childCount>0 && (m_rebuildNeeded || item->hasBeenBuilt))
	{
		treemap t;
		float s;

		if (m_rebuildNeeded)
		{
			treemapCreate(&t, w, h);
			if (total>0.0f)
				s=w*h/(float)total;
			else
				s=0.0f;
			for (int i=0; i<childCount; i++)
			{
				treemapAddNode(&t, &item->children[i]->node, s*GetSize(item->children[i]));
			}
			treemapBuild(&t);
			item->builtW=w;
			item->builtH=h;
			item->hasBeenBuilt=TRUE;
		}
		float scaleX=w/item->builtW;
		float scaleY=h/item->builtH;
#if RENDER_3D
		glPopMatrix();
#endif
		for (int i=0; i<childCount; i++)
		{
			if (x+(item->children[i]->node.x+item->children[i]->node.w)*scaleX>=0 && x+item->children[i]->node.x*scaleX<=m_width)
			{
				if (y+(item->children[i]->node.y+item->children[i]->node.h)*scaleY>=0 && y+item->children[i]->node.y*scaleY<=m_height)
				{
					drawTreeFromControl(item->children[i], x+item->children[i]->node.x*scaleX, y+item->children[i]->node.y*scaleY, item->children[i]->node.w*scaleX, item->children[i]->node.h*scaleY, depth+1, childCount>1, item->childrenCollapse*collapse);
				}
			}
		}
#if RENDER_3D
		glPushMatrix();
		glTranslatef(threedOffset, 0.0f, 0.0f);
#endif
	}
	glPushMatrix();
	glEnable(GL_BLEND);
#if !RENDER_3D
	glColorMask(1,1,1,0);
#endif
	glScissor((GLint)(oldX+threedOffset),oldY,oldW,oldH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#if INTERFACE_TYPE==0
	float fade=w*h/(m_width*m_height);
	fade=1.0f-SQR(1.0f-fade);
	float textHeight=m_font.getHeight();
	float textWidth=m_font.calcStringWidth(std::string(item->name));
	float scaleY=h/textHeight;
	float scaleX=w/textWidth;
	float scale=MAX(MIN(scaleX, scaleY),0.0f);
	glColor4f(1.0f, 1.0f, 1.0f, fade);
	glScalef(scale, scale, 1.0f);
	m_font.drawText(x/scale,(y+h/2.0f)/scale-(textHeight/2.0f),std::string(item->name));
#elif INTERFACE_TYPE==1












































	if (scaleY>TEXT_CUTOFF)
	{
		char wrappedTitle[512];
		int numTitleLines;
		if (item->collapseDirection)
		{
			char tempString[4096];
			if (item->collapseDirection<0.0f)
			{
				sprintf(tempString, "[+] %s", itemTitle);
			}
			else
			{
				sprintf(tempString, "[-] %s", itemTitle);
			}
			numTitleLines = m_font.wordWrap(wrappedTitle, 512, tempString, (unsigned int) (w / scaleY));
		}
		else
		{
			numTitleLines = m_font.wordWrap(wrappedTitle, 512, itemTitle, (unsigned int) (w / scaleY));
		}

		glPushMatrix();
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		glScalef(scaleY, scaleY, 1.0f);
		m_font.drawText(x/scaleY,(y+h)/scaleY,wrappedTitle);
		glPopMatrix();
	}

	if (childCount==0)
	{
		char sizeName[128] = "";
		if (m_style)
			m_style->GetLeafNodeText(item, sizeName, sizeof(sizeName));

		float textWidth=m_font.getWidth(sizeName);
		float scaleX=w/textWidth;
		float scaleY=h/textHeight;
		float scale=MIN(scaleX, scaleY);
		if (scale>TEXT_CUTOFF)
		{
			glPushMatrix();
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glScalef(scale, scale, 1.0f);
			m_font.drawText((x+w/2.0f)/scale-textWidth/2.0f,(y+h/2.0f)/scale-textHeight/2.0f, sizeName);
			glPopMatrix();
		}
	}

#endif
	glPopMatrix();
	if (selected)
	{
		m_popupScale=1.0f;
	}
#if RENDER_3D
	glPopMatrix();
#endif
}

CMemReplayView* CGLTreemapWnd::GetView(void)
{
	CFrameWnd * pFrame = (CFrameWnd *)(AfxGetApp()->m_pMainWnd);
	CView * pView = pFrame->GetActiveView();
	if ( !pView )
		return NULL;
	if ( ! pView->IsKindOf( RUNTIME_CLASS(CMemReplayView) ) )
		return NULL;
	return (CMemReplayView*)pView;
}

#define ZOOMRATE	0.1f

void CGLTreemapWnd::oglDrawScene(void)
{
	if (m_firstRender)
	{
		m_firstRender--;
		return;
	}

	wglMakeCurrent(hdc, hrc);

	CRect rcClient;
	GetClientRect(rcClient);

	glViewport(0, 0, rcClient.Width(), rcClient.Height());
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, rcClient.Width(), 0, rcClient.Height());
	glMatrixMode(GL_MODELVIEW );
	glLoadIdentity();

	glColorMask(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#if RENDER_3D
	glColorMask(1,0,0,1);
	m_3dOffset=1.0f;
#endif
	glEnable(GL_SCISSOR_TEST);
	glLoadIdentity();
	if (m_zooming)
	{
		m_x=m_x*(1.0f-ZOOMRATE)+m_zoomTargetX*ZOOMRATE;
		m_y=m_y*(1.0f-ZOOMRATE)+m_zoomTargetY*ZOOMRATE;
		m_zoom=m_zoom*(1.0f-ZOOMRATE)+m_zoomTarget*ZOOMRATE;
		m_rebuildNeeded=TRUE;
	}
	if (m_treeRoot != NULL)
		drawTreeFromControl(m_treeRoot, m_x, m_y, m_zoom*m_width, m_zoom*m_height, 0, TRUE, 1.0f);

#if RENDER_3D
	glColorMask(0,1,0,1);
	m_3dOffset=-1.0f;
	if (m_treeRoot != NULL)
		drawTreeFromControl(m_treeRoot, m_x, m_y, m_zoom*m_width, m_zoom*m_height, 0, TRUE, 1.0f);
	glColorMask(1,1,1,1);
#endif

	glScissor(0,0,rcClient.Width(),rcClient.Height());
	/*
	glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
	glColorMask(1,1,1,0);
	glColor4f(0.0f,0.0f,0.0f,1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(m_width, 0);
	glVertex2f(m_width, m_height);
	glVertex2f(0, m_height);
	glEnd();
	*/
	if (m_liveNode && m_mouseX > -10000 && m_mouseY > -10000)
	{
		if (m_liveNode != m_lastPopupNode)
		{
			m_popupText.clear();
			GetPopupText(m_liveNode, m_popupText);
			m_lastPopupNode = m_liveNode;
		}

		int height=16;
		s8 temp[1024];
		float scale=height/(float)m_font.getHeight();
		float popupX=(float)m_mouseX;
		float popupY=(float)m_mouseY;

		int numRows = 0;
		float maxLength = 0;

		{
			size_t offset = 0, endOffset = 0;

			do
			{
				endOffset = m_popupText.find('\n', offset);
				if (endOffset == m_popupText.npos)
					endOffset = m_popupText.size();

				sprintf_s(temp, sizeof(temp), "%s", m_popupText.c_str()+offset);
				temp[MIN(endOffset-offset, sizeof(temp)-1)]='\0';

				maxLength = std::max(maxLength, m_font.getWidth(temp) * scale);
				offset = endOffset + 1;

				++ numRows;
			}
			while (endOffset != m_popupText.size());
		}

		if (GetKeyState(VK_CONTROL) & 0x80000000)
			m_popupFade = MIN(0.3f, m_popupFade);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPushMatrix();
		glColor4f(0.0f,0.0f,0.0f,0.5f*MAX(m_popupFade,0.0f));
		popupX+=20.0f;
		popupY+=5.0f-numRows*height;
		popupX-=5.0f;
		popupY-=5.0f;
		if (popupX+maxLength>m_width)
			popupX=m_width-maxLength;
		if (popupY+numRows*height>m_height)
			popupY=m_height-numRows*height;
		if (popupX<0)
			popupX=0;
		if (popupY<0)
			popupY=0;
		popupX+=5.0f;
		popupY+=5.0f;
		glBegin(GL_QUADS);
		glVertex2f(popupX,m_height-popupY);
		glVertex2f(popupX+maxLength, m_height-popupY);
		glVertex2f(popupX+maxLength, m_height-(popupY+numRows*height));
		glVertex2f(popupX, m_height-(popupY+numRows*height));
		glEnd();
		popupX-=5.0f;
		popupY-=5.0f;
		glColor4f(0.9f,0.9f,0.9f,MAX(m_popupFade,0.0f));
		glBegin(GL_QUADS);
		glVertex2f(popupX,m_height-popupY);
		glVertex2f(popupX+maxLength, m_height-popupY);
		glVertex2f(popupX+maxLength, m_height-(popupY+numRows*height));
		glVertex2f(popupX, m_height-(popupY+numRows*height));
		glEnd();
		glColor4f(0.0f,0.0f,0.0f,MAX(m_popupFade,0.0f));
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBegin(GL_QUADS);
		glVertex2f(popupX,m_height-popupY);
		glVertex2f(popupX+maxLength, m_height-popupY);
		glVertex2f(popupX+maxLength, m_height-(popupY+numRows*height));
		glVertex2f(popupX, m_height-(popupY+numRows*height));
		glEnd();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glScalef(scale, scale, 1.0f);

		{
			int row = 1;
			size_t offset = 0, endOffset;

			do
			{
				endOffset = m_popupText.find('\n', offset);
				if (endOffset == m_popupText.npos)
				{
					endOffset = m_popupText.size();
				}
				
				sprintf_s(temp, sizeof(temp), "%s", m_popupText.c_str()+offset);
				temp[MIN(endOffset-offset, sizeof(temp)-1)]='\0';

				m_font.drawText(popupX/scale,(m_height-(popupY+(row++)*height))/scale, temp);
				offset = endOffset + 1;
			}
			while (endOffset != m_popupText.size());
		}

		glPopMatrix();
	}
	m_rebuildNeeded=FALSE;
	if (m_moved)
	{
		m_popupFade-=0.2f;
		m_moved=FALSE;
	}
	else
	{
		m_popupFade+=0.1f;
	}
	m_popupFade=CLAMP(m_popupFade,-1.0f,1.0f);

	if (m_collapsing)
	{
		m_collapsedAmount-=0.1f;
	}
	else if (m_growing)
	{
		m_collapsedAmount+=0.1f;
	}
	m_collapsedAmount=CLAMP(m_collapsedAmount, 0.0f, 1.0f);

	SwapBuffers(hdc);
}

void CGLTreemapWnd::SetStyle(const ITreeMapStyle* style)
{
	m_style = style;
}

void CGLTreemapWnd::Zoom(short zDelta, const CPoint& pt)
{
	OnMouseWheel(0, zDelta, pt);
}

void CGLTreemapWnd::SetActiveItem(const void* clientNode)
{
	if (m_findZoomPoint == FALSE)
	{
		ClientNodeMap::iterator it = m_clientNodeIndex.find(clientNode);
		if (it != m_clientNodeIndex.end())
		{
			m_liveNode = it->second;
			m_mouseX = -10000;
			m_mouseY = -10000;
		}
	}
}

void CGLTreemapWnd::RecurseRefreshNodeSizes(treemapItem *r)
{
	r->oldSize=GetSize(r);
	r->size=m_style->MeasureNode(r);
	for (size_t i=0; i<r->children.size(); i++)
	{
		RecurseRefreshNodeSizes(r->children[i]);
	}
}

void CGLTreemapWnd::RefreshNodeSizes(void)
{
	RecurseRefreshNodeSizes(m_treeRoot);
	m_groupFilter=0.0f;
	m_groupRatio=m_treeRoot->oldSize/m_treeRoot->size;
}

void CGLTreemapWnd::OnIdle()
{
	if (GetSafeHwnd() && IsWindowVisible())
	{
		if (m_groupFilter<1.0f)
		{
			m_groupFilter+=0.025f;
			m_rebuildNeeded=TRUE;
			if (m_groupFilter>=1.0f)
			{
				m_groupFilter=1.0f;
			}
		}

		oglDrawScene();
	}
}
























































static float GetEfficiency(const SizeInfoGroups& sz)
{
	return (sz.GetTotal().consumed == 0)
		? 100.0f
		: (static_cast<float>(sz.GetTotal().requested) / static_cast<float>(sz.GetTotal().consumed)) * 100.0f;
}

void CGLTreemapWnd::freeTree(treemapItem *item)
{
	for (size_t i=0; i<item->children.size(); i++)
	{
		freeTree(item->children[i]);
	}
	delete item;
}

void CGLTreemapWnd::GetPopupText(const treemapItem* item, std::string& popupTextOut)
{
	if (m_style)
	{
		m_style->GetPopupText(item, popupTextOut);
	}
}

void CGLTreemapWnd::RaiseActiveItemChanged()
{
	NMHDR hdr;
	hdr.code = OGC_ACTIVEITEMCHANGED;
	hdr.hwndFrom = m_hWnd;
	hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&hdr));
}

void CGLTreemapWnd::RaiseActiveItemSelected()
{
	NMHDR hdr;
	hdr.code = OGC_ACTIVEITEMSELECTED;
	hdr.hwndFrom = m_hWnd;
	hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	::SendMessage(::GetParent(m_hWnd), WM_NOTIFY, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&hdr));
}

BOOL CGLTreemapWnd::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext /* = NULL */)
{
	oglCreate(rect, pParentWnd, nID);
	return TRUE;
}

BOOL CGLTreemapWnd::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam /* = NULL */)
{
	oglCreate(rect, pParentWnd, nID);
	return TRUE;
}

void CGLTreemapWnd::oglCreate(CRect rect, CWnd *parent, UINT id)
{
	CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS, NULL, (HBRUSH)GetStockObject(BLACK_BRUSH), NULL);

	CWnd::CreateEx(0, className, "OpenGL", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER, rect, parent, id);
}

void CGLTreemapWnd::oglInitialize(void)
{
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
		32, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0,
	};
	hdc = GetDC()->m_hDC;
	int nPixelFormat = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, nPixelFormat, &pfd);
	hrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hrc);
	m_font.loadFont("C:\\WINDOWS\\Fonts\\arial.TTF", 2048);
}

void CGLTreemapWnd::SetTree(treemapItem* tree)
{
	m_clientNodeIndex.clear();
	m_treeRoot = tree;

	std::vector<treemapItem*> stck;
	stck.reserve(1024);

	stck.push_back(tree);

	while (!stck.empty())
	{
		treemapItem* item = stck.back();
		stck.pop_back();

		m_clientNodeIndex[item->clientNode] = item;

		for (std::vector<treemapItem*>::iterator it = item->children.begin(), itEnd = item->children.end(); it != itEnd; ++ it)
			stck.push_back(*it);
	}

	Invalidate(FALSE);
}
