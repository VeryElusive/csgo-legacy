#pragma once
#include "sdk/datatypes/vector.h"
#include "sdk/entity.h"
#include "core/interfaces.h"
#include <deque>

struct PredictedNetvars_t {
	int m_MoveType{ };
	int m_iFlags{ };
	Vector m_vecOrigin{ };
	Vector m_vecVelocity{ };
	Vector m_vecAbsVelocity{ };
	int m_nTickBase{ };
};

struct FakeAnimData_t {
	float m_flSpawnTime{ };
	float m_flAbsYaw{ };
	CCSGOPlayerAnimState m_AnimState{ };
	std::array<float, MAXSTUDIOPOSEPARAM> m_flPoseParameter{ };

	matrix3x4_t m_matMatrix[ 256 ];
	CAnimationLayer m_pAnimLayers[ 13 ];
};


struct LocalData_t {
	int m_iCommandNumber{ };

	int m_MoveType{ };

	int m_iTickbase{ };
	int m_iShiftAmount{ };
	int m_iAdjustedTickbase{ };
	bool m_bOverrideTickbase{ };
	bool m_bRestoreTickbase{ };

	bool m_bThrowingNade{ };

	QAngle m_angViewAngles{ };

	float m_flSpawnTime{ };

	PredictedNetvars_t PredictedNetvars{ };


	FORCEINLINE void Reset( ) {
		m_iTickbase = 0;
		m_iCommandNumber = 0;
		m_iShiftAmount = 0;

		m_iAdjustedTickbase = 0;
		m_bOverrideTickbase = 0;
		m_bRestoreTickbase = 0;

		m_angViewAngles = { };

		m_flSpawnTime = 0;

		PredictedNetvars = { };
	}

	FORCEINLINE void Save( CBasePlayer* local, CUserCmd& cmd, CWeaponCSBase* weapon ) {
		this->m_flSpawnTime = local->m_flSpawnTime( );
		this->m_bOverrideTickbase = false;
		this->m_iShiftAmount = 0;
		this->m_iCommandNumber = cmd.iCommandNumber;
		this->m_iTickbase = local->m_nTickBase( );
		this->m_MoveType = local->m_MoveType( );
		if ( !weapon )
			return;

		this->m_bThrowingNade = weapon->IsGrenade( ) && weapon->m_fThrowTime( );;
	}

	FORCEINLINE void SavePredVars( CBasePlayer* local, CUserCmd& cmd ) {
		this->PredictedNetvars.m_MoveType = local->m_MoveType( );
		this->PredictedNetvars.m_iFlags = local->m_fFlags( );
		this->PredictedNetvars.m_nTickBase = local->m_nTickBase( );
		this->PredictedNetvars.m_vecOrigin = local->m_vecOrigin( );
		this->PredictedNetvars.m_vecVelocity = local->m_vecVelocity( );
		this->PredictedNetvars.m_vecAbsVelocity = local->m_vecAbsVelocity( );

		this->m_angViewAngles = cmd.viewAngles;
	}
};

struct LocalData_t;

struct HAVOCCTX {
	Vector2D m_ve2ScreenSize{ };
	CBasePlayer* m_pLocal{ };
	CWeaponCSBase* m_pWeapon{ };
	CCSWeaponData* m_pWeaponData{ };

	EClientFrameStage m_iLastFSNStage{ };

	matrix3x4_t m_matRealLocalBones[ 256 ];

	QAngle m_angOriginalViewangles{ };

	Vector m_vecEyePos{ };

	std::array< LocalData_t, MULTIPLAYER_BACKUP > m_cLocalData{ };

	std::vector<int> m_iSentCmds{ };
	std::vector<std::pair<std::string, void*>> m_strDbgLogs{ };

	//std::vector<std::pair<QAngle, int>> m_pQueuedCommands{ };
	bool m_bFilledAnims{ };

	float m_flAbsYaw{ };
	CAnimationLayer m_pAnimationLayers[ 13 ];
	std::array<float, 24> m_flPoseParameter{ };
	CCSGOPlayerAnimState m_cAnimstate{ };

	bool m_bCanShoot{ };
	bool m_bCanPenetrate{ };
	bool m_bSetupBones{ };
	bool m_bUpdatingAnimations{ };
	bool m_bForceBoneMask{ };
	bool m_bFakeDucking{ };
	bool m_bSendPacket{ };
	bool m_bClearKillfeed{ };
	bool m_bExploitsEnabled{ };

	int m_iTicksAllowed{ };
	int m_iLastSentCmdNumber{ };
	int m_iLastShotNumber{ };
	int m_iPenetrationDamage{ };
	int m_iBombCarrier{ };

	float m_iLastShotTime{ };
	float m_flOutLatency{ };
	float m_flInLatency{ };
	float m_flLerpTime{ };

	FORCEINLINE void GetLocal( ) {
		if ( !Interfaces::Engine->IsInGame( ) ) {
			m_pLocal = nullptr;
			return;
		}

		m_pLocal = static_cast< CBasePlayer * >( Interfaces::ClientEntityList->GetClientEntity( Interfaces::Engine->GetLocalPlayer( ) ) );

		//return *reinterpret_cast< CBasePlayer** >( Offsets::Sigs.LocalPlayer );
	};

	FORCEINLINE int CalcCorrectionTicks( ) {
		return Interfaces::Globals->nMaxClients <= 1
			? -1 : TIME_TO_TICKS( std::clamp<float>( Offsets::Cvars.sv_clockcorrection_msecs->GetFloat( ) / 1000.f, 0.f, 1.f ) );
	}
};
inline HAVOCCTX ctx;