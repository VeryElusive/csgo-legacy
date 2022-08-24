#include "displacement.h"
#include "../core/interfaces.h"

#define DEFCVAR( VAR ) Cvars.##VAR = Interfaces::ConVar->FindVar( _( #VAR ) );

#define DT_BaseEntity( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_BaseEntity" ), _( #VAR ) );
#define DT_BasePlayer( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_BasePlayer" ), _( #VAR ) );
#define DT_CSPlayer( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_CSPlayer" ), _( #VAR ) );
#define DT_BaseCombatCharacter( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_BaseCombatCharacter" ), _( #VAR ) );
#define DT_BaseCombatWeapon( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_BaseCombatWeapon" ), _( #VAR ) );
#define DT_WeaponCSBaseGun( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_WeaponCSBaseGun" ), _( #VAR ) );
#define DT_WeaponCSBase( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_WeaponCSBase" ), _( #VAR ) );
#define DT_BreakableSurface( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_BreakableSurface" ), _( #VAR ) );
#define DT_BaseCSGrenadeProjectile( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_BaseCSGrenadeProjectile" ), _( #VAR ) );
#define DT_SmokeGrenadeProjectile( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_SmokeGrenadeProjectile" ), _( #VAR ) );
#define DT_BaseAnimating( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_BaseAnimating" ), _( #VAR ) );
#define DT_EnvTonemapController( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_EnvTonemapController" ), _( #VAR ) );
#define DT_BaseCSGrenade( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_BaseCSGrenade" ), _( #VAR ) );
#define DT_BaseViewModel( VAR ) VAR = PropManager::Get( ).GetOffset( _( "DT_BaseViewModel" ), _( #VAR ) );

void Offsets::Init( ) {
	/* offsets */
	DT_BaseEntity( m_vecMins );
	DT_BaseEntity( m_vecMaxs );
	DT_BaseEntity( m_vecOrigin );
	DT_BaseEntity( m_iTeamNum );
	DT_BaseEntity( m_CollisionGroup );
	DT_BaseEntity( m_nRenderMode );
	DT_BaseEntity( m_flSimulationTime );

	m_vecViewOffset = PropManager::Get( ).GetOffset( _( "DT_BasePlayer" ), _( "m_vecViewOffset[0]" ) );// one off cuz diff name
	m_hViewModel = PropManager::Get( ).GetOffset( _( "DT_BasePlayer" ), _( "m_hViewModel[0]" ) );// one off cuz diff name
	DT_BasePlayer( m_lifeState );
	DT_BasePlayer( m_iHealth );
	DT_BasePlayer( m_fFlags );
	DT_BasePlayer( m_aimPunchAngle );
	DT_BasePlayer( m_viewPunchAngle );
	DT_BasePlayer( pl );
	DT_BasePlayer( m_hOwnerEntity );
	DT_BasePlayer( m_nTickBase );
	DT_BasePlayer( m_hViewEntity );
	DT_BasePlayer( m_vphysicsCollisionState );
	DT_BasePlayer( m_aimPunchAngleVel );
	DT_BasePlayer( deadflag );
	DT_BasePlayer( m_flDuckSpeed );
	DT_BasePlayer( m_flMaxSpeed );
	DT_BasePlayer( m_hGroundEntity );
	DT_BasePlayer( m_bStrafing );
	m_vecVelocity = PropManager::Get( ).GetOffset( _( "DT_BasePlayer" ), _( "m_vecVelocity[0]" ) );// one off cuz diff name

	m_angEyeAngles = PropManager::Get( ).GetOffset( _( "DT_CSPlayer" ), _( "m_angEyeAngles[0]" ) );// one off cuz diff name
	DT_CSPlayer( m_flFlashDuration );
	DT_CSPlayer( m_bIsScoped );
	DT_CSPlayer( m_ArmorValue );
	DT_CSPlayer( m_bHasHelmet );
	DT_CSPlayer( m_bHasHeavyArmor );
	DT_CSPlayer( m_flVelocityModifier );
	DT_CSPlayer( m_bWaitForNoAttack );
	DT_CSPlayer( m_iPlayerState );
	DT_CSPlayer( m_bIsDefusing );
	DT_CSPlayer( m_flNextAttack );
	DT_CSPlayer( m_iMoveState );
	DT_CSPlayer( m_iAddonBits );
	DT_CSPlayer( m_flPoseParameter );
	DT_CSPlayer( m_flDuckAmount );
	DT_CSPlayer( m_flLowerBodyYawTarget );
	DT_CSPlayer( m_bGunGameImmunity );
	DT_CSPlayer( m_flThirdpersonRecoil );
	DT_CSPlayer( m_bIsWalking );

	DT_BaseCombatCharacter( m_hActiveWeapon );
	DT_BaseCombatCharacter( m_hMyWeapons );

	DT_BaseCombatWeapon( m_iItemDefinitionIndex );
	DT_BaseCombatWeapon( m_iClip1 );
	DT_BaseCombatWeapon( m_flNextPrimaryAttack );

	DT_WeaponCSBaseGun( m_zoomLevel );

	DT_WeaponCSBase( m_fAccuracyPenalty );
	DT_WeaponCSBase( m_flRecoilIndex );
	DT_WeaponCSBase( m_flPostponeFireReadyTime );

	DT_BreakableSurface( m_bIsBroken );

	DT_BaseAnimating( m_nSequence );
	DT_BaseAnimating( m_nHitboxSet );
	DT_BaseAnimating( m_bClientSideAnimation );
	DT_BaseAnimating( m_flCycle );

	DT_BaseCSGrenadeProjectile( m_nExplodeEffectTickBegin );

	DT_BaseViewModel( m_hWeapon );
	DT_BaseViewModel( m_nAnimationParity );

	DT_EnvTonemapController( m_bUseCustomBloomScale );
	DT_EnvTonemapController( m_bUseCustomAutoExposureMax );
	DT_EnvTonemapController( m_bUseCustomAutoExposureMin );
	DT_EnvTonemapController( m_flCustomAutoExposureMin );
	DT_EnvTonemapController( m_flCustomAutoExposureMax );
	DT_EnvTonemapController( m_flCustomBloomScale );

	m_vecVelocityGRENADE = PropManager::Get( ).GetOffset( _( "DT_BaseCSGrenadeProjectile" ), _( "m_vecVelocity" ) );// one off cuz diff name

	DT_BaseCSGrenade( m_fThrowTime );
	DT_BaseCSGrenade( m_flThrowStrength );
	DT_BaseCSGrenade( m_bPinPulled );

	DT_SmokeGrenadeProjectile( m_nSmokeEffectTickBegin );

	m_fog_enable = PropManager::Get( ).GetOffset( _( "DT_FogController" ), _( "m_fog.enable" ) );

	/* cvars */
	DEFCVAR( mp_teammates_are_enemies );
	DEFCVAR( cl_foot_contact_shadows );
	DEFCVAR( weapon_recoil_scale );
	DEFCVAR( view_recoil_tracking );	
	DEFCVAR( ff_damage_reduction_bullets );
	DEFCVAR( ff_damage_bullet_penetration );
	DEFCVAR( r_drawspecificstaticprop );
	DEFCVAR( r_modelAmbientMin );
	DEFCVAR( r_jiggle_bones );
	DEFCVAR( sv_maxunlag );
	DEFCVAR( cl_interp );
	DEFCVAR( cl_interp_ratio );
	DEFCVAR( cl_updaterate );
	DEFCVAR( sv_clockcorrection_msecs );
	DEFCVAR( sv_accelerate_use_weapon_speed );
	DEFCVAR( sv_accelerate );
	DEFCVAR( mat_ambient_light_r );
	DEFCVAR( mat_ambient_light_g );
	DEFCVAR( mat_ambient_light_b );
	DEFCVAR( sv_gravity );
	DEFCVAR( weapon_molotov_maxdetonateslope );
	DEFCVAR( molotov_throw_detonate_time );
	DEFCVAR( sv_showimpacts );
	DEFCVAR( cl_clock_correction );
	DEFCVAR( r_drawmodelstatsoverlay );
	DEFCVAR( cl_mouseenable );
	DEFCVAR( weapon_debug_spread_show );
	DEFCVAR( cl_csm_shadows );

	//Interfaces::ConVar->FindVar( _( "r_occlusion" ) )->SetValue( 0 );
	Interfaces::ConVar->FindVar( _( "r_jiggle_bones" ) )->SetValue( 0 );
	//Interfaces::ConVar->FindVar( _( "cl_extrapolate" ) )->SetValue( 0 );
	//Interfaces::ConVar->FindVar( _( "cl_extrapolate_amount" ) )->SetValue( 0 );
	//Interfaces::ConVar->FindVar( _( "cl_simulationtimefix " ) )->SetValue( 0 );

	/* sigs */
	Sigs.LocalPlayer = MEM::FindPattern( CLIENT_DLL, _( "8D 34 85 ? ? ? ? 89 15 ? ? ? ? 8B 41 08 8B 48 04 83 F9 FF" ) + 2 );

	Sigs.uPredictionRandomSeed = MEM::FindPattern( CLIENT_DLL, _( "8B 0D ? ? ? ? BA ? ? ? ? E8 ? ? ? ? 83 C4 04" ) ) + 0x2;
	Sigs.pPredictionPlayer = MEM::FindPattern( CLIENT_DLL, _( "89 35 ? ? ? ? F3 0F 10 48 20" ) ) + 0x2;
	
	Sigs.InitKeyValues = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 56 8B F1 33 C0 8B 4D 0C 81" ) );// @xref: "OldParticleSystem_Destroy"
	Sigs.DestructKeyValues = MEM::FindPattern( CLIENT_DLL, _( "56 8B F1 E8 ? ? ? ? 8B 4E 14" ) );// @xref: "OldParticleSystem_Destroy"
	Sigs.oFromString = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 81 EC ? ? ? ? 85 D2 53" ) );// @xref: "#empty#", "#int#"
	Sigs.oLoadFromBuffer = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89" ) );// @xref: "KeyValues::LoadFromBuffer(%s%s%s): Begin"
	Sigs.oLoadFromFile = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 83 EC 14 53 56 8B 75 08 57 FF" ) );// @xref: "rb"
	Sigs.oFindKey = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 EC 1C 53 8B D9 85 DB" ) );// @xref: "rb"
	Sigs.oSetString = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC A1 ? ? ? ? 53 56 57 8B F9 8B 08 8B 01" ) );// @xref: "rb"
	Sigs.oGetString = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 C0 81 EC ? ? ? ? 53 8B 5D 08" ) );

	Sigs.oCreateAnimationState = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 56 8B F1 B9 ? ? ? ? C7 46" ) );// @xref: "ggprogressive_player_levelup"
	Sigs.oUpdateAnimationState = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24" ) );// @xref: "%s_aim"
	Sigs.oResetAnimationState = MEM::FindPattern( CLIENT_DLL, _( "56 6A 01 68 ? ? ? ? 8B F1" ) );// @xref: "player_spawn"

	Sigs.uDisableRenderTargetAllocationForever = MEM::FindPattern( MATERIALSYSTEM_DLL, _( "80 B9 ? ? ? ? ? 74 0F" ) );// @xref: "Tried BeginRenderTargetAllocation after game startup. If I let you do this, all users would suffer.\n"

	Sigs.SetAbsOrigin = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8 ? ? ? ? 8B 7D 08 F3 0F 10 07 0F 2E 86" ) );
	Sigs.SetAbsAngles = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8" ) );

	Sigs.PostProcess = MEM::FindPattern( CLIENT_DLL, _( "80 3D ? ? ? ? ? 53 56 57 0F 85" ) );

	Sigs.SmokeCount = MEM::FindPattern( CLIENT_DLL, _( "A3 ? ? ? ? 57 8B CB" ) );

	Sigs.TakeDamageOffset = MEM::FindPattern( CLIENT_DLL, _( "80 BE ? ? ? ? ? 75 46 8B 86" ) );

	Sigs.LookupBone = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 53 56 8B F1 57 83 BE ? ? ? ? ? 75 14" ) );


	Sigs.ClearNotices = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 EC 0C 53 56 8B 71 58" ) );

	Sigs.StartDrawing = MEM::FindPattern( VGUI_DLL, _( "55 8B EC 83 E4 C0 83 EC 38" ) );
	Sigs.FinishDrawing = MEM::FindPattern( VGUI_DLL, _( "8B 0D ? ? ? ? 56 C6 05" ) );

	Sigs.ReturnToExtrapolate = ( MEM::FindPattern( CLIENT_DLL, _( "FF D0 A1 ? ? ? ? B9 ? ? ? ? D9 1D ? ? ? ? FF 50 34 85 C0 74 22 8B 0D" ) ) + 0x29 );

	Sigs.SetupVelocityReturn = MEM::FindPattern( CLIENT_DLL, _( "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80" ) );

	Sigs.uInsertIntoTree = ( MEM::FindPattern( CLIENT_DLL, _( "56 52 FF 50 18" ) ) + 0x5 );// @xref: "<unknown renderable>"


	Sigs.SetupBonesTiming = MEM::FindPattern( CLIENT_DLL, _( "84 C0 74 0A F3 0F 10 05 ? ? ? ? EB 05" ) );

	Sigs.uCAM_ThinkReturn = MEM::FindPattern( CLIENT_DLL, _( "85 C0 75 30 38 86" ) );

	Sigs.ReturnToEyePosAndVectors = MEM::FindPattern( CLIENT_DLL, _( "8B 55 0C 8B C8 E8 ? ? ? ? 83 C4 08 5E 8B E5" ) );

	Sigs.InvalidateBoneCache = MEM::FindPattern( CLIENT_DLL, _( "80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81" ) );

	Sigs.SetCollisionBounds = MEM::FindPattern( CLIENT_DLL, _( "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 18 56 57 8B 7B" ) );

	Sigs.m_pStudioHdr = ( MEM::FindPattern( CLIENT_DLL, _( "8B B7 ? ? ? ? 89 74 24 20" ) ) + 0x2 );

	Sigs.WriteUsercmd = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D" ) );

	Sigs.AddBoxOverlayReturn = MEM::FindPattern( CLIENT_DLL, _( "3B 3D ? ? ? ? 75 4C" ) );


	Sigs.GetSequenceActivity = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 53 8B 5D ? 56 8B F1 83 FB" ) );// @xref: "Need to handle the activity %d\n"

	Sigs.SetupBones_AttachmentHelper = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4" ) );

	Sigs.TraceFilterSkipTwoEntities = MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 81 EC BC 00 00 00 56 8B F1 8B 86" ) ) + 0x21E;


	Sigs.uAllocKeyValuesEngine = ( MEM::GetAbsoluteAddress( MEM::FindPattern( ENGINE_DLL, _( "E8 ? ? ? ? 83 C4 08 84 C0 75 10 FF 75 0C" ) ) + 0x1 ) + 0x4A );
	Sigs.uAllocKeyValuesClient = ( MEM::GetAbsoluteAddress( MEM::FindPattern( CLIENT_DLL, _( "E8 ? ? ? ? 83 C4 08 84 C0 75 10" ) ) + 0x1 ) + 0x3E );
	Sigs.CL_SendMove = MEM::FindPattern( ENGINE_DLL, _( "55 8B EC 8B 4D 04 81 EC ? ? ? ? 53 56 57 E8" ) );
	Sigs.ClanTag = MEM::FindPattern( ENGINE_DLL, _( "53 56 57 8B DA 8B F9 FF 15" ) );
	Sigs.CL_ReadPackets = MEM::FindPattern( ENGINE_DLL, _( "53 8A D9 8B 0D ? ? ? ? 56 57 8B B9" ) );
	Sigs.LoadNamedSkys = MEM::FindPattern( ENGINE_DLL, _( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) );
	Sigs.numticks = ( MEM::FindPattern( ENGINE_DLL, _( "03 05 ? ? ? ? 83 CF 10" ) ) + 2 );// @xref: "SV_StartSound: invalid sentence number: %s" ((float)(dword_13903F1C + numticks - dword_13903F08) * *(float *)&dword_107AAEFC;)
}

std::uintptr_t Offsets::FindInDataMap( DataMap_t* pMap, const char* name ) {
	while ( pMap ) {
		for ( int i = 0; i < pMap->nDataFields; i++ ) {
			if ( pMap->pDataDesc[ i ].szFieldName == nullptr )
				continue;

			if ( strcmp( name, pMap->pDataDesc[ i ].szFieldName ) == 0 )
				return pMap->pDataDesc[ i ].iFieldOffset;

			if ( pMap->pDataDesc[ i ].iFieldType == FIELD_EMBEDDED ) {
				if ( pMap->pDataDesc[ i ].pTypeDescription ) {
					unsigned int offset;

					if ( ( offset = FindInDataMap( pMap->pDataDesc[ i ].pTypeDescription, name ) ) != 0 )
						return offset;
				}
			}
		}
		pMap = pMap->pBaseMap;
	}

	return 0;
}