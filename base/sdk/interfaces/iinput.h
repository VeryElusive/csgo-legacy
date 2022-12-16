#pragma once
// used: usercmd
#include "../datatypes/usercmd.h"

#define MULTIPLAYER_BACKUP 150

class IInput
{
public:
	std::uint8_t        pad0[ 4u ]{ };
	bool                m_bTrackIR{ },
		bMouseInitialized{ },
		bMouseActive{ };
	std::byte			pad1[ 158u ];
	bool                bCameraInThirdPerson{ };
	std::uint8_t        pad2[ 1u ]{ };
	Vector         vecCameraOffset{ };
	std::uint8_t        pad3[ 54u ]{ };
	CUserCmd* pCommands{ };
	CVerifiedUserCmd* pVerifiedCommands{ };

	[[nodiscard]] CUserCmd* GetUserCmd(const int nSequenceNumber) const
	{
		return &pCommands[nSequenceNumber % MULTIPLAYER_BACKUP];
	}

	[[nodiscard]] CVerifiedUserCmd* GetVerifiedCmd(const int nSequenceNumber) const
	{
		return &pVerifiedCommands[nSequenceNumber % MULTIPLAYER_BACKUP];
	}

	__forceinline int CAM_IsThirdPerson( int slot = -1 )
	{
		return MEM::CallVFunc<int>( this, 32, slot );
	}

	__forceinline void CAM_ToThirdPerson( ) {
		return MEM::CallVFunc<void>( this, 35 );
	}

	__forceinline void CAM_ToFirstPerson( ) {
		return MEM::CallVFunc<void>( this, 36 );
	}
};

