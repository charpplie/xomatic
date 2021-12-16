#ifndef __IFlashUI__h__
#define __IFlashUI__h__

#include <CryExtension/ICryUnknown.h>
#include <CryExtension/CryCreateClassInstance.h>
#include <IFlashPlayer.h>
#include <IFlowSystem.h>

#define IFlashUIExtensionName "FlashUI"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// UI variant data /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef NTypelist::CConstruct<
int,
float,
EntityId,
Vec3,
string,
bool
>::TType TUIDataTypes;

//	Default conversion uses C++ rules.
template <class From, class To>
struct SUIConversion
{
	static ILINE bool ConvertValue( const From& from, To& to )
	{
		to = (To)from;
		return true;
	}
};

//	same type conversation
#define UIDATA_NO_CONVERSION(T) \
	template <> struct SUIConversion<T,T> { \
	static ILINE bool ConvertValue( const T& from, T& to ) { to = from; return true; } \
}
UIDATA_NO_CONVERSION(int);
UIDATA_NO_CONVERSION(float);
UIDATA_NO_CONVERSION(EntityId);
UIDATA_NO_CONVERSION(Vec3);
UIDATA_NO_CONVERSION(string);
UIDATA_NO_CONVERSION(bool);
#undef FLOWSYSTEM_NO_CONVERSION

//	Specialization for converting to bool to avoid compiler warnings.
template <class From>
struct SUIConversion<From, bool>
{
	static ILINE bool ConvertValue( const From& from, bool& to )
	{
		to = (from != 0);
		return true;
	}
};

//	Strict conversation from float to int
template <>
struct SUIConversion<float, int>
{
	static ILINE bool ConvertValue( const float& from, int& to )
	{
		int tmp = (int) from;
		if ( fabs(from - (float) tmp) < FLT_EPSILON)
		{
			to = tmp;
			return true;
		}
		return false;
	}
};

//	Vec3 conversions...
template <class To>
struct SUIConversion<Vec3, To>
{
	static ILINE bool ConvertValue( const Vec3& from, To& to )
	{
		return SUIConversion<float, To>::ConvertValue( from.x, to );
	}
};

template <class From>
struct SUIConversion<From, Vec3>
{
	static ILINE bool ConvertValue( const From& from, Vec3& to )
	{
		float temp;
		if (!SUIConversion<From, float>::ConvertValue( from, temp ))
			return false;
		to.x = to.y = to.z = temp;
		return true;
	}
};

template <>
struct SUIConversion<Vec3, bool>
{
	static ILINE bool ConvertValue( const Vec3& from, bool& to )
	{
		to = from.GetLengthSquared() > 0;
		return true;
	}
};

//	String conversions...
#define UIDATA_STRING_CONVERSION(type,fmt, fct) \
	template <> \
struct SUIConversion<type, string> \
{ \
	static ILINE bool ConvertValue( const type& from, string& to ) \
{ \
	to.Format( fmt, from ); \
	return true; \
} \
}; \
	template <> \
struct SUIConversion<string, type> \
{ \
	static ILINE bool ConvertValue( const string& from, type& to ) \
{ \
	char *pEnd; \
	to = fct; \
	return *pEnd == '\0'; \
} \
};

#define SINGLE_FCT(fct) (float) fct (from.c_str(),&pEnd)
#define DOUBLE_FCT(fct) fct (from.c_str(),&pEnd,10)

UIDATA_STRING_CONVERSION(int, "%d", DOUBLE_FCT(strtol) );
UIDATA_STRING_CONVERSION(float, "%f", SINGLE_FCT(strtod) );
UIDATA_STRING_CONVERSION(EntityId, "%u", DOUBLE_FCT(strtoul) );

#undef UIDATA_STRING_CONVERSION
#undef SINGLE_FCT
#undef DOUBLE_FCT

template <>
struct SUIConversion<bool, string>
{
	static ILINE bool ConvertValue( const bool& from, string& to )
	{
		to.Format( "%d", from );
		return true;
	}	
};

template <>
struct SUIConversion<string, bool>
{
	static ILINE bool ConvertValue( const string& from, bool& to )
	{
		int to_i;
		if ( SUIConversion<string, int>::ConvertValue(from, to_i) )
		{
			to = !!to_i;
			return true;
		}
		if (0 == stricmp (from.c_str(), "true"))
		{
			to = true;
			return true;
		}
		if (0 == stricmp (from.c_str(), "false"))
		{
			to = false;
			return true;
		}
		return false;
	}	
};

template <>
struct SUIConversion<Vec3, string>
{
	static ILINE bool ConvertValue( const Vec3& from, string& to )
	{
		to.Format( "%f,%f,%f", from.x, from.y, from.z );
		return true;
	}
};

template <>
struct SUIConversion<string, Vec3>
{
	static ILINE bool ConvertValue( const string& from, Vec3& to )
	{
		return 3 == sscanf( from.c_str(), "%f,%f,%f", &to.x, &to.y, &to.z );
	}
};

enum EUIDataTypes
{
	eUIDT_Any = -1,
	eUIDT_Int = NTypelist::IndexOf<int, TUIDataTypes>::value,
	eUIDT_Float = NTypelist::IndexOf<float, TUIDataTypes>::value,
	eUIDT_EntityId = NTypelist::IndexOf<EntityId, TUIDataTypes>::value,
	eUIDT_Vec3 = NTypelist::IndexOf<Vec3, TUIDataTypes>::value,
	eUIDT_String = NTypelist::IndexOf<string, TUIDataTypes>::value,
	eUIDT_Bool = NTypelist::IndexOf<bool, TUIDataTypes>::value,
};

typedef CConfigurableVariant<TUIDataTypes, sizeof(void*), SUIConversion> TUIData;


////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// UI Arguments //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SUIArguments
{
	SUIArguments() {};
	SUIArguments( const char* sArgs )  { SetArguments( sArgs ); }
	SUIArguments( const SFlashVarValue* vArgs, int iNumArgs )  { SetArguments( vArgs, iNumArgs ); }
	SUIArguments( const TUIData& data )  { AddArgument( data ); }

	void SetArguments( const char* sArgStringList )
	{
		Clear();
		AddArguments( sArgStringList );
	}

	void AddArguments( const char* sArgStringList )
	{
		string str = sArgStringList;
		while ( str.length() > 0 ) 
		{
			string::size_type loc = str.find( ",", 0 );
			if ( loc != string::npos )
			{
				string arg = str.substr( 0, loc );
				AddArgument( arg );
				str = str.substr( loc + 1 );
			}
			else
			{
				AddArgument( str );
				break;
			}
		}
	}

	void SetArguments( const SFlashVarValue* vArgs, int iNumArgs )
	{
		Clear();
		AddArguments( vArgs, iNumArgs );
	}

	void AddArguments( const SFlashVarValue* vArgs, int iNumArgs )
	{
		for (int i = 0; i < iNumArgs; ++i)
		{
			switch( vArgs[i].GetType() )
			{
			case SFlashVarValue::eBool:			AddArgument( vArgs[i].GetBool() );					break;
			case SFlashVarValue::eInt:			AddArgument( vArgs[i].GetInt() );					break;
			case SFlashVarValue::eUInt:			AddArgument( vArgs[i].GetUInt() );					break;
			case SFlashVarValue::eFloat:		AddArgument( vArgs[i].GetFloat() );					break;
			case SFlashVarValue::eDouble:		AddArgument( (float) vArgs[i].GetDouble() );		break;
			case SFlashVarValue::eConstStrPtr:	AddArgument( string(vArgs[i].GetConstStrPtr()) );	break;
			case SFlashVarValue::eConstWstrPtr:	assert(false);	AddArgument( string("UNDEFINED") );	break; // Not supported yet!
			case SFlashVarValue::eNull:			AddArgument( string("NULL") );						break;
			case SFlashVarValue::eObject:		AddArgument( string("OBJECT") );					break;
			case SFlashVarValue::eUndefined:	AddArgument( string("UNDEFINED") );					break;
			}
		}
	}

	template< class T >
	void AddArgument( const T& arg )
	{
		m_ArgList.push_back( TUIData( arg ) );
	}

	void Clear()
	{
		m_ArgList.clear();
	}

	int GetArgCount() const { return m_ArgList.size(); }

	const char* GetAsString() const { return updateStringBuffer(); }
	const SFlashVarValue* GetAsList( bool bStrictTypeConversation = false ) const { return updateFlashBuffer( bStrictTypeConversation ); }

	const TUIData& GetArg( int index ) const
	{
		if ( index >= 0 && index < m_ArgList.size() )
			return m_ArgList[index];
		static TUIData undef( string( "undefined" ) );
		return undef;
	}

	template< class T >
	bool GetArg( int index, T &val ) const
	{
		if ( index >= 0 && index < m_ArgList.size() )
			return m_ArgList[index].GetValueWithConversion( val );
		return false;
	}

private:
	DynArray< TUIData > m_ArgList;
	mutable string m_sArgStringBuffer;
	mutable DynArray< SFlashVarValue > m_FlashValueBuffer;

	SFlashVarValue* updateFlashBuffer( bool bStrictTypeConversation ) const
	{
		m_FlashValueBuffer.clear();
		for ( DynArray< TUIData >::const_iterator it = m_ArgList.begin(); it != m_ArgList.end(); ++it )
		{
			bool bConverted = false;

			if ( bStrictTypeConversation )
			{
				switch ( it->GetType() )
				{
				case eUIDT_Bool:
					bConverted = TryAddValue<bool>( *it );
					break;
				case eUIDT_Int:
					bConverted = TryAddValue<EntityId>( *it );
					break;
				case eUIDT_EntityId:
					bConverted = TryAddValue<int>( *it );
					break;
				case eUIDT_Float:
					bConverted = TryAddValue<float>( *it );
					break;
				case eUIDT_Any:
				case eUIDT_String:
					bConverted = TryAddValue<string>( *it );
					break;
				}
			}
			else
			{
				bConverted =	TryAddValue<int>( *it )
							||	TryAddValue<float>( *it )
// 							||	TryAddValue<bool>( *it )
							||	TryAddValue<string>( *it );
			}
			if ( !bConverted )
				m_FlashValueBuffer.push_back( SFlashVarValue::CreateUndefined() );
		}
		return m_FlashValueBuffer.size() > 0 ? &m_FlashValueBuffer[0] : NULL;
	}

	template < class T >
	bool TryAddValue( const TUIData& data ) const
	{
		T val;
		if ( data.GetValueWithConversion( val ) )
		{
			m_FlashValueBuffer.push_back( SFlashVarValue(val) );
			return true;
		}
		return false;
	}

	string& updateStringBuffer() const
	{
		m_sArgStringBuffer = "";
		for ( DynArray< TUIData >::const_iterator it = m_ArgList.begin(); it != m_ArgList.end(); ++it )
		{
 			m_sArgStringBuffer += m_sArgStringBuffer.size() > 0 ? "," : "";
			string val;
			it->GetValueWithConversion(val);
 			m_sArgStringBuffer += val;
		}
		return m_sArgStringBuffer;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// UI Descriptions /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SUIParameterDesc
{
	SUIParameterDesc() : sName("undefined"), sDisplayName("undefined"), sDesc("undefined") {} 
	SUIParameterDesc( string name, string displ, string desc) : sName(name), sDisplayName(displ), sDesc(desc) {}
	string sName;
	string sDisplayName;
	string sDesc;
};
typedef DynArray< SUIParameterDesc > TUIParams;

struct SUIEventDesc : public SUIParameterDesc
{
	SUIEventDesc() : IsDynamic(false) {}
	SUIEventDesc( string name, string displ, string desc, bool isDyn = false ) : SUIParameterDesc( name, displ, desc ), IsDynamic(isDyn) {}
	TUIParams Params;
	bool IsDynamic;
};
typedef DynArray< SUIEventDesc > TUIEvents;

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// UI Element ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
struct IUIElementEventListener
{
	virtual void OnUIEvent( const char* sEvent, const char* sArgs ) = 0;
};

UNIQUE_IFACE struct IUIElement
{
	struct SUIConstraints
	{
		enum EPositionType
		{
			ePT_Fixed,
			ePT_Fullscreen,
			ePT_Dynamic
		};

		enum EPositionAlign
		{
			ePA_Lower,
			ePA_Mid,
			ePA_Upper,
		};

		SUIConstraints() 
			: eType(ePT_Dynamic)
			, iLeft(0)
			, iTop(0)
			, iWidth(1024)
			, iHeight(768)
			, eHAlign(ePA_Mid)
			, eVAlign(ePA_Mid)
			, bScale(true)
		{
		}

		SUIConstraints( EPositionType type, int left, int top, int width, int height, EPositionAlign halign, EPositionAlign valign, bool scale )
			: eType(type)
			, iLeft(left)
			, iTop(top)
			, iWidth(width)
			, iHeight(height)
			, eHAlign(halign)
			, eVAlign(valign)
			, bScale(scale)
		{
		}

		EPositionType eType;
		int iLeft;
		int iTop;
		int iWidth;
		int iHeight;
		EPositionAlign eHAlign;
		EPositionAlign eVAlign;
		bool bScale;
	};

	enum EFlashUIFlags
	{
		eFUI_HARDWARECURSOR = 0x001,
		eFUI_MOUSEEVENTS	= 0x002,
		eFUI_KEYEVENTS		= 0x004,
		eFUI_CONSOLE_MOUSE	= 0x008,
		eFUI_CONSOLE_CURSOR = 0x010,
	};

	virtual ~IUIElement() {}

	virtual void SetName( const char* sName ) = 0;
	virtual const char* GetName() const = 0;

	virtual void SetGroupName( const char* sGroupName ) = 0;
	virtual const char* GetGroupName() const = 0;

	virtual void SetFlashFile( const char* sFlashFile ) = 0;
	virtual const char* GetFlashFile() const = 0;

	virtual bool Init( bool bLoadAsset = true ) = 0;
	virtual void Unload() = 0; 
	virtual void Draw( float fDeltaTime ) = 0;

	virtual void RequestHide() = 0;

	virtual void SetVisible( bool bVisible ) = 0;
	virtual bool IsVisible() const = 0;

	virtual void SetFlag( EFlashUIFlags flag, bool bSet ) = 0;
	virtual bool HasFlag( EFlashUIFlags flag ) const = 0;

	virtual float GetAlpha() const = 0;
	virtual void SetAlpha( float fAlpha ) = 0;

	virtual int GetLayer() const = 0;
	virtual void SetLayer( int iLayer ) = 0;

	virtual void SetConstraints( const SUIConstraints& newConstraints ) = 0;
	virtual const IUIElement::SUIConstraints& GetConstraints() const = 0;

	virtual IFlashPlayer* GetFlashPlayer() = 0;

	virtual const SUIParameterDesc* GetVariableDesc( int index ) const = 0;
	virtual const SUIParameterDesc* GetVariableDesc( const char* sVarName ) const = 0;
	virtual int GetVariableCount() const = 0;

	virtual const SUIParameterDesc* GetArrayDesc( int index ) const = 0;
	virtual const SUIParameterDesc* GetArrayDesc( const char* sArrayName ) const = 0;
	virtual int GetArrayCount() const = 0;

	virtual const SUIEventDesc* GetEventDesc( int index ) const = 0;
	virtual const SUIEventDesc* GetEventDesc( const char* sEventName ) const = 0;
	virtual int GetEventCount() const = 0;

	virtual const SUIEventDesc* GetFunctionDesc( int index ) const = 0;
	virtual const SUIEventDesc* GetFunctionDesc( const char* sFunctionName ) const = 0;
	virtual int GetFunctionCount() const = 0;

	virtual void UpdateViewPort() = 0;

	virtual bool Serialize( XmlNodeRef& xmlNode, bool bIsLoading ) = 0;

	virtual void AddEventListener( IUIElementEventListener* pListener ) = 0;
	virtual void RemoveEventListener( IUIElementEventListener* pListener ) = 0;

	virtual bool CallFunction( const char* pFctName, const SUIArguments& args = SUIArguments(), TUIData* pDataRes = NULL ) = 0;
	virtual bool CallFunction( const SUIEventDesc* pFctDesc, const SUIArguments& args = SUIArguments(), TUIData* pDataRes = NULL ) = 0;

	virtual bool SetVariable( const char* pVarName, const TUIData& value ) = 0;
	virtual bool SetVariable( const SUIParameterDesc* pVarDesc, const TUIData& value ) = 0;

	virtual bool GetVariable( const char* pVarName, TUIData& valueOut ) = 0;
	virtual bool GetVariable( const SUIParameterDesc* pVarDesc, TUIData& valueOut ) = 0;

	virtual bool SetArray( const char* pArrayName, const SUIArguments& values ) = 0;
	virtual bool SetArray( const SUIParameterDesc* pArrayDesc, const SUIArguments& values ) = 0;

	virtual bool GetArray( const char* pArrayName, SUIArguments& valuesOut ) = 0;
	virtual bool GetArray( const SUIParameterDesc* pArrayDesc, SUIArguments& valuesOut ) = 0;

	template <class T>
	bool SetVar( const char* pVarName, const T& value)
	{
		return SetVariable( pVarName, TUIData(value) );
	}

	template <class T>
	T GetVar( const char* pVarName )
	{
		TUIData out;
		if ( GetVariable( pVarName, out ) )
		{
			T res;
			if ( out.GetValueWithConversion( res ) )
				return res;
		}
		assert(false);
		return T();
	}

	enum EButtonLink
	{
		eBL_Default,
		eBL_Up,
		eBL_Down,
		eBL_Left,
		eBL_Right,
	};

	virtual bool GetNextButtonPos( EButtonLink eLink, int &iX, int &iY ) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// UI Action ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
UNIQUE_IFACE struct IUIAction
{
	virtual ~IUIAction() {}

	virtual const char* GetName() const = 0;
	virtual void SetName( const char* sName ) = 0;

	virtual bool Init() = 0;

	virtual IFlowGraphPtr GetFlowGraph() const = 0;

	virtual bool Serialize( XmlNodeRef& xmlNode, bool bIsLoading ) = 0;
};

struct IUIActionListener
{
	virtual void OnStart( IUIAction* pAction ) = 0;
	virtual void OnEnd( IUIAction* pAction ) = 0;
};

UNIQUE_IFACE struct IUIActionManager
{
	virtual void StartAction( IUIAction* pAction ) = 0;
	virtual void EndAction( IUIAction* pAction ) = 0;
	virtual void EnableAction( IUIAction* pAction, bool bEnable ) = 0;

	virtual void AddListener( IUIActionListener* pListener ) = 0;
	virtual void RemoveListener( IUIActionListener* pListener ) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// UI Events ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SUIEvent
{
	SUIEvent( uint evt, SUIArguments agruments ) : event(evt), args(agruments) {}
	uint event;
	SUIArguments args;
};

struct IUIEventListener
{
	virtual void OnEvent( const SUIEvent& event ) = 0;
};

UNIQUE_IFACE struct IUIEventSystem
{
	enum EEventSystemType
	{
		eEST_UI_TO_SYSTEM,
		eEST_SYSTEM_TO_UI,
	};

	virtual const char* GetName() const = 0;
	virtual IUIEventSystem::EEventSystemType GetType() const = 0;

	virtual uint RegisterEvent( const SUIEventDesc& sEventDesc ) = 0;

	virtual void RegisterListener( IUIEventListener* pListener ) = 0;
	virtual void UnregisterListener( IUIEventListener* pListener ) = 0;

	virtual void SendEvent( const SUIEvent& event ) = 0;

	virtual const SUIEventDesc* GetEventDesc( int index ) const = 0;
	virtual const SUIEventDesc* GetEventDesc( const char* sEventName ) const = 0;
	virtual int GetEventCount() const = 0;
	
	virtual uint GetEventId( const char* sEventName ) = 0;
};

UNIQUE_IFACE struct IUIEventSystemIterator
{
	virtual void AddRef() = 0;
	virtual void Release() = 0;
	virtual IUIEventSystem* Next( string &sName ) = 0;
};

TYPEDEF_AUTOPTR(IUIEventSystemIterator);
typedef IUIEventSystemIterator_AutoPtr IUIEventSystemIteratorPtr;

template<class T>
struct SUIEventHelper
{
	typedef void (T::*TEventFct) ( const SUIEvent& event );
	void RegisterEvent( IUIEventSystem* pEventSystem, const SUIEventDesc &event, TEventFct fct )
	{
		mFunctionMap[pEventSystem->RegisterEvent(event)] = fct;
	}
	void Dispatch( T* pThis, const SUIEvent& event )
	{
		TFunctionMapIter it = mFunctionMap.find( event.event );
		if (it != mFunctionMap.end())
			(pThis->*it->second)( event );
	}

private:
	typedef std::map<uint, TEventFct> TFunctionMap;
	typedef typename TFunctionMap::iterator TFunctionMapIter;
	TFunctionMap mFunctionMap;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// UI Interface ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
UNIQUE_IFACE struct IFlashUI : public ICryUnknown
{
	CRYINTERFACE_DECLARE( IFlashUI, 0xE1161004DA5B4F04, 0x9DFF8FC0EACE3BD4 )

public:
	DEVIRTUALIZATION_VTABLE_FIX

	// init the Flash UI system
	virtual void Init() = 0;
	virtual bool PostInit() = 0;

	// reload UI xml files
	virtual void Reload() = 0;

	// shut down
	virtual void ShutDown() = 0;

	virtual bool LoadElementsFromFile( const char* sFileName ) = 0;
	virtual bool LoadActionFromFile( const char* sFileName ) = 0;

	// access for IUIElements
	virtual IUIElement* GetUIElement( const char* sName ) const = 0;
	virtual IUIElement* GetUIElement( int index ) const = 0;
	virtual int GetUIElementCount() const = 0;

	// access for IUIActions
	virtual IUIAction* GetUIAction( const char* sName ) const = 0;
	virtual IUIAction* GetUIAction( int index ) const = 0;
	virtual int GetUIActionCount() const = 0;

	virtual IUIActionManager* GetUIActionManager() const = 0;

	// event system to auto create flownodes for communication between flash and c++
	virtual IUIEventSystem* CreateEventSystem( const char* sName, IUIEventSystem::EEventSystemType eType ) = 0;
	virtual IUIEventSystem* GetEventSystem( const char* name, IUIEventSystem::EEventSystemType eType ) = 0;
	virtual IUIEventSystemIteratorPtr CreateEventSystemIterator( IUIEventSystem::EEventSystemType eType ) = 0; 

	// ui input for controller
	enum EControllerInputEvent
	{
		eCIE_Default,
		eCIE_Up,
		eCIE_Down,
		eCIE_Left,
		eCIE_Right,
		eCIE_Click,
		eCIE_Back,
	};

	enum EControllerInputState
	{
		eCIS_OnPress,
		eCIS_OnRelease,
	};

	virtual bool SendControllerInput( EControllerInputEvent event, EControllerInputState state ) = 0;
};

typedef cryshared_ptr< IFlashUI > IFlashUIPtr;

static IFlashUIPtr GetIFlashUIPtr()
{
	IFlashUIPtr pFlashUI;
	CryCreateClassInstance(IFlashUIExtensionName, pFlashUI);
	return pFlashUI;
}

#endif