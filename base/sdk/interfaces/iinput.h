#pragma once
// used: usercmd
#include "../datatypes/usercmd.h"

#define MULTIPLAYER_BACKUP 150

class IInput
{
public:
	void*				vtable;					// 0x00
	bool				m_bTrackIR;				// 0x04
	bool				bMouseInitialized;		// 0x05
	bool				bMouseActive;			// 0x06
	std::byte			pad1[ 0x96 ];			// 0x07
	bool				bCameraInThirdPerson;	// 0x9D
	std::byte			pad2[0x2];				// 0x9E
	Vector				vecCameraOffset;		// 0xA0
	std::byte			pad3[ 0x40 ];			// 0xAC
	CUserCmd*			pCommands;				// 0xEC
	CVerifiedUserCmd*	pVerifiedCommands;		// 0xF0

	[[nodiscard]] CUserCmd* GetUserCmd(const int nSequenceNumber) const
	{
		return &pCommands[nSequenceNumber % MULTIPLAYER_BACKUP];
	}

	[[nodiscard]] CVerifiedUserCmd* GetVerifiedCmd(const int nSequenceNumber) const
	{
		return &pVerifiedCommands[nSequenceNumber % MULTIPLAYER_BACKUP];
	}
};
