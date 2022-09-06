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
	void Yaw( CUserCmd& cmd, bool sendPacket );
	void FakeLag( );
	bool Condition( CUserCmd& cmd );

	int ManualSide{ };
	bool m_bCanBreakLBY{ };

private:
	float m_flLowerBodyRealignTimer{ };

	bool ChokeCycleJitter{ };
	float BaseYaw( CUserCmd& cmd );
	void PickYaw( float& yaw );
	void AtTarget( float& yaw );
	//bool AutoDirection( float& yaw );
};

namespace Features { inline CAntiAim Antiaim; };