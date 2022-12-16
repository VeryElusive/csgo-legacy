#pragma once
#include "../../context.h"
#include "../../utils/math.h"
#include "../../core/config.h"

struct ExtrapolationData_t {
	__forceinline constexpr ExtrapolationData_t( ) = default;

	__forceinline ExtrapolationData_t(
		CBasePlayer* const player
	) : m_pPlayer{ player }, m_iFlags{ player->m_fFlags( ) },
		m_bWasInAir{ !( player->m_fFlags( ) & FL_ONGROUND ) }, m_vecOrigin{ player->m_vecOrigin( ) },
		m_vecVelocity{ player->m_vecVelocity( ) }, m_vecMins{ player->m_vecMins( ) }, m_vecMaxs{ player->m_vecMaxs( ) } {}

	CBasePlayer* m_pPlayer{ };

	int m_iFlags{ };
	bool m_bWasInAir{ };

	Vector m_vecOrigin{ }, m_vecVelocity{ }, m_vecMins{ }, m_vecMaxs{ };
};

class CMisc {
public:
	void Thirdperson( );
	void Movement( CUserCmd& cmd );
	void MoveMINTFix( CUserCmd& cmd, QAngle wish_angles, int flags, int move_type );
	void NormalizeMovement( CUserCmd& cmd );
	void AutoPeek( CUserCmd& cmd );
	bool AutoStop( CUserCmd& cmd );
	bool InPeek( );
	void PlayerMove( ExtrapolationData_t& data );

	// FAKE PING
	void UpdateIncomingSequences( INetChannel* pNetChannel );
	void ClearIncomingSequences( );
	void AddLatencyToNetChannel( INetChannel* pNetChannel, float flLatency );

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
	void LimitSpeed( CUserCmd& cmd, float speed );


	// FAKE PING
	// Values
	std::deque<SequenceObject_t> m_vecSequences = { };
	/* our real incoming sequences count */
	int m_nRealIncomingSequence = 0;
	/* count of incoming sequences what we can spike */
	int m_nLastIncomingSequence = 0;
};

namespace Features { inline CMisc Misc; };