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

#include "StdAfx.h"

#include "Schematyc_DocGraphView.h"

#include <ITimer.h>

#include <Schematyc/Schematyc_DocUtils.h>
#include <Schematyc/Schematyc_IEnvRegistry.h>
#include <Schematyc/Schematyc_TypeId.h>

#include "Schematyc_BrowserIcons.h"
#include "Schematyc_AddGraphNodeOutputDlg.h"

namespace Schematyc
{
	namespace
	{
		static const struct { DocGraphNodeType::EValue docGraphNodeType; Gdiplus::Color color; } HEADER_COLORS[] =
		{
			{	DocGraphNodeType::BEGIN,									Gdiplus::Color(38, 184, 33)		},
			{ DocGraphNodeType::BEGIN_CONSTRUCTOR,			Gdiplus::Color(38, 184, 33)		},
			{ DocGraphNodeType::BEGIN_SIGNAL_RECEIVER,	Gdiplus::Color(38, 184, 33)		},
			{ DocGraphNodeType::RETURN,									Gdiplus::Color(38, 184, 33)		},
			{ DocGraphNodeType::BRANCH,									Gdiplus::Color(38, 184, 33)		},
			{ DocGraphNodeType::SET_VARIABLE,						Gdiplus::Color(0, 108, 217)		},
			{ DocGraphNodeType::GET_VARIABLE,						Gdiplus::Color(0, 108, 217)		},
			{ DocGraphNodeType::SEND_SIGNAL,						Gdiplus::Color(38, 184, 33)		},
			{ DocGraphNodeType::CALL_FUNCTION,					Gdiplus::Color(215, 55, 55)		},
			{ DocGraphNodeType::CALL_CONDITION,					Gdiplus::Color(250, 232, 12)	},
			{ DocGraphNodeType::START_TIMER,						Gdiplus::Color(38, 184, 33)		},
			{ DocGraphNodeType::STOP_TIMER,							Gdiplus::Color(38, 184, 33)		},
			{ DocGraphNodeType::STATE,									Gdiplus::Color(0, 108, 217)		}
		};

		static const Gdiplus::Color DEFAULT_HEADER_COLOR = Gdiplus::Color(38, 184, 33);

		inline Gdiplus::Color GetHeaderColor(DocGraphNodeType::EValue docGraphNodeType)
		{
			for(size_t iHeaderColor = 0; iHeaderColor < ARRAY_LENGTH(HEADER_COLORS); ++ iHeaderColor)
			{
				if(HEADER_COLORS[iHeaderColor].docGraphNodeType == docGraphNodeType)
				{
					return HEADER_COLORS[iHeaderColor].color;
				}
			}
			return DEFAULT_HEADER_COLOR;
		}

		static const struct { CTypeId typeId; Gdiplus::Color color; } PORT_COLORS[] =
		{
			{ GetTypeId<bool>(),				Gdiplus::Color(0, 108, 217)		},
			{ GetTypeId<int32>(),				Gdiplus::Color(215, 55, 55)		},
			{ GetTypeId<uint32>(),			Gdiplus::Color(215, 55, 55)		},
			{ GetTypeId<float>(),				Gdiplus::Color(185, 185, 185)	},
			{ GetTypeId<Vec3>(),				Gdiplus::Color(250, 232, 12)	},
			{ GetTypeId<SGUID>(),				Gdiplus::Color(38, 184, 33)		},
			{ GetTypeId<CPoolString>(),	Gdiplus::Color(128, 100, 162)	},
			{ GetTypeId<TEntityId>(),		Gdiplus::Color(215, 55, 55)		}
		};

		static const Gdiplus::Color DEFAULT_PORT_COLOR = Gdiplus::Color(28, 212, 22);

		inline Gdiplus::Color GetPortColor(const CTypeId& typeId)
		{
			for(size_t iPortColor = 0; iPortColor < ARRAY_LENGTH(PORT_COLORS); ++ iPortColor)
			{
				if(PORT_COLORS[iPortColor].typeId == typeId)
				{
					return PORT_COLORS[iPortColor].color;
				}
			}
			return DEFAULT_PORT_COLOR;
		}
	}

	static const float DRAG_AND_DROP_OFFSET					= 10.0f;
	static const int64 SIMULATION_STEP_FREQUENCY_MS	= 650;

	//////////////////////////////////////////////////////////////////////////
	CDocGraphViewNode::CDocGraphViewNode(const SGraphViewGrid& grid, Gdiplus::PointF pos, IDoc& doc, IDocGraph& docGraph, IDocGraphNode& docGraphNode)
		: CGraphViewNode(grid, pos)
		, m_doc(doc)
		, m_docGraph(docGraph)
		, m_docGraphNode(docGraphNode)
	{}

	//////////////////////////////////////////////////////////////////////////
	const char* CDocGraphViewNode::GetName() const
	{
		return m_docGraphNode.GetName();
	}

	//////////////////////////////////////////////////////////////////////////
	Gdiplus::Color CDocGraphViewNode::GetHeaderColor() const
	{
		return Schematyc::GetHeaderColor(m_docGraphNode.GetType());
	}

	//////////////////////////////////////////////////////////////////////////
	size_t CDocGraphViewNode::GetInputCount() const
	{
		return m_docGraphNode.GetInputCount();
	}

	//////////////////////////////////////////////////////////////////////////
	size_t CDocGraphViewNode::FindInput(const char* name) const
	{
		return DocUtils::FindGraphNodeInput(m_docGraphNode, name);
	}

	//////////////////////////////////////////////////////////////////////////
	const char* CDocGraphViewNode::GetInputName(size_t iInput) const
	{
		return m_docGraphNode.GetInputName(iInput);
	}

	//////////////////////////////////////////////////////////////////////////
	DocGraphNodePortFlags::EValue CDocGraphViewNode::GetInputFlags(size_t iInput) const
	{
		return m_docGraphNode.GetInputFlags(iInput);
	}

	//////////////////////////////////////////////////////////////////////////
	Gdiplus::Color CDocGraphViewNode::GetInputColor(size_t iInput) const
	{
		return Schematyc::GetPortColor(GetInputTypeId(iInput));
	}

	//////////////////////////////////////////////////////////////////////////
	size_t CDocGraphViewNode::GetOutputCount() const
	{
		return m_docGraphNode.GetOutputCount();
	}

	//////////////////////////////////////////////////////////////////////////
	size_t CDocGraphViewNode::FindOutput(const char* name) const
	{
		return DocUtils::FindGraphNodeOutput(m_docGraphNode, name);
	}

	//////////////////////////////////////////////////////////////////////////
	const char* CDocGraphViewNode::GetOutputName(size_t iOutput) const
	{
		return m_docGraphNode.GetOutputName(iOutput);
	}

	//////////////////////////////////////////////////////////////////////////
	DocGraphNodePortFlags::EValue CDocGraphViewNode::GetOutputFlags(size_t iOutput) const
	{
		return m_docGraphNode.GetOutputFlags(iOutput);
	}

	//////////////////////////////////////////////////////////////////////////
	Gdiplus::Color CDocGraphViewNode::GetOutputColor(size_t iOutput) const
	{
		return Schematyc::GetPortColor(GetOutputTypeId(iOutput));
	}
	
	//////////////////////////////////////////////////////////////////////////
	IDocGraphNode& CDocGraphViewNode::GetDocGraphNode()
	{
		return m_docGraphNode;
	}

	//////////////////////////////////////////////////////////////////////////
	DocGraphNodeType::EValue CDocGraphViewNode::GetType() const
	{
		return m_docGraphNode.GetType();
	}

	//////////////////////////////////////////////////////////////////////////
	SGUID CDocGraphViewNode::GetGUID() const
	{
		return m_docGraphNode.GetGUID();
	}

	//////////////////////////////////////////////////////////////////////////
	CTypeId CDocGraphViewNode::GetInputTypeId(size_t iInput) const
	{
		return m_docGraphNode.GetInputTypeId(iInput);
	}

	//////////////////////////////////////////////////////////////////////////
	CTypeId CDocGraphViewNode::GetOutputTypeId(size_t iOutput) const
	{
		return m_docGraphNode.GetOutputTypeId(iOutput);
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphViewNode::Serialize(Serialization::IArchive& archive)
	{
		m_docGraphNode.Serialize(archive);
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphViewNode::OnMove(Gdiplus::RectF paintRect)
	{
		m_docGraphNode.SetPos(Vec2(paintRect.X, paintRect.Y));
	}

	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CDocGraphView, CGraphView)
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	CDocGraphView::CDocGraphView()
		: CGraphView(SGraphViewGrid(Gdiplus::PointF(10.0f, 10.0f), Gdiplus::RectF(-5000.0f, -5000.0f, 10000.0f, 10000.0f)))
		, m_pDoc(NULL)
		, m_pDocGraph(NULL)
		, m_sequencePos(INVALID_INDEX)
	{
		CGraphView::Enable(false);
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::Refresh()
	{
		if(m_pDocGraph != NULL)
		{
			m_pDocGraph->Refresh(DocRefreshReason::DEPEDENCY_MODIFIED);	// TODO : Is really the right reason?
		}
		CGraphView::Refresh();
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::Load(IDoc* pDoc, IDocGraph* pDocGraph)
	{
		if(m_pDocGraph != NULL)
		{
			CGraphView::Clear();
		}
		m_pDoc			= pDoc;
		m_pDocGraph = pDocGraph;
		if((m_pDoc != NULL) && (m_pDocGraph != NULL))
		{
			CGraphView::Enable(true);
			m_pDocGraph->VisitNodes(MAKE_MEMBER_DELEGATE(CDocGraphView::VisitGraphNode, *this));
			for(size_t iLink = 0, linkCount = m_pDocGraph->GetLinkCount(); iLink < linkCount; )
			{
				if(AddLink(*m_pDocGraph->GetLink(iLink)))
				{
					++ iLink;
				}
				else
				{
					// TODO : May make more sense to strip out broken links when loading the doc.
					SCHEMATYC_WARNING("Removing broken link from graph : %s::%s", pDoc->GetFileName(), pDocGraph->GetName());
					m_pDocGraph->RemoveLink(iLink);
					-- linkCount;
				}
			}
		}
		else
		{
			CGraphView::Enable(false);
		}
		UnrollSequence();
		__super::Invalidate(TRUE);
	}

	//////////////////////////////////////////////////////////////////////////
	IDoc* CDocGraphView::GetDoc() const
	{
		return m_pDoc;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocGraph* CDocGraphView::GetDocGraph() const
	{
		return m_pDocGraph;
	}

	//////////////////////////////////////////////////////////////////////////
	CDocGraphView::TNodeSelectionSignal& CDocGraphView::GetNodeSelectionSignal()
	{
		return m_signals.nodeSelection;
	}

	//////////////////////////////////////////////////////////////////////////
	CDocGraphView::TLinkSelectionSignal& CDocGraphView::GetLinkSelectionSignal()
	{
		return m_signals.linkSelection;
	}

	//////////////////////////////////////////////////////////////////////////
	CDocGraphView::TDocModifiedSignal& CDocGraphView::GetDocModifiedSignal()
	{
		return m_signals.docModified;
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::OnNodeSelection(const CGraphViewNodePtr& pSelectedNode)
	{
		m_signals.nodeSelection.Send(boost::static_pointer_cast<CDocGraphViewNode>(pSelectedNode));
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::OnLinkSelection(SGraphViewLink* pSelectedLink)
	{
		m_signals.linkSelection.Send(pSelectedLink);
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::OnLinkModify(const SGraphViewLink& link) {}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::OnNodeRemoved(const CGraphViewNode& node)
	{
		CRY_ASSERT((m_pDoc != NULL) && (m_pDocGraph != NULL));
		if((m_pDoc != NULL) && (m_pDocGraph != NULL))
		{
			m_pDocGraph->RemoveNode(static_cast<const CDocGraphViewNode&>(node).GetGUID());
			InvalidateDoc();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool CDocGraphView::CanCreateLink(const CGraphViewNode& srcNode, const char* srcOutputName, const CGraphViewNode& dstNode, const char* dstInputName) const
	{
		CRY_ASSERT((srcOutputName != NULL) && (dstInputName != NULL) && (m_pDocGraph != NULL));
		if((srcOutputName != NULL) && (dstInputName != NULL) && (m_pDocGraph != NULL))
		{
			const CDocGraphViewNode&	srcNodeImpl = static_cast<const CDocGraphViewNode&>(srcNode);
			const CDocGraphViewNode&	dstNodeImpl = static_cast<const CDocGraphViewNode&>(dstNode);
			return m_pDocGraph->CanAddLink(srcNodeImpl.GetGUID(), srcOutputName, dstNodeImpl.GetGUID(), dstInputName);
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::OnLinkCreated(const SGraphViewLink& link)
	{
		CRY_ASSERT((m_pDoc != NULL) && (m_pDocGraph != NULL));
		if((m_pDoc != NULL) && (m_pDocGraph != NULL))
		{
			CDocGraphViewNodePtr	pSrcNode = boost::static_pointer_cast<CDocGraphViewNode>(link.pSrcNode.lock());
			CDocGraphViewNodePtr	pDstNode = boost::static_pointer_cast<CDocGraphViewNode>(link.pDstNode.lock());
			CRY_ASSERT((pSrcNode != NULL) && (pDstNode != NULL));
			if((pSrcNode != NULL) && (pDstNode != NULL))
			{
				m_pDocGraph->AddLink(pSrcNode->GetGUID(), link.srcOutputName.c_str(), pDstNode->GetGUID(), link.dstInputName.c_str());
				InvalidateDoc();
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::OnLinkRemoved(const SGraphViewLink& link)
	{
		CRY_ASSERT((m_pDoc != NULL) && (m_pDocGraph != NULL));
		if((m_pDoc != NULL) && (m_pDocGraph != NULL))
		{
			CDocGraphViewNodePtr	pSrcNode = boost::static_pointer_cast<CDocGraphViewNode>(link.pSrcNode.lock());
			CDocGraphViewNodePtr	pDstNode = boost::static_pointer_cast<CDocGraphViewNode>(link.pDstNode.lock());
			CRY_ASSERT((pSrcNode != NULL) && (pDstNode != NULL));
			if((pSrcNode != NULL) && (pDstNode != NULL))
			{
				const size_t	iLink = m_pDocGraph->FindLink(pSrcNode->GetGUID(), link.srcOutputName.c_str(), pDstNode->GetGUID(), link.dstInputName.c_str());
				if(iLink != INVALID_INDEX)
				{
					m_pDocGraph->RemoveLink(iLink);
					InvalidateDoc();
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	DROPEFFECT CDocGraphView::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
	{
		if(m_pDoc && m_pDocGraph)
		{
			PluginUtils::SDragAndDropData	dragAndDropData;
			if(PluginUtils::GetDragAndDropData(pDataObject, dragAndDropData))
			{
				switch(dragAndDropData.icon)
				{
				case BrowserIcon::BRANCH:
					{
						if(CanAddNode(DocGraphNodeType::BRANCH))
						{
							return DROPEFFECT_MOVE;
						}
						break;
					}
				case BrowserIcon::FOR_LOOP:
					{
						if(CanAddNode(DocGraphNodeType::FOR_LOOP))
						{
							return DROPEFFECT_MOVE;
						}
						break;
					}
				case BrowserIcon::RETURN:
					{
						if(CanAddNode(DocGraphNodeType::RETURN))
						{
							return DROPEFFECT_MOVE;
						}
						break;
					}
				default:
					{
						if(CanAddNode(dragAndDropData.guid))
						{
							return DROPEFFECT_MOVE;
						}
						break;
					}
				}
			}
		}
		return DROPEFFECT_NONE;
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CDocGraphView::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
	{
		if(m_pDoc && m_pDocGraph)
		{
			PluginUtils::SDragAndDropData	dragAndDropData;
			if(PluginUtils::GetDragAndDropData(pDataObject, dragAndDropData))
			{
				const Gdiplus::PointF	graphPos = CGraphView::ClientToGraph(Gdiplus::PointF(static_cast<float>(point.x), static_cast<float>(point.y)));
				switch(dragAndDropData.icon)
				{
				case BrowserIcon::BRANCH:
					{
						AddNode(DocGraphNodeType::BRANCH, SGUID(), Gdiplus::PointF(graphPos.X - DRAG_AND_DROP_OFFSET, graphPos.Y - DRAG_AND_DROP_OFFSET));
						InvalidateDoc();
						return true;
					}
				case BrowserIcon::FOR_LOOP:
					{
						AddNode(DocGraphNodeType::FOR_LOOP, SGUID(), Gdiplus::PointF(graphPos.X - DRAG_AND_DROP_OFFSET, graphPos.Y - DRAG_AND_DROP_OFFSET));
						InvalidateDoc();
						return true;
					}
				case BrowserIcon::RETURN:
					{
						AddNode(DocGraphNodeType::RETURN, SGUID(), Gdiplus::PointF(graphPos.X - DRAG_AND_DROP_OFFSET, graphPos.Y - DRAG_AND_DROP_OFFSET));
						InvalidateDoc();
						return true;
					}
				default:
					{
						TSizeTVector	availableNodes;
						m_pDocGraph->RefreshAvailableNodes(CTypeId());
						for(size_t iAvalailableNode = 0, availableNodeCount = m_pDocGraph->GetAvailableNodeCount(); iAvalailableNode < availableNodeCount; ++ iAvalailableNode)
						{
							if(m_pDocGraph->GetAvailableNodeRefGUID(iAvalailableNode) == dragAndDropData.guid)
							{
								availableNodes.push_back(iAvalailableNode);
							}
						}
						if(!availableNodes.empty())
						{
							int	nodeSelection = availableNodes.front() + 1;
							if(availableNodes.size() > 1)
							{
								CMenu popupMenu;
								popupMenu.CreatePopupMenu();
								for(TSizeTVector::const_iterator iAvailableNode = availableNodes.begin(), iEndAvailableNode = availableNodes.end(); iAvailableNode != iEndAvailableNode; ++ iAvailableNode)
								{
									popupMenu.AppendMenu(MF_STRING, *iAvailableNode + 1, m_pDocGraph->GetAvailableNodeName(*iAvailableNode));
								}
								CPoint cursorPos;
								GetCursorPos(&cursorPos);
								nodeSelection = popupMenu.TrackPopupMenuEx(TPM_RETURNCMD, cursorPos.x, cursorPos.y, this, NULL);
							}
							if(nodeSelection > 0)
							{
								AddNode(m_pDocGraph->GetAvailableNodeType(nodeSelection - 1), dragAndDropData.guid, Gdiplus::PointF(graphPos.X - DRAG_AND_DROP_OFFSET, graphPos.Y - DRAG_AND_DROP_OFFSET));
								InvalidateDoc();
								return true;
							}
						}
						break;
					}
				}
			}
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::GetPopupMenuItems(CMenu& popupMenu, const CGraphViewNodePtr& pNode, size_t iNodeInput, size_t iNodeOutput, CPoint point)
	{
		if(pNode)
		{
			CDocGraphViewNodePtr	pNodeImpl = boost::static_pointer_cast<CDocGraphViewNode>(pNode);
			IDocGraphNode&				docGraphNode = pNodeImpl->GetDocGraphNode();
			popupMenu.AppendMenu(MF_STRING, EPopupMenuItemEx::ADD_OUTPUT, "Add Output");	// TODO : Check number of optional outputs first?
			if(iNodeOutput != INVALID_INDEX)
			{
				if(docGraphNode.GetOutputFlags(iNodeOutput) & DocGraphNodePortFlags::IS_OPTIONAL)
				{
					popupMenu.AppendMenu(MF_STRING, EPopupMenuItemEx::REMOVE_OUTPUT, "Remove Output");
				}
			}
		}
		CGraphView::GetPopupMenuItems(popupMenu, pNode, iNodeInput, iNodeOutput, point);
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::OnPopupMenuResult(BOOL popupMenuItem, const CGraphViewNodePtr& pNode, size_t iNodeInput, size_t iNodeOutput, CPoint point)
	{
		if(m_pDoc)
		{
			switch(popupMenuItem)
			{
			case EPopupMenuItemEx::ADD_OUTPUT:
				{
					CRY_ASSERT(pNode);
					if(pNode)
					{
						SET_LOCAL_RESOURCE_SCOPE
						CDocGraphViewNodePtr	pNodeImpl = boost::static_pointer_cast<CDocGraphViewNode>(pNode);
						CAddGraphNodeOutputDialog(this, point, pNodeImpl->GetDocGraphNode()).DoModal();
						m_signals.docModified.Send(*m_pDoc);
						__super::Invalidate(TRUE);
					}
					break;
				}
			case EPopupMenuItemEx::REMOVE_OUTPUT:
				{
					CRY_ASSERT(pNode);
					if(pNode)
					{
						CDocGraphViewNodePtr	pNodeImpl = boost::static_pointer_cast<CDocGraphViewNode>(pNode);
						string	message = "Remove node output named '";
						message.append(pNodeImpl->GetOutputName(iNodeOutput));
						message.append("'?");
						if(MessageBox(message.c_str(), "Remove Node Output", 1) == IDOK)
						{
							CGraphView::RemoveLinksConnectedToNodeOutput(pNode, pNode->GetOutputName(iNodeOutput));
							pNodeImpl->GetDocGraphNode().RemoveOptionalOutput(iNodeOutput);
							m_signals.docModified.Send(*m_pDoc);
							__super::Invalidate(TRUE);
						}
					}
					break;
				}
			default:
				{
					CGraphView::OnPopupMenuResult(popupMenuItem, pNode, iNodeInput, iNodeOutput, point);
					break;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	const IQuickSearchOptions* CDocGraphView::GetQuickSearchOptions(CPoint point, const CGraphViewNodePtr& pNode, size_t iNodeOutput)
	{
		if(m_pDoc && m_pDocGraph)
		{
			CDocGraphViewNodePtr	pNodeImpl = boost::static_pointer_cast<CDocGraphViewNode>(pNode);
			m_quickSearchOptions.Refresh(*m_pDocGraph, pNode ? pNodeImpl->GetOutputTypeId(iNodeOutput) : CTypeId());
			return &m_quickSearchOptions;
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::OnQuickSearchResult(CPoint point, const CGraphViewNodePtr& pNode, size_t iNodeOutput, size_t iSelectedOption)
	{
		CRY_ASSERT(m_pDoc);
		CRY_ASSERT(iSelectedOption < m_quickSearchOptions.GetCount());
		if(m_pDoc && (iSelectedOption < m_quickSearchOptions.GetCount()))
		{
			CGraphView::ScreenToClient(&point);
			if(CDocGraphViewNodePtr pNewNode = AddNode(m_quickSearchOptions.GetNodeType(iSelectedOption), m_quickSearchOptions.GetNodeRefGUID(iSelectedOption), CGraphView::ClientToGraph(Gdiplus::PointF(point.x, point.y))))
			{
				if(pNode && (iNodeOutput != INVALID_INDEX))
				{
					const DocGraphNodePortFlags::EValue	outputFlags = pNode->GetOutputFlags(iNodeOutput);
					const CTypeId												outputTypeId = boost::static_pointer_cast<CDocGraphViewNode>(pNode)->GetOutputTypeId(iNodeOutput);
					for(size_t iNodeInput = 0, nodeInputCount = pNewNode->GetInputCount(); iNodeInput < nodeInputCount; ++ iNodeInput)
					{
						const DocGraphNodePortFlags::EValue	inputFlags = pNewNode->GetInputFlags(iNodeInput);
						const CTypeId												inputTypeId = pNewNode->GetInputTypeId(iNodeInput);
						if(((inputFlags & DocGraphNodePortFlags::EXECUTE) && (outputFlags & DocGraphNodePortFlags::EXECUTE)) || (inputTypeId == outputTypeId))
						{
							CGraphView::CreateLink(pNode, pNode->GetOutputName(iNodeOutput), pNewNode, pNewNode->GetInputName(iNodeInput));
							break;
						}
					}
				}
				InvalidateDoc();
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	size_t CDocGraphView::CQuickSearchOptions::GetCount() const
	{
		return m_options.size();
	}

	//////////////////////////////////////////////////////////////////////////
	const char* CDocGraphView::CQuickSearchOptions::GetName(size_t iOption) const
	{
		return iOption < m_options.size() ? m_options[iOption].name.c_str() : "";
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::CQuickSearchOptions::Refresh(IDocGraph& docGraph, const CTypeId& inputTypeId)
	{
		m_options.clear();
		docGraph.RefreshAvailableNodes(inputTypeId);
		for(size_t iAvalailableNode = 0, availableNodeCount = docGraph.GetAvailableNodeCount(); iAvalailableNode < availableNodeCount; ++ iAvalailableNode)
		{
			m_options.push_back(SOption(docGraph.GetAvailableNodeName(iAvalailableNode), docGraph.GetAvailableNodeType(iAvalailableNode), docGraph.GetAvailableNodeRefGUID(iAvalailableNode)));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	DocGraphNodeType::EValue CDocGraphView::CQuickSearchOptions::GetNodeType(size_t iOption) const
	{
		return iOption < m_options.size() ? m_options[iOption].nodeType : DocGraphNodeType::UNKNOWN;
	}

	//////////////////////////////////////////////////////////////////////////
	SGUID CDocGraphView::CQuickSearchOptions::GetNodeRefGUID(size_t iOption) const
	{
		return iOption < m_options.size() ? m_options[iOption].nodeRefGuid : SGUID();
	}

	//////////////////////////////////////////////////////////////////////////
	CDocGraphView::CQuickSearchOptions::SOption::SOption(const char* _name, DocGraphNodeType::EValue _nodeType, const SGUID& _nodeRefGuid)
		: name(_name)
		, nodeType(_nodeType)
		, nodeRefGuid(_nodeRefGuid)
	{}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CDocGraphView::VisitGraphNode(const IDocGraphNodePtr& pGraphNode)
	{
		AddNode(*pGraphNode);
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CDocGraphView::CanAddNode(DocGraphNodeType::EValue type) const
	{
		CRY_ASSERT(m_pDocGraph);
		if(m_pDocGraph)
		{
			m_pDocGraph->RefreshAvailableNodes(CTypeId());
			for(size_t iAvalailableNode = 0, availableNodeCount = m_pDocGraph->GetAvailableNodeCount(); iAvalailableNode < availableNodeCount; ++ iAvalailableNode)
			{
				if(m_pDocGraph->GetAvailableNodeType(iAvalailableNode) == type)
				{
					return true;
				}
			}
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CDocGraphView::CanAddNode(const SGUID& refGUID) const
	{
		CRY_ASSERT(m_pDocGraph);
		if(m_pDocGraph)
		{
			m_pDocGraph->RefreshAvailableNodes(CTypeId());
			for(size_t iAvalailableNode = 0, availableNodeCount = m_pDocGraph->GetAvailableNodeCount(); iAvalailableNode < availableNodeCount; ++ iAvalailableNode)
			{
				if(m_pDocGraph->GetAvailableNodeRefGUID(iAvalailableNode) == refGUID)
				{
					return true;
				}
			}
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CDocGraphView::CanAddNode(DocGraphNodeType::EValue type, const SGUID& refGUID) const
	{
		CRY_ASSERT(m_pDocGraph);
		if(m_pDocGraph)
		{
			m_pDocGraph->RefreshAvailableNodes(CTypeId());
			for(size_t iAvalailableNode = 0, availableNodeCount = m_pDocGraph->GetAvailableNodeCount(); iAvalailableNode < availableNodeCount; ++ iAvalailableNode)
			{
				if((m_pDocGraph->GetAvailableNodeType(iAvalailableNode) == type) && (m_pDocGraph->GetAvailableNodeRefGUID(iAvalailableNode) == refGUID))
				{
					return true;
				}
			}
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	CDocGraphViewNodePtr CDocGraphView::AddNode(DocGraphNodeType::EValue type, const SGUID& refGUID, Gdiplus::PointF pos)
	{
		CRY_ASSERT(m_pDocGraph);
		if(m_pDocGraph)
		{
			if(IDocGraphNodePtr pDocGraphNode = m_pDocGraph->AddNode(type, refGUID, Vec2(pos.X, pos.Y)))
			{
				CDocGraphViewNodePtr	pNode(new CDocGraphViewNode(CGraphView::GetGrid(), pos, *m_pDoc, *m_pDocGraph, *pDocGraphNode));
				CGraphView::AddNode(pNode, true);
				return pNode;
			}
		}
		return CDocGraphViewNodePtr();
	}

	//////////////////////////////////////////////////////////////////////////
	CDocGraphViewNodePtr CDocGraphView::AddNode(IDocGraphNode& docGraphNode)
	{
		CRY_ASSERT(m_pDocGraph);
		if(m_pDocGraph)
		{
			const Vec2						pos = docGraphNode.GetPos();
			CDocGraphViewNodePtr	pNode(new CDocGraphViewNode(CGraphView::GetGrid(), Gdiplus::PointF(pos.x, pos.y), *m_pDoc, *m_pDocGraph, docGraphNode));
			CGraphView::AddNode(pNode, false);
			return pNode;
		}
		return CDocGraphViewNodePtr();
	}

	//////////////////////////////////////////////////////////////////////////
	CDocGraphViewNodePtr CDocGraphView::GetNode(const SGUID& guid) const
	{
		const TGraphViewNodePtrVector&	nodes = GetNodes();
		for(TGraphViewNodePtrVector::const_iterator iNode = nodes.begin(), iEndNode = nodes.end(); iNode != iEndNode; ++ iNode)
		{
			const CDocGraphViewNodePtr&	pNode = boost::static_pointer_cast<CDocGraphViewNode>(*iNode);
			if(pNode->GetGUID() == guid)
			{
				return pNode;
			}
		}
		return CDocGraphViewNodePtr();
	}

	//////////////////////////////////////////////////////////////////////////
	bool CDocGraphView::AddLink(const IDocGraphLink& docGraphLink)
	{
		CDocGraphViewNodePtr	pSrcNode = boost::static_pointer_cast<CDocGraphViewNode>(GetNode(docGraphLink.GetSrcNodeGUID()));
		CDocGraphViewNodePtr	pDstNode = boost::static_pointer_cast<CDocGraphViewNode>(GetNode(docGraphLink.GetDstNodeGUID()));
		if((pSrcNode != NULL) && (pDstNode != NULL))
		{
			CGraphView::AddLink(pSrcNode, docGraphLink.GetSrcOutputName(), pDstNode, docGraphLink.GetDstInputName());
			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::InvalidateDoc()
	{
		if((m_pDoc != NULL) && (m_pDocGraph != NULL))
		{
			m_pDocGraph->Refresh(DocRefreshReason::DEPEDENCY_MODIFIED);	// TODO : Is really the right reason?
			m_signals.docModified.Send(*m_pDoc);
			UnrollSequence();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CDocGraphView::UnrollSequence()
	{
		m_sequenceNodes.clear();
		if(m_pDoc && m_pDocGraph)
		{
			TDocGraphNodeConstVector	docGraphNodes;
			DocUtils::CollectGraphNodes(*m_pDocGraph, docGraphNodes);
			for(TDocGraphNodeConstVector::const_iterator iNode = docGraphNodes.begin(), iEndNode = docGraphNodes.end(); iNode != iEndNode; ++ iNode)
			{
				const IDocGraphNode&	node = *(*iNode);
				for(size_t iOutput = 0, outputCount = node.GetOutputCount(); iOutput < outputCount; ++ iOutput)
				{
					if((node.GetOutputFlags(iOutput) & DocGraphNodePortFlags::BEGIN_SEQUENCE) != 0)
					{
						DocUtils::UnrollGraphSequenceRecursive_Deprecated(*m_pDocGraph, node.GetGUID(), m_sequenceNodes);
					}
				}
			}

			TGraphViewNodePtrVector& nodes = CGraphView::GetNodes();
			for(TGraphViewNodePtrVector::iterator iNode = nodes.begin(), iEndNode = nodes.end(); iNode != iEndNode; ++ iNode)
			{
				CDocGraphViewNodePtr	pNode = boost::static_pointer_cast<CDocGraphViewNode>(*iNode);
				pNode->Enable(std::find(m_sequenceNodes.begin(), m_sequenceNodes.end(), pNode->GetGUID()) != m_sequenceNodes.end());
			}
		}
	}
}