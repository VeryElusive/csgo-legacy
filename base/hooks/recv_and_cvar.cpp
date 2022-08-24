#include "../core/hooks.h"
#include "../context.h"
#include <intrin.h>

bool FASTCALL Hooks::hkSvCheatsGetBool( CConVar* thisptr, int edx ) {
	static auto oSvCheatsGetBool = DTR::SvCheatsGetBool.GetOriginal<decltype( &hkSvCheatsGetBool )>( );
	if ( reinterpret_cast< std::uintptr_t >( _ReturnAddress( ) ) == Offsets::Sigs.uCAM_ThinkReturn )
		return true;

	return oSvCheatsGetBool( thisptr, edx );
}

void CDECL Hooks::m_bClientSideAnimationHook( CRecvProxyData* data, void* entity, void* output ) {
	if ( entity != ctx.m_pLocal
		&& !( ( CBasePlayer* )entity )->IsHostage( ) )
		*( int* )output = ( ctx.m_bUpdatingAnimations ? 1 : 0 );

	bClientSideAnimation( data, entity, output );
}