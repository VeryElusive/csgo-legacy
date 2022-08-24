#include "../../core/hooks.h"
#include "../../context.h"
#include "../../features/rage/exploits.h"
#include "../../features/visuals/visuals.h"
#include <intrin.h>

void FASTCALL Hooks::hkModifyEyePosition( CCSGOPlayerAnimState* ecx, void* edx, Vector& pos ) {
	static auto oModifyEyePosition = DTR::ModifyEyePosition.GetOriginal<decltype( &hkModifyEyePosition )>( );

	const auto BackupMoveWeightSmoothed = ecx->flMoveWeightSmoothed;
	const auto BackupCameraSmoothHeight = ecx->flCameraSmoothHeight;

	ecx->bSmoothHeightValid = false;

	oModifyEyePosition( ecx, edx, pos );

	ecx->bSmoothHeightValid = BackupMoveWeightSmoothed;
	ecx->flCameraSmoothHeight = BackupCameraSmoothHeight;

	const auto ent = static_cast<CBasePlayer*>( ecx->pEntity );
	if ( !ent )
		return;

	pos.z -= ent->m_vecViewOffset( ).z - std::floor( ent->m_vecViewOffset( ).z );
}

void FASTCALL Hooks::hkCalcView( CBasePlayer* pPlayer, void* edx, Vector& vecEyeOrigin, QAngle& angEyeAngles, float& flZNear, float& flZFar, float& flFov ) {
	static auto oCalcView = DTR::CalcView.GetOriginal<decltype( &hkCalcView )>( );
	if ( !pPlayer || pPlayer != ctx.m_pLocal )
		return oCalcView( pPlayer, edx, vecEyeOrigin, angEyeAngles, flZNear, flZFar, flFov );

	const bool backupUseNewAnimstateBackup = *reinterpret_cast< bool* >( uintptr_t( pPlayer ) + 0x9B14 );
	const auto backupAimPunch = pPlayer->m_aimPunchAngle( );
	const auto backupViewPunch = pPlayer->m_viewPunchAngle( );

	*reinterpret_cast< bool* >( uintptr_t( pPlayer ) + 0x9B14 ) = false;

	if ( Config::Get<bool>( Vars.RemovalPunch ) )
		ctx.m_pLocal->m_viewPunchAngle( ) = pPlayer->m_aimPunchAngle( ) = { };

	oCalcView( pPlayer, edx, vecEyeOrigin, angEyeAngles, flZNear, flZFar, flFov );

	*reinterpret_cast< bool* >( uintptr_t( pPlayer ) + 0x9B14 ) = backupUseNewAnimstateBackup;
	ctx.m_pLocal->m_viewPunchAngle( ) = backupViewPunch;
	pPlayer->m_aimPunchAngle( ) = backupAimPunch;
}

float FASTCALL Hooks::hkCalcViewmodelBob( CWeaponCSBase* pWeapon, void* edx ) {
	return 0.f;
}

bool FASTCALL Hooks::hkShouldInterpolate( CBasePlayer* ecx, const std::uintptr_t edx ) {
	static auto oShouldInterpolate = DTR::ShouldInterpolate.GetOriginal<decltype( &hkShouldInterpolate )>( );
	if ( ecx == ctx.m_pLocal
		&& Features::Exploits.m_iRechargeCmd == Interfaces::ClientState->iLastOutgoingCommand ) {

		return false;
	}

	return oShouldInterpolate( ecx, edx );
}


bool CDECL Hooks::hkGlowEffectSpectator( CBasePlayer* const player, CBasePlayer* const local, int& style,
	Vector& clr, float& alpha_from, float& alpha_to,
	float& time_from, float& time_to, bool& animate ) {
	static auto oGlowEffectSpectator = DTR::GlowEffectSpectator.GetOriginal<decltype( &hkGlowEffectSpectator )>( );
	int type{ ENEMY };
	if ( player == ctx.m_pLocal )
		type = LOCAL;
	else if ( player->IsTeammate( ) )
		type = TEAM;

	Color col{ };
	GetPlayerColorFig( type, VisGlowCol, col );

	CheckIfPlayer( VisGlow, type ) { 
		style = 0;

		clr.x = col.Get<COLOR_R>( ) / 255.f;
		clr.y = col.Get<COLOR_G>( ) / 255.f;
		clr.z = col.Get<COLOR_B>( ) / 255.f;

		alpha_to = col.Get<COLOR_A>( ) / 255.f;

		return true;
	}

	return oGlowEffectSpectator( player, local, style, clr, alpha_from, alpha_to, time_from, time_to, animate );
}

void FASTCALL Hooks::hkGetColorModulation( IMaterial* const ecx, const std::uintptr_t edx, float* const r, 
	float* const g, float* const b ) {
	static auto oGetColorModulation = DTR::GetColorModulation.GetOriginal<decltype( &hkGetColorModulation )>( );

	oGetColorModulation( ecx, edx, r, g, b );

	const auto textureGroupName = ecx->GetTextureGroupName( );

	// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/public/texture_group_names.h
	
	// *reinterpret_cast< const std::uint32_t* >( "SkyBox textures" )
	if ( *reinterpret_cast< const std::uint32_t* >( textureGroupName ) == 0x42796B53u ) {
		if ( Config::Get<bool>( Vars.VisWorldSkyboxMod ) ) {
			const auto& col = Config::Get<Color>( Vars.WorldSkyboxCol );

			*r = col.Get<COLOR_R>( ) / 255.f;
			*g = col.Get<COLOR_G>( ) / 255.f;
			*b = col.Get<COLOR_B>( ) / 255.f;
		}

		return;
	}	

	// *reinterpret_cast< const std::uint32_t* >( "StaticProp textures" )
	if ( *reinterpret_cast< const std::uint32_t* >( textureGroupName ) == 0x74617453u ) {
		if ( Config::Get<bool>( Vars.VisWorldPropMod ) ) {
			const auto& col = Config::Get<Color>( Vars.VisWorldPropCol );

			*r = col.Get<COLOR_R>( ) / 255.f;
			*g = col.Get<COLOR_G>( ) / 255.f;
			*b = col.Get<COLOR_B>( ) / 255.f;
		}

		return;
	}

	// *reinterpret_cast< const std::uint32_t* >( "World textures" )
	if ( *reinterpret_cast< const std::uint32_t* >( textureGroupName ) == 0x6c726f57u ) {
		if ( Config::Get<bool>( Vars.WorldModulation ) ) {
			const auto& col = Config::Get<Color>( Vars.WorldModulationCol );

			*r = col.Get<COLOR_R>( ) / 255.f;
			*g = col.Get<COLOR_G>( ) / 255.f;
			*b = col.Get<COLOR_B>( ) / 255.f;
		}

		return;
	}
}

float FASTCALL Hooks::hkGetAlphaModulation( IMaterial* ecx, uint32_t ebx ) {
	static auto oGetAlphaModulation = DTR::GetAlphaModulation.GetOriginal<decltype( &hkGetAlphaModulation )>( );

	const auto textureGroupName = ecx->GetTextureGroupName( );

	if ( *reinterpret_cast< const std::uint32_t* >( textureGroupName ) == 0x74617453u
		&& Config::Get<bool>( Vars.VisWorldPropMod ) ) {
		const auto& col = Config::Get<Color>( Vars.VisWorldPropCol );

		return col.Get<COLOR_A>( ) / 255.f;
	}

	return oGetAlphaModulation( ecx, ebx );
}