#include "../core/hooks.h"
#include "../context.h"
#include <intrin.h>

bool FASTCALL Hooks::hkInPrediction( void* ecx, void* edx ) {
	static auto oInPrediction{ DTR::InPrediction.GetOriginal<decltype( &hkInPrediction )>( ) };

	if ( reinterpret_cast< uintptr_t >( _ReturnAddress( ) ) == Offsets::Sigs.SetupBonesTiming )
		return false;

	return oInPrediction( ecx, edx );
}

void FASTCALL Hooks::hkProcessMovement( void* ecx, DWORD edx, CBasePlayer* basePlayer, CMoveData* moveData ) {
	static auto oProcessMovement{ DTR::ProcessMovement.GetOriginal<decltype( &hkProcessMovement )>( ) };

	// fix prediction errors when jumping
	moveData->bGameCodeMovedPlayer = false;

	oProcessMovement( ecx, edx, basePlayer, moveData );
}