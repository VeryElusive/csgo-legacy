#pragma once
#include "../../sdk/entity.h"
#include "../../context.h"
#include "../rage/antiaim.h"

enum SideIndexes{
	MIDDLE,
	LEFT,
	RIGHT
};

struct sides_t {
	float m_flAbsYaw{ };
	float m_flOldAbsYaw{ };
	float m_flMoveYaw{ };
	float m_flMoveYawCurtoIdeal{ };
	float m_flMoveYawIdeal{ };

	int m_iBonesCount{ 256 };

	CAnimationLayer m_pLayers[ 13 ]{ };

	//std::array<float, 24> pose;

	matrix3x4_t m_pMatrix[ 256 ]{ };
};

struct AnimData_t {
	inline AnimData_t( CBasePlayer* player ) :
		m_flSimulationTime( player->m_flSimulationTime( ) ),
		m_vecOrigin( player->m_vecOrigin( ) ),
		m_iFlags( player->m_fFlags( ) ),
		m_vecVelocity( player->m_vecVelocity( ) ),
		m_flDuckAmount( player->m_flDuckAmount( ) ),
		m_pWeapon( player->GetWeapon( ) )
	{
		memcpy( m_pLayers, player->m_AnimationLayers( ), 0x38 * player->m_iAnimationLayersCount( ) );
	}

	inline void Update( CBasePlayer* player ) {
		this->m_vecOrigin = player->m_vecOrigin( );
		this->m_iFlags = player->m_fFlags( );
		this->m_vecVelocity = player->m_vecVelocity( );
		this->m_flDuckAmount = player->m_flDuckAmount( );
		this->m_pWeapon = player->GetWeapon( );

		memcpy( m_pLayers, player->m_AnimationLayers( ), 0x38 * player->m_iAnimationLayersCount( ) );
	}

	int m_iFlags{ };

	float m_flSimulationTime{ };
	float m_flDuckAmount{ };

	Vector m_vecOrigin{ };
	Vector m_vecVelocity{ };

	CAnimationLayer m_pLayers[ 13 ];

	CWeaponCSBase* m_pWeapon{ };

	std::array<sides_t, 3> m_cAnimSides;
};

struct PlayerEntry;
struct LagRecord_t {
	inline LagRecord_t( CBasePlayer* player ) :
		m_cAnimData{ player },
		m_iNewCmds( TIME_TO_TICKS( player->m_flSimulationTime( ) - player->m_flOldSimulationTime( ) ) ),
		m_vecMins( player->m_vecMins( ) ), m_vecMaxs( player->m_vecMaxs( ) ), m_angEyeAngles( player->m_angEyeAngles( ) ),
		m_iReceiveTick(Interfaces::ClientState->iServerTick ), m_bDormant( player->IsDormant( ) )
	{ }

	AnimData_t m_cAnimData;

	bool m_bBrokeLC{ true };
	bool m_bAnimated{ };
	bool m_bMultiMatrix{ };
	bool m_bDormant{ };

	std::optional<bool> m_bLanded{ };
	float m_flOnGroundTime{ };

	int m_iResolverSide{ };
	int m_iNewCmds{ };
	int m_iReceiveTick{ };

	Vector m_vecMins{ };
	Vector m_vecMaxs{ };

	QAngle m_angEyeAngles{ };

	inline void FinalAdjustments( CBasePlayer* player, std::optional <AnimData_t>& previous );
	inline bool IsValid( );
	inline void Apply( CBasePlayer* ent, bool onlyAnim = false );
	inline void SelectResolverSide( PlayerEntry& entry );
};

struct Interpolated_t {
	Interpolated_t( ) :
		m_flSimulationTime( 0.f ),
		m_flDuckAmount( 0.f ),
		m_iFlags( 0 ),
		m_vecVelocity( 0, 0, 0 ) {}

	Interpolated_t( const float sim_time, const float duck_amount, const int flags, const Vector& velocity ) :
		m_flSimulationTime( sim_time ),
		m_flDuckAmount( duck_amount ),
		m_iFlags( flags ),
		m_vecVelocity( velocity ) {}

	float m_flSimulationTime{ };
	float m_flDuckAmount{ };

	int m_iFlags{ };

	Vector m_vecVelocity{ };
};

struct PlayerEntry {
	CBasePlayer* m_pPlayer{ };
	std::optional < AnimData_t > m_optPreviousData{ };
	std::deque< std::shared_ptr<LagRecord_t>> m_pRecords{ };
	std::vector< Interpolated_t > m_pInterpolatedData{ };

	//std::optional < std::string > m_optDbgData{ };
	matrix3x4_t m_matMatrix[ 256 ]{ };
	Vector m_vecUpdatedOrigin{ };
	Vector m_vecLastOrigin{ };

	float m_flSpawnTime{ };
	float m_flJitterAmount{ };

	bool m_bBrokeLC{ };

	int m_iMissedShots{ };
	int m_iResolverSide{ };
	int m_iFirstResolverSide{ };

	int m_iRealChoked{ };
	int m_iLastUnchoked{ };

	inline void reset( );
};

class CAnimationSys {
public:
	void RunAnimationSystem( );
	void AnimatePlayer( LagRecord_t* current, PlayerEntry& entry );
	void UpdateSide( PlayerEntry& entry, LagRecord_t* current, const int side );
	FORCEINLINE void SetupInterp( LagRecord_t* to, PlayerEntry& entry );

	bool SetupBonesFixed( CBasePlayer* const player, matrix3x4_t bones[ 256 ], const float time, const int flags );

	void SetupLocalMatrix( );
	void UpdateLocal( const QAngle& view_angles, const bool only_anim_state );
	void UpdateCommands( );
	void UpdatePlayerMatrixes( );
	void GetSide( PlayerEntry& entry );

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