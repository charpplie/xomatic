/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   dag_grid.cpp
$Id$
$DateTime$
Description: This helper class stores the dag information as a grid
to simplify the loading process and later debug visualization.
---------------------------------------------------------------------
History:
- 03:06:2007 : Created by Ricardo Pillosu

*********************************************************************/
#include "StdAfx.h"
#include "dag_grid.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CDagGrid::CDagGrid()
{
};

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CDagGrid::~CDagGrid()
{
  Free();
};

// Description:
//   Constructor
// Arguments:
//
// Return:
//
void CDagGrid::Allocate( uint32 uRows, uint32 uColumns )
{
  Free();

  m_Grid.resize( uRows );
  for( uint32 uRow = 0; uRow < uRows; ++uRow )
  {
    m_Grid[uRow].resize( uColumns, NULL );
  }
};

// Description:
//   Constructor
// Arguments:
//
// Return:
//
void CDagGrid::Free()
{
  for( uint32 uIndex = 0; uIndex < m_Grid.size(); ++uIndex )
  {
    m_Grid[uIndex].clear();
  }

  m_Grid.clear();
};

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CDagNode* CDagGrid::Get( uint32 uRow, uint32 uCol )
{
  return( m_Grid[uRow][uCol] );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
void CDagGrid::Set( uint32 uRow, uint32 uCol, CDagNode* pNode )
{
  m_Grid[uRow][uCol] = pNode;
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
uint32 CDagGrid::GetRows() const
{
  return( m_Grid.size() );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CDagNode const* CDagGrid::Get( uint32 uRow, uint32 uCol ) const
{
  return( m_Grid[uRow][uCol] );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
uint32 CDagGrid::GetColumns() const
{
  return( m_Grid[0].size() );
}
