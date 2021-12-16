///********************************************************************
//CryGame Source File.
//Copyright (C), StalTech Studios, 2001-2004.
//-------------------------------------------------------------------------
//File name:   GenericTreeNode.h
//Version:     v0.1
//Description: 
//
//-------------------------------------------------------------------------
//History:
//- 7:7:2008   Created by Mieszko Zielinski
//
//*********************************************************************/
#ifndef GENERICTREENODE_H
#define GENERICTREENODE_H

#include <vector>

template< typename CustomDataType >
class TGenericTreeNode
{
	typedef CustomDataType						tData;
	typedef TGenericTreeNode< tData >	tSelf;
	
public:

	typedef std::vector< tSelf* >					tChildrenContainer;
	typedef typename tChildrenContainer::iterator	tChildrenIterator;
	typedef typename tChildrenContainer::const_iterator	tChildrenConstIterator;

	// ----------------------------------------------------------------
	// methods
	// ----------------------------------------------------------------

	TGenericTreeNode<CustomDataType>();
	
	void	Reset();	// removes all children and clears data

	void	SetData( const tData& data );
	tData& GetData();
	const tData& GetData() const;

	bool	PresetChildrenCount( int nCount );	// resizes vector containing children. Mainly for efficiency.
	uint32	GetChildrenCount() const;	
	void	AddChild( tSelf* pChild );

	tSelf*				operator[]( int nIndex );
	tSelf*				GetChild( int nIndex );
	const tSelf*	GetChild( int nIndex ) const;
	
	tChildrenIterator				GetChildrenIterator();
	tChildrenConstIterator		GetChildrenIterator() const;
	tChildrenIterator				GetChildrenEndIterator();
	tChildrenConstIterator		GetChildrenEndIterator() const;

	tSelf*	GetFirstChild();							// a shorthand for getting data from children straight away.
	const tSelf*	GetFirstChild() const;	// very useful when node has only one child

	//tSelf GetChild()

protected:

	// ----------------------------------------------------------------
	// member variables
	// ----------------------------------------------------------------
	uint32								m_nChildrenCount;
	tData								m_data;
	tChildrenContainer	m_children;
	

};

//*----------------------------------------------------------------
//	Method implementations
//----------------------------------------------------------------*/

template< typename CustomDataType >
TGenericTreeNode<CustomDataType>::TGenericTreeNode()
: m_nChildrenCount( 0 )
{

}

template< typename CustomDataType >
inline void TGenericTreeNode<CustomDataType>::SetData( const tData& data )
{
	m_data = data;
}

template< typename CustomDataType >
inline CustomDataType& TGenericTreeNode<CustomDataType>::GetData()
{
	return m_data;
}

template< typename CustomDataType >
inline const CustomDataType& TGenericTreeNode<CustomDataType>::GetData() const
{
	return m_data;
}

template< typename CustomDataType >
inline bool TGenericTreeNode<CustomDataType>::PresetChildrenCount( int nCount )
{
	assert( m_children.size() == 0 && "Trying to change previously set number of children!" );
	bool bSuccess = m_children.size() == 0;

	if( bSuccess )
	{
		m_children.resize( nCount, NULL );
	}
	
	return bSuccess;
}

template< typename CustomDataType >
inline uint32 TGenericTreeNode<CustomDataType>::GetChildrenCount() const
{
	return m_nChildrenCount;
}

template< typename CustomDataType >
inline void TGenericTreeNode<CustomDataType>::AddChild( tSelf* pChild )
{
	assert( m_nChildrenCount < m_children.size() && "Trying to add more children than declared!" );

	if( m_nChildrenCount < m_children.size() )
	{
		m_children[ m_nChildrenCount ] = pChild;

		++m_nChildrenCount;
	}
}

template< typename CustomDataType >
inline void TGenericTreeNode<CustomDataType>::Reset()
{
	m_children.resize(0);
	m_nChildrenCount = 0;
	m_data.Reset();	
}

template< typename CustomDataType >
inline TGenericTreeNode<CustomDataType>* TGenericTreeNode<CustomDataType>::operator[]( int nIndex )
{
	return m_children[ nIndex ];
}

template< typename CustomDataType >
inline TGenericTreeNode<CustomDataType>* TGenericTreeNode<CustomDataType>::GetChild( int nIndex )
{
	return m_children[ nIndex ];
}

template< typename CustomDataType >
inline const TGenericTreeNode<CustomDataType>* TGenericTreeNode<CustomDataType>::GetChild( int nIndex ) const
{
	return m_children[ nIndex ];
}


template< typename CustomDataType >
typename TGenericTreeNode<CustomDataType>::tChildrenIterator
inline TGenericTreeNode<CustomDataType>::GetChildrenIterator()
{
	// failing this assert will result in getting iterator that will get invalidated
	// as soon as PresetChildrenCount will be called
	assert( m_children.size() > 0 && "Trying to obtain iterator to children collection before setting number of children!" );
	
	return m_children.begin();
}

template< typename CustomDataType >
typename TGenericTreeNode<CustomDataType>::tChildrenConstIterator
inline TGenericTreeNode<CustomDataType>::GetChildrenIterator() const
{
	// failing this assert will result in getting iterator that will get invalidated
	// as soon as PresetChildrenCount will be called
	assert( m_children.size() > 0 && "Trying to obtain iterator to children collection before setting number of children!" );

	return m_children.begin();
}

template< typename CustomDataType >
typename TGenericTreeNode<CustomDataType>::tChildrenIterator
inline TGenericTreeNode<CustomDataType>::GetChildrenEndIterator() 
{
	// failing this assert will result in getting iterator that will get invalidated
	// as soon as PresetChildrenCount will be called
	assert( m_children.size() > 0 && "Trying to obtain iterator to children collection before setting number of children!" );

	return m_children.begin() + m_nChildrenCount;
}

template< typename CustomDataType >
typename TGenericTreeNode<CustomDataType>::tChildrenConstIterator
inline TGenericTreeNode<CustomDataType>::GetChildrenEndIterator() const
{
	// failing this assert will result in getting iterator that will get invalidated
	// as soon as PresetChildrenCount will be called
	assert( m_children.size() > 0 && "Trying to obtain iterator to children collection before setting number of children!" );

	return m_children.begin() + m_nChildrenCount;
}

template< typename CustomDataType >
inline TGenericTreeNode<CustomDataType>* TGenericTreeNode<CustomDataType>::GetFirstChild()
{
	assert( m_nChildrenCount > 0 && "This tree node has no children!" );
	return m_children.front();
}

template< typename CustomDataType >
inline const TGenericTreeNode<CustomDataType>* TGenericTreeNode<CustomDataType>::GetFirstChild() const
{
	assert( m_nChildrenCount > 0 && "This tree node has no children!" );
	return m_children.front();
}

#endif //GENERICTREENODE_H