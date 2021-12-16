/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   dag_grid.h
$Id$
$DateTime$
Description: This helper class stores the dag information as a grid
to simplify the loading process and later debug visualization.
---------------------------------------------------------------------
History:
- 03:06:2007 : Created by Ricardo Pillosu

*********************************************************************/
#ifndef _DAG_GRID_H_
  #define _DAG_GRID_H_

class CDagNode;

class CDagGrid
{

  public:

    /*$1- Basics -------------------------------------------------------------*/
    CDagGrid();
    ~ CDagGrid();

    /*$1- Utils --------------------------------------------------------------*/
    void              Allocate( uint32 uRows, uint32 uColumns );
    void              Free();
    CDagNode*         Get( uint32 uRow, uint32 uCol );
    CDagNode const*   Get( uint32 uRow, uint32 uCol ) const;
    void              Set( uint32 uRow, uint32 uCol, CDagNode* pNode );
    uint32              GetRows() const;
    uint32              GetColumns() const;

  private:

    /*$1- Members ------------------------------------------------------------*/
    typedef std::vector<CDagNode *> GridRow;
    std::vector<GridRow>            m_Grid;
};
#endif // _DAG_GRID_H_
