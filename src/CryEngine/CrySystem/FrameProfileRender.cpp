////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   frameprofilerender.cpp
//  Version:     v1.00
//  Created:     24/6/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: Rendering of FrameProfiling information.
// -------------------------------------------------------------------------
//  History: Some ideas taken from Jonathan Blow`s profiler from GDmag article.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FrameProfileSystem.h"
#include <IRenderer.h>
#include <IInput.h>
#include <ILog.h>
#include <IRenderAuxGeom.h>
#include <IScriptSystem.h>
#include "ITextModeConsole.h"

#include <CryFile.h>

#include "Statistics.h"

#ifdef WIN32
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#ifdef LINUX
#include <dlfcn.h>
#endif

#ifdef USE_FRAME_PROFILER

//! Time is in milliseconds.
#define PROFILER_MIN_DISPLAY_TIME 0.01f

#define VARIANCE_MULTIPLIER 2.0f

//! 5 seconds from hot to cold in peaks.
#define MAX_DISPLAY_ROWS 80

extern int CryMemoryGetAllocatedSize();
extern int CryMemoryGetPoolSize();














//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::DrawLabel( float col,float row,float* fColor,float glow,const char* szText,float fScale )
{
  float ColumnSize = COL_SIZE;
  float RowSize = ROW_SIZE;

	SDrawTextInfo ti;
	ti.xscale = ti.yscale = fScale*1.2f;
	ti.flags = eDrawText_2D | eDrawText_FixedSize | eDrawText_Monospace;
	ti.color[0] = fColor[0]; ti.color[1] = fColor[1]; ti.color[2] = fColor[2]; ti.color[3] = fColor[3];

  if (glow > 0.1f)
  {
    ti.color[0] = fColor[0]; ti.color[1] = fColor[1]; ti.color[2] = fColor[2]; ti.color[3] = glow;
		m_pRenderer->DrawTextQueued( Vec3(ColumnSize*col+1,m_baseY+RowSize*row+1 - m_offset,0.5f),ti,szText );
  }
	ti.color[0] = fColor[0]; ti.color[1] = fColor[1]; ti.color[2] = fColor[2]; ti.color[3] = fColor[3];
	m_pRenderer->DrawTextQueued( Vec3(ColumnSize*col+1,m_baseY+RowSize*row+1 - m_offset,0.5f),ti,szText );

  if (ITextModeConsole * pTC = gEnv->pSystem->GetITextModeConsole())
  {
    pTC->PutText( (int)col, (int)(m_textModeBaseExtra+row + std::max(0.0f, (m_baseY-120.0f)/ROW_SIZE)) , szText );
  }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::DrawRect( float x1,float y1,float x2,float y2,float *fColor )
{
  m_pRenderer->SetMaterialColor( fColor[0],fColor[1],fColor[2],fColor[3] );
  m_pRenderer->SetMaterialColor( fColor[0],fColor[1],fColor[2],fColor[3] );
  int w = m_pRenderer->GetWidth();
  int h = m_pRenderer->GetHeight();

	y1 -= m_offset;
	y2 -= m_offset;
  //float dx = 800.0f/w;
  //float dy = 600.0f/h;
  float dx = 1.0f/w;
  float dy = 1.0f/h;
  x1 *= dx; x2 *= dx;
  y1 *= dy; y2 *= dy;

  ColorB col((uint8)(fColor[0]*255.0f),(uint8)(fColor[1]*255.0f),(uint8)(fColor[2]*255.0f),(uint8)(fColor[3]*255.0f));

  IRenderAuxGeom *pAux = m_pRenderer->GetIRenderAuxGeom();
  SAuxGeomRenderFlags flags = pAux->GetRenderFlags();
  flags.SetMode2D3DFlag( e_Mode2D );
  pAux->SetRenderFlags(flags);
  pAux->DrawLine( Vec3(x1,y1,0),col,Vec3(x2,y1,0),col );
  pAux->DrawLine( Vec3(x1,y2,0),col,Vec3(x2,y2,0),col );
  pAux->DrawLine( Vec3(x1,y1,0),col,Vec3(x1,y2,0),col );
  pAux->DrawLine( Vec3(x2,y1,0),col,Vec3(x2,y2,0),col );
}

//////////////////////////////////////////////////////////////////////////
inline float CalculateVarianceFactor( float value,float variance )
{
  //variance = fabs(variance - value*value);
  float difference = (float)sqrt_tpl(variance);

  const float VALUE_EPSILON = 0.000001f;
  value = (float)fabs(value);
  // Prevent division by zero.
  if (value < VALUE_EPSILON)
  {
    return 0;
  }
  float factor = 0;
  if (value > 0.01f)
    factor = (difference/value) * VARIANCE_MULTIPLIER;

  return factor;
}

//////////////////////////////////////////////////////////////////////////
inline void CalculateColor( float value, float variance,float *outColor,float &glow )
{
  float ColdColor[4] = { 0.15f,0.9f,0.15f,1 };
  float HotColor[4] = { 1,1,1,1 };

  glow = 0;

  float factor = CalculateVarianceFactor( value,variance );
  if (factor < 0)
    factor = 0;
  if (factor > 1)
    factor = 1;

  // Interpolate Hot to Cold color with variance factor.
  for (int k = 0; k < 4; k++)
    outColor[k] = HotColor[k]*factor + ColdColor[k]*(1.0f-factor);

  // Figure out whether to start up the glow as well.
  const float GLOW_RANGE = 0.5f;
  const float GLOW_ALPHA_MAX = 0.5f;
  float glow_alpha = (factor - GLOW_RANGE) / (1 - GLOW_RANGE);
  glow = glow_alpha * GLOW_ALPHA_MAX;
}

//////////////////////////////////////////////////////////////////////////
inline bool CompareFrameProfilersValue( const CFrameProfileSystem::SProfilerDisplayInfo &p1,const CFrameProfileSystem::SProfilerDisplayInfo &p2 )
{
  return p1.pProfiler->m_displayedValue > p2.pProfiler->m_displayedValue;
}

//////////////////////////////////////////////////////////////////////////
inline bool CompareFrameProfilersCount( const CFrameProfileSystem::SProfilerDisplayInfo &p1,const CFrameProfileSystem::SProfilerDisplayInfo &p2 )
{
  return p1.averageCount > p2.averageCount;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::AddDisplayedProfiler( CFrameProfiler *pProfiler,int level )
{
  //if (pProfiler->m_displayedValue == 0)
  //return;

  char bExpended = pProfiler->m_bExpended;

  int newLevel = level + 1;
  if (!m_bSubsystemFilterEnabled || pProfiler->m_subsystem == (uint8)m_subsystemFilter)
  {
    SProfilerDisplayInfo info;
    info.level = level;
    info.pProfiler = pProfiler;
    m_displayedProfilers.push_back( info );
  }
  else
  {
    bExpended = 1;
    newLevel = level;
  }
  // Find childs.
  //@TODO Very Slow, optimize that.
  if (bExpended && pProfiler->m_bHaveChildren)
  {
    for (int i = 0; i < (int)m_pProfilers->size(); i++)
    {
      CFrameProfiler *pCur = (*m_pProfilers)[i];
      if (pCur->m_pParent == pProfiler)
        AddDisplayedProfiler( pCur,newLevel );
    }
  }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::CalcDisplayedProfilers()
{
  if (m_bDisplayedProfilersValid)
    return;

  m_bDisplayedProfilersValid = true;
  m_displayedProfilers.reserve( m_pProfilers->size() );
  m_displayedProfilers.resize(0);

  //////////////////////////////////////////////////////////////////////////
  // Get all displayed profilers.
  //////////////////////////////////////////////////////////////////////////
  if (m_displayQuantity == TOTAL_TIME || m_bEnableHistograms)
  {
    if (m_bEnableHistograms)
    {
      // In extended mode always add first item selected profiler.
      if (m_pGraphProfiler)
      {
        SProfilerDisplayInfo info;
        info.level = 0;
        info.pProfiler = m_pGraphProfiler;
        m_displayedProfilers.push_back( info );
      }
    }
    // Go through all profilers.
    for (int i = 0; i < (int)m_pProfilers->size(); i++)
    {
      CFrameProfiler *pProfiler = (*m_pProfilers)[i];
      if (!pProfiler->m_pParent && pProfiler->m_displayedValue >= 0.01f)
      {
        //pProfiler->m_bExpended = true;
        AddDisplayedProfiler( pProfiler,0 );
      }
    }
    if (m_displayedProfilers.empty())
      m_bDisplayedProfilersValid = false;
    return;
  }

  if (m_displayQuantity == COUNT_INFO)
  {
    // Go through all profilers.
    for (int i = 0; i < (int)m_pProfilers->size(); i++)
    {
      CFrameProfiler *pProfiler = (*m_pProfilers)[i];
      // Skip this profiler if its filtered out.
      if (m_bSubsystemFilterEnabled && pProfiler->m_subsystem != (uint8)m_subsystemFilter)
        continue;
      int count = pProfiler->m_countHistory.GetAverage();
      if (count > 1)
      {
        SProfilerDisplayInfo info;
        info.level = 0;
        info.averageCount = count;
        info.pProfiler = pProfiler;
        m_displayedProfilers.push_back( info );
      }
    }
    std::sort( m_displayedProfilers.begin(),m_displayedProfilers.end(),CompareFrameProfilersCount );
    if ((int)m_displayedProfilers.size() > m_maxProfileCount)
      m_displayedProfilers.resize(m_maxProfileCount);
    return;
  }

  // Go through all profilers.
  for (int i = 0; i < (int)m_pProfilers->size(); i++)
  {
    CFrameProfiler *pProfiler = (*m_pProfilers)[i];
    // Skip this profiler if its filtered out.
    if (m_bSubsystemFilterEnabled && pProfiler->m_subsystem != (uint8)m_subsystemFilter)
      continue;

    if (pProfiler->m_displayedValue > PROFILER_MIN_DISPLAY_TIME)
    {
      SProfilerDisplayInfo info;
      info.level = 0;
      info.pProfiler = pProfiler;
      m_displayedProfilers.push_back( info );
    }
  }
  //if (m_displayQuantity != EXTENDED_INFO)
  {
    //////////////////////////////////////////////////////////////////////////
    // sort displayed profilers by thier time.
    // Sort profilers by display value or count.
    std::sort( m_displayedProfilers.begin(),m_displayedProfilers.end(),CompareFrameProfilersValue );
    if ((int)m_displayedProfilers.size() > m_maxProfileCount)
      m_displayedProfilers.resize(m_maxProfileCount);
  }
}

//////////////////////////////////////////////////////////////////////////
CFrameProfiler* CFrameProfileSystem::GetSelectedProfiler()
{
  if (m_displayedProfilers.empty())
    return 0;
  if (m_selectedRow < 0)
    m_selectedRow = 0;
  if (m_selectedRow > (int)m_displayedProfilers.size()-1)
    m_selectedRow = (int)m_displayedProfilers.size()-1;
  return m_displayedProfilers[m_selectedRow].pProfiler;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::Render()
{
  m_textModeBaseExtra = 0;

  if (m_bDisplayMemoryInfo)
    RenderMemoryInfo();

  if (!m_bDisplay)
    return;

  FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_SYSTEM,g_bProfilerEnabled );

#ifndef XENON
  m_baseY = 120;


#endif
  m_textModeBaseExtra = 2;
  ROW_SIZE = 11;
  COL_SIZE = 11;

  m_pRenderer = m_pSystem->GetIRenderer();
  if (!m_pRenderer)
    return;

  if (m_bCollectionPaused)
  {
#ifdef WIN32
    HWND hWnd = (HWND)m_pRenderer->GetHWND();
    POINT p;
    ::GetCursorPos(&p);
    if (hWnd)
      ::ScreenToClient(hWnd,&p);
    m_mouseX = (float)p.x;
    m_mouseY = (float)p.y;
#endif
  }
  else
  {
    m_selectedRow = -1;
  }




  float colText = 42.0f - 4*m_pSystem->IsDedicated();

  float colExtended = 1.0f;
  float row = 0;

  int width = m_pRenderer->GetWidth();
  int height = m_pRenderer->GetHeight();
  m_pRenderer->Set2DMode( true, width, height);

  //////////////////////////////////////////////////////////////////////////
  // Check if displayed profilers must be recalculated.
  if (m_displayQuantity == TOTAL_TIME || m_bEnableHistograms)
  {
    //if (m_bCollectionPaused)
    //m_bDisplayedProfilersValid = false;
  }
  else
    m_bDisplayedProfilersValid = false;
  //////////////////////////////////////////////////////////////////////////

  // Calculate which profilers must be displayed, and sort by importance.
  if (m_displayQuantity != SUBSYSTEM_INFO)
    CalcDisplayedProfilers();

  if (m_bEnableHistograms)
  {
    m_baseY = 50;
  }

  if (m_bEnableHistograms)
    RenderProfilerHeader( colExtended,row,m_bEnableHistograms );
  else
    RenderProfilerHeader( colText,row,m_bEnableHistograms );

  if (m_bEnableHistograms)
  {
    ROW_SIZE = (float)m_histogramsHeight+4;
  }

  //////////////////////////////////////////////////////////////////////////
  if (m_bEnableHistograms)
    RenderProfilers( colExtended,0,true );
  else if (m_displayQuantity == SUBSYSTEM_INFO)
    RenderSubSystems( colText,0 );
  else
    RenderProfilers( colText,0,false );
  //////////////////////////////////////////////////////////////////////////


  // Render Peaks.
  if (m_displayQuantity == PEAK_TIME || m_bDrawGraph || m_bPageFaultsGraph)
  {
    DrawGraph();
  }

	float fpeaksLastRow = 0;




  if (m_peaks.size() > 0)

  {
    fpeaksLastRow = RenderPeaks();
  }

	if (GetAdditionalSubsystems())
	{





		float colPeaks = 16.0f;

		RenderSubSystems(colPeaks, std::max(30.0f, fpeaksLastRow + 2));
	}

  m_pRenderer->Set2DMode( false, 0, 0);

  if (m_bEnableHistograms)
    RenderHistograms();

  // Check keys.
#if defined(WIN32) || defined(PS3)
  if (m_bCollectionPaused)
  {
    if (GetAsyncKeyState(VK_UP) & 1)
    {
      m_selectedRow--;
      m_pGraphProfiler = GetSelectedProfiler();
    }
    if (GetAsyncKeyState(VK_DOWN) & 1)
    {
      m_selectedRow++;
      m_pGraphProfiler = GetSelectedProfiler();
    }
    if (GetAsyncKeyState(VK_RIGHT) & 1)
    {
      m_bDisplayedProfilersValid = false;
      if (m_pGraphProfiler)
        m_pGraphProfiler->m_bExpended = 1;
    }
    if (GetAsyncKeyState(VK_LEFT) & 1)
    {
      m_bDisplayedProfilersValid = false;
      if (m_pGraphProfiler)
        m_pGraphProfiler->m_bExpended = 0;
    }
    if (GetAsyncKeyState(VK_ESCAPE) & 1)
    {
      m_pGraphProfiler = 0;
      m_selectedRow = -1;
    }

    if (GetAsyncKeyState(VK_LBUTTON) & 1)
    {
      if (m_selectedRow >= 0)
      {
        m_pGraphProfiler = GetSelectedProfiler();
        m_bDisplayedProfilersValid = false;
        if (m_pGraphProfiler)
          m_pGraphProfiler->m_bExpended = !m_pGraphProfiler->m_bExpended;
      }
    }
  }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::RenderProfilers( float col,float row,bool bExtended )
{
//  float HeaderColor[4] = { 1,1,0,1 };
  float CounterColor[4] = { 0,0.8f,1,1 };

  int width = m_pRenderer->GetWidth();
  int height = m_pRenderer->GetHeight();

  // Header.
  m_baseY += 40;
  row = 0;

  CFrameProfiler *pSelProfiler = 0;
  if (m_bCollectionPaused)
    pSelProfiler = GetSelectedProfiler();

  if (CFrameProfileSystem::profile_log)
  {
    CryLogAlways( "======================= Start Profiler Frame %d ==========================",gEnv->pRenderer->GetFrameID(false) );
    CryLogAlways( "|\tCount\t|\tSelf\t|\tTotal\t|\tModule\t|" );
    CryLogAlways( "|\t____\t|\t_____\t|\t_____\t|\t_____\t|" );

    int logType = abs(CFrameProfileSystem::profile_log);
    for (int i = 0; i < (int)m_displayedProfilers.size(); i++)
    {
      CFrameProfiler *pProfiler = m_displayedProfilers[i].pProfiler;

      //float fSelfTimeMs = CFrameProfilerTimer::TicksToMilliseconds(pProfiler->m_selfTimeHistory.GetAverage());
      //float fTotalTimeMs = CFrameProfilerTimer::TicksToMilliseconds(pProfiler->m_totalTime);
      if (logType == 1)
      {
        int cave = pProfiler->m_countHistory.GetAverage();
        float fTotalTimeMs = pProfiler->m_totalTimeHistory.GetAverage();
        float fSelfTimeMs = pProfiler->m_selfTimeHistory.GetAverage();
        CryLogAlways( "|\t%d\t|\t%.2f\t|\t%.2f%%\t|\t%s\t|\t %s",cave,fSelfTimeMs,fTotalTimeMs,GetModuleName(pProfiler),GetFullName(pProfiler) );
      }
      else if (logType == 2)
      {
        int c_min = pProfiler->m_countHistory.GetMin();
        int c_max = pProfiler->m_countHistory.GetMax();
        float t1_min = pProfiler->m_totalTimeHistory.GetMin();
        float t1_max = pProfiler->m_totalTimeHistory.GetMax();
        float t0_min = pProfiler->m_selfTimeHistory.GetMin();
        float t0_max = pProfiler->m_selfTimeHistory.GetMax();
        CryLogAlways( "|\t%d/%d\t|\t%.2f/%.2f\t|\t%.2f/%.2f%%\t|\t%s\t|\t %s",c_min,c_max,t0_min,t0_max,t1_min,t1_max,GetModuleName(pProfiler),GetFullName(pProfiler) );
      }
    }

    CryLogAlways( "======================= End Profiler Frame %d ==========================",gEnv->pRenderer->GetFrameID(false) );
    if (CFrameProfileSystem::profile_log > 0) // reset logging so only logs one frame.
      CFrameProfileSystem::profile_log = 0;
  }






































































































  // Go through all profilers.
  for (int i = 0; i < (int)m_displayedProfilers.size(); i++)
  {
    SProfilerDisplayInfo &dispInfo = m_displayedProfilers[i];
    CFrameProfiler *pProfiler = m_displayedProfilers[i].pProfiler;
		//filter stall profilers (caused by averaging)
		if(m_displayQuantity != STALL_TIME && pProfiler->m_isStallProfiler ||
			 m_displayQuantity == STALL_TIME && !pProfiler->m_isStallProfiler)
			 continue;
    if (i > m_selectedRow + MAX_DISPLAY_ROWS)
    {
      break;
    }

    float rectX1 = col*COL_SIZE;
    float rectX2 = width - 2.0f;
    float rectY1 = m_baseY + row*ROW_SIZE + 2;
    float rectY2 = m_baseY + (row+1)*ROW_SIZE + 2;

    dispInfo.x = rectX1;
    dispInfo.y = rectY1;

    if (dispInfo.y - m_offset + ROW_SIZE >= height)
      continue;

    /*
    if (m_bCollectionPaused)
    {
    if (m_mouseX > rectX1 && m_mouseX < rectX2 && m_mouseY > rectY1 && m_mouseY < rectY2)
    {
    // Mouse inside this rectangle.
    m_selectedRow = i;
    }
    }
    */

    if (i == m_selectedRow && m_bCollectionPaused)
    {
      float SelColor[4] = { 1,1,1,1 };
      DrawRect( rectX1,rectY1,rectX2,rectY2,SelColor );
    }

    RenderProfiler( pProfiler,dispInfo.level,col,row,bExtended,(pSelProfiler==pProfiler) );
    row += 1.0f;
  }
  m_baseY -= 40;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::RenderProfilerHeader( float col,float row,bool bExtended )
{
  char szText[256];
  float MainHeaderColor[4] = { 0,1,1,1 };
  float HeaderColor[4] = { 1,1,0,1 };
  float CounterColor[4] = { 0,0.8f,1,1 };

  float frameTime = m_frameTimeHistory.GetAverage();
  float frameLostTime = m_frameTimeLostHistory.GetAverage();











    const char *sValueName = "Time";
		if (m_bMemoryProfiling)
		{
			sValueName = "KB(s)";
		}

    strcpy( szText,"" );
    // Draw general statistics.
    switch ((int)m_displayQuantity)
    {
    case SELF_TIME:
    case SELF_TIME_EXTENDED:
      strcpy_s( szText,"Profile Mode: Self Time" );
      break;
    case TOTAL_TIME:
    case TOTAL_TIME_EXTENDED:
      strcpy_s( szText,"Profile Mode: Hierarchical Time" );
      break;
    case PEAK_TIME:
      strcpy_s( szText,"Profile Mode: Peak Time" );
      break;
    case COUNT_INFO:
      strcpy_s( szText,"Profile Mode: Calls Number" );
      break;
    case SUBSYSTEM_INFO:
      strcpy_s( szText,"Profile Mode: Subsystems" );
      break;
    case STANDART_DEVIATION:
      strcpy_s( szText,"Profile Mode: Standard Deviation" );
      sValueName = "StdDev";
      break;
		case ALLOCATED_MEMORY:
			strcpy_s( szText,"Profile Mode: Memory Allocations" );
			sValueName = "KB(s)";
			break;
		case ALLOCATED_MEMORY_BYTES:
			strcpy_s( szText,"Profile Mode: Memory Allocations (Bytes)" );
			sValueName = "Bytes";
			break;
    }
    if (m_bCollectionPaused)
      strcat_s( szText," (Paused)" );

		if (!m_filterThreadName.empty())
		{
			strcat_s( szText," (Thread:" );
			strcat_s( szText,m_filterThreadName.c_str() );
			strcat_s( szText,")" );
		}

    row--;









    DrawLabel( col,row++,MainHeaderColor,0,szText );




			if(m_displayQuantity != STALL_TIME)
			{



		    sprintf_s( szText,"FrameTime: %4.2fms, LostTime: %4.2fms, PF/Sec: %d",frameTime,frameLostTime,m_nPagesFaultsPerSec );

			  DrawLabel( col,row++,MainHeaderColor,0,szText );
			}





    // Header.
    if (bExtended)
    {
      row = 0;
      m_baseY += 24;
      DrawLabel( col,row,HeaderColor,0,"Max" );
      DrawLabel( col+5,row,HeaderColor,0,"Min" );
      DrawLabel( col+10,row,HeaderColor,0,"Ave" );
      if (m_displayQuantity == TOTAL_TIME_EXTENDED)
        DrawLabel( col+15,row,HeaderColor,0,"Self" );
      else
        DrawLabel( col+15,row,HeaderColor,0,"Now" );
      DrawLabel( col+2,row,CounterColor,0,"/cnt" );
      DrawLabel( col+5+2,row,CounterColor,0,"/cnt" );
      DrawLabel( col+10+2,row,CounterColor,0,"/cnt" );
      DrawLabel( col+15+2,row,CounterColor,0,"/cnt" );
      m_baseY -= 24;
    }
    else if (m_displayQuantity == SUBSYSTEM_INFO)
    {
      DrawLabel( col,row,CounterColor,0,"Percent" );
      DrawLabel( col+4,row,HeaderColor,0,sValueName );
      DrawLabel( col+8,row,HeaderColor,0," Function" );
    }
    else
    {
      //		col = 45;
      DrawLabel( col-3.5f,row,CounterColor,0,"Count/" );
      DrawLabel( col+1,row,HeaderColor,0,sValueName );
			if(m_displayQuantity != STALL_TIME)
				DrawLabel( col+5,row,HeaderColor,0," Function" );
			else
				DrawLabel( col+5,row,HeaderColor,0," Function (Action)" );
    }



}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::RenderProfiler( CFrameProfiler *pProfiler,int level,float col,float row,bool bExtended,bool bSelected )
{
  char szText[128];
//  float HeaderColor[4] = { 1,1,0,1 };
  float ValueColor[4] = { 0,1,0,1 };
  float CounterColor[4] = { 0,0.8f,1,1 };
  float TextColor[4] = { 1,1,1,1 };



  float SelectedColor[4] = { 1,0,0,1 };

  float GraphColor[4] = { 1,0.3f,0,1 };

  float colTextOfs = 3.5f + 4*m_pSystem->IsDedicated();
  float colCountOfs = -4.5f - 3*m_pSystem->IsDedicated();
  float glow = 0;

	const char *sValueFormat = "%4.2f";
	if (m_bMemoryProfiling)
	{
		sValueFormat = "%6.f";
		colTextOfs += 2;
	}

  if (!bExtended)
  {
    col += level;
    // Simple info.
    float value = pProfiler->m_displayedValue;
    float variance = pProfiler->m_variance;
    CalculateColor( value,variance,ValueColor,glow );

    if (bSelected)
    {
      memcpy( ValueColor,SelectedColor,sizeof(ValueColor) );
      glow = 0;
    }
    else if (m_pGraphProfiler == pProfiler)
    {
      memcpy( ValueColor,GraphColor,sizeof(ValueColor) );
      glow = 0;
    }

    sprintf_s( szText, sValueFormat,value );
    DrawLabel( col,row,ValueColor,0,szText );

    int cave = pProfiler->m_countHistory.GetAverage();
    if (cave > 1)
    {
      sprintf_s( szText, "%6d/",cave );
      DrawLabel( col+colCountOfs,row,CounterColor,0,szText );
    }

    if (m_displayQuantity == TOTAL_TIME && pProfiler->m_bHaveChildren)
    {
      if (pProfiler->m_bExpended)
        strcpy( szText, "-" );
      else
        strcpy( szText, "+" );
    }
    else
      *szText = 0;
	  strcat(szText, GetFullName(pProfiler));
		if(m_displayQuantity == STALL_TIME && pProfiler->m_stallCause)
		{
			char buf[128];
			sprintf_s( buf, " (%s)",pProfiler->m_stallCause );
			strcat(szText, buf);
		}
    DrawLabel( col+colTextOfs,row,ValueColor,glow,szText );

    // If it is selected profile also render Min/Max values.
    if (m_pGraphProfiler == pProfiler)
    {
      float tmin=0,tmax=0;
      if (m_displayQuantity == TOTAL_TIME)
      {
        tmin = pProfiler->m_totalTimeHistory.GetMin();
        tmax = pProfiler->m_totalTimeHistory.GetMax();
      }
      else if (m_displayQuantity == SELF_TIME)
      {
        tmin = pProfiler->m_selfTimeHistory.GetMin();
        tmax = pProfiler->m_selfTimeHistory.GetMax();
      }
      else if (m_displayQuantity == COUNT_INFO)
      {
        tmin = (float)pProfiler->m_countHistory.GetMin();
        tmax = (float)pProfiler->m_countHistory.GetMax();
      }
      sprintf_s( szText, "min:%4.2f",tmin );
      DrawLabel( col+colTextOfs-11,row-0.4f,ValueColor,glow,szText,0.8f );
      sprintf_s( szText, "max:%4.2f",tmax );
      DrawLabel( col+colTextOfs-11,row+0.4f,ValueColor,glow,szText,0.8f );
    }
  }
  else
  {
    // Extended display.
    if (bSelected)
    {
      memcpy( ValueColor,SelectedColor,sizeof(ValueColor) );
      glow = 1;
    }

    float tmin,tmax,tave,tnow;
    int cmin,cmax,cave,cnow;
    if (m_displayQuantity == TOTAL_TIME_EXTENDED)
    {
      tmin = pProfiler->m_totalTimeHistory.GetMin();
      tmax = pProfiler->m_totalTimeHistory.GetMax();
      tave = pProfiler->m_totalTimeHistory.GetAverage();
      tnow = pProfiler->m_selfTimeHistory.GetAverage();
      //tnow = pProfiler->m_totalTimeHistory.GetLast();
    }
    else
    {
      tmin = pProfiler->m_selfTimeHistory.GetMin();
      tmax = pProfiler->m_selfTimeHistory.GetMax();
      tave = pProfiler->m_selfTimeHistory.GetAverage();
      tnow = pProfiler->m_selfTimeHistory.GetLast();
    }

    cmin = pProfiler->m_countHistory.GetMin();
    cmax = pProfiler->m_countHistory.GetMax();
    cave = pProfiler->m_countHistory.GetAverage();
    cnow = pProfiler->m_countHistory.GetLast();
    // Extensive info.
    sprintf_s( szText, sValueFormat,tmax );
    DrawLabel( col,row,ValueColor,0,szText );
    sprintf_s( szText, sValueFormat,tmin );
    DrawLabel( col+5,row,ValueColor,0,szText );
    sprintf_s( szText, sValueFormat,tave );
    DrawLabel( col+10,row,ValueColor,0,szText );
    sprintf_s( szText, sValueFormat,tnow );
    DrawLabel( col+15,row,ValueColor,0,szText );

    if (cmax > 1)
    {
      sprintf_s( szText, "/%d",cmax );
      DrawLabel( col+2,row,CounterColor,0,szText );
    }
    if (cmin > 1)
    {
      sprintf_s( szText, "/%d",cmin );
      DrawLabel( col+5+2,row,CounterColor,0,szText );
    }
    if (cave > 1)
    {
      sprintf_s( szText, "/%d",cave );
      DrawLabel( col+10+2,row,CounterColor,0,szText );
    }
    if (cnow > 1)
    {
      sprintf_s( szText, "/%d",cnow );
      DrawLabel( col+15+2,row,CounterColor,0,szText );
    }

    // Simple info.
//    float value = pProfiler->m_displayedValue;
//    float variance = pProfiler->m_variance;
    //CalculateColor( value,variance,TextColor,glow );

    if (pProfiler->m_bHaveChildren)
    {
      if (pProfiler->m_bExpended)
        strcpy( szText, "-" );
      else
        strcpy( szText, "+" );
    }
    else
      *szText = 0;
    strcat( szText, GetFullName(pProfiler) );

    DrawLabel( col+20+level,row,TextColor,glow,szText );

    //float x1 = (col-1)*COL_SIZE + 2;
    //float y1 = m_baseY + row*ROW_SIZE;
    //float x2 = x1 + ROW_SIZE;
    //float y2 = y1 + ROW_SIZE;
    //float half = ROW_SIZE/2.0f;
    //DrawRect( x1,y1+half,x2,y1+half+1,ValueColor );
    //DrawRect( x1+half,y1,x1+half+1,y2,ValueColor );
    //DrawRect( x1,y1,x2,y2,HeaderColor );
  }
}

//////////////////////////////////////////////////////////////////////////
float CFrameProfileSystem::RenderPeaks()
{
  char szText[128];
  float PeakColor[4] = { 1,1,1,1 };
  float HotPeakColor[4] = { 1,1,1,1 };
  float ColdPeakColor[4] = { 0.0f,0.0f,0.0f,0.0f };
  float PeakHeaderColor[4] = { 0,1,1,1 };
#if !defined(PS3)
  float PeakCounterColor[4] = { 0,0.8f,1,1 };
  float CounterColor[4] = { 0,0.8f,1,1 };
  float PageFaultsColor[4] = { 1,0.2f,1,1 };
#endif
	// changed from define to adjustable value
	float fHotToColdTime = GetPeakDisplayDuration();





  float colPeaks = 8.0f;


  float row = 0;
  float col = colPeaks;












  sprintf_s( szText,"Latest Peaks");

  DrawLabel( colPeaks,row++,PeakHeaderColor,0,szText );

  float currTimeSec = CFrameProfilerTimer::TicksToSeconds(m_totalProfileTime);








  std::vector<SPeakRecord>& rPeaks = m_peaks;

  // Go through all peaks.
  for (int i = 0; i < (int)rPeaks.size(); i++)
  {
    SPeakRecord &peak = rPeaks[i];

    float age = (currTimeSec-1.0f) - peak.when;
    float ageFactor = age / fHotToColdTime;
    if (ageFactor < 0) ageFactor = 0;
    if (ageFactor > 1) ageFactor = 1;
    for (int k = 0; k < 4; k++)
      PeakColor[k] = ColdPeakColor[k]*ageFactor + HotPeakColor[k]*(1.0f - ageFactor);

		if (!m_bMemoryProfiling)
			sprintf_s( szText, "%4.2fms",peak.peakValue );
		else
			sprintf_s( szText, "%6.f",peak.peakValue );
    DrawLabel( col,row,PeakColor,0,szText );
#if !defined(PS3)
    if (peak.count > 1)
    {
      for (int k = 0; k < 4; k++)
        PeakCounterColor[k] = CounterColor[k]*(1.0f - ageFactor);
      sprintf_s( szText, "%4d/",peak.count );
      DrawLabel( col-3,row,PeakCounterColor,0,szText );
    }
    if (peak.pageFaults > 0)
    {
      for (int k = 0; k < 4; k++)
        PeakCounterColor[k] = PageFaultsColor[k]*(1.0f - ageFactor);
      sprintf_s( szText, "(%4d)",peak.pageFaults );
      DrawLabel( col-6,row,PeakCounterColor,0,szText );
    }

    strcpy( szText, GetFullName(peak.pProfiler) );



#endif
    DrawLabel( col+5,row,PeakColor,0,szText );

    row += 1.0f;

    if (age > fHotToColdTime)
    {



      rPeaks.erase( m_peaks.begin()+i );

      i--;
    }
  }




	return row;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::DrawGraph()
{
  // Draw frametime graph.
  int h = m_pRenderer->GetHeight();
  int w = m_pRenderer->GetWidth();
  if (w != m_timeGraph.size())
    m_timeGraph.resize(w);

  int type = 2; // draw lines.
  float fValueScale = m_histogramScale/100.0f;
  float fTextScale = (255.0f/1000.0f)/fValueScale;





































  if (!m_bPageFaultsGraph)
  {



    ColorF graphColor(0,1,0,1);

    char *sFuncName = "FrameTime";
    if (m_pGraphProfiler)
      sFuncName = "";
    float value = 255.0f - (m_frameTimeHistory.GetLast()*fValueScale);

    if (value < 0) value = 0;
    if (value > 255) value = 255;

    m_timeGraph[m_timeGraphCurrentPos] = (unsigned char)value;
    if (m_displayQuantity != COUNT_INFO)
    {
      m_pRenderer->Graph( &m_timeGraph[0],0, h-280, w, 256, m_timeGraphCurrentPos,type,sFuncName,graphColor,fTextScale );
    }

    if (m_pGraphProfiler)
    {
      if (m_displayQuantity == TOTAL_TIME)
        value = 255.0f - (m_pGraphProfiler->m_totalTimeHistory.GetLast()*fValueScale);
      else if (m_displayQuantity == COUNT_INFO)
        value = 255.0f - (m_pGraphProfiler->m_countHistory.GetLast()*fValueScale);
      else
        value = 255.0f - (m_pGraphProfiler->m_selfTimeHistory.GetLast()*fValueScale);
      if (value < 0) value = 0;
      if (value > 255) value = 255;

      ColorF graphColor2(1,0.3f,0,1);
      sFuncName = const_cast<char*>((const char*)GetFullName(m_pGraphProfiler));

      if (w != m_timeGraph2.size())
        m_timeGraph2.resize(w);
      m_timeGraph2[m_timeGraphCurrentPos] = (unsigned char)value;
      m_pRenderer->Graph( &m_timeGraph2[0],0, h-280, w, 256, m_timeGraphCurrentPos,type,sFuncName,graphColor2,fTextScale );
    }
  }
  else
  {
    // Page Faults graph.
    char *sFuncName = "PageFaults";
    float value = 255.0f - (m_nPagesFaultsLastFrame*fValueScale*10.0f);

    ColorF graphColor(1,0,0,1);

    if (value < 0) value = 0;
    if (value > 255) value = 255;

    m_timeGraph[m_timeGraphCurrentPos] = (unsigned char)value;
    m_pRenderer->Graph( &m_timeGraph[0],0, h-280, w, 256, m_timeGraphCurrentPos,type,sFuncName,graphColor,fTextScale );
  }

  if (!m_bCollectionPaused)
  {





    m_timeGraphCurrentPos++;
    if (m_timeGraphCurrentPos >= (int)m_timeGraph.size()-1)
      m_timeGraphCurrentPos = 0;
  }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::RenderHistograms()
{
  ColorF HistColor( 0,1,0,1 );

  // Draw histograms.
  int h = m_pRenderer->GetHeight();
  int w = m_pRenderer->GetWidth();

  int graphStyle = 2; // histogram.

  float fScale = 1.0f; // histogram.

  m_pRenderer->SetMaterialColor( 1,1,1,1 );
  for (int i = 0; i < (int)m_displayedProfilers.size(); i++)
  {
    if (i > MAX_DISPLAY_ROWS)
    {
      break;
    }
    SProfilerDisplayInfo &dispInfo = m_displayedProfilers[i];
    CFrameProfilerGraph *pGraph = dispInfo.pProfiler->m_pGraph;
    if (!pGraph)
      continue;

    // Add a value to graph.
    pGraph->m_x = (int)(dispInfo.x + 40*COL_SIZE);
    pGraph->m_y = (int)(dispInfo.y);
    if (pGraph->m_y >= h)
      continue;
    // Render histogram.
    m_pRenderer->Graph( &pGraph->m_data[0],pGraph->m_x,pGraph->m_y,pGraph->m_width,pGraph->m_height,m_histogramsCurrPos,graphStyle,0,HistColor,fScale );
  }
  if (!m_bCollectionPaused)
  {
    m_histogramsCurrPos++;
    if (m_histogramsCurrPos >= m_histogramsMaxPos)
      m_histogramsCurrPos = 0;
  }
}

void CFrameProfileSystem::RenderSubSystems( float col,float row )
{
  char szText[128];
  float HeaderColor[4] = { 1,1,0,1 };
  float ValueColor[4] = { 0,1,0,1 };
  float CounterColor[4] = { 0,0.8f,1,1 };

  float colPercOfs = -3.0f;
  float colTextOfs = 4.0f;
	colTextOfs = 9.0f;

  int height = m_pRenderer->GetHeight();


  m_baseY += 40;

  float frameTime = m_frameTimeHistory.GetAverage();
  float onePercent = frameTime / 100.0f; // One Percent of frame time.

  // Go through all profilers.
  for (int i = 0; i < PROFILE_LAST_SUBSYSTEM; i++)
  {
    // Simple info.
    float value = m_subsystems[i].selfTime;
    m_subsystems[i].selfTime = 0;
    const char *sName = m_subsystems[i].name;
    if (!sName)
      continue;
    
		//sprintf_s( szText, "%4.2fms %4.2fms",value, maxVal);
		if (!m_bMemoryProfiling)
		{
			sprintf_s( szText, "%4.2fms",value );
		}
		else
		{
			sprintf_s( szText, "%6.0f",value );
		}

		if ( value > m_subsystems[i].budgetTime )
		{
			DrawLabel( col,row,HeaderColor,0,szText );
			DrawLabel( col+colTextOfs,row,HeaderColor,0,sName );
		}
		else
		{
			DrawLabel( col,row,ValueColor,0,szText );
			DrawLabel( col+colTextOfs,row,ValueColor,0,sName );
		}

    if(onePercent!=0)
    {
      int percent = FtoI(value / onePercent);
      sprintf_s( szText, "%2d%%",percent );
      DrawLabel( col+colPercOfs,row,CounterColor,0,szText );
    }

    row += 1.0f;
  }
  m_baseY -= 40;
}

#if defined(WIN32) || defined(XENON)
#pragma pack(push,1)
const struct PEHeader
{
  DWORD signature;
  IMAGE_FILE_HEADER _head;
  IMAGE_OPTIONAL_HEADER opt_head;
  IMAGE_SECTION_HEADER *section_header;  // actual number in NumberOfSections
};
#pragma pack(pop)
#endif 

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::RenderMemoryInfo()
{

#if defined(WIN32) || defined(XENON) || defined(LINUX) || defined(PS3)

  m_pRenderer = gEnv->pRenderer;
  if (!m_pRenderer)
    return;






  m_baseY = 0;
  m_textModeBaseExtra = -1;
  ROW_SIZE = 11;
  COL_SIZE = 11;

  float col = 1;
  float row = 1;

  float HeaderColor[4] = { 0.3f,1,1,1 };
  float ModuleColor[4] = { 1,1,1,1 };
  float StaticColor[4] = { 1,1,1,1 };
  float NumColor[4] = { 1,0,1,1 };
  float TotalColor[4] = { 1,1,1,1 };

  char szText[128];
  float fLabelScale = 1.1f;

  ILog *pLog = gEnv->pLog;
  //////////////////////////////////////////////////////////////////////////
  // Show memory usage.
  //////////////////////////////////////////////////////////////////////////
  int memUsage = 0;//CryMemoryGetAllocatedSize();
  int64 totalAll = 0;
  int luaMemUsage = gEnv->pScriptSystem->GetScriptAllocSize();

  row++; // reserve for static.
  row++;
  row++;

  sprintf_s( szText,"Lua Allocated Memory: %d KB",luaMemUsage/1024 );
  DrawLabel( col,row++,HeaderColor,0,szText,fLabelScale );




  if (m_bLogMemoryInfo) pLog->Log( szText );
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  float col1 = col + 15;
  float col2 = col1 + 8;
#if defined(PS3) || defined(LINUX)
  float col3 = col2;
#else
  float col3 = col2 + 7;
#endif
  float col4 = col3 + 7;
















  DrawLabel( col,row++,HeaderColor,0,"-----------------------------------------------------------------------------------------------------------------------------------",fLabelScale );
  DrawLabel( col,row,HeaderColor,0,"Module",fLabelScale );
  DrawLabel( col1,row,HeaderColor,0,"Dynamic(KB)",fLabelScale );
#if !defined(PS3) && !defined(LINUX)
  DrawLabel( col2,row,HeaderColor,0,"Static(KB)",fLabelScale );
#endif
  DrawLabel( col3,row,HeaderColor,0,"Num Allocs",fLabelScale );
  DrawLabel( col4,row,HeaderColor,0,"Total Allocs(KB)",fLabelScale );
#if !defined(PS3)
  float col5 = col4 + 10;
  DrawLabel( col5,row,HeaderColor,0,"Total Wasted(KB)",fLabelScale );
  int totalUsedInModulesStatic = 0;
#endif
  row++;

  int totalUsedInModules = 0;
  int countedMemoryModules = 0;
  uint64 totalAllocatedInModules = 0;
  int totalNumAllocsInModules = 0;

	const std::vector<const char*>& szModules = GetModuleNames();
	const int numModules = szModules.size();

  for (int i = 0; i < numModules; i++)
  {
#if !defined(PS3) && !defined(LINUX)
    PEHeader pe_header;
    PEHeader *header;
#endif

    CryModuleMemoryInfo memInfo;
    memset( &memInfo,0,sizeof(memInfo) );

    uint32 moduleStaticSize = 0;
#ifndef _LIB
    const char *szModule = szModules[i];
#ifdef LINUX
		char path[_MAX_PATH];
		sprintf_s(path, "./%s", szModule);
		HMODULE hModule = dlopen(path, RTLD_LAZY | RTLD_NOLOAD);
#else
    HMODULE hModule = GetModuleHandle( szModule );
#endif
    if (!hModule) {



        continue;
    } else {

      //totalStatic += me.modBaseSize;
      typedef void (*PFN_MODULEMEMORY)( CryModuleMemoryInfo* );
#ifdef LINUX
			PFN_MODULEMEMORY fpCryModuleGetAllocatedMemory = (PFN_MODULEMEMORY)dlsym(hModule, "CryModuleGetMemoryInfo");
#else
#ifndef XENON
      PFN_MODULEMEMORY fpCryModuleGetAllocatedMemory = (PFN_MODULEMEMORY)::GetProcAddress( hModule,"CryModuleGetMemoryInfo" );


#endif
#endif // LINUX
      if (!fpCryModuleGetAllocatedMemory)
        continue;
#else // _LIB
    if (i == eCryM_Num)
      break;
    const char *szModule = CM_Name[i];
    typedef void (*PFN_MODULEMEMORY)( CryModuleMemoryInfo*, ECryModule );
    PFN_MODULEMEMORY fpCryModuleGetAllocatedMemory = &CryModuleGetMemoryInfo;
#endif


#if !defined(PS3) && !defined(LINUX)

    header = &pe_header;























#if defined(WIN32)
    const IMAGE_DOS_HEADER *dos_head = (IMAGE_DOS_HEADER*)hModule;
    if (dos_head->e_magic != IMAGE_DOS_SIGNATURE)
    {
      // Wrong pointer, not to PE header.
      continue;
    }
    header = (PEHeader*)(const void *)((char *)dos_head + dos_head->e_lfanew);
#endif
#endif//PS3

#ifndef _LIB
    fpCryModuleGetAllocatedMemory( &memInfo );
  }
#else









    fpCryModuleGetAllocatedMemory( &memInfo, (ECryModule)i );

#endif

		int usedInModule = (int)(memInfo.allocated - memInfo.freed);
#ifndef _LIB

#if !defined(PS3) && !defined(LINUX)
    PREFAST_SUPPRESS_WARNING(6001) moduleStaticSize = header->opt_head.SizeOfInitializedData + header->opt_head.SizeOfUninitializedData + header->opt_head.SizeOfCode + header->opt_head.SizeOfHeaders;
#else
		moduleStaticSize = 0;
#endif
    if (numModules - 1 == i) 
    {





      usedInModule = 46 * 1024 * 1024; // PS3
			moduleStaticSize = 0;
    } 


#endif

    totalNumAllocsInModules += memInfo.num_allocations;
    totalAllocatedInModules += memInfo.allocated;
    totalUsedInModules += usedInModule;
    countedMemoryModules++;
    memUsage += usedInModule + moduleStaticSize;
#if !defined(PS3) && !defined(LINUX)
    totalUsedInModulesStatic += moduleStaticSize;
#endif
    sprintf_s( szText,"%s",szModule );
    DrawLabel(col, row, ModuleColor, 0, szText, fLabelScale );
    sprintf_s( szText,"%5.2f",usedInModule/1024.0f/1024.0f );
    DrawLabel( col1,row,StaticColor,0,szText,fLabelScale );
#if !defined(PS3) && !defined(LINUX)
    sprintf_s( szText,"%d",moduleStaticSize/1024 );
    DrawLabel( col2,row,StaticColor,0,szText,fLabelScale );
#endif
    sprintf_s( szText,"%d",memInfo.num_allocations );
    DrawLabel( col3,row,NumColor,0,szText,fLabelScale );
#if defined(PS3) || defined(LINUX)
    sprintf_s( szText,"%lld",memInfo.allocated/1024 );
#else
    sprintf_s( szText,"%I64d",memInfo.allocated/1024 );
#endif
    DrawLabel( col4,row,TotalColor,0,szText,fLabelScale );
#if !defined(PS3) && !defined(LINUX)
    sprintf_s( szText,"%I64d",(memInfo.allocated - memInfo.requested)/1024 );
    DrawLabel( col5,row,TotalColor,0,szText,fLabelScale );
    if (m_bLogMemoryInfo)
    {
      pLog->Log( "    %20s | Alloc: %6d Kb  |  Num: %7d  |  TotalAlloc: %8I64d KB  | StaticTotal: %6d KB  | Code: %6d KB |  Init. Data: %6d KB  |  Uninit. Data: %6d KB | %6d | %6d/%6d",
        szModule,
        usedInModule/1024,memInfo.num_allocations,memInfo.allocated/1024,
        moduleStaticSize/1024,
        header->opt_head.SizeOfCode/1024,
        header->opt_head.SizeOfInitializedData/1024,
        header->opt_head.SizeOfUninitializedData/1024,
        (uint32)memInfo.CryString_allocated/1024,(uint32)memInfo.STL_allocated/1024,(uint32)memInfo.STL_wasted/1024);
    }
#else
	if (m_bLogMemoryInfo)
  {
		pLog->Log( "    %20s | Alloc: %6d Kb  |  Num: %7d  |  TotalAlloc: %"PRIu64" KB",
        szModule,
        usedInModule/1024,memInfo.num_allocations,memInfo.allocated/1024);
   }
#endif
    row++;
  }

  DrawLabel( col,row++,HeaderColor,0,"-----------------------------------------------------------------------------------------------------------------------------------",fLabelScale );
  sprintf_s( szText,"Sum %d Modules",countedMemoryModules );
  DrawLabel( col,row,HeaderColor,0,szText,fLabelScale );
  sprintf_s( szText,"%d",totalUsedInModules/1024 );
  DrawLabel( col1,row,HeaderColor,0,szText,fLabelScale );
#if !defined(PS3) && !defined(LINUX)
  sprintf_s( szText,"%d",totalUsedInModulesStatic/1024 );
  DrawLabel( col2,row,StaticColor,0,szText,fLabelScale );
#endif
  sprintf_s( szText,"%d",totalNumAllocsInModules );
  DrawLabel( col3,row,NumColor,0,szText,fLabelScale );
#if defined(PS3) || defined(LINUX)
  sprintf_s( szText,"%lld",totalAllocatedInModules/1024 );
#else
  sprintf_s( szText,"%I64d",totalAllocatedInModules/1024 );
#endif
  DrawLabel( col4,row,TotalColor,0,szText,fLabelScale );
  row++;

#ifdef WIN32
  //	col = 0;;


  PROCESS_MEMORY_COUNTERS ProcessMemoryCounters = { sizeof(ProcessMemoryCounters) };
  GetProcessMemoryInfo(GetCurrentProcess(), &ProcessMemoryCounters, sizeof(ProcessMemoryCounters));
  SIZE_T WorkingSetSize = ProcessMemoryCounters.WorkingSetSize;
  SIZE_T QuotaPagedPoolUsage = ProcessMemoryCounters.QuotaPagedPoolUsage;
  SIZE_T PagefileUsage = ProcessMemoryCounters.PagefileUsage;
  SIZE_T PageFaultCount = ProcessMemoryCounters.PageFaultCount;

  sprintf_s(szText, "WindowsInfo: PagefileUsage: %u WorkingSetSize: %u, QuotaPagedPoolUsage: %u PageFaultCount: %u\n", 
    (unsigned)PagefileUsage / 1024,
    (unsigned)WorkingSetSize / 1024,
    (unsigned)QuotaPagedPoolUsage / 1024,
    (unsigned) PageFaultCount);

  DrawLabel( col,row++,HeaderColor,0,"-----------------------------------------------------------------------------------------------------------------------------------",fLabelScale );
  //	sprintf_s( szText,"WindowsInfo",countedMemoryModules );
  DrawLabel( col,row,HeaderColor,0,szText,fLabelScale );
#endif
#if !defined(PS3) && !defined(LINUX)
  if (m_bLogMemoryInfo)
    pLog->Log( "Sum of %d Modules %6d Kb  (Static: %6d Kb)  (Num: %8d) (TotalAlloc: %8I64d KB)",countedMemoryModules,totalUsedInModules/1024,
    totalUsedInModulesStatic/1024,totalNumAllocsInModules,totalAllocatedInModules/1024 );
#endif

  int memUsageInMB_SysCopyMeshes = ( *( (int*) m_pRenderer->EF_Query( EFQ_Alloc_APIMesh ) ) );
  int memUsageInMB_SysCopyTextures = ( *( (int*) m_pRenderer->EF_Query( EFQ_Alloc_APITextures ) ) );
#if !defined(PS3)
  totalAll += memUsage;
  totalAll += memUsageInMB_SysCopyMeshes;
  totalAll += memUsageInMB_SysCopyTextures;
#endif












  sprintf_s( szText,"Total Allocated Memory: %" PRId64" KB (DirectX Textures: %d KB, VB: %d Kb)",totalAll/1024,memUsageInMB_SysCopyTextures/1024,memUsageInMB_SysCopyMeshes/1024 );

  DrawLabel( col,1,HeaderColor,0,szText,fLabelScale );


































  m_bLogMemoryInfo = false;

#endif //#if defined(WIN32) || defined(XENON)
  //#endif
}
#undef VARIANCE_MULTIPLIER

#endif // USE_FRAME_PROFILER




