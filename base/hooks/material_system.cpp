#include "../core/hooks.h"
#include "../context.h"
#include "../features/visuals/visuals.h"

void FASTCALL Hooks::hkDrawModel( IStudioRender* thisptr, int edx, DrawModelResults_t* pResults, const DrawModelInfo_t& info, matrix3x4_t* pBoneToWorld, float* flFlexWeights, float* flFlexDelayedWeights, const Vector& vecModelOrigin, int nFlags )
{
	static auto oDrawModel = DTR::DrawModel.GetOriginal<decltype( &hkDrawModel )>( );

	if ( !Interfaces::Engine->IsInGame( ) || Features::Visuals.Chams.m_bFakeModel )
		return oDrawModel( thisptr, edx, pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );

	Features::Visuals.Chams.Main( pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );
}


bool FASTCALL Hooks::hkOverrideConfig( IMaterialSystem* ecx, void* edx, MaterialSystemConfig_t& config, bool bForceUpdate ) {
	static auto oOverrideConfig = DTR::OverrideConfig.GetOriginal<decltype( &hkOverrideConfig )>( );

	if ( Config::Get<bool>(Vars.WorldFullbright ) )
		config.uFullbright = true;

	return oOverrideConfig( ecx, edx, config, bForceUpdate );
}

int FASTCALL Hooks::hkListLeavesInBox( void* thisptr, int edx, const Vector& vecMins, const Vector& vecMaxs, unsigned short* puList, int nListMax )
{
	static auto oListLeavesInBox = DTR::ListLeavesInBox.GetOriginal<decltype( &hkListLeavesInBox )>( );

	if ( uintptr_t( _ReturnAddress( ) ) != Offsets::Sigs.uInsertIntoTree )
		return oListLeavesInBox( thisptr, edx, vecMins, vecMaxs, puList, nListMax );

	// get current renderable info from stack ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L1470 )
	auto info = *( RenderableInfo_t** )( ( uintptr_t )_AddressOfReturnAddress( ) + 0x14 );
	if ( !info || !info->pRenderable )
		return oListLeavesInBox( thisptr, edx, vecMins, vecMaxs, puList, nListMax );

	// check if disabling occulusion for players ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L1491 )
	auto baseEntity = info->pRenderable->GetIClientUnknown( )->GetBaseEntity( );
	if ( !baseEntity || !baseEntity->IsPlayer( ) )
		return oListLeavesInBox( thisptr, edx, vecMins, vecMaxs, puList, nListMax );

	// fix render order, force translucent group ( https://www.unknowncheats.me/forum/2429206-post15.html )
	// AddRenderablesToRenderLists: https://i.imgur.com/hcg0NB5.png ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L2473 )
	const auto backupFlags{ info->uFlags };
	info->uFlags &= ~0x100;
	info->uFlags2 |= 0xC0;

	// extend world space bounds to maximum ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L707 )
	static Vector mapMin = Vector( MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT );
	static Vector mapMax = Vector( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT );
	return oListLeavesInBox( thisptr, edx, mapMin, mapMax, puList, nListMax );
}