////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ixml.h
//  Version:     v1.00
//  Created:     16/7/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include DEVIRTUALIZE_HEADER_FIX(IXml.h)

#ifndef __ixml_h__
#define __ixml_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include <platform.h>
#include <Cry_Math.h>
#include <Cry_Color.h>

#ifdef  SANDBOX_API
#include "Util\GuidUtil.h"
#endif //SANDBOX_API

class IXMLBinarySerializer;
struct IReadWriteXMLSink;
struct ISerialize;

/*

This is wrapper arround expat library to provide DOM type of access for xml.
Do not use IXmlNode class directly instead always use XmlNodeRef wrapper that
takes care of memory managment issues.

Usage Example:
-------------------------------------------------------
void testXml()
{
XmlParser xml;
XmlNodeRef root = xml.parse( "test.xml" );

if (root) {
for (int i = 0; i < root->getChildCount(); i++) {
XmlNodeRef child = root->getChild(i);
if (child->isTag("world")) {
if (child->getAttr("name") == "blah") {
}
}
}
}
};
*/

// Summary:
//	 Special string wrapper for xml nodes.
class XmlString : public string
{
public:
	XmlString() {};
	XmlString( const char *str ) : string(str) {};
#ifdef  SANDBOX_API
	XmlString( const CString &str ) : string( (const char*)str ) {};
#endif // SANDBOX_API

	operator const char*() const { return c_str(); }
};

// Summary:
//	 XML string data.
UNIQUE_IFACE struct IXmlStringData
{
	virtual void AddRef() = 0;
	virtual void Release() = 0;
	virtual const char* GetString() = 0;
	virtual size_t      GetStringLength() = 0;
};

class IXmlNode;

// Summary:
//	 XmlNodeRef, wrapper class implementing reference counting for IXmlNode.
// See also:
//	 IXmlNode
class XmlNodeRef {
private:
	IXmlNode* p;
public:
	XmlNodeRef() : p(NULL) {}
	explicit XmlNodeRef( int Null ) : p(NULL) {}
	XmlNodeRef( IXmlNode* p_ );
	XmlNodeRef( const XmlNodeRef &p_ );
	//explicit XmlNodeRef( const char *tag,IXmlNode *node );
	~XmlNodeRef();

	operator IXmlNode*() const { return p; }
	operator const IXmlNode*() const { return p; }
	IXmlNode& operator*() const { return *p; }
	IXmlNode* operator->(void) const { return p; }

	XmlNodeRef&  operator=( IXmlNode* newp );
	XmlNodeRef&  operator=( const XmlNodeRef &newp );

	operator bool() const { return p != NULL; };
	bool operator !() const { return p == NULL; };


	// Misc compare functions.
	//##@{
	bool  operator == ( const IXmlNode* p2 ) const { return p == p2; };
	bool  operator == ( IXmlNode* p2 ) const { return p == p2; };
	bool  operator != ( const IXmlNode* p2 ) const { return p != p2; };
	bool  operator != ( IXmlNode* p2 ) const { return p != p2; };
	bool  operator <  ( const IXmlNode* p2 ) const { return p < p2; };
	bool  operator >  ( const IXmlNode* p2 ) const { return p > p2; };

	bool  operator == ( const XmlNodeRef &n ) const { return p == n.p; };
	bool  operator != ( const XmlNodeRef &n ) const { return p != n.p; };
	bool  operator <  ( const XmlNodeRef &n ) const { return p < n.p; };
	bool  operator >  ( const XmlNodeRef &n ) const { return p > n.p; };

	friend bool operator == ( const XmlNodeRef &p1,int null );
	friend bool operator != ( const XmlNodeRef &p1,int null );
	friend bool operator == ( int null,const XmlNodeRef &p1 );
	friend bool operator != ( int null,const XmlNodeRef &p1 );
	//##@}
};

// Summary:
//	 IXmlNode class
// Notes:
//	 Never use IXmlNode directly instead use reference counted XmlNodeRef.
// See also:
//	 XmlNodeRef
class IXmlNode
{
protected:
	int m_nRefCount;

protected:
	virtual void DeleteThis() = 0;
	virtual ~IXmlNode() {};

public:
	// Summary:
	//	 Creates new XML node.
	virtual XmlNodeRef createNode( const char *tag ) = 0;

	// Notes:
	// AddRef/Release need to be virtual to permit overloading from CXMLNodePool

	// Summary:
	//	 Reference counting.
	virtual void AddRef() { m_nRefCount++; };
	// Notes:
	//	 When ref count reach zero XML node dies.
	virtual void Release() { if (--m_nRefCount <= 0) DeleteThis(); };

	// Summary:
	//	 Gets XML node tag.
	virtual const char *getTag() const = 0;
	// Summary:
	//	 Sets XML node tag.
	virtual void	setTag( const char *tag ) = 0;

	// Summary:
	//	 Returns true if a given tag equal to node tag.
	virtual bool isTag( const char *tag ) const = 0;

	// Summary:
	//	 Gets XML Node attributes.
	virtual int getNumAttributes() const = 0;
	// Summary:
	//	 Returns attribute key and value by attribute index.
	virtual bool getAttributeByIndex( int index,const char **key,const char **value ) = 0;

	// Summary:
	//	 Copies attributes to this node from a given node.
	virtual void copyAttributes( XmlNodeRef fromNode ) = 0;

	// Summary:
	//	 Gets XML Node attribute for specified key.
	virtual const char* getAttr( const char *key ) const = 0;

   // Summary:
	//  Gets XML Node attribute for specified key.
   // Return Value:
	//  True if the attribute exists, false otherwise.
	virtual bool getAttr(const char *key, const char **value) const = 0;

   // Summary:
	//  Checks if attributes with specified key exist.
	virtual bool haveAttr( const char *key ) const = 0;

	// Summary:
	//	 Adds new child node.
	virtual void addChild( const XmlNodeRef &node ) = 0;

	// Summary:
	//	 Creates new xml node and add it to childs list.
	virtual XmlNodeRef newChild( const char *tagName ) = 0;

	// Summary:
	//	 Removes child node.
	virtual void removeChild( const XmlNodeRef &node ) = 0;

	// Summary:
	//	 Removes all child nodes.
	virtual void removeAllChilds() = 0;

	// Summary:
	//	 Removes child node at known position.
	virtual void deleteChildAt( int nIndex ) = 0;

	// Summary:
	//	 Gets number of child XML nodes.
	virtual int	getChildCount() const = 0;

	// Summary:
	//	 Gets XML Node child nodes.
	virtual XmlNodeRef getChild( int i ) const = 0;

	// Summary:
	//	 Finds node with specified tag.
	virtual XmlNodeRef findChild( const char *tag ) const = 0;

	// Summary:
	//	 Gets parent XML node.
	virtual XmlNodeRef getParent() const = 0;

	// Summary:
	//	 Returns content of this node.
	virtual const char* getContent() const = 0;
	// Summary:
	//	 Sets content of this node.
	virtual void setContent( const char *str ) = 0;

	// Summary:
	//	 Deep clone of this and all child xml nodes.
	virtual XmlNodeRef clone() = 0;

	// Summary:
	//	 Returns line number for XML tag.
	virtual int getLine() const = 0;
	// Summary:
	//	 Set line number in xml.
	virtual void setLine( int line ) = 0;

	// Summary:
	//	 Returns XML of this node and sub nodes.
	// Notes:
	//	 IXmlStringData pointer must be release when string is not needed anymore.
	// See also:
	//	 IXmlStringData
	virtual IXmlStringData* getXMLData( int nReserveMem=0 ) const = 0;
	// Summary:
	//	 Returns XML of this node and sub nodes.
	virtual XmlString getXML( int level=0 ) const = 0;
	virtual bool saveToFile( const char *fileName ) = 0;
	// Notes:
	//	 Save in small memory chunks.
	virtual bool saveToFile( const char *fileName, size_t chunkSizeBytes, FILE *file = NULL) = 0; 

	// Summary:
	//	 Sets new XML Node attribute (or override attribute with same key).
	//##@{
	virtual void setAttr( const char* key,const char* value ) = 0;
	virtual void setAttr( const char* key,int value ) = 0;
	virtual void setAttr( const char* key,unsigned int value ) = 0;
	virtual void setAttr( const char* key,int64 value ) = 0;
	virtual void setAttr( const char* key,uint64 value,bool useHexFormat = true ) = 0;
	virtual void setAttr( const char* key,float value ) = 0;
	virtual void setAttr( const char* key,const Vec2& value ) = 0;
	virtual void setAttr( const char* key,const Ang3& value ) = 0;
	virtual void setAttr( const char* key,const Vec3& value ) = 0;
	virtual void setAttr( const char* key,const Quat &value ) = 0;
	//##@}

	// Summary:
	//	 Inline Helpers.
	//##@{
#ifndef LINUX64
	void setAttr( const char* key,unsigned long value ) { setAttr( key,(unsigned int)value ); };
	void setAttr( const char* key,long value ) { setAttr( key,(int)value ); };
#endif
	void setAttr( const char* key,double value ) { setAttr( key,(float)value ); };
	//##@}

	// Summary:
	//	 Deletes attribute.
	virtual void delAttr( const char* key ) = 0;
	// Summary:
	//	 Removes all node attributes.
	virtual void removeAllAttributes() = 0;

	// Summary:
	//	 Gets attribute value of node.
	//##@{
	virtual bool getAttr( const char *key,int &value ) const = 0;
	virtual bool getAttr( const char *key,unsigned int &value ) const = 0;
	virtual bool getAttr( const char *key,int64 &value ) const = 0;
	virtual bool getAttr( const char *key,uint64 &value,bool useHexFormat = true ) const = 0;
	virtual bool getAttr( const char *key,float &value ) const = 0;
	virtual bool getAttr( const char *key,Vec2& value ) const = 0;
	virtual bool getAttr( const char *key,Ang3& value ) const = 0;
	virtual bool getAttr( const char *key,Vec3& value ) const = 0;
	virtual bool getAttr( const char *key,Quat &value ) const = 0;
	virtual bool getAttr( const char *key,bool &value ) const = 0;
	virtual bool getAttr( const char *key,XmlString &value ) const = 0;
	virtual bool getAttr( const char *key,ColorB &value ) const = 0;

	//##@}

	// Summary:
	//	 Inline Helpers.
	//##@{
#ifndef LINUX64
	bool getAttr( const char *key,long &value ) const { int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool getAttr( const char *key,unsigned long &value ) const { unsigned int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
#endif
	bool getAttr( const char *key,unsigned short &value ) const { unsigned int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool getAttr( const char *key,unsigned char &value ) const { unsigned int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool getAttr( const char *key,short &value ) const { int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool getAttr( const char *key,char &value ) const { int v; if (getAttr(key,v)) { value = v; return true; } else return false; }
	bool getAttr( const char *key,double &value ) const { float v; if (getAttr(key,v)) { value = (double)v; return true; } else return false; }
	//##@}

#ifdef  SANDBOX_API
	// Summary:
	//	 Gets CString attribute.
	bool getAttr( const char *key,CString &value ) const
	{
		if (!haveAttr(key))
			return false;
		value = getAttr(key);
		return true;
	}

	// Summary:
	//	 Sets GUID attribute.
	void setAttr( const char* key,REFGUID value )
	{
		const char *str = GuidUtil::ToString(value);
		setAttr( key,str );
	};

	// Summary:
	//	 Gets GUID from attribute.
	bool getAttr( const char *key,GUID &value ) const
	{
		if (!haveAttr(key))
			return false;
		const char *guidStr = getAttr(key);
		value = GuidUtil::FromString( guidStr );
		if (value.Data1 == 0)
		{
			memset( &value,0,sizeof(value) );
			// If bad GUID, use old guid system.
			value.Data1 = atoi(guidStr);
		}
		return true;
	}
#endif //SANDBOX_API

	// Summary:
	//	 Lets be friendly to him.
	friend class XmlNodeRef;
};

/*
// Summary:
//	 Inline Implementation of XmlNodeRef
inline XmlNodeRef::XmlNodeRef( const char *tag,IXmlNode *node )
{
if (node)
p = node->createNode( tag );
else
p = new XmlNode( tag );
p->AddRef();
}
*/

//////////////////////////////////////////////////////////////////////////
inline XmlNodeRef::XmlNodeRef( IXmlNode* p_ ) : p(p_)
{
	if (p) p->AddRef();
}

inline XmlNodeRef::XmlNodeRef( const XmlNodeRef &p_ ) : p(p_.p)
{
	if (p) p->AddRef();
}

inline XmlNodeRef::~XmlNodeRef()
{
	if (p) p->Release();
}

inline XmlNodeRef&  XmlNodeRef::operator=( IXmlNode* newp )
{
	if (newp) newp->AddRef();
	if (p) p->Release();
	p = newp;
	return *this;
}

inline XmlNodeRef&  XmlNodeRef::operator=( const XmlNodeRef &newp )
{
	if (newp.p) newp.p->AddRef();
	if (p) p->Release();
	p = newp.p;
	return *this;
}

inline bool operator == ( const XmlNodeRef &p1,int null )	{
	return p1.p == 0;
}

inline bool operator != ( const XmlNodeRef &p1,int null )	{
	return p1.p != 0;
}

inline bool operator == ( int null,const XmlNodeRef &p1 )	{
	return p1.p == 0;
}

inline bool operator != ( int null,const XmlNodeRef &p1 )	{
	return p1.p != 0;
}

//////////////////////////////////////////////////////////////////////////
UNIQUE_IFACE struct IXmlSerializer
{
	virtual void AddRef() = 0;
	virtual void Release() = 0;

	virtual ISerialize* GetWriter( XmlNodeRef &node ) = 0;
	virtual ISerialize* GetReader( XmlNodeRef &node ) = 0;
};

// Summary:
//	 XML Parser interface.
UNIQUE_IFACE struct IXmlParser
{
	virtual void AddRef() = 0;
	virtual void Release() = 0;

	// Summary:
	//	 Parses xml file.
	virtual XmlNodeRef ParseFile( const char *filename,bool bCleanPools ) = 0;

	// Summary:
	//	 Parses xml from memory buffer.
	virtual XmlNodeRef ParseBuffer( const char *buffer,int nBufLen,bool bCleanPools ) = 0;
};

// Summary:
//	 IXmlUtils structure.
UNIQUE_IFACE struct IXmlUtils
{
	// Summary:
	//	 Creates XML Parser.
	// Notes:
	//	 WARNING!!!
	//	 IXmlParser does not normally support recursive XML loading, all nodes loaded by this parser are invalidated on loading new file.
	//	 This is a specialized interface for fast loading of many XMLs, 
	//	 After use it must be released with call to Release method.
	virtual IXmlParser* CreateXmlParser() = 0;

	// Summary:
	//	 Loads xml file, return 0 if load failed.
	virtual XmlNodeRef LoadXmlFile( const char *sFilename ) = 0;
	// Summary:
	//	 Loads xml from string, return 0 if load failed.
	virtual XmlNodeRef LoadXmlFromString( const char *sXmlString ) = 0;	

	// Summary:
	//	 Creates an MD5 hash of an XML file
	virtual const char * HashXml( XmlNodeRef node ) = 0;

	// Summary:
	//	 Gets an object that can read a xml into a IReadXMLSink 
	//	 and writes a xml from a IWriteXMLSource
	virtual IReadWriteXMLSink* GetIReadWriteXMLSink() = 0;

	// Summary:
	//	 Creates XML Writer for ISerialize interface.
	// See also:
	//	 IXmlSerializer
	virtual IXmlSerializer* CreateXmlSerializer() = 0;

	// Summary:
	//	 Creates XML to file in the binary form.
	virtual bool SaveBinaryXmlFile( const char *sFilename,XmlNodeRef root ) = 0;
	// Summary:
	//	 Reads XML data from file in the binary form.
	virtual XmlNodeRef LoadBinaryXmlFile( const char *sFilename ) = 0;

	// Summary:
	//	 Enables or disables checking for binary xml files.
	// Return Value:
	//	 The previous status.
	virtual bool EnableBinaryXmlLoading( bool bEnable ) = 0;

	// Init xml stats nodes pool
	virtual void InitStatsXmlNodePool( uint32 nPoolSize ) = 0;

	// Creates new xml node for statistics.
	virtual XmlNodeRef CreateStatsXmlNode( const char *sNodeName ) = 0;

};

#endif // __ixml_h__
