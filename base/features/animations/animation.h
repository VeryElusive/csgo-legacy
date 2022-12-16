#pragma once
#include "../../sdk/entity.h"
#include "../../context.h"
#include "../rage/antiaim.h"

struct AnimData_t {
	inline AnimData_t( CBasePlayer* player ) :
		m_flSimulationTime( player->m_flSimulationTime( ) ),
		m_vecOrigin( player->m_vecOrigin( ) ),
		m_iFlags( player->m_fFlags( ) ),
		m_flDuckAmount( player->m_flDuckAmount( ) ),
		m_vecMins( player->m_vecMins( ) ), 
		m_vecMaxs( player->m_vecMaxs( ) ),
		m_vecVelocity( player->m_vecVelocity( ) ),
		m_pWeapon( player->GetWeapon( ) )
	{
		memcpy( m_pLayers, player->m_AnimationLayers( ), 0x38 * player->m_iAnimationLayersCount( ) );
	}

	int m_iFlags{ };

	float m_flSimulationTime{ };
	float m_flDuckAmount{ };

	Vector m_vecOrigin{ };
	Vector m_vecMins{ };
	Vector m_vecMaxs{ };
	Vector m_vecVelocity{ };

	CAnimationLayer m_pLayers[ 13 ];

	CWeaponCSBase* m_pWeapon{ };

	float m_flAbsYaw{ };
	matrix3x4_t m_pMatrix[ 256 ]{ };
	std::array<float, 24> m_flPoseParameter;
	CIKContext m_cIk{ };
};

struct PlayerEntry;
struct LagRecord_t {
	inline LagRecord_t( CBasePlayer* player ) :
		m_cAnimData{ player },
		m_angEyeAngles( player->m_angEyeAngles( ) ),
		m_nSequence( player->m_nSequence( ) ),
		m_flCycle( player->m_flCycle( ) ),
		m_iNewCmds( TIME_TO_TICKS( player->m_flSimulationTime( ) - player->m_flOldSimulationTime( ) ) ),
		m_iReceiveTick(Interfaces::ClientState->iServerTick ),
		m_flReceiveTime(Interfaces::Globals->flRealTime ), m_bDormant( player->IsDormant( ) ) { }

	AnimData_t m_cAnimData;

	bool m_bBrokeLC{ true };
	bool m_bAnimated{ };
	bool m_bDormant{ };
	bool m_bResolverThisTick{ };

	std::optional<bool> m_bLanded{ };
	float m_flOnGroundTime{ };

	int m_iReceiveTick{ };
	int m_iNewCmds{ };
	float m_flReceiveTime{ };

	QAngle m_angEyeAngles{ };

	int m_nSequence{ };
	float m_flCycle{ };

	int m_iBonesCount{ 256 };

	FORCEINLINE void FinalAdjustments( CBasePlayer* player, const std::optional <AnimData_t>& previous );
	FORCEINLINE bool IsValid( );
	FORCEINLINE void Apply( CBasePlayer* ent, bool onlyAnim = false );
};

struct PlayerEntry {
	CBasePlayer* m_pPlayer{ };
	std::optional < AnimData_t > m_optPreviousData{ };
	std::deque< std::shared_ptr<LagRecord_t>> m_pRecords{ };
	//std::vector< Interpolated_t > m_pInterpolatedData{ };

	//std::optional < std::string > m_optDbgData{ };
	//matrix3x4_t m_matMatrix[ 256 ]{ };
	Vector m_vecUpdatedOrigin{ };
	Vector m_vecLastOrigin{ };

	float m_flSpawnTime{ };
	float m_flTrackSimulationTime{ };
	//float m_flJitterAmount{ };

	bool m_bBrokeLC{ };

	int m_iMissedShots{ };

	int m_iLastChoked{ };

	FORCEINLINE void reset( );
};

enum ESetupBonesFlags {
	INVALIDATEBONECACHE = 1,
	SETUPBONESFRAME = 2,
	NULLIK = 4,
	OCCLUSIONINTERP = 8,
	CLAMPBONESINBBOX = 16,
};

#define USEALLSETUPBONESFLAGS ( 1 | 2 | 4 | 8 | 16 )

class CAnimationSys {
public:
	void RunAnimationSystem( );
	void AnimatePlayer( LagRecord_t* current, PlayerEntry& entry );
	void UpdateSide( PlayerEntry& entry, LagRecord_t* current );
	void OleksiiReznikov( CBasePlayer* player, LagRecord_t* current );
	//FORCEINLINE void SetupInterp( LagRecord_t* to, PlayerEntry& entry );

	bool SetupBonesFixed( CBasePlayer* const player, matrix3x4_t bones[ 256 ], const int mask, const float time, const int flags, CIKContext* ik = 0 );

	//FORCEINLINE float SetupVelocityRebuild( CCSGOPlayerAnimState* animstate, float newFootYaw, float eyeYaw );

	void SetupLocalMatrix( );
	void UpdateLocal( const QAngle& view_angles, const bool only_anim_state );
	void UpdateLocalFull( CUserCmd& cmd, bool sendPacket );
	void UpdatePlayerMatrixes( );

	std::array< PlayerEntry, 64> m_arrEntries;
};

class LagBackup_t {
public:
	__forceinline LagBackup_t( CBasePlayer* ent );

	__forceinline void Apply( CBasePlayer* ent );


	Vector m_vecOrigin;
	Vector m_vecAbsOrigin;
	Vector m_vecMins;
	Vector m_vecMaxs;

	unsigned long m_lModelBoneCounter;

	int m_iReadableBones;
	int m_iWriteableBones;
	int m_iBonesCount;

	float m_flAbsYaw;

	matrix3x4_t m_matMatrix[ 256 ];
};

namespace Features { inline CAnimationSys AnimSys; };

#include "anim_utils.inl"