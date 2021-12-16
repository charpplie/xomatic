/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   SteeringEnviroment.h
Description: 
---------------------------------------------------------------------
History:
- 11:02:2008 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _STEERING_ENVIROMENT_H_
	#define _STEERING_ENVIROMENT_H_

class CSteeringEnviroment
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CSteeringEnviroment();
		~			CSteeringEnviroment();
		bool	Init();
		void	Update( float fDeltaTime );

		/*$1- Util ---------------------------------------------------------------*/

		/*$1- Debug  -------------------------------------------------------------*/
		void	DebugDraw() const;

	private:

	/*$1- Util -----------------------------------------------------------------*/
	private:

		/*$1- Members ------------------------------------------------------------*/

		/*$1- Static debug data --------------------------------------------------*/
		bool	m_bInit;
};
#endif // _STEERING_ENVIROMENT_H_
