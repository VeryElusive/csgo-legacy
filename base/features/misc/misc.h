#pragma once
#include "../../context.h"
#include "../../utils/math.h"
#include "../../core/config.h"

class CMisc {
public:
	void Thirdperson( );
	void Movement( CUserCmd& cmd );
	void MoveMINTFix( CUserCmd& cmd, QAngle wish_angles, int flags, int move_type );
	void NormalizeMovement( CUserCmd& cmd );
	void AutoPeek( CUserCmd& cmd );
	bool AutoStop( CUserCmd& cmd );

	float TPFrac{ };
	Vector OldOrigin{ };
	bool AutoPeeking{ };

	Vector2D m_ve2OldMovement{ };
	bool m_bWasJumping{ };
private:
	QAngle MovementAngle{ };
	float OldYaw{ };
	bool ShouldRetract{ };

	void QuickStop( CUserCmd& cmd );
	bool SlowWalk( CUserCmd& cmd );
	void AutoStrafer( CUserCmd& cmd );
	void FakeDuck( CUserCmd& cmd );
	bool MicroMove( CUserCmd& cmd );
	void LimitSpeed( CUserCmd& cmd, float speed );
};

namespace Features { inline CMisc Misc; };