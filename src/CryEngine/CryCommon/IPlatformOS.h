////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   IPlatformOS.h
//  Created:     18/12/2009 by Alex Weighell.
//  Description: Interface to the Platform OS
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include DEVIRTUALIZE_HEADER_FIX(IPlatformOS.h)

#ifndef __IPLATFORMOS_H_
#define __IPLATFORMOS_H_

#include <CryArray.h>
#include <CryExtension/ICryUnknown.h>
#include <CryExtension/Impl/ClassWeaver.h>
#include <CryFixedString.h>




#if defined(WIN32) || defined(WIN64)
#include <Lmcons.h>
#endif

// Interface platform OS (Operating System) functionality
UNIQUE_IFACE struct IPlatformOS
{
	enum
	{






#if defined(WIN32) || defined(WIN64)
		USER_MAX_NAME = UNLEN+1,
		FILE_MAX_NAME = PATHLEN+1,
#else			
		USER_MAX_NAME = 256,
		FILE_MAX_NAME = 256,
#endif
	};

	enum EFileOperationCode { EFOC_SUCCESS, EFOC_FAILURE };

	enum EMsgBoxResult
	{
		eMsgBox_OK,
		eMsgBox_Cancel,
		eMsgBoxNumButtons
	};

	// each entry fills in the relevant member of SStatAnyValue
	enum EUserProfilePreference
	{
		EUPP_CONTROLLER_INVERT_Y,		// eVT_Int (0,1)
		EUPP_CONTROLLER_SENSITIVITY,	// eVT_Float (0.0f..2.0f)
		EUPP_GAME_DIFFICULTY,			// eVT_Int (0,1,2)=(easy,medium,hard)
	};

	struct SUserProfileVariant
	{
		enum EVariantType
		{
			eVT_Invalid,
			eVT_Bool,
			eVT_Int,
			eVT_Float,
		};

		SUserProfileVariant() : m_type(eVT_Invalid), m_iValue(0) {}
		SUserProfileVariant(bool bBool) : m_type(eVT_Bool), m_bValue(bBool) {}
		SUserProfileVariant(int iValue) : m_type(eVT_Int), m_iValue(iValue) {}
		SUserProfileVariant(float fValue) : m_type(eVT_Float), m_fValue(fValue) {}

		EVariantType GetType() const { return m_type; }

		bool GetBool() const { assert(GetType() == eVT_Bool); return m_bValue; }
		int GetInt() const { assert(GetType() == eVT_Int); return m_iValue; }
		float GetFloat() const { assert(GetType() == eVT_Float); return m_fValue; }

	private:
		EVariantType m_type;

		union
		{
			bool	m_bValue;
			int		m_iValue;
			float	m_fValue;
		};
	};

	struct ISaveReader
	{
		enum ESeekMode
		{
			ESM_BEGIN,
			ESM_CURRENT,
			ESM_END,
		};

		template<class T>
		EFileOperationCode ReadItem(T& data)
		{
			return ReadBytes(reinterpret_cast<void*>(&data), sizeof(T));
		}

		virtual EFileOperationCode Seek(long seek, ESeekMode mode) = 0;
		virtual EFileOperationCode GetFileCursor(long& fileCursor) = 0;
		virtual EFileOperationCode ReadBytes(void* data, size_t numBytes) = 0;
		virtual EFileOperationCode GetNumBytes(size_t& numBytes) = 0;
		virtual EFileOperationCode Close() = 0;
	};
	typedef cryshared_ptr<ISaveReader> ISaveReaderPtr;

	struct ISaveWriter
	{
		template<class T>
		void AppendItem(const T& data)
		{
			AppendBytes(reinterpret_cast<const void*>(&data), sizeof(T));
		}

		template<class T>
		void AppendItems(T *data, size_t elems)
		{
			AppendBytes(static_cast<const void*>(data), sizeof(T) * elems);
		}

		virtual void AppendBytes(const void* data, size_t length) = 0;
		virtual EFileOperationCode Close() = 0;
	};
	typedef cryshared_ptr<ISaveWriter> ISaveWriterPtr;

	typedef CryFixedStringT<USER_MAX_NAME> TUserName;

	typedef CryFixedStringT<FILE_MAX_NAME> TFileName;
	typedef CryFixedWStringT<FILE_MAX_NAME> TDisplayName;


	// Call once to create and initialize. Use delete to destroy.
	static IPlatformOS* Create();
	virtual ~IPlatformOS() {}

	// Tick with each frame to determine if there are any system messages requiring handling
	virtual void Tick(float realFrameTime) = 0;

	// Local user profile functions to check/initiate user sign in:

	// UserGetMaximumSignedInUsers:
	//   Returns the maximum number of signed in users. EG: On Xbox is 4.
	virtual unsigned int 	UserGetMaximumSignedInUsers() const = 0;

	// UserIsSignedIn:
	//   Returns true if the user is signed in to the OS.
	virtual bool 			UserIsSignedIn(unsigned int userIndex) const = 0;

	// UserDoSignIn:
	//   Initiate signin dialog box in the OS. Returns true on success.
	virtual bool			UserDoSignIn(unsigned int numUsersRequested = 1) = 0;

	// UserGetName:
	//   Get the name of a user. Returns true on success.
	virtual bool 			UserGetName(unsigned int userIndex, IPlatformOS::TUserName& outName) const = 0;

	// UserSelectStorageDevice:
	//   Get the user to select a storage device location for save data.
	virtual bool			UserSelectStorageDevice(unsigned int userIndex) = 0;

	// GetUserProfilePreference:
	//   Get a specific preference for the signed in user
	virtual bool			GetUserProfilePreference(EUserProfilePreference ePreference, SUserProfileVariant& outResult) const = 0;

	// GetFirstSignedInUser:
	//   Returns the user ID of the first signed in user, or -1 if no one is signed in.
	int						GetFirstSignedInUser() const;

	//[AlexMcC|12.02.10]: PS3 devirtualization needs ISaveReaderPtr (and writer) to be fully qualified)
	// SaveGetReader:
	//   Get a reader object to read from a save file. The file is automatically opened and closed.
	virtual IPlatformOS::ISaveReaderPtr SaveGetReader(const IPlatformOS::TFileName& fileName) = 0;

	// SaveGetReader:
	//   Get a writer object to write to a save file. The file is automatically opened and closed.
	virtual IPlatformOS::ISaveWriterPtr SaveGetWriter(const IPlatformOS::TFileName& fileName) = 0;


	//////////////////////////////////////////////////////////////////////////
	//   Platform listener
	// 

	// Internal event handle passed to observers from listeners. Derive platform specific events from this
	struct SPlatformEvent 
	{
		enum EEventType
		{
			eET_PlatformSpecific,
			// Add platform agnostic events if required
		} m_eEventType;
	};

	// Derive listeners from this interface and cast event to the appropriate platform event interface
	struct IPlatformListener
	{
		virtual void OnEvent(const SPlatformEvent& event) = 0;
	};

	virtual void AddListener(IPlatformListener* pListener, const char* szName) = 0;
	virtual void RemoveListener(IPlatformListener* pListener) = 0;
	virtual void NotifyListeners(SPlatformEvent& event) = 0;

	// DebugMessageBox:
	//   Displays an OS dialog box for debugging messages.
	//   A modal (blocking) dialog box with OK and Cancel options.
	// Arguments:
	//   body   -  text body of the message
	//   title	-  title text of the message
	//   flags  -  reserved for future use
	virtual IPlatformOS::EMsgBoxResult DebugMessageBox(const char* body, const char* title, unsigned int flags=0) const = 0;
};

ILINE int IPlatformOS::GetFirstSignedInUser() const
{
	unsigned int maximumSignedInUsers = UserGetMaximumSignedInUsers();
	for (unsigned int user = 0; user < maximumSignedInUsers; ++user)
	{
		if (UserIsSignedIn(user))
		{
			return static_cast<int>(user);
		}
	}

	return -1;
}

#endif

