#pragma once
#include "../../sdk/entity.h"
#include "../../context.h"
#include "../../utils/math.h"
#include "../../core/config.h"
#include "../misc/misc.h"
#include "autowall.h"

class CAntiAim {
public:
	float Pitch( );
	void FakeLag( int cmdNum );
	void RunLocalModifications( CUserCmd& cmd, int tickbase );
	float BaseYaw( CUserCmd& cmd );
	bool Condition( CUserCmd& cmd, bool checkCmd = false );

	int ManualSide{ };
	bool Invert{ };
	int m_iChoiceSide{ };
private:
	bool ChokeCycleJitter{ };
	bool m_bAntiBackstab{ };
	int Freestanding( );
	void PickYaw( float& yaw );
	void AtTarget( float& yaw );
	//bool AutoDirection( float& yaw );
};

namespace Features { inline CAntiAim Antiaim; };