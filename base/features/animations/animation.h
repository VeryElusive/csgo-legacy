#pragma once
#include "../../sdk/entity.h"
#include "../../context.h"
#include "../rage/antiaim.h"

struct AnimData_t {
	inline AnimData_t( CBasePlayer* player ) :
		m_flSimulationTime( player->m_flSimulationTime( ) ),
		m_flOldSimulationTime( player->m_flOldSimulationTime( ) ),
		m_vecOrigin( player->m_vecOrigin( ) ),
		m_iFlags( player->m_fFlags( ) ),
		m_flDuckAmount( player->m_flDuckAmount( ) ),
		m_vecMins( player->m_vecMins( ) ), 
		m_vecMaxs( player->m_vecMaxs( ) ),
		m_flLowerBodyYawTarget( player->m_flLowerBodyYawTarget( ) ),
		m_vecVelocity( player->m_vecVelocity( ) ),
		m_pWeapon( player->GetWeapon( ) )
	{
		memcpy( m_pLayers, player->m_AnimationLayers( ), 0x38 * 13 );
	}

	int m_iFlags{ };

	float m_flSimulationTime{ };
	float m_flOldSimulationTime{ };
	float m_flDuckAmount{ };
	float m_flLowerBodyYawTarget{ };

	Vector m_vecOrigin{ };
	Vector m_vecMins{ };
	Vector m_vecMaxs{ };
	Vector m_vecVelocity{ };


	CAnimationLayer m_pLayers[ 13 ];

	CWeaponCSBase* m_pWeapon{ };
};

struct PlayerEntry;
struct LagRecord_t {
	inline LagRecord_t( CBasePlayer* player ) :
		m_cAnimData{ player },
		m_angEyeAngles( player->m_angEyeAngles( ) ),
		m_nSequence( player->m_nSequence( ) ),
		m_flCycle( player->m_flCycle( ) ),
		m_iNewCmds( TIME_TO_TICKS( player->m_flSimulationTime( ) - player->m_flOldSimulationTime( ) ) ),
		m_iReceiveTick( Interfaces::ClientState->iServerTick ),
		m_bDormant( player->IsDormant( ) ) { }

	AnimData_t m_cAnimData;

	matrix3x4a_t m_pMatrix[ 256 ]{ };

	bool m_bBrokeLC{ true };
	bool m_bDormant{ };
	bool m_bResolverThisTick{ };

	bool m_bFixJumpFall{ };
	float m_flLeftGroundTime{ };

	int m_iReceiveTick{ };
	int m_iNewCmds{ 1 };
	//float m_flReceiveTime{ };

	QAngle m_angEyeAngles{ };

	int m_nSequence{ };
	float m_flCycle{ };

	int m_iBonesCount{ 256 };

	FORCEINLINE void FinalAdjustments( CBasePlayer* player, const std::optional <AnimData_t>& previous );
	FORCEINLINE uint8_t Validity( );
	FORCEINLINE void Apply( CBasePlayer* ent );
};

struct PlayerEntry {
	CBasePlayer* m_pPlayer{ };
	std::optional < AnimData_t > m_optPreviousData{ };
	std::vector< std::shared_ptr<LagRecord_t>> m_pRecords{ };
	//std::vector<std::pair<int, float> > m_flPreviousYaws{ };
	//std::array<float, 50> m_arrPreviousYaws{ };
	//std::vector< Interpolated_t > m_pInterpolatedData{ };

	//std::optional < std::string > m_optDbgData{ };
	matrix3x4a_t m_matMatrix[ 256 ]{ };
	//Vector m_vecMatrixOriginDelta[ 256 ]{ };
	Vector m_vecUpdatedOrigin{ };
	Vector m_vecLastMergeOrigin{ };

	float m_flSpawnTime{ };
	//float m_flLastPacketTime{ };
	//float m_flTrackSimulationTime{ };
	//float m_flExtrapolatedEyeYaw{ };
	//float m_flJitterAmount{ };

	bool m_bBrokeLC{ };
	bool m_bBot{ };
	bool m_bRecordAdded{ };
	//bool m_bExtrapolatedOnGround{ };

	int m_iMissedShots{ };
	//int m_iLastResolvedSide{ 0 };
	//int m_iResolverSide{ 1 };
	//int m_iFirstResolverSide{ };
	int m_iLastRecievedTick{ };

	int m_iLastNewCmds{ };
	bool m_bCanExtrapolate{ };

	FORCEINLINE void OutOfDormancy( );
	FORCEINLINE void OnNewRound( );
	FORCEINLINE void OnPlayerChange( CBasePlayer* player );
};

class CAnimationSys {
public:
	void RunAnimationSystem( );
	void AnimatePlayerThread( PlayerEntry& entry );
	void AnimatePlayer( LagRecord_t* current, PlayerEntry& entry );
	void UpdateSide( PlayerEntry& entry, LagRecord_t* current );
	void InterpolateFromLastData( CBasePlayer* player, LagRecord_t* current, std::optional < AnimData_t >& from );
	void Rezik( CBasePlayer* player, LagRecord_t* current, std::optional < AnimData_t >& from );
	//FORCEINLINE void SetupInterp( LagRecord_t* to, PlayerEntry& entry );

	bool SetupBonesRebuilt( CBasePlayer* const player, matrix3x4a_t* bones, const int mask, const float time, const bool clampbonesinbbox );
	void GetSkeleton( CBasePlayer* player, CStudioHdr* hdr, Vector* pos, Quaternion* q, int boneMask, CIKContext* ik );
	void BuildMatrices( CBasePlayer* player, CStudioHdr* hdr, Vector* pos, Quaternion* q, matrix3x4a_t* bones, int boneMask, uint8_t boneComputed[ ] );

	//FORCEINLINE float SetupVelocityRebuild( CCSGOPlayerAnimState* animstate, float newFootYaw, float eyeYaw );
	void AnimateToThisPoint( );
	void UpdateLocal( QAngle view_angles, const bool only_anim_state, CUserCmd& cmd );
	void UpdateServerLayers( CUserCmd& cmd );
	void UpdateCommands( );

	std::array< PlayerEntry, 64> m_arrEntries;

	//std::vector<void*> m_vecTrackingAnimstates{ };

	float m_flLowerBodyRealignTimer{ };

	bool m_bJumping{ };
	//bool m_bLandedAfterJump{ };
};

class LagBackup_t {
public:
	__forceinline LagBackup_t( CBasePlayer* ent );

	__forceinline void Apply( CBasePlayer* ent );

	Vector m_vecAbsOrigin;
	Vector m_vecMins;
	Vector m_vecMaxs;

	float m_flNewBoundsTime{ };
	float m_flNewBoundsMaxs{ };

	matrix3x4_t m_matMatrix[ 256 ];
};

namespace Features { inline CAnimationSys AnimSys; };

#include "record_utils.inl"