#include "../core/hooks.h"
#include "../core/config.h"
#include "../context.h"
#include <intrin.h>

float FASTCALL Hooks::hkGetScreenAspectRatio( void* ecx, void* edx, int32_t iWidth, int32_t iHeight ) {
	static auto oGetScreenAspectRatio{ DTR::GetScreenAspectRatio.GetOriginal<decltype( &hkGetScreenAspectRatio )>( ) };

	if ( Config::Get<bool>(Vars.MiscAspectRatio ) )
		return Config::Get<float>( Vars.MiscAspectRatioAmt );
	else
		return oGetScreenAspectRatio( ecx, edx, iWidth, iHeight );
}


bool FASTCALL Hooks::hkIsPaused( void* ecx, void* edx ) {
	static auto oIsPaused{ DTR::IsPaused.GetOriginal<decltype( &hkIsPaused )>( ) };
	if ( reinterpret_cast< uintptr_t >( _ReturnAddress( ) ) == Offsets::Sigs.ReturnToExtrapolate )
		return true;

	return oIsPaused( ecx, edx );
}

bool FASTCALL Hooks::hkIsHltv( void* ecx, void* edx ) {
	static auto oIsHltv{ DTR::IsHLTV.GetOriginal<decltype( &hkIsHltv )>( ) };
	if ( reinterpret_cast< uintptr_t >( _ReturnAddress( ) ) == Offsets::Sigs.SetupVelocityReturn && ctx.m_bUpdatingAnimations )
		return true;

	return oIsHltv( ecx, edx );
}