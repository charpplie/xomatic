////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   3DConnexionDriver.cpp
//  Version:     v1.00
//  Created:     03/04/2008 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "3DConnexionDriver.h"
#include "..\Viewport.h"

namespace connexion_3D
{
//	#include "3DConnexionDriver_API.h"
}
//using namespace connexion_3D;

/*
//[ event_receiver(com) ]
class C3DConnexionDriverImpl
{
public:
	CComPtr<connexion_3D::ISensor> m_pISensor;
	CComPtr<connexion_3D::IKeyboard> m_pIKeyboard;
	
public:
	// COM Event handlers
	HRESULT InitializeCOM();
	HRESULT UninitializeCOM();
	HRESULT OnDeviceChange (long reserved);
	HRESULT OnKeyDown (int keyCode);
	HRESULT OnKeyUp (int keyCode);
	HRESULT OnSensorInput(void);
};


//////////////////////////////////////////////////////////////////////////
HRESULT C3DConnexionDriverImpl::InitializeCOM()
{
	HRESULT hr;
	CComPtr<IUnknown> _3DxDevice;

	// Create the device object
	hr = _3DxDevice.CoCreateInstance(__uuidof(Device));
	if (SUCCEEDED(hr))
	{

		CComPtr<ISimpleDevice> _3DxSimpleDevice;

		hr = _3DxDevice.QueryInterface(&_3DxSimpleDevice);
		if (SUCCEEDED(hr))
		{

			//hr = __hook(&_ISimpleDeviceEvents::DeviceChange, _3DxSimpleDevice, &C3DConnexionDriverImpl::OnDeviceChange);

			// Get the interfaces to the sensor and the keyboard;
			hr = _3DxSimpleDevice->get_Sensor(&m_pISensor);

			//hr = __hook(&_ISensorEvents::SensorInput, m_pISensor, &C3DConnexionDriverImpl::OnSensorInput);

			hr = _3DxSimpleDevice->get_Keyboard(&m_pIKeyboard);

			//hr = __hook(&_IKeyboardEvents::KeyDown, m_pIKeyboard, &C3DConnexionDriverImpl::OnKeyDown);

			//hr = __hook(&_IKeyboardEvents::KeyUp, m_pIKeyboard, &C3DConnexionDriverImpl::OnKeyUp);

			// Set the preferences we want to use
			//_3DxSimpleDevice->LoadPreferences(_T("AtlCude3D"));

			// Connect to the driver
			_3DxSimpleDevice->Connect();
		}
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////
HRESULT C3DConnexionDriverImpl::UninitializeCOM()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////
HRESULT C3DConnexionDriverImpl::OnDeviceChange( long reserved )
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////
HRESULT C3DConnexionDriverImpl::OnKeyDown( int keyCode )
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////
HRESULT C3DConnexionDriverImpl::OnKeyUp( int keyCode )
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////
HRESULT C3DConnexionDriverImpl::OnSensorInput( void )
{
	return 0;
}
*/
//////////////////////////////////////////////////////////////////////////
C3DConnexionDriver::C3DConnexionDriver()
{
	g_pRawInputDeviceList = 0;
	g_pRawInputDevices = 0;
	g_nUsagePage1Usage8Devices = 0;

	//m_pImpl = new C3DConnexionDriverImpl;
	//m_pImpl->InitializeCOM();

	m_fMultiplier = 1.0f;

	InitDevice();
}

//////////////////////////////////////////////////////////////////////////
C3DConnexionDriver::~C3DConnexionDriver()
{
	//if (m_pImpl)
		//m_pImpl->UninitializeCOM();
	//delete m_pImpl;
}

bool C3DConnexionDriver::InitDevice()
{
	// Find the Raw Devices
	UINT nDevices;
	// Get Number of devices attached
	if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
	{ 
		return false;
	}
	// Create list large enough to hold all RAWINPUTDEVICE structs
	if ((g_pRawInputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * nDevices)) == NULL)
	{
		return false;
	}
	// Now get the data on the attached devices
	if (GetRawInputDeviceList(g_pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST)) == -1) 
	{
		return false;
	}

	g_pRawInputDevices = (PRAWINPUTDEVICE)malloc( nDevices * sizeof(RAWINPUTDEVICE) );
	g_nUsagePage1Usage8Devices = 0;

	// Look through device list for RIM_TYPEHID devices with UsagePage == 1, Usage == 8
	for(UINT i=0; i<nDevices; i++)
	{
		if (g_pRawInputDeviceList[i].dwType == RIM_TYPEHID)
		{
			UINT nchars = 300;
			TCHAR deviceName[300];
			if (GetRawInputDeviceInfo( g_pRawInputDeviceList[i].hDevice,
				RIDI_DEVICENAME, deviceName, &nchars) >= 0)
			{
				//_RPT3(_CRT_WARN, "Device[%d]: handle=0x%x name = %S\n", i, g_pRawInputDeviceList[i].hDevice, deviceName);
			}

			RID_DEVICE_INFO dinfo;
			UINT sizeofdinfo = sizeof(dinfo);
			dinfo.cbSize = sizeofdinfo;
			if (GetRawInputDeviceInfo( g_pRawInputDeviceList[i].hDevice,
				RIDI_DEVICEINFO, &dinfo, &sizeofdinfo ) >= 0)
			{
				if (dinfo.dwType == RIM_TYPEHID)
				{
					RID_DEVICE_INFO_HID *phidInfo = &dinfo.hid;
					//_RPT1(_CRT_WARN, "VID = 0x%x\n", phidInfo->dwVendorId);
					//_RPT1(_CRT_WARN, "PID = 0x%x\n", phidInfo->dwProductId);
					//_RPT1(_CRT_WARN, "Version = 0x%x\n", phidInfo->dwVersionNumber);
					//_RPT1(_CRT_WARN, "UsagePage = 0x%x\n", phidInfo->usUsagePage);
					//_RPT1(_CRT_WARN, "Usage = 0x%x\n", phidInfo->usUsage);

					// Add this one to the list of interesting devices?
					// Actually only have to do this once to get input from all usage 1, usagePage 8 devices
					// This just keeps out the other usages.
					// You might want to put up a list for users to select amongst the different devices.
					// In particular, to assign separate functionality to the different devices.
					if (phidInfo->usUsagePage == 1 && phidInfo->usUsage == 8)
					{
						g_pRawInputDevices[g_nUsagePage1Usage8Devices].usUsagePage = phidInfo->usUsagePage;
						g_pRawInputDevices[g_nUsagePage1Usage8Devices].usUsage     = phidInfo->usUsage;
						g_pRawInputDevices[g_nUsagePage1Usage8Devices].dwFlags     = 0;
						g_pRawInputDevices[g_nUsagePage1Usage8Devices].hwndTarget  = NULL;
						g_nUsagePage1Usage8Devices++;
					}
				}
			}
		}
	}

	// Register for input from the devices in the list
	if (RegisterRawInputDevices( g_pRawInputDevices, g_nUsagePage1Usage8Devices, sizeof(RAWINPUTDEVICE) ) == false )
	{
		//MessageBox( NULL, _T("Error calling RegisterRawInputDevices"), _T("WM_INPUTExamples error"), MB_OK );
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool C3DConnexionDriver::GetInputMessageData( LPARAM lParam,S3DConnexionMessage &msg )
{
	ZeroStruct( msg );

	RAWINPUTHEADER header;
	UINT size = sizeof(header);
	if ( GetRawInputData( (HRAWINPUT)lParam, RID_HEADER, &header,  &size, sizeof(RAWINPUTHEADER) ) == -1)
	{
		//MessageBox( NULL, _T("Error from GetRawInputData(RID_HEADER)"), _T("WM_INPUTExample"), MB_OK );
		return false;
	}
	else
	{
		//_RPT1( _CRT_WARN, "rawEvent.header: hDevice = 0x%x\n", header.hDevice );
	}

	// Set aside enough memory for the full event
	char rawbuffer[128];
	LPRAWINPUT event = (LPRAWINPUT)rawbuffer;
	size = sizeof(rawbuffer);
	if (GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, event, &size, sizeof(RAWINPUTHEADER) ) == -1)
	{
		//MessageBox( NULL, _T("Error from GetRawInputData(RID_INPUT)"), _T("WM_INPUTExample"), MB_OK );
		return false;
	}
	else
	{
		if (event->header.dwType == RIM_TYPEHID)
		{
			static BOOL bGotTranslation = FALSE, 
				bGotRotation    = FALSE;
			static int all6DOFs[6] = {0};
			LPRAWHID pRawHid = &event->data.hid;

			// Translation or Rotation packet?  They come in two different packets.
			if (pRawHid->bRawData[0] == 1) // Translation vector
			{
				msg.raw_translation[0] = (pRawHid->bRawData[1] & 0x000000ff) | ((signed short)(pRawHid->bRawData[2]<<8) & 0xffffff00); 
				msg.raw_translation[1] = (pRawHid->bRawData[3] & 0x000000ff) | ((signed short)(pRawHid->bRawData[4]<<8) & 0xffffff00); 
				msg.raw_translation[2] = (pRawHid->bRawData[5] & 0x000000ff) | ((signed short)(pRawHid->bRawData[6]<<8) & 0xffffff00);
				msg.vTranslate.x = msg.raw_translation[0] / 255.f * m_fMultiplier;
				msg.vTranslate.y = msg.raw_translation[1] / 255.f * m_fMultiplier;
				msg.vTranslate.z = msg.raw_translation[2] / 255.f * m_fMultiplier;
				msg.bGotTranslation = true;
			}
			else if (pRawHid->bRawData[0] == 2) // Rotation vector
			{
				msg.raw_rotation[0] = (pRawHid->bRawData[1] & 0x000000ff) | ((signed short)(pRawHid->bRawData[2]<<8) & 0xffffff00); 
				msg.raw_rotation[1] = (pRawHid->bRawData[3] & 0x000000ff) | ((signed short)(pRawHid->bRawData[4]<<8) & 0xffffff00); 
				msg.raw_rotation[2] = (pRawHid->bRawData[5] & 0x000000ff) | ((signed short)(pRawHid->bRawData[6]<<8) & 0xffffff00);
				msg.vRotate.x = msg.raw_rotation[0] / 255.f * m_fMultiplier;
				msg.vRotate.y = msg.raw_rotation[1] / 255.f * m_fMultiplier;
				msg.vRotate.z = msg.raw_rotation[2] / 255.f * m_fMultiplier;
				msg.bGotRotation = true;
			}
			else if (pRawHid->bRawData[0] == 3) // Buttons (display most significant byte to least)
			{
				msg.buttons[0] = (unsigned char)pRawHid->bRawData[1];
				msg.buttons[1] = (unsigned char)pRawHid->bRawData[2];
				msg.buttons[2] = (unsigned char)pRawHid->bRawData[3];
				//_RPT3(_CRT_WARN, "Button mask: %.2x %.2x %.2x\n",(unsigned char)pRawHid->bRawData[3],(unsigned char)pRawHid->bRawData[2],(unsigned char)pRawHid->bRawData[1]);
				CryLog( "Button mask: %.2x %.2x %.2x\n",(unsigned char)pRawHid->bRawData[3],(unsigned char)pRawHid->bRawData[2],(unsigned char)pRawHid->bRawData[1] );

				if (msg.buttons[0] == 1)
					m_fMultiplier /= 2.0f;
				else if (msg.buttons[0] == 2)
					m_fMultiplier *= 2.0f;
			}
		}
	}
	return true;
}
