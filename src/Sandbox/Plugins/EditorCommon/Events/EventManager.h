////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   EventManager.h
//  Created:     29/01/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "platform.h"

#include "EditorCommonAPI.h"
#include "IEditor.h"

#include "Serialization/IArchive.h"

// We disable C4251 ("'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'") 
// here, because the STL containers the compiler complains about are all private and thus never accessed by other DLLs
#pragma warning(push)
#pragma warning(disable: 4251)

struct SSystemGlobalEnvironment;

class EDITOR_COMMON_API CEventConnection
{
	friend class CEventManager;
	friend class CScopedEventConnection;
public:
	CEventConnection() : m_bConnected(false) {}
	void Disconnect() const;

	uint GetHandlerId() const { return m_handlerId; }

private:
	CEventConnection(const uint address, const string& eventName, const uint handlerId)
		: m_address(address), m_eventName(eventName), m_handlerId(handlerId), m_bConnected(true) {}

	bool m_bConnected;
	uint m_address;
	uint m_handlerId;
	string m_eventName;
};

class EDITOR_COMMON_API CScopedEventConnection : public CEventConnection
{
public:
	CScopedEventConnection() : CEventConnection() {}
	CScopedEventConnection(CScopedEventConnection&& connection)
		: CEventConnection(connection.m_address, connection.m_eventName, connection.m_handlerId)
	{
		connection.m_bConnected = false;
		connection.m_address = 0;
		connection.m_handlerId = 0;
		m_eventName = "";
	}

	CScopedEventConnection(const CEventConnection& connection)
		: CEventConnection(connection.m_address, connection.m_eventName, connection.m_handlerId) {}
	CScopedEventConnection& operator =(const CEventConnection& connection)
	{
		m_address = connection.m_address;
		m_eventName = connection.m_eventName;
		m_handlerId = connection.m_handlerId;
		m_bConnected = true;
		return *this;
	}

	~CScopedEventConnection() { Disconnect(); }

private:
	CScopedEventConnection(const CScopedEventConnection&); // no implementation
	CScopedEventConnection& operator =(const CScopedEventConnection&); // no implementation
};

class EDITOR_COMMON_API CEventManager
{
	friend class CEventConnection;

public:
	CEventManager(SSystemGlobalEnvironment* env);

	static CEventManager* GetInstance();

	// Registers an address and returns its ID. Multiple event handlers can listen to the same address, allowing broadcasts.
	uint GetAddressId(const string& name);

	// Registers a new unique address
	uint GetUniqueAddressId();

	// Sends an event to an address
	//
	// TMessageType must be a serializable struct
	//
	template <class TMessageType> void SendEvent(const uint address, const TMessageType& message)
	{
		const string json = SerializeMessageToJSON(Serialization::SStruct(message));
		SendEventRaw(address, TMessageType::GetName(), json);
	}

	template <class TMessageType> void SendEvent(const uint address, const TMessageType& message, const std::vector<uint>& excludedHandlers) const
	{
		const string json = SerializeMessageToJSON(Serialization::SStruct(message));
		SendEventRaw(address, TMessageType::GetName(), json, excludedHandlers);
	}

	// For sending a raw JSON message.
	void SendEventRaw(const uint address, const string& eventName, const string& message) const;
	void SendEventRaw(const uint address, const string& eventName, const string& message, const std::vector<uint>& excludedHandlers) const;

	// Tests if a call to SendEvent would actually send a message (there is someone listening to this message)
	bool CanDeliverRaw(const uint address, const string& eventName) const;
	template<class TMessageType> bool CanDeliver(const uint address) const
	{
		string eventName = TMessageType::GetName();
		return CanDeliverRaw(address, eventName);
	}

	// This should be the most common way to add an event callback
	//
	// This example will install an event handler for OnEvent(const SMessageType &message) that is sent to a specific address:
	// CEventManager::GetInstance()->AddEventCallback(componentId, this, &CEventHandler::OnEvent);
	//
	// TMessageType must be a serializable struct
	//
	// Returns a CEventConnection. The callback is removed when this object is destroyed.
	//
	template <class TClassType, class TMessageType> CEventConnection AddEventCallback(const uint address, TClassType* pThis, void (TClassType::*pMethod)(const TMessageType&))
	{
		return AddEventCallback<TMessageType>(address, std::bind(pMethod, pThis, std::placeholders::_1));
	}

	// Same as above, but you can pass in any function object that takes TMessageType& as an argument directly.
	//
	template <class TMessageType> CEventConnection AddEventCallback(const uint address, std::function<void (const TMessageType&)> callback)
	{
		return AddEventCallbackRaw(address, TMessageType::GetName(), [=](const string& json)
		{
			TMessageType message;
			DeserializeFromJSON(Serialization::SStruct(message), json);
			callback(message);
		});
	}

	// This can be used if raw parsing of JSON is preferred.
	typedef std::function<void (const string&)> TEventHandlerFunc;
	CEventConnection AddEventCallbackRaw(const uint componentId, const string& eventName, TEventHandlerFunc callback);

private:
	void SendEventImplementation(const uint address, const string& eventName, const string &message, const std::vector<uint>& excludedHandlers) const;

	string SerializeMessageToJSON(const Serialization::SStruct& ref) const;
	void DeserializeFromJSON(const Serialization::SStruct& ref, const string& json);

	uint m_nextAddress;
	uint m_nextHandlerId;
	std::map<string, uint, stl::less_stricmp<string>> m_nameToAddressMap;
	std::map<std::pair<uint, string>, std::vector<std::pair<uint, TEventHandlerFunc>>> m_messageRoutingMap;

	static CEventManager* ms_pEventManager;
};

#pragma warning(pop)