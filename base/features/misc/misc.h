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
	bool InPeek( );

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
	void SlowWalk( CUserCmd& cmd );
	void AutoStrafer( CUserCmd& cmd );
	void FakeDuck( CUserCmd& cmd );
	bool MicroMove( CUserCmd& cmd );


	// autostop shiiiit
	void LimitSpeed( CUserCmd& cmd, float speed );
	void FullWalkMoveRebuild( CUserCmd& user_cmd, Vector& fwd, Vector& right, Vector& velocity, float maxSpeed );
	void WalkMoveRebuild( CUserCmd& user_cmd, Vector& fwd, Vector& right, Vector& velocity, float maxSpeed );
	void AccelerateRebuild( CUserCmd& user_cmd, const Vector& wishdir, const float wishspeed, Vector& velocity, float acceleration, float maxSpeed );
};

namespace Features { inline CMisc Misc; };