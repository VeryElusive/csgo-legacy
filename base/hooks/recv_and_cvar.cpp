#include "../core/hooks.h"
#include "../features/animations/animation.h"
#include "../context.h"
#include <intrin.h>

bool FASTCALL Hooks::hkSvCheatsGetBool( CConVar* thisptr, int edx ) {
	static auto oSvCheatsGetBool = DTR::SvCheatsGetBool.GetOriginal<decltype( &hkSvCheatsGetBool )>( );
	if ( reinterpret_cast< std::uintptr_t >( _ReturnAddress( ) ) == Offsets::Sigs.uCAM_ThinkReturn )
		return true;

	return oSvCheatsGetBool( thisptr, edx );
}

void CDECL Hooks::m_bClientSideAnimationHook( CRecvProxyData* data, void* entity, void* output ) {
	if ( !( ( CBasePlayer* )entity )->IsHostage( ) )
		return;

	m_bClientSideAnimation( data, entity, output );
}

void CDECL Hooks::m_flSimulationTimeHook( CRecvProxyData* data, void* entity, void* output ) {
	// fix simtime being inaccurate due to rounding
	// when its smaller than network base (caused by defensive dt).
	if ( data->Value.Int == 0 )
		return;

	m_flSimulationTime( data, entity, output );
}