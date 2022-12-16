﻿#include <intrin.h>
#include <array>
#include <thread>
#include <windows.h>
#include "hooks.h"
// used: global variables
#include "../context.h"
#include "prop_manager.h"


#pragma region hooks_get
bool Hooks::Setup( ) {
	if ( MH_Initialize( ) != MH_OK )
		throw std::runtime_error( _( "failed initialize minhook" ) );

	if ( !DTR::LockCursor.Create( MEM::GetVFunc( Interfaces::Surface, VTABLE::LOCKCURSOR ), &hkLockCursor ) )
		return false;

	if ( !DTR::CreateMoveProxy.Create( MEM::GetVFunc( Interfaces::Client, VTABLE::CREATEMOVE ), &hkCreateMoveProxy ) )
		return false;

	if ( !DTR::Paint.Create( MEM::GetVFunc( Interfaces::EngineVGui, VTABLE::VGUI_PAINT ), &HkPaint ) )
		return false;

	if ( !DTR::FrameStageNotify.Create( MEM::GetVFunc( Interfaces::Client, VTABLE::FRAMESTAGENOTIFY ), &hkFrameStageNotify ) )
		return false;

	if ( !DTR::OverrideView.Create( MEM::GetVFunc( Interfaces::ClientMode, VTABLE::OVERRIDEVIEW ), &hkOverrideView ) )
		return false;	

	if ( !DTR::RunCommand.Create( MEM::GetVFunc( Interfaces::Prediction, VTABLE::RUNCOMMAND ), &hkRunCommand ) )
		return false;

	//if ( !DTR::DoPostScreenEffects.Create( MEM::GetVFunc( Interfaces::ClientMode, VTABLE::DOPOSTSCREENEFFECTS ), &hkDoPostScreenEffects ) )
	//	return false;		
	
	if ( !DTR::GetUserCmd.Create( MEM::GetVFunc( Interfaces::Input, 8 ), &hkGetUserCmd ) )
		return false;
	
	//if ( !DTR::PredictionUpdate.Create( MEM::GetVFunc( Interfaces::Prediction, 3 ), &hkPredictionUpdate ) )
	//	return false;			
	
	if ( !DTR::OverrideConfig.Create( MEM::GetVFunc( Interfaces::MaterialSystem, VTABLE::OVERRIDECONFIG ), &hkOverrideConfig ) )
		return false;
	
	if ( !DTR::GetScreenAspectRatio.Create( MEM::GetVFunc( Interfaces::Engine, VTABLE::GETSCREENASPECTRATIO ), &hkGetScreenAspectRatio ) )
		return false;	

	if ( !DTR::IsPaused.Create( MEM::GetVFunc( Interfaces::Engine, VTABLE::ISPAUSED ), &hkIsPaused ) )
		return false;

	if ( !DTR::IsHLTV.Create( MEM::GetVFunc( Interfaces::Engine, VTABLE::ISHLTV ), &hkIsHltv ) )
		return false;
	
	/*if ( !DTR::EmitSound.Create( MEM::GetVFunc( Interfaces::EngineSound, VTABLE::EMITSOUND ), &hkEmitSound ) )
		return false;

	if ( !DTR::ListLeavesInBox.Create( MEM::GetVFunc( Interfaces::Engine->GetBSPTreeQuery( ), VTABLE::LISTLEAVESINBOX ), &hkListLeavesInBox ) )
		return false;*/

	void* pClientStateSwap = ( void* )( uint32_t( Interfaces::ClientState ) + 8 );
	if ( !DTR::PacketEnd.Create( MEM::GetVFunc( pClientStateSwap, 6 ), &Hooks::hkPacketEnd ) )
		return false;

	if ( !DTR::PacketStart.Create( MEM::GetVFunc( pClientStateSwap, 5 ), &Hooks::hkPacketStart ) )
		return false;

	if ( !DTR::WriteUserCmdDeltaToBuffer.Create( MEM::GetVFunc( Interfaces::Client, VTABLE::WRITEUSERCMDDELTATOBUFFER ), &hkWriteUserCmdDeltaToBuffer ) )
		return false;

	if ( !DTR::ProcessMovement.Create( MEM::GetVFunc( Interfaces::GameMovement, VTABLE::PROCESSMOVEMENT ), &hkProcessMovement ) )
		return false;			
	
	if ( !DTR::SvCheatsGetBool.Create( MEM::GetVFunc( Interfaces::ConVar->FindVar( _( "sv_cheats" ) ), VTABLE::GETBOOL ), &hkSvCheatsGetBool ) )
		return false;

	m_bClientSideAnimation = PropManager::Get( ).Hook( m_bClientSideAnimationHook, _( "DT_BaseAnimating" ), _( "m_bClientSideAnimation" ) );
	//m_flSimulationTime = PropManager::Get( ).Hook( m_flSimulationTimeHook, _( "DT_BaseEntity" ), _( "m_flSimulationTime" ) );

	D3DDEVICE_CREATION_PARAMETERS creationParameters = { };
	while ( FAILED( Interfaces::DirectDevice->GetCreationParameters( &creationParameters ) ) )
		std::this_thread::sleep_for( 200ms );

	hWindow = creationParameters.hFocusWindow;
	if ( !hWindow )
		return false;

	pOldWndProc = reinterpret_cast< WNDPROC >( SetWindowLongPtr( hWindow, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >( Hooks::hkWndProc ) ) );
	if ( !pOldWndProc )
		return false;

	if ( !DTR::DoExtraBonesProcessing.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 8B F1 57 89 74 24 1C" ) ) ),
		&Hooks::hkDoExtraBonesProcessing ) )
		return false;

	if ( !DTR::StandardBlendingRules.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 8B 75 08 57 8B F9 85 F6" ) ) ),
		&Hooks::hkStandardBlendingRules ) )
		return false;

	/*if ( !DTR::GlowEffectSpectator.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 EC 14 53 8B 5D 0C 56 57 85 DB 74 47 " ) ) ),
		&Hooks::hkGlowEffectSpectator ) )
		return false;*/	
	
	if ( !DTR::GetColorModulation.Create(
		( byte* )( MEM::FindPattern( MATERIALSYSTEM_DLL, _( "55 8B EC 83 EC ? 56 8B F1 8A 46" ) ) ),
		&Hooks::hkGetColorModulation ) )
		return false;		
	
	if ( !DTR::GetAlphaModulation.Create(
		( byte* )( MEM::FindPattern( MATERIALSYSTEM_DLL, _( "56 8B F1 8A 46 20 C0 E8 02 A8 01 75 0B 6A 00 6A 00 6A 00 E8 ? ? ? ? 80 7E 22 05 76 0E" ) ) ),
		&Hooks::hkGetAlphaModulation ) )
		return false;	
	
	if ( !DTR::AccumulateLayers.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 57 8B F9 8B 0D ? ? ? ? 8B 01 8B 80" ) ) ),
		&Hooks::hkAccumulateLayers ) )
		return false;

	if ( !DTR::ShouldSkipAnimFrame.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "57 8B F9 8B 07 8B 80 ? ? ? ? FF D0 84 C0 75 02" ) ) ),
		&Hooks::hkShouldSkipAnimFrame ) )
		return false;

	if ( !DTR::Setupbones.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F0 B8 D8" ) ) ),
		&Hooks::hkSetupbones ) )
		return false;

	if ( !DTR::CalcViewmodelBob.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC A1 ? ? ? ? 83 EC 10 8B 40 34" ) ) ),
		&Hooks::hkCalcViewmodelBob ) )
		return false;		
	
	if ( !DTR::CalcView.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 53 8B 5D 08 56 57 FF 75 18 8B F1 FF 75 14 FF 75" ) ) ),
		&Hooks::hkCalcView ) )
		return false;		
	
	if ( !DTR::OnLatchInterpolatedVariables.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 EC 10 53 56 8B F1 57 80 BE ? ? ? ? ? 75 41" ) ) ),
		&Hooks::hkOnLatchInterpolatedVariables ) )
		return false;
	
	if ( !DTR::OnNewCollisionBounds.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 8B 45 10 F3 0F 10 81" ) ) ),
		&Hooks::hkOnNewCollisionBounds ) )
		return false;	
	
	if ( !DTR::UpdatePostProcessingEffects.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 51 53 56 57 8B F9 8B 4D 04 E8 ? ? ? ? 8B 35" ) ) ),
		&Hooks::hkUpdatePostProcessingEffects ) )
		return false;
	
	if ( !DTR::CHudScopePaint.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 83 EC 78 56 57 8B 3D" ) ) ),
		&Hooks::hkCHudScopePaint ) )
		return false;		


#ifdef SERVER_DBGING
	if ( !DTR::ServerSetupBones.Create(
		( byte* )( MEM::FindPattern( SERVER_DLL, _( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 8B C1 56 57 89 44" ) ) ),
		&Hooks::hkServerSetupBones ) )
		return false;
#endif

	return true;
}

void Hooks::Restore( ) {
	//todo: restore prop hooks
	PropManager::Get( ).Hook( m_bClientSideAnimation, _( "DT_BaseAnimating" ), _( "m_bClientSideAnimation" ) );

	MH_DisableHook( MH_ALL_HOOKS );
	MH_RemoveHook( MH_ALL_HOOKS );

	MH_Uninitialize( );
	
	if ( pOldWndProc ) {
		SetWindowLongPtrW( hWindow, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >( pOldWndProc ) );
		pOldWndProc = nullptr;
	}

	// reset input state
	Interfaces::InputSystem->EnableInput( true );
}
#pragma endregion