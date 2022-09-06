#include "visuals.h"
#include "../animations/animation.h"

void CVisuals::Main( ) {
	if ( !ctx.m_pLocal )
		return;

	DormantESP.Start( );

	for ( int i{ }; i <= Interfaces::ClientEntityList->GetHighestEntityIndex( ); i++ ) {
		const auto ent{ static_cast< CBaseEntity* >( Interfaces::ClientEntityList->GetClientEntity( i ) ) };
		if ( !ent )
			continue;

		if ( ent->IsPlayer( ) )
			PlayerESP.Main( static_cast< CBasePlayer* >( ent ) );
		else {
			EntModulate( ent );
			OtherEntities( ent );
		}
	}

	auto& w = ctx.m_ve2ScreenSize.x;
	auto& h = ctx.m_ve2ScreenSize.y;

	if ( Config::Get<bool>( Vars.RemovalScope ) && ctx.m_pLocal->m_bIsScoped( ) 
		&& ctx.m_pWeaponData && ctx.m_pWeaponData->nWeaponType == WEAPONTYPE_SNIPER_RIFLE ) {
		Render::Line( Vector2D( 0, h / 2 ), Vector2D( w, h / 2 ), Color( 0, 0, 0 ) );
		Render::Line( Vector2D( w / 2, 0 ), Vector2D( w / 2, h ), Color( 0, 0, 0 ) );
	}
	
	if ( !ctx.m_pLocal->IsDead( ) ) {
		if ( Config::Get<bool>( Vars.VisPenetrationCrosshair ) ) {
			const auto center = ctx.m_ve2ScreenSize / 2;
			const Color color = ctx.m_bCanPenetrate ? Color( 0, 255, 0, 155 ) : Color( 255, 0, 0, 155 );

			Render::Line( center - Vector2D( 1, 0 ), center + Vector2D( 2, 0 ), color );
			Render::Line( center - Vector2D( 0, 1 ), center + Vector2D( 0, 2 ), color );
		}

		if ( Config::Get<bool>( Vars.AntiAimManualDir ) ) {
			const auto& col{ Config::Get<Color>( Vars.AntiaimManualCol ) };

			Vector2D
				p1{ w / 2 - 55, h / 2 + 10 },
				p2{ w / 2 - 75, h / 2 },
				p3{ w / 2 - 55, h / 2 - 10 };

			Render::Triangle( p1, p2, p3, Features::Antiaim.ManualSide == 1 ? col : Color( 125, 125, 125, 150 ) );

			p1 = { w / 2 + 55, h / 2 - 10 };
			p2 = { w / 2 + 75, h / 2 };
			p3 = { w / 2 + 55, h / 2 + 10 };

			Render::Triangle( p1, p2, p3, Features::Antiaim.ManualSide == 2 ? col : Color( 125, 125, 125, 150 ) );
		}
	}

	AutoPeekIndicator( );
	ManageHitmarkers( );

	GrenadePrediction.Paint( );

}

void CVisuals::Watermark( ) {
	if ( !Config::Get<bool>( Vars.MiscWatermark ) )
		return;

	const auto nci = Interfaces::Engine->GetNetChannelInfo( );
	int ping = 0;

	if ( nci ) {
		const auto latency = Interfaces::Engine->IsPlayingDemo( ) ? 0.0f : nci->GetAvgLatency( FLOW_OUTGOING ) - ( 0.5f / Offsets::Cvars.cl_updaterate->GetFloat( ) );
		ping = static_cast<int>( std::max( 0.f, latency ) * 1000 );
	}

	const auto name = _( "Havoc [beta] " );
	const auto pstr = _( "| ping: " ) + std::to_string( ping );

	const auto name_size = Render::GetTextSize( name, Fonts::Menu );
	auto ping_size = Render::GetTextSize( pstr, Fonts::Menu );

	const auto size = Vector2D( name_size.x + ping_size.x + 40, 20 );
	const auto pos = Vector2D( ctx.m_ve2ScreenSize.x - size.x - 20, 15 );

	Render::FilledRectangle( pos, size, Menu::BackgroundCol );

	for ( int i{ }; i < 3; ++i ) {
		Render::Line( pos + Vector2D( size.x - 16 - i, 0 ), pos + Vector2D( size.x, 16 + i ), Menu::AccentCol );
		Render::Line( pos + Vector2D( size.x - 10 - i, 0 ), pos + Vector2D( size.x, 10 + i ), Menu::Accent2Col );


		Render::Line( pos + Vector2D( 0, size.y - 16 - i ), pos + Vector2D( 16 + i, size.y ), Menu::AccentCol );
		Render::Line( pos + Vector2D( 0, size.y - 10 - i ), pos + Vector2D( 10 + i, size.y ), Menu::Accent2Col );
	}

	Render::Text( Fonts::Menu, pos + Vector2D( 20, 3 ), Menu::AccentCol, 0, name );
	Render::Text( Fonts::Menu, pos + Vector2D( name_size.x + 20, 3 ), Color( 150, 150, 150 ), 0, pstr.c_str( ) );

	/*int i{ 1 };
	for ( auto& dbg : ctx.m_strDbgLogs ) {
		Render::Text( Fonts::Menu, pos + Vector2D( 0, 10 * i ), Color( 255, 255, 255 ), 0, dbg.first.c_str( ) );
		++i;
	}*/


	//Render::Text( Fonts::Menu, pos + Vector2D( 0, 10 ), Color( 255, 255, 255 ), 0, std::to_string( ctx.m_pLocal->m_angRotation( ).y ).c_str( ) );
	//Render::Text( Fonts::Menu, pos + Vector2D( 0, 20 ), Color( 255, 255, 255 ), 0, std::to_string( ctx.m_pLocal->m_pAnimState( )->flAbsYaw ).c_str( ) );
	//Render::Text( Fonts::Menu, pos + Vector2D( 0, 30 ), Color( 255, 255, 255 ), 0, std::to_string( ctx.m_pLocal->m_angEyeAngles( ).y ).c_str( ) );
}

void CVisuals::OtherEntities( CBaseEntity* ent ) {
	if ( !ent )
		return;

	auto client_class = ent->GetClientClass( );
	if ( !client_class )
		return;

	const auto& class_id = client_class->nClassID;


	if ( class_id == EClassIndex::CPlantedC4 ) {
		const auto owner{ static_cast< CBaseEntity* >( Interfaces::ClientEntityList->GetClientEntityFromHandle( ent->m_hOwnerEntity( ) ) ) };
		if ( owner )
			ctx.m_iBombCarrier = owner->Index( );

		if ( strstr( client_class->szNetworkName, _( "Planted" ) ) ) {
			ctx.m_iBombCarrier = -1;
			if ( Config::Get<bool>( Vars.VisBomb ) ) {
				Vector2D world;
					if ( Math::WorldToScreen( ent->m_vecOrigin( ), world ) )
						Render::Text( Fonts::HealthESP, world, Color( 255, 255, 255 ), FONT_CENTER, _( "BOMB" ) );
			}
		}
	}

	const auto distTo = ent->GetAbsOrigin( ).DistTo( ctx.m_pLocal->GetAbsOrigin( ) );
	if ( distTo > 1500.f )
		return;

	float maxAlpha{ std::min( distTo < 1245.f ? 255.f : 1500.f - distTo, 255.f ) };

	if ( !maxAlpha )
		return;

	if ( class_id != EClassIndex::CBaseWeaponWorldModel && ( strstr( client_class->szNetworkName, _( "Weapon" ) ) ) ) {
		if ( Config::Get<bool>( Vars.VisDroppedWeapon ) ) {
			const auto& origin{ ent->m_vecOrigin( ) };
			if ( origin.IsZero( ) )
				return;

			Vector2D world;
			if ( Math::WorldToScreen( origin, world ) )
				Render::Text( Fonts::HealthESP, world,
					Config::Get<Color>( Vars.VisDroppedWeaponCol ).Set<COLOR_A>( maxAlpha ),
					FONT_CENTER, static_cast< CWeaponCSBase* >( ent )->GetGunName( ).c_str( ) );
		}
	}
	else {
		const auto owner{ static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientNetworkableFromHandle( ent->m_hOwnerEntity( ) ) ) };
		if ( !owner || !owner->IsPlayer( ) )
			return;

		if ( strstr( client_class->szNetworkName, _( "Projectile" ) ) ) {
			if ( ( ( owner->IsTeammate( ) && Config::Get<bool>( Vars.VisGrenadesTeam ) )
				|| ( !owner->IsTeammate( ) && Config::Get<bool>( Vars.VisGrenadesEnemy ) ) ) )
				DrawGrenade( ent, maxAlpha );
		}

		if ( ( ( owner->IsTeammate( ) && Config::Get<bool>( Vars.VisGrenadesTeam ) )
			|| ( !owner->IsTeammate( ) && Config::Get<bool>( Vars.VisGrenadesEnemy ) ) )
			&& class_id == EClassIndex::CInferno ) {
			Vector min, max;
			ent->GetClientRenderable( )->GetRenderBounds( min, max );

			const auto radius = ( max - min ).Length2D( ) * 0.5f;

			DrawWrappingRing( ent, 7, _( "FIRE" ), static_cast< CBaseCSGrenadeProjectile* >( ent )->m_flSpawnTime( ), radius, maxAlpha );
		}
	}
}

void CVisuals::AutoPeekIndicator( ) {
	if ( ctx.m_pLocal->IsDead( ) )
		return;

	if ( !Config::Get<bool>( Vars.MiscAutoPeek ) )
		return;

	if ( !ctx.m_pWeapon 
		|| ctx.m_pLocal->IsDead( )
		|| ctx.m_pWeapon->IsKnife( ) || ctx.m_pWeapon->IsGrenade( ) )
		return;

	if ( !Features::Misc.AutoPeeking && !m_flAutoPeekSize )
		return;

	const float multiplier = static_cast< float >( 20.f * Interfaces::Globals->flFrameTime );

	if ( Config::Get<keybind_t>( Vars.MiscAutoPeekKey ).enabled )
		m_flAutoPeekSize += multiplier * ( 1.0f - m_flAutoPeekSize );
	else {
		if ( m_flAutoPeekSize > 0.01f )
			m_flAutoPeekSize += multiplier * -m_flAutoPeekSize;
		else
			m_flAutoPeekSize = 0.0f;
	}

	m_flAutoPeekSize = std::clamp<float>( m_flAutoPeekSize, 0.0f, 1.f );

	const auto& col = Config::Get<Color>( Vars.MiscAutoPeekCol );

	Render::WorldCircle( Features::Misc.OldOrigin, static_cast<int>( m_flAutoPeekSize * 20.0f ), col, col.Set<COLOR_A>( col.Get<COLOR_A>( ) / 5.f ) );
}

void CVisuals::DrawGrenade( CBaseEntity* ent, int maxAlpha ) {
	const auto model = ent->GetModel( );
	if ( !model ) 
		return;

	const auto hdr = Interfaces::ModelInfo->GetStudioModel( model );
	if ( !hdr ) 
		return;

	const char* model_name = hdr->szName;

	if ( !strstr( model_name, _( "thrown" ) ) && !strstr( model_name, _( "dropped" ) ) )
		return;

	const auto grenade = ( CBaseCSGrenadeProjectile* )ent;
	const char* name = _( "HE GRENADE" );

	if ( strstr( hdr->szName, _( "flash" ) ) )
		name = _( "FLASH" );
	else if ( strstr( hdr->szName, _( "smoke" ) ) ) {
		name = _( "SMOKE" );
		DrawWrappingRing( ent, 18.f, name, TICKS_TO_TIME( ( ( CWeaponCSBase* )ent )->m_nSmokeEffectTickBegin( ) ), 120.f, maxAlpha );
		return;
	}
	else if ( strstr( hdr->szName, _( "decoy" ) ) ) {
		name = _( "DECOY" );
		if ( grenade->m_nExplodeEffectTickBegin( ) )
			return;
	}
	else if ( strstr( hdr->szName, _( "incendiary" ) ) || strstr( hdr->szName, _( "molotov" ) ) )
		name = _( "FIRE" );
	else {
		if ( grenade->m_nExplodeEffectTickBegin( ) )
			return;
	}

	Vector2D world;
	if ( Math::WorldToScreen( ent->m_vecOrigin( ), world ) )
		Render::Text( Fonts::HealthESP, world, Color( 255, 255, 255, maxAlpha ), FONT_CENTER, name );
}

void CVisuals::DrawWrappingRing( CBaseEntity* entity, float seconds, const char* name, float spawntime, float radius, int maxAlpha ) {
	const auto fadeAlpha = std::min( std::max( seconds - ( Interfaces::Globals->flCurTime - spawntime ), 0.f ) * 2, 1.f );

	const auto owner = static_cast< CBasePlayer * >( Interfaces::ClientEntityList->GetClientEntityFromHandle( entity->m_hOwnerEntity( ) ) );
	if ( !owner || !owner->IsPlayer( ) )
		return;

	const auto color{ owner->IsTeammate() ? Config::Get<Color>( Vars.VisGrenadesTeamCol ) : Config::Get<Color>( Vars.VisGrenadesEnemyCol ) };

	Vector text_pos = entity->m_vecOrigin( );
	text_pos.z += 5.f;

	const float delta = Interfaces::Globals->flCurTime - spawntime;
	Vector2D world;
	if ( Math::WorldToScreen( entity->m_vecOrigin( ), world ) ) {
		Vector last_pos;

		const float fill = ( seconds - delta ) / ( seconds ) * 180.f;

		for ( float rot = -fill; rot <= fill; rot += 3.f ) {
			auto rotation = rot + delta * 90.f;

			while ( rotation > 360.f )
				rotation -= 360.f;

			Vector rotated_pos = entity->m_vecOrigin( );

			rotated_pos.z -= 5.f;

			rotated_pos.x += std::cos( DEG2RAD( rotation ) ) * radius;
			rotated_pos.y += std::sin( DEG2RAD( rotation ) ) * radius;

			if ( rot != -fill ) {
				Vector2D w2s_new, w2s_old;

				if ( Math::WorldToScreen( rotated_pos, w2s_new ) && Math::WorldToScreen( last_pos, w2s_old ) ) {
					auto alpha = 1.f - ( std::abs( rot ) / fill );

					float threshold = seconds * 0.2f;

					if ( seconds - delta < threshold ) {
						float diff = ( seconds - delta ) / ( seconds ) * 5.f;

						alpha *= diff;
					}

					Vertex_t v[ ] = {
						{ world },
						{ w2s_old },
						{ w2s_new }
					};
					Color col = color;
					col[ 3 ] *= alpha;

					Render::Line( Vector2D( w2s_old.x, w2s_old.y ), Vector2D( w2s_new.x, w2s_new.y ), col.Set<COLOR_A>( std::min( maxAlpha, static_cast< int >( col[ 3 ] * fadeAlpha ) ) ) );
				}
			}

			last_pos = rotated_pos;
		}

		Render::Text( Fonts::HealthESP, world, Color( 255, 255, 255, static_cast<int>( maxAlpha * fadeAlpha ) ), FONT_CENTER, name );//color.Set<COLOR_A>( static_cast<int>( maxAlpha * fadeAlpha ) )
	}
}

void CVisuals::EntModulate( CBaseEntity* ent ) {
	if ( !ent || ent->IsDormant( ) )
		return;

	const auto client_class = ent->GetClientClass( );
	if ( !client_class )
		return;

	auto rgb_to_int = [ ]( int red, int green, int blue ) -> int {
		int r;
		int g;
		int b;

		r = red & 0xFF;
		g = green & 0xFF;
		b = blue & 0xFF;
		return ( r << 16 | g << 8 | b );
	};

	// bloom
	if ( client_class->nClassID == EClassIndex::CEnvTonemapController ) {
		if ( Config::Get<bool>( Vars.VisWorldBloom ) ) {
			*( bool* )( uintptr_t( ent ) + Offsets::m_bUseCustomAutoExposureMin ) = true;
			*( bool* )( uintptr_t( ent ) + Offsets::m_bUseCustomAutoExposureMax ) = true;
			*( bool* )( uintptr_t( ent ) + Offsets::m_bUseCustomBloomScale ) = true;

			*( float* )( uintptr_t( ent ) + Offsets::m_flCustomAutoExposureMin ) = float( Config::Get<int>( Vars.VisWorldBloomExposure ) ) / 50.f;
			*( float* )( uintptr_t( ent ) + Offsets::m_flCustomAutoExposureMax ) = float( Config::Get<int>( Vars.VisWorldBloomExposure ) ) / 50.f;
			*( float* )( uintptr_t( ent ) + Offsets::m_flCustomBloomScale ) = float( Config::Get<int>( Vars.VisWorldBloomScale ) ) / 50.f;
		}
		else {
			*( bool* )( uintptr_t( ent ) + Offsets::m_bUseCustomAutoExposureMin ) = true;
			*( bool* )( uintptr_t( ent ) + Offsets::m_bUseCustomAutoExposureMax ) = true;
			*( bool* )( uintptr_t( ent ) + Offsets::m_bUseCustomBloomScale ) = true;
			*( float* )( uintptr_t( ent ) + Offsets::m_flCustomAutoExposureMin ) = 0.f;
			*( float* )( uintptr_t( ent ) + Offsets::m_flCustomAutoExposureMax ) = 0.f;
			*( float* )( uintptr_t( ent ) + Offsets::m_flCustomBloomScale ) = 0.f;
		}
	}

	// fog
	if ( client_class->nClassID == EClassIndex::CFogController ) { 
		*( byte* )( ( uintptr_t )ent + Offsets::m_fog_enable ) = Config::Get<bool>( Vars.VisWorldFog );				// m_fog.enable

		*( bool* )( uintptr_t( ent ) + 0xA1D ) = Config::Get<bool>( Vars.VisWorldFog );					// m_fog.blend
		*( float* )( uintptr_t( ent ) + 0x9F8 ) = 0;
		*( float* )( uintptr_t( ent ) + 0x9FC ) = Config::Get<int>( Vars.VisWorldFogDistance );			// m_fog.start
		*( float* )( uintptr_t( ent ) + 0xA04 ) = Config::Get<int>( Vars.VisWorldFogDensity ) * 0.01f;	// m_fog.maxdensity
		*( float* )( uintptr_t( ent ) + 0xA24 ) = Config::Get<int>( Vars.VisWorldFogHDR ) * 0.01f;		// fog_hdrcolorscale

		const auto& col = Config::Get<Color>( Vars.VisWorldFogCol );
		*( int* )( uintptr_t( ent ) + 0x9E8 ) = rgb_to_int( ( int )( col.Get<COLOR_R>( ) ), ( int )( col.Get<COLOR_G>( ) ), ( int )( col.Get<COLOR_R>( ) ) ); // m_fog.colorPrimary
		*( int* )( uintptr_t( ent ) + 0x9EC ) = rgb_to_int( ( int )( col.Get<COLOR_B>( ) ), ( int )( col.Get<COLOR_G>( ) ), ( int )( col.Get<COLOR_R>( ) ) ); // m_fog.colorSecondary
	}
}

// so embarrassed about this func 
void CVisuals::KeybindsList( ) {
	auto addBind = [ & ]( const char* name, float& newXSize, std::vector<BindInfo_t>& binds, int mode ) {
		if ( mode == EKeyMode::AlwaysOn )
			return;

		const auto& b = binds.emplace_back( name, mode );

		if ( b.m_iTextLength + 40 > newXSize )
			newXSize = b.m_iTextLength + 40;
	};

	if ( !Config::Get<bool>( Vars.MiscKeybindList ) ) {
		m_vec2KeyBindAbsSize = { 120, 20 };
		return;
	}

	// gimme setup
	const bool topBarHovered = Inputsys::hovered( m_vec2KeyBindPos, Vector2D( std::max( m_vec2KeyBindAbsSize.x, 100.f ), 20 ) );
	Vector2D keyBindSize = { 120, 20 };
	std::vector<BindInfo_t> binds;

	// gimme binds
	if ( Config::Get<bool>( Vars.ExploitsDoubletap ) && Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).enabled )
		addBind( _( "Doubletap" ), keyBindSize.x, binds, Config::Get<keybind_t>( Vars.ExploitsDoubletapKey ).mode );	
	
	if ( Config::Get<bool>( Vars.ExploitsHideshots ) && Config::Get<keybind_t>( Vars.ExploitsHideshotsKey ).enabled )
		addBind( _( "Hideshots" ), keyBindSize.x, binds, Config::Get<keybind_t>( Vars.ExploitsHideshotsKey ).mode );

	if ( Config::Get<bool>( Vars.RagebotDamageOverride ) && Config::Get<keybind_t>( Vars.RagebotDamageOverrideKey ).enabled )
		addBind( _( "Damage override" ), keyBindSize.x, binds, Config::Get<keybind_t>( Vars.RagebotDamageOverrideKey ).mode );

	if ( Config::Get<keybind_t>( Vars.RagebotForceBaimKey ).enabled )
		addBind( _( "Force baim" ), keyBindSize.x, binds, Config::Get<keybind_t>( Vars.RagebotForceBaimKey ).mode );

	if ( Config::Get<bool>( Vars.MiscSlowWalk ) && Config::Get<keybind_t>( Vars.MiscSlowWalkKey ).enabled )
		addBind( _( "Slow walk" ), keyBindSize.x, binds, Config::Get<keybind_t>( Vars.MiscSlowWalkKey ).mode );

	if ( Config::Get<bool>( Vars.MiscAutoPeek ) && Config::Get<keybind_t>( Vars.MiscAutoPeekKey ).enabled )
		addBind( _( "Auto peek" ), keyBindSize.x, binds, Config::Get<keybind_t>( Vars.MiscAutoPeekKey ).mode );

	if ( Config::Get<bool>( Vars.MiscFakeDuck ) && Config::Get<keybind_t>( Vars.MiscFakeDuckKey ).enabled )
		addBind( _( "Fake duck" ), keyBindSize.x, binds, Config::Get<keybind_t>( Vars.MiscFakeDuckKey ).mode );

	constexpr auto appendLength{ 17 };

	// gimme extra room
	if ( !binds.empty( ) || Menu::MenuAlpha )
		keyBindSize.y += 9 + binds.size( ) * appendLength;

	// gimme that lerped size
	m_vec2KeyBindAbsSize.x = Math::Interpolate( m_vec2KeyBindAbsSize.x, keyBindSize.x, 10.f * Interfaces::Globals->flFrameTime );
	m_vec2KeyBindAbsSize.y = Math::Interpolate( m_vec2KeyBindAbsSize.y, keyBindSize.y, 10.f * Interfaces::Globals->flFrameTime );

	// gimme alpha
	auto alpha_mod = 1.f;
	if ( ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) ) && Menu::MenuAlpha < 1.f )
		alpha_mod = Menu::MenuAlpha;

	if ( alpha_mod < 0.001f )
		return;

	// gimme drag
	if ( !m_bKeybindDragging && Inputsys::pressed( VK_LBUTTON ) && topBarHovered && Menu::MenuAlpha )
		m_bKeybindDragging = true;
	else if ( m_bKeybindDragging && Inputsys::down( VK_LBUTTON ) )
		m_vec2KeyBindPos -= Inputsys::MouseDelta;
	else if ( m_bKeybindDragging && !Inputsys::down( VK_LBUTTON ) )
		m_bKeybindDragging = false;

	if ( m_vec2KeyBindAbsSize.y < 1 )
		return;

	auto& pos = m_vec2KeyBindPos;
	auto& size = m_vec2KeyBindAbsSize;


	Interfaces::Surface->SetClipRect( pos.x, pos.y, size.x, size.y );
	{
		// gimme header
		Render::FilledRectangle( pos, size, Menu::BackgroundCol.Set<COLOR_A>( 255 * alpha_mod ) );

		for ( int i{ }; i < 3; ++i ) {
			Render::Line( pos + Vector2D( size.x - 16 - i, 0 ), pos + Vector2D( size.x, 16 + i ), Menu::AccentCol.Set<COLOR_A>( 255 * alpha_mod ) );
			Render::Line( pos + Vector2D( size.x - 10 - i, 0 ), pos + Vector2D( size.x, 10 + i ), Menu::Accent2Col.Set<COLOR_A>( 255 * alpha_mod ) );


			Render::Line( pos + Vector2D( 0, size.y - 16 - i ), pos + Vector2D( 16 + i, size.y ), Menu::AccentCol.Set<COLOR_A>( 255 * alpha_mod ) );
			Render::Line( pos + Vector2D( 0, size.y - 10 - i ), pos + Vector2D( 10 + i, size.y ), Menu::Accent2Col.Set<COLOR_A>( 255 * alpha_mod ) );
		}

		if ( alpha_mod >= 1.f )
			Render::Gradient( pos.x, pos.y + 20, size.x, 20, Color( 0, 0, 0, static_cast< int >( 180 * alpha_mod ) ), Menu::BackgroundCol.Set<COLOR_A>( 0 ), false );

		/*18.0000000 key
		25.0000000 binds*/

		Render::Text( Fonts::Menu, pos + Vector2D( size.x / 2 - 21, 3 ), Color( 255, 255, 255, static_cast< int >( 255 * alpha_mod ) ), FONT_LEFT, _( "Key" ) );
		Render::Text( Fonts::Menu, pos + Vector2D( size.x / 2 - 3, 3 ), Menu::AccentCol.Set<COLOR_A>( 255 * alpha_mod ), FONT_LEFT, _( "binds" ) );

		// gimme text
		for ( auto i{ 0 }; i < binds.size( ); i++ ) {
			const auto& bind = binds.at( i );
			Render::Text( Fonts::Menu, pos + Vector2D( 20, 25 + appendLength * i + 1 ), Color( 255, 255, 255, static_cast< int >( 255 * alpha_mod ) ), FONT_LEFT, bind.m_szName );

			Render::Text( Fonts::Menu, pos + Vector2D( size.x - 20, 25 + appendLength * i ), Menu::AccentCol.Set<COLOR_A>( 255 * alpha_mod ), FONT_RIGHT, bind.m_szMode );
		}
	}
	Interfaces::Surface->SetClipRect( 0, 0, ctx.m_ve2ScreenSize.x, ctx.m_ve2ScreenSize.y );
}