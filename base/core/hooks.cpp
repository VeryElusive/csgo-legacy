#include <intrin.h>
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

	if ( !DTR::PaintTraverse.Create( MEM::GetVFunc( Interfaces::Panel, VTABLE::PAINTTRAVERSE ), &hkPaintTraverse ) )
		return false;

	if ( !DTR::CreateMoveProxy.Create( MEM::GetVFunc( Interfaces::Client, VTABLE::CREATEMOVE ), &hkCreateMoveProxy ) )
		return false;

	//if ( !DTR::DrawModel.Create( MEM::GetVFunc( Interfaces::StudioRender, VTABLE::DRAWMODEL ), &hkDrawModel ) )
	//	return false;

	if ( !DTR::Paint.Create( MEM::GetVFunc( Interfaces::EngineVGui, VTABLE::VGUI_PAINT ), &HkPaint ) )
		return false;

	if ( !DTR::FrameStageNotify.Create( MEM::GetVFunc( Interfaces::Client, VTABLE::FRAMESTAGENOTIFY ), &hkFrameStageNotify ) )
		return false;

	if ( !DTR::OverrideView.Create( MEM::GetVFunc( Interfaces::ClientMode, VTABLE::OVERRIDEVIEW ), &hkOverrideView ) )
		return false;	

	if ( !DTR::DoPostScreenEffects.Create( MEM::GetVFunc( Interfaces::ClientMode, VTABLE::DOPOSTSCREENEFFECTS ), &hkDoPostScreenEffects ) )
		return false;	
	
	//if ( !DTR::CMCreateMove.Create( MEM::GetVFunc( Interfaces::ClientMode, 24 ), &hkCMCreateMove ) )
	//	return false;

	if ( !DTR::OverrideConfig.Create( MEM::GetVFunc( Interfaces::MaterialSystem, VTABLE::OVERRIDECONFIG ), &hkOverrideConfig ) )
		return false;	
	
	if ( !DTR::GetScreenAspectRatio.Create( MEM::GetVFunc( Interfaces::Engine, VTABLE::GETSCREENASPECTRATIO ), &hkGetScreenAspectRatio ) )
		return false;	

	if ( !DTR::IsPaused.Create( MEM::GetVFunc( Interfaces::Engine, VTABLE::ISPAUSED ), &hkIsPaused ) )
		return false;

	if ( !DTR::IsHLTV.Create( MEM::GetVFunc( Interfaces::Engine, VTABLE::ISHLTV ), &hkIsHltv ) )
		return false;
	
	//if ( !DTR::EmitSound.Create( MEM::GetVFunc( Interfaces::EngineSound, VTABLE::EMITSOUND ), &hkEmitSound ) )
	//	return false;

	void* pClientStateSwap = ( void* )( uint32_t( Interfaces::ClientState ) + 8 );
	if ( !DTR::PacketEnd.Create( MEM::GetVFunc( pClientStateSwap, 6 ), &Hooks::hkPacketEnd ) )
		return false;

	if ( !DTR::PacketStart.Create( MEM::GetVFunc( pClientStateSwap, 5 ), &Hooks::hkPacketStart ) )
		return false;

	if ( !DTR::WriteUserCmdDeltaToBuffer.Create( MEM::GetVFunc( Interfaces::Client, VTABLE::WRITEUSERCMDDELTATOBUFFER ), &hkWriteUserCmdDeltaToBuffer ) )
		return false;

	if ( !DTR::InPrediction.Create( MEM::GetVFunc( Interfaces::Prediction, VTABLE::INPREDICTION ), &hkInPrediction ) )
		return false;

	if ( !DTR::ProcessMovement.Create( MEM::GetVFunc( Interfaces::GameMovement, VTABLE::PROCESSMOVEMENT ), &hkProcessMovement ) )
		return false;
	
	if ( !DTR::SvCheatsGetBool.Create( MEM::GetVFunc( Interfaces::ConVar->FindVar( _( "sv_cheats" ) ), VTABLE::GETBOOL ), &hkSvCheatsGetBool ) )
		return false;

	bClientSideAnimation = PropManager::Get( ).Hook( m_bClientSideAnimationHook, _( "DT_BaseAnimating" ), _( "m_bClientSideAnimation" ) );

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

	if ( !DTR::UpdateClientsideAnimation.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 51 56 8B F1 80 BE ? ? 00 00 00 74 36" ) ) ),
		&Hooks::hkUpdateClientsideAnimation ) )
		return false;

	if ( !DTR::GetEyeAngles.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "56 8B F1 85 F6 74 32" ) ) ),
		&Hooks::hkGetEyeAngles ) )
		return false;		
	
	/*if ( !DTR::GlowEffectSpectator.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 EC 14 53 8B 5D 0C 56 57 85 DB 74 47" ) ) ),//wrong sig
		&Hooks::hkGlowEffectSpectator ) )
		return false;		*/
	
	if ( !DTR::GetColorModulation.Create(
		( byte* )( MEM::FindPattern( MATERIALSYSTEM_DLL, _( "55 8B EC 83 EC ? 56 8B F1 8A 46" ) ) ),
		&Hooks::hkGetColorModulation ) )
		return false;		
	
	if ( !DTR::GetAlphaModulation.Create(
		( byte* )( MEM::FindPattern( MATERIALSYSTEM_DLL, _( "56 8B F1 8A 46 20 C0 E8 02 A8 01 75 0B 6A 00 6A 00 6A 00 E8 ? ? ? ? 80 7E 22 05 76 0E" ) ) ),
		&Hooks::hkGetAlphaModulation ) )
		return false;	
	
	/*if ( !DTR::PostDataUpdate.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 53 56 8B F1 57 80 BE ? ? ? ? ? 74 0A 83 7D 08" ) ) ),
		&Hooks::hkPostDataUpdate ) )
		return false; */

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

	if ( !DTR::PhysicsSimulate.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 83 EC 0C 56 8B F1 8B 0D ? ? ? ? 80" ) ) ),// TODO: IDK IF THIS IS EVEN RIGHT BUT I WILL HOOK IT PROPERLY I PROMISE I CANNOT BE FUCKED CUZ IM UPDATING SO MUCH SHIT RN
		&Hooks::hkPhysicsSimulate ) )
		return false;

	if ( !DTR::ModifyEyePosition.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 E4 F8 83 EC 58 56 57 8B F9 83 7F 60 00 0F 84" ) ) ),
		&Hooks::hkModifyEyePosition ) )
		return false;
	
	if ( !DTR::CalcViewmodelBob.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC A1 ? ? ? ? 83 EC 10 8B 40 34" ) ) ),
		&Hooks::hkCalcViewmodelBob ) )
		return false;		
	
	if ( !DTR::CalcView.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 53 8B 5D 08 56 57 FF 75 18 8B F1 FF 75 14 FF 75" ) ) ),
		&Hooks::hkCalcView ) )
		return false;		

	// TODO: hook sendnetmsg
	
	if ( !DTR::OnLatchInterpolatedVariables.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "55 8B EC 83 EC 10 53 56 8B F1 57 80 BE ? ? ? ? ? 75 41" ) ) ),
		&Hooks::hkOnLatchInterpolatedVariables ) )
		return false;		

	/*if ( !DTR::CL_Move.Create(
		( byte* )( MEM::FindPattern( ENGINE_DLL, _( "55 8B EC 81 EC ? ? ? ? 53 56 8A F9 F3 0F 11 45 ? 8B 4D 04" ) ) ),
		&Hooks::hkCL_Move ) )
		return false;

		if ( !DTR::CClientStateIsPaused.Create(
		( byte* )( MEM::FindPattern( ENGINE_DLL, _( "80 B9 ? ? ? ? ? 75 62" ) ) ),
		&Hooks::hkCClientStateIsPaused ) )
		return false;

	if ( !DTR::ShouldInterpolate.Create(
		( byte* )( MEM::FindPattern( CLIENT_DLL, _( "56 8B F1 E8 ? ? ? ? 3B F0" ) ) ),
		&Hooks::hkShouldInterpolate ) )
		return false;*/

	return true;
}

void Hooks::Restore( ) {
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