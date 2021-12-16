/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc document graph view.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_DOCGRAPHVIEW_H__
#define __SCHEMATYC_DOCGRAPHVIEW_H__

#include "BoostHelpers.h"

#include "Schematyc_PluginUtils.h"
#include "Schematyc_GraphView.h"

// TODO : Reduce number of calls to IDocGraph::RefreshAvailableNodes?

namespace Schematyc
{
	class CDocGraphViewNode : public CGraphViewNode
	{
	public:

		CDocGraphViewNode(const SGraphViewGrid& grid, Gdiplus::PointF pos, IDoc& doc, IDocGraph& docGraph, IDocGraphNode& docGraphNode);

		// CGraphViewNode
		virtual const char* GetName() const OVERRIDE;
		virtual Gdiplus::Color GetHeaderColor() const OVERRIDE;
		virtual size_t GetInputCount() const OVERRIDE;
		virtual size_t FindInput(const char* name) const OVERRIDE;
		virtual const char* GetInputName(size_t iInput) const OVERRIDE;
		virtual DocGraphNodePortFlags::EValue GetInputFlags(size_t iInput) const OVERRIDE;
		virtual Gdiplus::Color GetInputColor(size_t iInput) const OVERRIDE;
		virtual size_t GetOutputCount() const OVERRIDE;
		virtual size_t FindOutput(const char* name) const OVERRIDE;
		virtual const char* GetOutputName(size_t iOutput) const OVERRIDE;
		virtual DocGraphNodePortFlags::EValue GetOutputFlags(size_t iOutput) const OVERRIDE;
		virtual Gdiplus::Color GetOutputColor(size_t iInput) const OVERRIDE;
		// ~CGraphViewNode

		IDocGraphNode& GetDocGraphNode();
		DocGraphNodeType::EValue GetType() const;
		SGUID GetGUID() const;
		CTypeId GetInputTypeId(size_t iInput) const;
		CTypeId GetOutputTypeId(size_t iOutput) const;
		void Serialize(Serialization::IArchive& archive);

	protected:

		virtual void OnMove(Gdiplus::RectF paintRect);

	private:

		IDoc&						m_doc;
		IDocGraph&			m_docGraph;
		IDocGraphNode&	m_docGraphNode;
	};

	DECLARE_BOOST_POINTERS(CDocGraphViewNode)

	class CDocGraphView : public CGraphView
	{
		DECLARE_MESSAGE_MAP()

	public:
		
		typedef TemplateUtils::CSignal<void (const CDocGraphViewNodePtr&)>	TNodeSelectionSignal;
		typedef TemplateUtils::CSignal<void (SGraphViewLink*)>							TLinkSelectionSignal;
		typedef TemplateUtils::CSignal<void (IDoc&)>												TDocModifiedSignal;

		CDocGraphView();

		// CGraphView
		virtual void Refresh() OVERRIDE;
		// ~CGraphView

		void Load(IDoc* pDoc, IDocGraph* pDocGraph);
		IDoc* GetDoc() const;
		IDocGraph* GetDocGraph() const;
		TNodeSelectionSignal& GetNodeSelectionSignal();
		TLinkSelectionSignal& GetLinkSelectionSignal();
		TDocModifiedSignal& GetDocModifiedSignal();

	protected:

		// CGraphView
		virtual void OnNodeSelection(const CGraphViewNodePtr& pSelectedNode) OVERRIDE;
		virtual void OnLinkSelection(SGraphViewLink* pSelectedLink) OVERRIDE;
		virtual void OnLinkModify(const SGraphViewLink& link) OVERRIDE;
		virtual void OnNodeRemoved(const CGraphViewNode& node);
		virtual bool CanCreateLink(const CGraphViewNode& srcNode, const char* srcOutputName, const CGraphViewNode& dstNode, const char* dstInputName) const OVERRIDE;
		virtual void OnLinkCreated(const SGraphViewLink& link) OVERRIDE;
		virtual void OnLinkRemoved(const SGraphViewLink& link) OVERRIDE;
		virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) OVERRIDE;
		virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) OVERRIDE;
		void GetPopupMenuItems(CMenu& popupMenu, const CGraphViewNodePtr& pNode, size_t iNodeInput, size_t iNodeOutput, CPoint point) OVERRIDE;
		virtual void OnPopupMenuResult(BOOL popupMenuItem, const CGraphViewNodePtr& pNode, size_t iNodeInput, size_t iNodeOutput, CPoint point) OVERRIDE;
		virtual const IQuickSearchOptions* GetQuickSearchOptions(CPoint point, const CGraphViewNodePtr& pNode, size_t iNodeOutput) OVERRIDE;
		virtual void OnQuickSearchResult(CPoint point, const CGraphViewNodePtr& pNode, size_t iNodeOutput, size_t iSelectedOption) OVERRIDE;
		// ~CGraphView

	private:

		struct EPopupMenuItemEx
		{
			enum
			{
				ADD_OUTPUT = EPopupMenuItem::CUSTOM,
				REMOVE_OUTPUT
			};
		};

		class CQuickSearchOptions : public IQuickSearchOptions
		{
		public:

			// IQuickSearchOptions
			virtual size_t GetCount() const;
			virtual const char* GetName(size_t iOption) const;
			// IQuickSearchOptions

			void Refresh(IDocGraph& docGraph, const CTypeId& inputTypeId);
			DocGraphNodeType::EValue GetNodeType(size_t iOption) const;
			SGUID GetNodeRefGUID(size_t iOption) const;

		private:

			struct SOption
			{
				SOption(const char* _name, DocGraphNodeType::EValue _nodeType, const SGUID& _nodeRefGuid);

				string										name;
				DocGraphNodeType::EValue	nodeType;
				SGUID											nodeRefGuid;
			};

			typedef std::vector<SOption> TOptionVector;

			TOptionVector	m_options;
		};

		struct SQuickSearchOption
		{
			SQuickSearchOption(const char* _name, DocGraphNodeType::EValue _nodeType, const SGUID& _nodeRefGuid);

			string										name;
			DocGraphNodeType::EValue	nodeType;
			SGUID											nodeRefGuid;
		};

		typedef std::vector<SQuickSearchOption> TQuickSearchOptionVector;

		VisitStatus::EValue VisitGraphNode(const IDocGraphNodePtr& pGraphNode);
		
		bool CanAddNode(DocGraphNodeType::EValue type) const;
		bool CanAddNode(const SGUID& refGUID) const;
		bool CanAddNode(DocGraphNodeType::EValue type, const SGUID& refGUID) const;
		CDocGraphViewNodePtr AddNode(DocGraphNodeType::EValue type, const SGUID& refGUID, Gdiplus::PointF pos);
		CDocGraphViewNodePtr AddNode(IDocGraphNode& docGraphNode);
		CDocGraphViewNodePtr GetNode(const SGUID& guid) const;
		bool AddLink(const IDocGraphLink& docGraphLink);

		void InvalidateDoc();
		void UnrollSequence();
		void UpdateSimulation(bool firstUpdate);

		IDoc*								m_pDoc;
		IDocGraph*					m_pDocGraph;
		bool								m_simulating;
		int64								m_simulationTimeMs;
		size_t							m_sequencePos;
		TGUIDVector					m_sequenceNodes;
		CQuickSearchOptions	m_quickSearchOptions;

		struct
		{
			TNodeSelectionSignal	nodeSelection;
			TLinkSelectionSignal	linkSelection;
			TDocModifiedSignal		docModified;
		} m_signals;
	};
}

#endif //__SCHEMATYC_DOCGRAPHVIEW_H__
