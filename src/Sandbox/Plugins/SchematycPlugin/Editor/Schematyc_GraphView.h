/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc graph view.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_GRAPHVIEW_H__
#define __SCHEMATYC_GRAPHVIEW_H__

#include "BoostHelpers.h"

#include <Controls/PropertyCtrl.h>

#include <Schematyc/Schematyc_IDoc.h>	// TODO : Can we remove this include? Right now it's only needed for access to SGrapNodePortFlags.

#include "Schematyc_QuickSearchDlg.h"

// TODO : Move CDefaultGraphViewPainter (along with port colors from function and graph views) to a separate file.

namespace Schematyc
{
	class CGraphView;

	typedef std::vector<size_t> TSizeTVector;

	struct SGraphViewGrid
	{
		inline SGraphViewGrid(Gdiplus::PointF _spacing, Gdiplus::RectF _bounds)
			: spacing(_spacing)
			, bounds(_bounds)
		{}

		inline float Snap(float value, float spacing) const
		{
			const float	mod = fmodf(value, spacing);
			if(mod > (spacing * 0.5f))
			{
				return value + (spacing - mod);
			}
			else if(mod < (spacing * -0.5f))
			{
				return value - (spacing + mod);
			}
			else
			{
				return value - mod;
			}
		}

		inline Gdiplus::PointF SnapPos(Gdiplus::PointF pos) const
		{
			pos.X	= Snap(pos.X, spacing.X);
			pos.Y	= Snap(pos.Y, spacing.Y);
			return pos;
		}

		inline Gdiplus::SizeF SnapSize(Gdiplus::SizeF size) const
		{
			size.Width	= size.Width + (spacing.X - fmodf(size.Width, spacing.X));
			size.Height	= size.Height + (spacing.X - fmodf(size.Height, spacing.X));
			return size;
		}

		inline Gdiplus::RectF SnapRect(Gdiplus::RectF rect) const
		{
			Gdiplus::PointF	pos;
			rect.GetLocation(&pos);
			pos = SnapPos(pos);

			Gdiplus::SizeF	size;
			rect.GetSize(&size);
			size = SnapSize(size);

			return Gdiplus::RectF(pos, size);
		}

		Gdiplus::PointF	spacing;
		Gdiplus::RectF	bounds;
	};

	class CGraphViewNode
	{
	public:

		CGraphViewNode(const SGraphViewGrid& grid, Gdiplus::PointF pos);
		virtual ~CGraphViewNode();

		const SGraphViewGrid&	GetGrid() const;
		void SetPos(Gdiplus::PointF pos, bool snapToGrid);
		Gdiplus::PointF GetPos() const;
		void SetPaintRect(Gdiplus::RectF paintRect);
		Gdiplus::RectF GetPaintRect() const;
		void Enable(bool enable);
		bool IsEnabled() const;
		void Select(bool select);
		bool IsSelected() const;
		size_t FindInput(Gdiplus::PointF pos) const;
		void SetInputPaintRect(size_t iInput, Gdiplus::RectF paintRect);
		Gdiplus::RectF GetInputPaintRect(size_t iInput) const;
		size_t FindOutput(Gdiplus::PointF pos) const;
		void SetOutputPaintRect(size_t iOutput, Gdiplus::RectF paintRect);
		Gdiplus::RectF GetOutputPaintRect(size_t iOutput) const;

		virtual const char* GetName() const;
		virtual Gdiplus::Color GetHeaderColor() const;
		virtual size_t GetInputCount() const;
		virtual size_t FindInput(const char* name) const;
		virtual const char* GetInputName(size_t iInput) const;
		virtual DocGraphNodePortFlags::EValue GetInputFlags(size_t iInput) const;
		virtual Gdiplus::Color GetInputColor(size_t iInput) const;
		virtual size_t GetOutputCount() const;
		virtual size_t FindOutput(const char* name) const;
		virtual const char* GetOutputName(size_t iOutput) const;
		virtual DocGraphNodePortFlags::EValue GetOutputFlags(size_t iOutput) const;
		virtual Gdiplus::Color GetOutputColor(size_t iInput) const;

	protected:

		virtual void OnMove(Gdiplus::RectF paintRect);

	private:

		typedef std::vector<Gdiplus::RectF> TRectVector;

		const SGraphViewGrid&	m_grid;
		bool									m_enabled;
		bool									m_selected;
		Gdiplus::RectF				m_paintRect;
		TRectVector						m_inputPaintRects;
		TRectVector						m_outputPaintRects;
	};

	DECLARE_BOOST_POINTERS(CGraphViewNode)

	typedef std::vector<CGraphViewNodePtr> TGraphViewNodePtrVector;

	// TODO : Convert SGraphViewLink to a class?
	struct SGraphViewLink
	{
		SGraphViewLink(CGraphView* _pGraphView, const CGraphViewNodeWeakPtr& _pSrcNode, const char* _srcOutputName, const CGraphViewNodeWeakPtr& _pDstNode, const char* _dstInputName);

		CGraphView*						pGraphView;
		CGraphViewNodeWeakPtr	pSrcNode;
		string								srcOutputName;
		CGraphViewNodeWeakPtr	pDstNode;
		string								dstInputName;
		Gdiplus::RectF				helperRect;
		bool									selected;
	};

	typedef std::vector<SGraphViewLink> TGraphViewLinkVector;

	struct IGraphViewPainter
	{
		virtual ~IGraphViewPainter() {}

		virtual Gdiplus::Color GetBackgroundColor() const = 0;
		virtual void PaintGrid(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid) const = 0;
		virtual void UpdateNodePaintRect(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const = 0;
		virtual void PaintNode(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const = 0;
		virtual void PaintLink(Gdiplus::Graphics& graphics, const CGraphViewNode& startNode, Gdiplus::RectF startRect, Gdiplus::PointF endPos, Gdiplus::Color color, bool highlight) const = 0;
		virtual Gdiplus::RectF PaintLink(Gdiplus::Graphics& graphics, const CGraphViewNode& startNode, Gdiplus::RectF startRect, const CGraphViewNode& endNode, Gdiplus::RectF endRect, Gdiplus::Color color, bool highlight) const = 0;
		virtual void PaintSelectionRect(Gdiplus::Graphics& graphics, Gdiplus::RectF rect) const = 0;
	};

	DECLARE_BOOST_POINTERS(IGraphViewPainter)

	struct IGraphViewDragState
	{
		virtual ~IGraphViewDragState() {}

		virtual void Update(CGraphView& graphView, Gdiplus::PointF pos) = 0;
		virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter) = 0;
		virtual void Exit(CGraphView& graphView) = 0;
	};

	DECLARE_BOOST_POINTERS(IGraphViewDragState)

	class CDefaultGraphViewPainter : public IGraphViewPainter
	{
	public:

		// IGraphViewPainter
		virtual Gdiplus::Color GetBackgroundColor() const OVERRIDE;
		virtual void PaintGrid(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid) const OVERRIDE;
		virtual void UpdateNodePaintRect(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const OVERRIDE;
		virtual void PaintNode(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const OVERRIDE;
		virtual void PaintLink(Gdiplus::Graphics& graphics, const CGraphViewNode& startNode, Gdiplus::RectF start, Gdiplus::PointF end, Gdiplus::Color color, bool highlight) const OVERRIDE;
		virtual Gdiplus::RectF PaintLink(Gdiplus::Graphics& graphics, const CGraphViewNode& startNode, Gdiplus::RectF start, const CGraphViewNode& endNode, Gdiplus::RectF end, Gdiplus::Color color, bool highlight) const OVERRIDE;
		virtual void PaintSelectionRect(Gdiplus::Graphics& graphics, Gdiplus::RectF rect) const OVERRIDE;
		// ~IGraphViewPainter

	private:

		Gdiplus::PointF CalculateNodeHeaderSize(Gdiplus::Graphics& graphics, const CGraphViewNode& node) const;
		Gdiplus::PointF CalculateNodeInputSize(Gdiplus::Graphics& graphics, const char* name) const;
		Gdiplus::PointF CalculateNodeOutputSize(Gdiplus::Graphics& graphics, const char* name) const;
		void PaintNodeBody(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const;
		void PaintNodeHeader(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const;
		void PaintNodeOutline(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const;
		void UpdateNodeInputPaintRect(Gdiplus::Graphics& graphics, CGraphViewNode& node, size_t iInput, Gdiplus::PointF pos, float maxWidth) const;
		void PaintNodeInput(Gdiplus::Graphics& graphics, CGraphViewNode& node, size_t iInput) const;
		void PaintNodeInputIcon(Gdiplus::Graphics& graphics, Gdiplus::PointF pos, Gdiplus::Color color, bool active) const;
		void UpdateNodeOutputPaintRect(Gdiplus::Graphics& graphics, CGraphViewNode& node, size_t iOutput, Gdiplus::PointF pos, float maxWidth) const;
		void PaintNodeOutput(Gdiplus::Graphics& graphics, CGraphViewNode& node, size_t iOutput) const;
		void PaintNodeOutputIcon(Gdiplus::Graphics& graphics, Gdiplus::PointF pos, Gdiplus::Color color, bool active) const;

		static const Gdiplus::Color		BACKGROUND_COLOR;
		static const Gdiplus::Color		GRID_COLOR;
		static const float						GRID_WIDTH;
		static const float						NODE_BEVEL;
		static const BYTE							NODE_HEADER_ALPHA;
		static const float						NODE_HEADER_TEXT_BORDER_X;
		static const float						NODE_HEADER_TEXT_BORDER_Y;
		static const Gdiplus::Color		NODE_HEADER_TEXT_COLOR;
		static const Gdiplus::Color		NODE_BODY_FILL_COLOR;
		static const Gdiplus::Color		NODE_BODY_FILL_COLOR_DISABLED;
		static const Gdiplus::Color		NODE_BODY_OUTLINE_COLOR;
		static const Gdiplus::Color		NODE_BODY_OUTLINE_COLOR_HIGHLIGHT;
		static const float						NODE_BODY_OUTLINE_WIDTH;
		static const float						NODE_INPUT_OUPUT_HORZ_SPACING;
		static const float						NODE_INPUT_OUPUT_VERT_SPACING;
		static const float						NODE_INPUT_ICON_BORDER;
		static const float						NODE_INPUT_ICON_WIDTH;
		static const float						NODE_INPUT_ICON_HEIGHT;
		static const Gdiplus::Color		NODE_INPUT_ICON_OUTLINE_COLOR;
		static const float						NODE_INPUT_ICON_OUTLINE_WIDTH;
		static const float						NODE_INPUT_NAME_BORDER;
		static const float						NODE_INPUT_NAME_WIDTH_MAX;
		static const Gdiplus::Color		NODE_INPUT_NAME_COLOR;
		static const float						NODE_OUTPUT_ICON_BORDER;
		static const float						NODE_OUTPUT_ICON_WIDTH;
		static const float						NODE_OUTPUT_ICON_HEIGHT;
		static const Gdiplus::Color		NODE_OUTPUT_ICON_OUTLINE_COLOR;
		static const float						NODE_OUTPUT_ICON_OUTLINE_WIDTH;
		static const float						NODE_OUTPUT_NAME_BORDER;
		static const float						NODE_OUTPUT_NAME_WIDTH_MAX;
		static const Gdiplus::Color		NODE_OUTPUT_NAME_COLOR;
		static const float						LINK_WIDTH;
		static const float						LINK_CURVE_OFFSET;
		static const float						LINK_DODGE_X;
		static const float						LINK_DODGE_Y;
		static const float						LINK_HELPER_SIZE;
		static const float						ALPHA_HIGHLIGHT_MIN;
		static const float						ALPHA_HIGHLIGHT_MAX;
		static const float						ALPHA_HIGHLIGHT_SPEED;
		static const Gdiplus::Color		SELECTION_FILL_COLOR;
		static const Gdiplus::Color		SELECTION_OUTLINE_COLOR;
	};

	class CGraphView : public CView
	{
		friend struct SGraphViewLink;

		DECLARE_MESSAGE_MAP()

	public:

		virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
		virtual void Refresh();

		void DoQuickSearch(const CGraphViewNodePtr& pNode = CGraphViewNodePtr(), size_t iNodeOutput = INVALID_INDEX, const CPoint* pPoint = NULL);

	protected:

		struct EPopupMenuItem
		{
			enum
			{
				QUICK_SEARCH = 1,
				REMOVE_NODE,
				CUSTOM
			};
		};

		CGraphView(const SGraphViewGrid& grid, const IGraphViewPainterPtr& pPainter = IGraphViewPainterPtr());

		const SGraphViewGrid& GetGrid() const;
		const IGraphViewPainterPtr& GetPainter() const;
		void Enable(bool enable);
		bool IsEnabled() const;
		void ClearSelection();
		void AddNode(const CGraphViewNodePtr& pNode, bool scrollToFit);
		void RemoveNode(CGraphViewNodePtr pNode);
		CGraphViewNodePtr FindNode(Gdiplus::PointF pos);
		TGraphViewNodePtrVector& GetNodes();
		const TGraphViewNodePtrVector& GetNodes() const;
		void ClearNodeSelection();
		void SelectNode(const CGraphViewNodePtr& pNode);
		void SelectNodesInRect(const Gdiplus::RectF& rect);
		const TGraphViewNodePtrVector& GetSelectedNodes() const;
		void MoveSelectedNodes(Gdiplus::PointF clientTransform, bool snapToGrid);
		bool CreateLink(const CGraphViewNodePtr& pSrcNode, const char* srcOutputName, const CGraphViewNodePtr& pDstNode, const char* dstInputName);
		void AddLink(const CGraphViewNodePtr& pSrcNode, const char* srcOutputName, const CGraphViewNodePtr& pDstNode, const char* dstInputName);
		void RemoveLink(size_t iLink);
		void RemoveLinksConnectedToNode(const CGraphViewNodePtr& pNode);
		void RemoveLinksConnectedToNodeInput(const CGraphViewNodePtr& pNode, const char* inputName);
		void RemoveLinksConnectedToNodeOutput(const CGraphViewNodePtr& pNode, const char* outputName);
		void FindLinks(const CGraphViewNodePtr& pDstNode, const char* inputName, TSizeTVector& output) const;
		size_t FindLink(Gdiplus::PointF pos);
		TGraphViewLinkVector& GetLinks();
		const TGraphViewLinkVector& GetLinks() const;
		void ClearLinkSelection();
		void SelectLink(size_t iLink);
		size_t GetSelectedLink() const;
		void Clear();

		Gdiplus::PointF ClientToGraph(Gdiplus::PointF pos) const;
		Gdiplus::PointF ClientToGraphTransform(Gdiplus::PointF transform) const;
		Gdiplus::RectF ClientToGraph(Gdiplus::RectF rect) const;
		Gdiplus::PointF GraphToClient(Gdiplus::PointF pos) const;
		Gdiplus::PointF GraphToClientTransform(Gdiplus::PointF transform) const;
		Gdiplus::RectF GraphToClient(Gdiplus::RectF rect) const;
		void ScrollToFit(Gdiplus::RectF rect);

		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual void OnDraw(CDC* pDC);

		afx_msg void OnKillFocus(CWnd* pNewWnd);
		afx_msg void OnPaint();
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint point);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

		virtual void OnNodeSelection(const CGraphViewNodePtr& pSelectedNode);
		virtual void OnLinkSelection(SGraphViewLink* pSelectedLink);
		virtual void OnLinkModify(const SGraphViewLink& link);
		virtual void OnNodeRemoved(const CGraphViewNode& node);
		virtual bool CanCreateLink(const CGraphViewNode& srcNode, const char* srcOutputName, const CGraphViewNode& dstNode, const char* dstInputName) const;
		virtual void OnLinkCreated(const SGraphViewLink& link);
		virtual void OnLinkRemoved(const SGraphViewLink& link);
		virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
		virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
		virtual void GetPopupMenuItems(CMenu& popupMenu, const CGraphViewNodePtr& pNode, size_t iNodeInput, size_t iNodeOutput, CPoint point);
		virtual void OnPopupMenuResult(BOOL popupMenuItem, const CGraphViewNodePtr& pNode, size_t iNodeInput, size_t iNodeOutput, CPoint point);
		virtual const IQuickSearchOptions* GetQuickSearchOptions(CPoint point, const CGraphViewNodePtr& pNode, size_t iNodeOutput);
		virtual void OnQuickSearchResult(CPoint point, const CGraphViewNodePtr& pNode, size_t iNodeOutput, size_t iSelectedOption);
		virtual bool ShouldUpdate() const;
		virtual void OnUpdate();

	private:

		class CScrollDragState : public IGraphViewDragState
		{
		public:

			CScrollDragState(Gdiplus::PointF pos);

			// IDragState
			virtual void Update(CGraphView& graphView, Gdiplus::PointF pos);
			virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter);
			virtual void Exit(CGraphView& graphView);
			// ~IDragState

		private:

			Gdiplus::PointF	m_prevPos;
		};

		class CMoveDragState : public IGraphViewDragState
		{
		public:

			CMoveDragState(Gdiplus::PointF pos);

			// IDragState
			virtual void Update(CGraphView& graphView, Gdiplus::PointF pos);
			virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter);
			virtual void Exit(CGraphView& graphView);
			// ~IDragState

		private:

			Gdiplus::PointF	m_prevPos;
		};

		class CLinkDragState : public IGraphViewDragState
		{
		public:

			CLinkDragState(const CGraphViewNodeWeakPtr& pSrcNode, const char* srcOutputName, Gdiplus::RectF start, Gdiplus::PointF end, Gdiplus::Color color, bool newLink);

			// IDragState
			virtual void Update(CGraphView& graphView, Gdiplus::PointF pos);
			virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter);
			virtual void Exit(CGraphView& graphView);
			// ~IDragState

		private:

			CGraphViewNodeWeakPtr	m_pSrcNode;
			string								m_srcOutputName;
			Gdiplus::RectF				m_start;
			Gdiplus::PointF				m_end;
			Gdiplus::Color				m_color;
			bool									m_newLink;
		};

		class CSelectDragState : public IGraphViewDragState
		{
		public:

			CSelectDragState(Gdiplus::PointF pos);

			// IDragState
			virtual void Update(CGraphView& graphView, Gdiplus::PointF pos);
			virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter);
			virtual void Exit(CGraphView& graphView);
			// ~IDragState

		private:

			Gdiplus::RectF GetSelectionRect() const;

			Gdiplus::PointF	m_startPos;
			Gdiplus::PointF	m_endPos;
		};

		class CDropTarget : public COleDropTarget
		{
		public:

			CDropTarget(CGraphView& graphView);

			// COleDropTarget
			virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
			virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
			// ~COleDropTarget

		private:

			CGraphView&	m_graphView;
		};

		afx_msg void OnTimer(UINT_PTR nIDEvent);

		void OnLeftMouseButtonDrag(CPoint point);
		void OnRightMouseButtonDrag(CPoint point);
		void OnMiddleMouseButtonDrag(CPoint point);
		void Scroll(Gdiplus::PointF delta);
		void ExitDragState();
		void OnDelete();

		SGraphViewGrid					m_grid;
		IGraphViewPainterPtr		m_pPainter;
		bool										m_enabled;
		TGraphViewNodePtrVector	m_nodes;
		TGraphViewLinkVector		m_links;
		Gdiplus::PointF					m_scrollOffset;
		CPoint									m_prevMousePoint;
		float										m_zoom;
		IGraphViewDragStatePtr	m_pDragState;
		TGraphViewNodePtrVector	m_selectedNodes;
		size_t									m_iSelectedLink;
		CDropTarget							m_dropTarget;

		static const float	MIN_ZOOM;
		static const float	MAX_ZOOM;
		static const float	DELTA_ZOOM;
		static const float	DEFAULT_NODE_SPACING;
	};
}

#endif //__SCHEMATYC_GRAPHVIEW_H__
