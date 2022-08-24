#include "../core/hooks.h"
#include "../core/config.h"
#include "../context.h"

int FASTCALL Hooks::hkEmitSound( void* _this, int edx, IRecipientFilter& filter, int iEntIndex, int iChannel, const char* pSoundEntry, unsigned int nSoundEntryHash, const char* pSample, float flVolume, int nSeed, float flAttenuation, int iFlags, int iPitch, const Vector* pOrigin, const Vector* pDirection, void* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity, int unk ) {
	static auto oEmitSound = DTR::EmitSound.GetOriginal<decltype( &hkEmitSound )>( );

	if ( Config::Get<int>(Vars.MiscWeaponVolume ) < 100 ) {
		const float volume_scale = static_cast< float >( Config::Get<int>( Vars.MiscWeaponVolume ) ) / 100;

		if ( strstr( pSample, _( "weapon" ) ) )
			flVolume = std::clamp( flVolume * volume_scale, 0.0f, 1.0f );
	}

	/*if ( !ctx.m_pLocal )
		return oEmitSound( _this, edx, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, nSeed, flAttenuation, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity, unk );

	
	static int lastEmitTickDraw{ };
	if ( strstr( pSample, _( "draw" ) ) ) {
		if ( Interfaces::Globals->iTickCount - lastEmitTickDraw > 16 )
			lastEmitTickDraw = Interfaces::Globals->iTickCount;
		else {
			if ( iEntIndex == ctx.m_pLocal->Index( ) ) {
				flVolume = 0;
				lastEmitTickDraw = Interfaces::Globals->iTickCount;
			}
		}
	}

	static int lastEmitTickDeploy{ };
	if ( strstr( pSample, _( "deploy" ) ) ) {
		if ( Interfaces::Globals->iTickCount - lastEmitTickDeploy > 16 )
			lastEmitTickDeploy = Interfaces::Globals->iTickCount;
		else {
			if ( iEntIndex == ctx.m_pLocal->Index( ) ) {
				flVolume = 0;
				lastEmitTickDeploy = Interfaces::Globals->iTickCount;
			}
		}
	}*/

	return oEmitSound( _this, edx, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, nSeed, flAttenuation, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity, unk );
}