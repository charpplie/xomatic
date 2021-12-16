#include "pch.h"
#include "OffMeshLinkObject.h"

#include "GameEngine.h"
#include "ShapePanel.h"
#include "AI\NavDataGeneration\Navigation.h"
#include "AI\AIManager.h"

#include <IGame.h>

#include "..\..\Game_Hunt\GameDll\Game_P1\Core\IObjectDesc.h"
#include "..\..\Game_Hunt\GameDll\Game_P1\Core\IGameInterface.h"
#include "..\..\Game_Hunt\GameDll\Game_P1\AI\OffMeshLinkData.h"

using namespace CryGame;


IMPLEMENT_DYNCREATE(COffMeshLinkObject, CGameShapeObject)

#define OMLO_VERSION 3

COffMeshLinkObject::COffMeshLinkObject()
{
	m_entityClass = "OffMeshLink";

	SetColor(RGB(180, 180, 180));
	SetClosed(false);

	m_bDisplayFilledWhenSelected = false;
}

COffMeshLinkObject::~COffMeshLinkObject()
{}

void COffMeshLinkObject::InitVariables()
{
	// Add 'Direction' drop-down
	m_dir.AddEnumItem("Forward", (int)CryGame::eOMLD_Forward);
	m_dir.AddEnumItem("Reverse", (int)CryGame::eOMLD_Reverse);
	m_dir.AddEnumItem("Both", (int)CryGame::eOMLD_Both);
	m_dir = (int)CryGame::eOMLD_Both;  // Set Default
	AddVariable(m_dir, "Direction", functor(*this, &COffMeshLinkObject::OnPropertyChange));

	// Add 'Validate' field
	m_trimExcess = true;
	AddVariable(m_trimExcess, "TrimExcess", functor(*this, &COffMeshLinkObject::OnPropertyChange));

	IGameInterface* igame = (IGameInterface*)gEnv->pGame->GetGameInterface();
	CryGame::ObjectDescVector descs = igame->GetObjectDescFactory()->GetDescList("COffMeshLinkExtensionDesc");

	for (int i = 0; i < 2; ++i)
	{
		//setup enum
		for (CryGame::ObjectDescVector::iterator d = descs.begin(); d != descs.end(); ++d)
		{
			m_desc[i].AddEnumItem((*d)->GetNameID().c_str() , (*d)->GetNameID().c_str());
		}

		// Add 'Desc' field
		m_desc[i] = CString("Continue");
		AddVariable(m_desc[i], (i == 0) ? "EntryType" : "ExitType", functor(*this, &COffMeshLinkObject::OnPropertyChange));

		// Add 'EntryEnabled
		m_enabled[i] = true;
		AddVariable(m_enabled[i], (i == 0) ? "EntryEnabled" : "ExitEnabled", functor(*this, &COffMeshLinkObject::OnPropertyChange));
	}
}

void COffMeshLinkObject::BeginEditParams(IEditor* ie, int flags)
{
	CBaseObject::BeginEditParams(ie, flags);

	if (!m_panel)
	{
		m_panel = new CShapePanel;
		m_panel->Create(CShapePanel::IDD);
		m_rollupId = ie->AddRollUpPage(ROLLUP_OBJECTS, "Shape Parameters", m_panel);
	}
	if (m_panel)
		m_panel->SetShape(this);
}

void COffMeshLinkObject::PostLoad(CObjectArchive& ar)
{
	__super::PostLoad(ar);

	// Update properties
	if (m_pProperties)
	{
		IVariable* pVersion = m_pProperties->FindVariable("nVersion", false);
		if (pVersion)
		{
			int nVersion = 0;
			// Grab saved version
			pVersion->Get(nVersion);

			///> Perform any fix-ups

			if (nVersion == 0)
			{
				// Previous implementations were storing absolute coordinates which
				// do not work well with randomly generated worlds so fix that here.
				const Matrix34& wtm = GetWorldTM();

				for (int i = 0; i < m_points.size(); ++i)
				{
					IVariable* pPoint = m_pProperties->FindVariable((i == 0) ? "Entry" : "Exit", false);
					if (pPoint)
					{
						IVariable* pPos = pPoint->FindVariable("vPosition");
						if (pPos)
						{
							// Store relative positions
							pPos->FindVariable("x")->Set(m_points[i].x);
							pPos->FindVariable("y")->Set(m_points[i].y);
							pPos->FindVariable("z")->Set(m_points[i].z);
						}
					}
				}
			}

			if (nVersion <= 1)
			{
				// Descs were introduced in version 2 and all previous versions
				// should default to "Default"
				IVariable* pDesc = m_pProperties->FindVariable("sDesc", false);
				if (pDesc)
				{
					pDesc->Set("Default");
				}
			}

			if (nVersion == 2)
			{
				// Descs moved to end point data, replacing types
				for (int i = 0; i < m_points.size(); ++i)
				{
					IVariable* pPoint = m_pProperties->FindVariable((i == 0) ? "Entry" : "Exit", false);
					if (pPoint)
					{
						IVariable* pType = pPoint->FindVariable("eType");
						IVariable* pDesc = pPoint->FindVariable("sDesc");
						if (pType && pDesc)
						{
							int nType;
							pType->Get(nType);

							const int numEnums = 6;
							const char* enums[] = { "Continue", "JumpUp", "JumpDown", "Vault", "Ladder", "Door" };
							if (nType < numEnums)
							{
								pDesc->Set(enums[nType]);
							}
						}
					}
				}
			}

			// Set to latest version
			pVersion->Set(OMLO_VERSION);
		}

		// HACK: Force entry/exit types to display in list as this doesn't appear to happen automatically on load
		// TODO: Look for a better method.
		{
			// Descs moved to end point data, replacing types
			for (int i = 0; i < m_points.size(); ++i)
			{
				IVariable* pPoint = m_pProperties->FindVariable((i == 0) ? "Entry" : "Exit", false);
				if (pPoint)
				{
					IVariable* pDesc = pPoint->FindVariable("sDesc");
					if (pDesc)
					{
						CString strTemp;
						pDesc->Get(strTemp);
						m_desc[i] = strTemp;
					}
				}
			}
		}
	}

	if (m_points.size() > 1)
	{
		// update the shape in realtime
		NotifyPropertyChange();
	}

	// Notify the game-object that properties have changed
	OnPropertyChange(NULL);
}

bool COffMeshLinkObject::GetProperties(SmartScriptTable& out)
{
	if (m_pEntity != NULL)
	{
		IScriptTable* pScriptTable = m_pEntity->GetScriptTable();
		if (pScriptTable != NULL)
		{
			return pScriptTable->GetValue("Properties", out);
		}
	}

	return NULL;
}

void COffMeshLinkObject::EndCreation()
{
	// Update version
	if (m_pProperties)
	{
		IVariable* pVersion = m_pProperties->FindVariable("nVersion", false);
		if (pVersion)
		{
			pVersion->Set(OMLO_VERSION);
		}
	}

	// Notify the game-object that properties have changed
	OnPropertyChange(NULL);
}

void COffMeshLinkObject::OnPropertyChange(IVariable* var)
{
	if (m_bIgnoreGameUpdate)
		return;

	// Update properties
	if (m_pProperties)
	{
		IVariable* pDirection = m_pProperties->FindVariable("eDirection", false);
		if (pDirection)
		{
			pDirection->Set((int)m_dir);
		}

		IVariable* pTrimExcess = m_pProperties->FindVariable("bTrimExcess", false);
		if (pTrimExcess)
		{
			pTrimExcess->Set((bool)m_trimExcess);
		}

		for (int i = 0; i < 2; ++i)
		{
			IVariable* pPoint = m_pProperties->FindVariable((i == 0) ? "Entry" : "Exit", false);
			if (pPoint)
			{
				IVariable* pDesc = pPoint->FindVariable("sDesc", false);
				if (pDesc)
				{
					pDesc->Set(m_desc[i].GetDisplayValue());
				}

				IVariable* pEnabled = pPoint->FindVariable("bEnabled");
				if (pEnabled)
				{
					pEnabled->Set((bool)m_enabled[i]);
				}
			}
		}
	}

	// Force update
	UpdateGameArea(false);
}

void COffMeshLinkObject::Display(DisplayContext& dc)
{
	Vec3 iconPos = GetWorldPos();

	// Display the direction of the path
	if (m_points.size() > 1)
	{
		const Matrix34& wtm = GetWorldTM();
		Vec3 p0 = wtm.TransformPoint(m_points[0]);
		Vec3 p1 = wtm.TransformPoint(m_points[1]);
		Vec3 d = (p1 - p0);
		Vec3 m = (p0 + ((p1 - p0) * 0.5f));

		dc.SetColor(RGB(255, 120, 0));  // Orange

		// Draw text
		// Lets not draw this, it clutters up the viewport and really doesn't provide much
		// useful info. -ColinB
//      static float s_textHeightOffset = 0.5f;
//      m.z += m_height + s_textHeightOffset;
//      string msg;
//      msg.Format("Length: %.2fm", d.len());
//      dc.DrawTextLabel(m, 1.2f, msg, true);

		iconPos = m;
	}

	SetDrawTextureIconProperties(dc, iconPos);
	DrawTextureIcon(dc, iconPos, 1.f);
}

void COffMeshLinkObject::SetPoint(int index, const Vec3& pos)
{
	__super::SetPoint(index, pos);

	// Update properties
	if (m_pProperties)
	{
		IVariable* pPoint = m_pProperties->FindVariable((index == 0) ? "Entry" : "Exit", false);
		if (pPoint)
		{
			IVariable* pPos = pPoint->FindVariable("vPosition");
			if (pPos)
			{
				// Store relative positions
				pPos->FindVariable("x")->Set(pos.x);
				pPos->FindVariable("y")->Set(pos.y);
				pPos->FindVariable("z")->Set(pos.z);
			}
		}
	}

	// Force
	UpdateGameArea(false);
}