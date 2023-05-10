#pragma once
#include "../../sdk/entity.h"
#include "../../context.h"
#include "../../utils/math.h"
#include "../../core/config.h"
#include "../misc/misc.h"
#include "autowall.h"

class CAntiAim {
public:
	bool Pitch( CUserCmd& cmd );
	void FakeLag( int cmdNum );
	void RunLocalModifications( CUserCmd& cmd, bool& sendPacket );

	int ManualSide{ };
	float m_flLowerBodyRealignTimer{};
private:
	bool ChokeCycleJitter{ };
	bool m_bAntiBackstab{ };

	bool Condition( CUserCmd& cmd, bool checkCmd = false );
	float BaseYaw( CUserCmd& cmd );
	void Yaw( CUserCmd& cmd, bool& sendPacket );
	void PickYaw( float& yaw );
	void AtTarget( float& yaw );
	//bool AutoDirection( float& yaw );

	bool m_bInvertFlick{ };
	int m_iFlickTimer{ };
};

namespace Features { inline CAntiAim Antiaim; };