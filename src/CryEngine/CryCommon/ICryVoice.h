#ifndef __ICRYVOICE_H__
#define __ICRYVOICE_H__

#pragma once

struct ICryVoice
{
	// Mute
	// Turn voice on/off between a local and remote user. When mute is on no voice will be sent to or received from the remote user.
	// localUser	- The pad number of the local user.
	// channelID	- The game channel the remote user is on.
	// remoteUser	-	The index of the remote user on channel. Since only 1 user can currently be on a channel this will be 0.
	// mute				- Mute on/off
	virtual	void	Mute(uint32 localUser, uint32 channelID, uint32 remoteUser, bool mute) = 0;

	// IsMuted
	// Has voice between a local and remote user been muted by game.
	// localUser	- The pad number of the local user.
	// channelID	- The game channel the remote user is on.
	// remoteUser	-	The index of the remote user on channel. Since only 1 user can currently be on a channel this will be 0.
	// return			- Will return true if the game has set mute to true.
	virtual	bool	IsMuted(uint32 localUser, uint32 channelID, uint32 remoteUser) = 0;

	// IsMutedExternally
	// Has voice between a local and remote user muted via the external SDK's GUI.
	// Microsoft TCR's say that a user must have no indication that they have been muted with the Xbox GUI
	// so this function must not be used for that purpose.
	// localUser	- The pad number of the local user.
	// channelID	- The game channel the remote user is on.
	// remoteUser	-	The index of the remote user on channel. Since only 1 user can currently be on a channel this will be 0.
	// return			- Will return true if the user has set mute to true via the external SDK's GUI.
	virtual	bool	IsMutedExternally(uint32 localUser, uint32 channelID, uint32 remoteUser) = 0;

	// IsSpeaking
	// Is the remote user currently speaking to the local user.
	// localUser	- The pad number of the local user.
	// channelID	- The game channel the remote user is on.
	// remoteUser	-	The index of the remote user on channel. Since only 1 user can currently be on a channel this will be 0.
	// return			- true if voice data is currently being received from the remote user to the local user.
	virtual bool	IsSpeaking(uint32 localUser, uint32 channelID, uint32 remoteUser) = 0;
};

#endif // __ICRYVOICE_H__