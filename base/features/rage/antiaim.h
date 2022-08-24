#pragma once
#include "../../sdk/entity.h"
#include "../../context.h"
#include "../../utils/math.h"
#include "../../core/config.h"
#include "../misc/misc.h"
#include "autowall.h"

class CAntiAim {
public:
	void Pitch( CUserCmd& cmd );
	void FakeLag( );
	void RunLocalModifications( CUserCmd& cmd, bool sendPacket );

	int ManualSide{ };

private:
	bool Invert{ };
	bool ChokeCycleJitter{ };
	bool Condition( CUserCmd& cmd );
	float BaseYaw( CUserCmd& cmd );
	void PickYaw( float& yaw );
	void AtTarget( float& yaw );
	//bool AutoDirection( float& yaw );
};

namespace Features { inline CAntiAim Antiaim; };