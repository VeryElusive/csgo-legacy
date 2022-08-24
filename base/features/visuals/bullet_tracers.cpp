#include "game_visual_abuse.h"
/*
bool PrecacheModel( const char* szModelName ) {
	INetworkStringTable* m_pModelPrecacheTable = Interfaces::NetworkContainer->FindTable( _( "modelprecache" ) );

	if ( m_pModelPrecacheTable ) {
		Interfaces::ModelInfo->FindOrLoadModel( szModelName );
		int idx = m_pModelPrecacheTable->AddString( false, szModelName );
		if ( idx == INVALID_STRING_TABLE )
			return false;
	}
	return true;
}

void CBulletTracer::Draw( ) {
	if ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) )
		return;

	float time = Interfaces::Globals->flCurTime;
	bool       is_final_impact;

	for ( size_t i{ }; i < m_vecBulletImpactInfo.size( ) && !m_vecBulletImpactInfo.empty( ); ++i ) {
		auto& tr = m_vecBulletImpactInfo[ i ];

		if ( tr.m_nIndex == ctx.m_pLocal->Index( ) ) {
			// is this the final impact?
			// last impact in the vector, it's the final impact.
			if ( i == ( m_vecBulletImpactInfo.size( ) - 1 ) )
				is_final_impact = true;

			// the current impact's tickbase is different than the next, it's the final impact.
			else if ( ( i + 1 ) < m_vecBulletImpactInfo.size( ) && tr.m_nTickbase != m_vecBulletImpactInfo.operator[ ]( i + 1 ).m_nTickbase )
				is_final_impact = true;

			else
				is_final_impact = false;

			if ( !is_final_impact )
				m_vecBulletImpactInfo.erase( m_vecBulletImpactInfo.begin( ) + i );
		}

		float delta = time - tr.m_flExpTime;
		if ( delta > 1.0f )
			m_vecBulletImpactInfo.erase( m_vecBulletImpactInfo.begin( ) + i );
	}

	if ( !m_vecBulletImpactInfo.empty( ) ) {
		for ( auto& it : m_vecBulletImpactInfo ) {
			float delta = time - it.m_flExpTime;
			Color col{ it.m_nIndex == ctx.m_pLocal->Index( ) ? Config::Get<Color>( Vars.VisLocalBulletTracersCol ) : Config::Get<Color>( Vars.VisOtherBulletTracersCol ) };
			col = col.Set<COLOR_A>( ( 1.0f - delta * 255 ) );
			Vector2D w2s_start, w2s_end;
			bool a = Math::WorldToScreen( it.m_vecStartPos, w2s_start );
			bool b = Math::WorldToScreen( it.m_vecHitPos, w2s_end );

			switch ( Config::Get<int>( Vars.VisBulletTracersType ) ) {
			case 0:
				if ( a && b )
					Render::Line( w2s_start, w2s_end, col );
				break;
			case 1:
				if ( !PrecacheModel( _( "materials/sprites/laserbeam.vmt" ) ) )
					break;

				BeamInfo_t beam_info;

				beam_info.nType = 0;
				beam_info.pszModelName = _( "materials/sprites/laserbeam.vmt" );
				beam_info.nModelIndex = Interfaces::ModelInfo->GetModelIndex( _( "materials/sprites/laserbeam.vmt" ) );
				beam_info.flHaloScale = 0.0f;
				beam_info.flLife = 0.09f; //0.09
				beam_info.flWidth = .6f;
				beam_info.flEndWidth = .75f;
				beam_info.flFadeLength = 3.0f;
				beam_info.flAmplitude = 0.f;
				beam_info.flBrightness = ( col.Get<COLOR_A>( ) - 255.f ) * 0.8f;
				beam_info.flSpeed = 1.f;
				beam_info.iStartFrame = 1;
				beam_info.flFrameRate = 60;
				beam_info.flRed = col.Get<COLOR_R>( );
				beam_info.flGreen = col.Get<COLOR_G>( );
				beam_info.flBlue = col.Get<COLOR_B>( );
				beam_info.nSegments = 4;
				beam_info.bRenderable = true;
				beam_info.nFlags = 0;

				beam_info.vecStart = it.m_vecStartPos;
				beam_info.vecEnd = it.m_vecHitPos;

				Beam_t* beam = Interfaces::ViewRenderBeams->CreateBeamPoints( beam_info );

				if ( beam )
					Interfaces::ViewRenderBeams->DrawBeam( beam );

				break;
			}
		}
	}
}*/