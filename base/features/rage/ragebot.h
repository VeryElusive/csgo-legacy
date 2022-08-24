#pragma once
#include "../animations/animation.h"
#include "../misc/engine_prediction.h"
#include "../misc/logger.h"
#include "../misc/shot_info.h"
#include "../../sdk/entity.h"
#include "../../context.h"
#include "autowall.h"

#define SETRAGEBOOL( name ) switch ( type ) { \
case WEAPONTYPE_PISTOL: if ( idx == WEAPON_DEAGLE || idx == WEAPON_REVOLVER ) name = Config::Get<bool>( Vars.name##HeavyPistol ); else name = Config::Get<bool>( Vars.name##Pistol );break; \
case WEAPONTYPE_SUBMACHINEGUN: name = Config::Get<bool>( Vars.name##SMG );break; \
case WEAPONTYPE_RIFLE: name = Config::Get<bool>( Vars.name##Rifle );break; \
case WEAPONTYPE_SHOTGUN: name = Config::Get<bool>( Vars.name##Shotgun );break; \
case WEAPONTYPE_SNIPER: if ( idx == WEAPON_AWP ) name = Config::Get<bool>( Vars.name##AWP );else if ( idx == WEAPON_SSG08 ) name = Config::Get<bool>( Vars.name##Scout ); else name = Config::Get<bool>( Vars.name##Auto );break; \
case WEAPONTYPE_MACHINEGUN: name = Config::Get<bool>( Vars.name##Machine );break; }\

#define SETRAGEINT( name ) switch ( type ) { \
case WEAPONTYPE_PISTOL: if ( idx == WEAPON_DEAGLE || idx == WEAPON_REVOLVER ) name = Config::Get<int>( Vars.name##HeavyPistol ); else name = Config::Get<int>( Vars.name##Pistol );break; \
case WEAPONTYPE_SUBMACHINEGUN: name = Config::Get<int>( Vars.name##SMG );break; \
case WEAPONTYPE_RIFLE: name = Config::Get<int>( Vars.name##Rifle );break; \
case WEAPONTYPE_SHOTGUN: name = Config::Get<int>( Vars.name##Shotgun );break; \
case WEAPONTYPE_SNIPER: if ( idx == WEAPON_AWP ) name = Config::Get<int>( Vars.name##AWP );else if ( idx == WEAPON_SSG08 ) name = Config::Get<int>( Vars.name##Scout ); else name = Config::Get<int>( Vars.name##Auto );break; \
case WEAPONTYPE_MACHINEGUN: name = Config::Get<int>( Vars.name##Machine );break; }\

struct AimPoint_t {
	Vector m_vecPoint{ };
	int m_iHitgroup{ };
	int m_iIntersections{ };
	float m_flDamage{ };

	bool m_bValid{ };
	bool m_bPenetrated{ };
	bool m_bScanned{ };

	AimPoint_t( ) {}

	AimPoint_t( Vector point, int ind ) : m_vecPoint{ point }, m_iHitgroup{ ind } {}
};

struct AimTarget_t {
	std::shared_ptr<LagRecord_t> m_pRecord{ };
	CBasePlayer* m_pPlayer{ };
	std::optional<AimPoint_t> m_cAimPoint{ };

	std::vector<AimPoint_t> m_cPoints{ };

	int m_iMissedShots{ };

	AimTarget_t( ) {}
	AimTarget_t( std::shared_ptr<LagRecord_t> r, CBasePlayer* p, int m ) : m_pRecord( r ), m_pPlayer( p ), m_iMissedShots( m ) {}
};

class CRageBot {
public:
	bool m_bShouldStop{ };
	bool RagebotAutoStop{ };
	bool RagebotBetweenShots{ };

	void Main( CUserCmd& cmd );
	std::shared_ptr<LagRecord_t> GetBestLagRecord( PlayerEntry& entry );
	std::vector<AimTarget_t> m_cAimTargets{ };
	std::shared_mutex m_pMutex = { };

private:

	std::vector<int> m_iHitboxes{ };

	int QuickScan( CBasePlayer* player, std::shared_ptr<LagRecord_t> record, bool& dontScan );
	int OffsetDelta( CBasePlayer* player, std::shared_ptr<LagRecord_t> record );
	bool CheckHeadSafepoint( CBasePlayer* player, std::shared_ptr<LagRecord_t> record );
	int SafePoint( CBasePlayer* player, std::shared_ptr<LagRecord_t> record, Vector aimpoint, int index );
	void ScanTargets( );
	bool CreatePoints( AimTarget_t& aim_target, std::vector<AimPoint_t>& aim_points );
	std::size_t CalcPointCount( mstudiohitboxset_t* hitboxSet );
	void Multipoint( Vector& center, matrix3x4_t& matrix, std::vector<AimPoint_t>& aim_points, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float& scale, int index );
	void CalcCapsulePoints( std::vector<AimPoint_t>& aimPoints, mstudiobbox_t* const hitbox, matrix3x4_t& matrix, float scale );
	void ScanPoint( CBasePlayer* player, std::shared_ptr<LagRecord_t> record, AimPoint_t& point );
	std::optional< AimPoint_t> PickPoints( CBasePlayer* player, std::vector<AimPoint_t>& aimPoints );
	std::optional<AimTarget_t> PickTarget( );
	void Fire( CUserCmd& cmd );
	bool HitChance( CBasePlayer* player, const QAngle& ang, int hitchance, int index );
	Vector2D CalcSpreadAngle( const int item_index, const float recoil_index, const int i );

	static void GetTargets( void* i );

	FORCEINLINE void Reset( ) {
		m_bShouldStop = false;
		m_iHitboxes.clear( );
		m_cAimTargets.clear( );
	}

	FORCEINLINE void SetupHitboxes( ) {
		if ( ctx.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_TASER )
			return m_iHitboxes.push_back( HITBOX_CHEST );

		if ( RagebotHBUpperChest )
			m_iHitboxes.push_back( HITBOX_UPPER_CHEST );

		if ( RagebotHBChest )
			m_iHitboxes.push_back( HITBOX_CHEST );

		if ( RagebotHBLowerChest )
			m_iHitboxes.push_back( HITBOX_LOWER_CHEST );

		if ( RagebotHBStomach )
			m_iHitboxes.push_back( HITBOX_STOMACH );

		if ( RagebotHBPelvis )
			m_iHitboxes.push_back( HITBOX_PELVIS );

		if ( RagebotHBArms ) {
			m_iHitboxes.push_back( HITBOX_RIGHT_UPPER_ARM );
			m_iHitboxes.push_back( HITBOX_LEFT_UPPER_ARM );
		}

		if ( RagebotHBLegs ) {
			m_iHitboxes.push_back( HITBOX_RIGHT_THIGH );
			m_iHitboxes.push_back( HITBOX_LEFT_THIGH );

			m_iHitboxes.push_back( HITBOX_RIGHT_CALF );
			m_iHitboxes.push_back( HITBOX_LEFT_CALF );
		}

		if ( RagebotHBFeet ) {
			m_iHitboxes.push_back( HITBOX_RIGHT_FOOT );
			m_iHitboxes.push_back( HITBOX_LEFT_FOOT );
		}

		if ( RagebotHBHead )
			m_iHitboxes.push_back( HITBOX_HEAD );
	}

	FORCEINLINE bool IsMultiPointEnabled( int hitbox ) {

		switch ( hitbox )
		{
		case HITBOX_HEAD:
		case HITBOX_NECK:
			return RagebotMPHead;
		case HITBOX_UPPER_CHEST:
			return RagebotMPUpperChest;
		case HITBOX_CHEST:
			return RagebotMPChest;
		case HITBOX_LOWER_CHEST:
			return RagebotMPLowerChest;
		case HITBOX_STOMACH:
			return RagebotMPStomach;
		case HITBOX_PELVIS:
			return RagebotMPPelvis;
		case HITBOX_RIGHT_UPPER_ARM:
		case HITBOX_LEFT_UPPER_ARM:
			return RagebotMPArms;
		case HITBOX_LEFT_THIGH:
		case HITBOX_RIGHT_THIGH:
		case HITBOX_RIGHT_CALF:
		case HITBOX_LEFT_CALF:
			return RagebotMPLegs;
		case HITBOX_RIGHT_FOOT:
		case HITBOX_LEFT_FOOT:
			return RagebotMPFeet;
		default:
			return false;
		}
	}

	/* cfg */
	// this makes me want to quit programming im so embarrassed
	FORCEINLINE void ParseCfgItems( int type ) {
		const int idx{ ctx.m_pWeapon->m_iItemDefinitionIndex( ) };

		m_iLastWeaponType = type;
		m_iLastWeaponIndex = idx;

		SETRAGEBOOL( RagebotHBHead );
		SETRAGEBOOL( RagebotHBUpperChest );
		SETRAGEBOOL( RagebotHBChest );
		SETRAGEBOOL( RagebotHBLowerChest );
		SETRAGEBOOL( RagebotHBStomach );
		SETRAGEBOOL( RagebotHBPelvis );
		SETRAGEBOOL( RagebotHBArms );
		SETRAGEBOOL( RagebotHBLegs );
		SETRAGEBOOL( RagebotHBFeet );

		SETRAGEBOOL( RagebotMPHead );
		SETRAGEBOOL( RagebotMPUpperChest );
		SETRAGEBOOL( RagebotMPChest );
		SETRAGEBOOL( RagebotMPLowerChest );
		SETRAGEBOOL( RagebotMPStomach );
		SETRAGEBOOL( RagebotMPPelvis );
		SETRAGEBOOL( RagebotMPArms );
		SETRAGEBOOL( RagebotMPLegs );
		SETRAGEBOOL( RagebotMPFeet );

		SETRAGEBOOL( RagebotAutoFire );
		SETRAGEBOOL( RagebotSilentAim );
		SETRAGEBOOL( RagebotHitchanceThorough );
		SETRAGEBOOL( RagebotAutowall );
		SETRAGEBOOL( RagebotScaleDamage );
		SETRAGEBOOL( RagebotAutoStop );
		SETRAGEBOOL( RagebotBetweenShots );
		SETRAGEBOOL( RagebotStaticPointscale );
		SETRAGEBOOL( RagebotIgnoreLimbs );
		SETRAGEBOOL( RagebotPreferBaim );
		SETRAGEBOOL( RagebotPreferBaimDoubletap );
		SETRAGEBOOL( RagebotPreferBaimLethal );

		SETRAGEINT( RagebotFOV );
		SETRAGEINT( RagebotHitchance );
		SETRAGEINT( RagebotMinimumDamage );
		SETRAGEINT( RagebotPenetrationDamage );
		SETRAGEINT( RagebotOverrideDamage );
		SETRAGEINT( RagebotHeadScale );
		SETRAGEINT( RagebotBodyScale );
	}
	bool RagebotHBHead{ };
	bool RagebotHBUpperChest{ };
	bool RagebotHBChest{ };
	bool RagebotHBLowerChest{ };
	bool RagebotHBStomach{ };
	bool RagebotHBPelvis{ };
	bool RagebotHBArms{ };
	bool RagebotHBLegs{ };
	bool RagebotHBFeet{ };

	bool RagebotMPHead{ };
	bool RagebotMPUpperChest{ };
	bool RagebotMPChest{ };
	bool RagebotMPLowerChest{ };
	bool RagebotMPStomach{ };
	bool RagebotMPPelvis{ };
	bool RagebotMPArms{ };
	bool RagebotMPLegs{ };
	bool RagebotMPFeet{ };

	bool RagebotAutoFire{ };
	bool RagebotSilentAim{ };
	bool RagebotHitchanceThorough{ };
	bool RagebotAutowall{ };
	bool RagebotScaleDamage{ };
	bool RagebotStaticPointscale{ };
	bool RagebotIgnoreLimbs{ };
	bool RagebotPreferBaim{ };
	bool RagebotPreferBaimDoubletap{ };
	bool RagebotPreferBaimLethal{ };

	int RagebotFOV{ };
	int RagebotHitchance{ };
	int RagebotMinimumDamage{ };
	int RagebotPenetrationDamage{ };
	int RagebotOverrideDamage{ };
	int RagebotHeadScale{ };
	int RagebotBodyScale{ };

	int m_iLastWeaponIndex{ -1 };
	int m_iLastWeaponType{ -1 };

public:
	FORCEINLINE const char* Hitgroup2Str( int hitgroup ) {
		switch ( hitgroup )
		{
		case HITGROUP_HEAD:
			return "head";
		case HITGROUP_CHEST:
			return "chest";
		case HITGROUP_STOMACH:
			return "stomach";
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			return "arm";
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			return "leg";
		default:
			return "pelvis";
		}
	}
};

namespace Features { inline CRageBot Ragebot; };