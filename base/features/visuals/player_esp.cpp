#include "visuals.h"
#include "../animations/animation.h"

void CPlayerESP::Main( CBasePlayer* ent ) {
	int type{ ENEMY };
	if ( ent == ctx.m_pLocal )
		type = LOCAL;
	else if ( ent->IsTeammate( ) ) 
		type = TEAM;

	if ( ent->IsDead( ) || ent->m_iHealth( ) <= 0 )
		return;

	CheckPlayerBoolFig( type, VisEnable )

	auto& entry = Entries.at( ent->Index( ) - 1 );
	entry.type = type;
	entry.ent = ent;

	// reset stored health
	if ( entry.health < std::min( ent->m_iHealth( ), 100 ) )
		entry.health = std::min( ent->m_iHealth( ), 100 );

	// animation
	if ( entry.health > ent->m_iHealth( ) )
		entry.health -= 6.f * Interfaces::Globals->flFrameTime * ( entry.health - ent->m_iHealth( ) );

	if ( ent->Dormant( ) ) {
		entry.Alpha -= Interfaces::Globals->flFrameTime * 0.2f;
		entry.DormancyFade += Interfaces::Globals->flFrameTime;
	}
	else {
		entry.Alpha += Interfaces::Globals->flFrameTime * 2.f;
		entry.DormancyFade -= Interfaces::Globals->flFrameTime * 2.f;
	}

	entry.Alpha = std::clamp<float>( entry.Alpha, 0.f, 1.f );
	entry.DormancyFade = std::clamp<float>( entry.DormancyFade, 0.f, 1.f );

	if ( !GetBBox( ent, entry.BBox ) )
		return;

	DrawBox( entry );
	DrawHealth( entry );
	DrawName( entry );
	DrawSkeleton( entry );
	DrawFlags( entry );
	DrawWeapon( entry, DrawAmmo( entry ) );

	if ( entry.m_iNadeDamage > 0 )
		Render::Text( Fonts::HealthESP, entry.BBox.x + entry.BBox.w / 2, entry.BBox.y - 23, Color( 255, 255, 255, static_cast<int>( 255 * entry.Alpha ) ), FONT_CENTER, std::to_string( entry.m_iNadeDamage ).c_str( ) );

	entry.m_iNadeDamage = 0;
}

void CPlayerESP::DrawBox( VisualPlayerEntry& entry ) {
	CheckPlayerBoolFig( entry.type, VisBox )

	Color last;
	GetPlayerColorFig( entry.type, VisBoxCol, last )

	last = last.Set<COLOR_A>( last.Get<COLOR_A>( ) * entry.Alpha );
	last = last.Lerp( DormantCol.Set<COLOR_A>( last.Get<COLOR_A>( ) * 0.4f ), entry.DormancyFade );

	const auto outline{ Color( 0, 0, 0, static_cast< int >( last.Get<COLOR_A>( ) * 0.8f ) ) };

	Render::Rectangle( entry.BBox.x - 1, entry.BBox.y - 1, entry.BBox.w + 2, entry.BBox.h + 2, outline );
	Render::Rectangle( entry.BBox.x + 1, entry.BBox.y + 1,  entry.BBox.w - 2, entry.BBox.h - 2, outline );
	Render::Rectangle( entry.BBox.x, entry.BBox.y,  entry.BBox.w, entry.BBox.h, last );
}

void CPlayerESP::DrawHealth( VisualPlayerEntry& entry ) {
	CheckPlayerBoolFig( entry.type, VisHealth );

	Color last;
	GetPlayerColorFig( entry.type, VisHealthCol, last );

	// rework this lmfaoooo
	switch ( entry.type ) {
		case 0:
			if ( Config::Get<bool>( Vars.VisHealthOverrideLocal ) )
				break;
		case 1:
			if ( Config::Get<bool>( Vars.VisHealthOverrideTeam ) )
				break;
		case 2:
			if ( Config::Get<bool>( Vars.VisHealthOverrideEnemy ) )
				break;

		const int green{ static_cast<int>( static_cast< float >( std::min( entry.health, 100 ) ) * 2.55f ) };
		const int red{ 255 - green };
		last = Color( red, green, 0 );
	}

	last = last.Set<COLOR_A>( last.Get<COLOR_A>( ) * entry.Alpha );
	last = last.Lerp( DormantCol.Set<COLOR_A>( last.Get<COLOR_A>( ) * 0.4f ), entry.DormancyFade );

	const auto outline{ Color( 0, 0, 0, static_cast< int >( last.Get<COLOR_A>( ) * 0.8f ) ) };

	const auto bar_height{ static_cast<int>( static_cast< float >( entry.health ) * static_cast< float >( entry.BBox.h ) / 100.0f ) };
	const auto offset{ entry.BBox.h - bar_height };

	Render::FilledRectangle( entry.BBox.x - 6, entry.BBox.y - 1, 4, entry.BBox.h + 2, outline );
	Render::FilledRectangle( entry.BBox.x - 5, entry.BBox.y + offset, 2, bar_height, last );

	if ( entry.health < 95 )
		Render::Text( Fonts::HealthESP, entry.BBox.x - 3, entry.BBox.y + offset, Color( 255, 255, 255 ).Set<COLOR_A>( last.Get<COLOR_A>( ) ), FONT_CENTER, std::to_string( entry.health ).c_str( ) );
}

bool CPlayerESP::DrawAmmo( VisualPlayerEntry& entry ) {
	switch ( entry.type ) {
		case 0: if ( !Config::Get<bool>( Vars.VisAmmoLocal ) ) return false; break;
		case 1: if ( !Config::Get<bool>( Vars.VisAmmoTeam ) ) return false; break;
		case 2: if ( !Config::Get<bool>( Vars.VisAmmoEnemy ) ) return false; break;
	}

	const auto weapon = entry.ent->GetWeapon( );
	if ( !weapon )
		return false;

	if ( weapon->m_iItemDefinitionIndex( ) == WEAPON_C4 || weapon->m_iItemDefinitionIndex( ) == WEAPON_HEALTHSHOT || weapon->IsGrenade( ) || weapon->IsKnife( ) )
		return false;

	const auto weapon_info = weapon->GetCSWeaponData( );
	if ( !weapon_info )
		return false;

	const auto ammo = weapon->m_iClip1( );
	const auto max_clip = weapon_info->iMaxClip1;

	Color last;
	GetPlayerColorFig( entry.type, VisAmmoCol, last )

	last = last.Set<COLOR_A>( last.Get<COLOR_A>( ) * entry.Alpha );
	last = last.Lerp( DormantCol.Set<COLOR_A>( last.Get<COLOR_A>( ) * 0.4f ), entry.DormancyFade );

	const auto outline = Color( 0, 0, 0, static_cast< int >( last.Get<COLOR_A>( ) * 0.8f ) );

	// outline
	Render::FilledRectangle( entry.BBox.x - 1, entry.BBox.y + entry.BBox.h + 2,  entry.BBox.w + 2, 4, outline );
	// color
	if ( ammo )
		Render::FilledRectangle( entry.BBox.x, entry.BBox.y + entry.BBox.h + 3,  ammo * entry.BBox.w / max_clip, 2, last );

	return true;
}

void CPlayerESP::DrawName( VisualPlayerEntry& entry ) {
	CheckPlayerBoolFig( entry.type, VisName )

	Color last;
	GetPlayerColorFig( entry.type, VisNameCol, last )

	last = last.Set<COLOR_A>( last.Get<COLOR_A>( ) * entry.Alpha );
	last = last.Lerp( DormantCol.Set<COLOR_A>( last.Get<COLOR_A>( ) * 0.4f ), entry.DormancyFade );

	static auto sanitize = [ ]( char* name ) -> std::string {
		name[ 127 ] = '\0';

		std::string tmp( name );

		if ( tmp.length( ) > 20 ) {
			tmp.erase( 20, tmp.length( ) - 20 );
			tmp.append( "..." );
		}

		return tmp;
	};

	auto player_info = Interfaces::Engine->GetPlayerInfo( entry.ent->Index( ) );
	if ( !player_info.has_value() )
		return;

	const auto name = sanitize( player_info.value( ).szName );

	Render::Text( Fonts::NameESP, entry.BBox.x + entry.BBox.w / 2, entry.BBox.y - 14, last, FONT_CENTER, name.c_str( ) );
}

void CPlayerESP::DrawWeapon( VisualPlayerEntry& entry, bool AmmoBar ) {
	switch ( entry.type ) {
		case 0: if ( !Config::Get<bool>( Vars.VisWeapIconLocal ) && !Config::Get<bool>( Vars.VisWeapTextLocal ) ) return; break;
		case 1: if ( !Config::Get<bool>( Vars.VisWeapIconTeam ) && !Config::Get<bool>( Vars.VisWeapTextTeam ) ) return; break;
		case 2: if ( !Config::Get<bool>( Vars.VisWeapIconEnemy ) && !Config::Get<bool>( Vars.VisWeapTextEnemy ) ) return; break;
	}

	const auto weapon = entry.ent->GetWeapon( );
	if ( !weapon )
		return;

	Color last;
	GetPlayerColorFig( entry.type, VisWeapCol, last )

	last = last.Set<COLOR_A>( last.Get<COLOR_A>( ) * entry.Alpha );
	last = last.Lerp( DormantCol.Set<COLOR_A>( last.Get<COLOR_A>( ) * 0.4f ), entry.DormancyFade );

	int append{ 2 };
	if ( AmmoBar )
		append += 6;
	CheckIfPlayer( VisWeapText, entry.type ) {
		Render::Text( Fonts::WeaponIcon, entry.BBox.x + entry.BBox.w / 2, entry.BBox.y + entry.BBox.h + append, last, FONT_CENTER, weapon->GetIcon( ).c_str( ) );

		append += 14;
	}

	CheckIfPlayer( VisWeapIcon, entry.type ) {
		Render::Text( Fonts::HealthESP, entry.BBox.x + entry.BBox.w / 2, entry.BBox.y + entry.BBox.h + append, last, FONT_CENTER, weapon->GetGunName( ).c_str( ) );
	}
}

void CPlayerESP::DrawFlags( VisualPlayerEntry& entry ) {
	if ( entry.ent->Dormant( ) )
		return;

	std::vector<std::pair< std::string, bool>> flags{ };
	CheckIfPlayer( VisFlagBLC, entry.type ) {
		if ( Features::AnimSys.m_arrEntries.at( entry.ent->Index( ) - 1 ).m_bBrokeLC )
			flags.push_back( std::make_pair( _( "LC" ), true ) );
	}

	CheckIfPlayer( VisFlagDefusing, entry.type ) {
		if ( entry.ent->m_bIsDefusing( ) )
			flags.push_back( std::make_pair( _( "DEFUSING" ), true ) );
	}

	/* TODO:
	CheckIfPlayer( VisFlagC4, entry.type ) {
		auto weapons{ entry.ent->m_hMyWeapons( ) };
		for ( size_t i{ }; i < 48; ++i ) {
			const auto weaponHandle{ weapons[ i ] };
			if ( !weaponHandle )
				break;

			auto weapon{ static_cast< CWeaponCSBase* >( Interfaces::ClientEntityList->GetClientEntityFromHandle( weaponHandle ) ) };
			if ( !weapon )
				continue;

			const auto index{ weapon->m_iItemDefinitionIndex( ) };

			if ( index == WEAPON_C4 )
				flags.push_back( std::make_pair( _( "C4" ), false ) );
		}
	}*/

	CheckIfPlayer( VisFlagArmor, entry.type ) {
		if ( entry.ent->m_ArmorValue( ) > 0 )
			flags.push_back( std::make_pair( entry.ent->m_bHasHelmet( ) ? _( "HK" ) : _( "K" ), 0 ) );
	}	
	
	CheckIfPlayer( VisFlagFlash, entry.type ) {
		if ( entry.ent->m_flFlashDuration( ) > 1.f )
			flags.push_back( std::make_pair( _( "FLASH" ), false ) );
	}	
	
	CheckIfPlayer( VisFlagReload, entry.type ) {
		const auto& layer{ entry.ent->m_AnimationLayers( )[ 1 ] };

		if ( entry.ent->GetSequenceActivity( layer.nSequence ) == 967u && layer.flWeight != 0.f )
			flags.push_back( std::make_pair( _( "RELOAD" ), false ) );
	}	
	
	CheckIfPlayer( VisFlagScoped, entry.type ) {
		if ( entry.ent->m_bIsScoped( ) )
			flags.push_back( std::make_pair( _( "SCOPED" ), false ) );
	}

	int i{ };
	for ( const auto& flag : flags ) {
		Render::Text( Fonts::HealthESP, entry.BBox.x + entry.BBox.w + 2, entry.BBox.y + 7 * i, flag.second ? Color( 250, 134, 92 ) : Color( 255, 255, 255, static_cast<int>( 255 * entry.Alpha ) ), 0, flag.first.c_str( ) );

		++i;
	}
}

void CPlayerESP::DrawSkeleton( VisualPlayerEntry& entry ) {
	CheckPlayerBoolFig( entry.type, VisSkeleton )

	if ( entry.ent->IsDormant( ) )
		return;

	const auto hdr{ entry.ent->m_pStudioHdr( )->pStudioHdr };
	if ( !hdr )
		return;

	Color last;
	GetPlayerColorFig( entry.type, VisSkeletonCol, last )

	last = last.Set<COLOR_A>( last.Get<COLOR_A>( ) * entry.Alpha );
	last = last.Lerp( DormantCol.Set<COLOR_A>( last.Get<COLOR_A>( ) * 0.4f ), entry.DormancyFade );

	Vector2D bone1, bone2;
	for ( size_t n{ }; n < hdr->nBones; ++n ) {
		auto* bone = hdr->GetBone( n );
		if ( !bone || !( bone->iFlags & 256 ) || bone->iParent == -1 ) {
			continue;
		}

		auto BonePos = [ & ]( int n ) -> Vector {
			return {
				entry.ent->m_CachedBoneData( ).Base( )[ n ][ 0 ][ 3 ],
				entry.ent->m_CachedBoneData( ).Base( )[ n ][ 1 ][ 3 ],
				entry.ent->m_CachedBoneData( ).Base( )[ n ][ 2 ][ 3 ]
			};
		};

		if ( !Math::WorldToScreen( BonePos( n ), bone1 ) || !Math::WorldToScreen( BonePos( bone->iParent ), bone2 ) )
			continue;

		Render::Line( bone1, bone2, last );
	}
}

bool CPlayerESP::GetBBox( CBasePlayer* ent, rect& box ) {
	const auto min = ent->m_vecMins( );
	const auto max = ent->m_vecMaxs( );

	QAngle dir = ctx.m_angOriginalViewangles;
	Vector vF, vR, vU;
	dir.x = 0;
	dir.z = 0;

	Math::AngleVectors( dir, &vF, &vR, &vU );

	const auto zh = vU * max.z + vF * max.y + vR * min.x; // = Front left front
	const auto e = vU * max.z + vF * max.y + vR * max.x; //  = Front right front
	const auto d = vU * max.z + vF * min.y + vR * min.x; //  = Front left back
	const auto c = vU * max.z + vF * min.y + vR * max.x; //  = Front right back

	const auto g = vU * min.z + vF * max.y + vR * min.x; //  = Bottom left front
	const auto f = vU * min.z + vF * max.y + vR * max.x; //  = Bottom right front
	const auto a = vU * min.z + vF * min.y + vR * min.x; //  = Bottom left back
	const auto b = vU * min.z + vF * min.y + vR * max.x; //  = Bottom right back*-

	Vector pointList[ ] = {
		a,
		b,
		c,
		d,
		e,
		f,
		g,
		zh,
	};

	Vector2D transformed[ ARRAYSIZE( pointList ) ];

	for ( int i = 0; i < ARRAYSIZE( pointList ); i++ )
	{
		auto origin = ent->GetAbsOrigin( );

		pointList[ i ] += origin;

		if ( !Math::WorldToScreen( pointList[ i ], transformed[ i ] ) )
			return false;
	}

	float left = FLT_MAX;
	float top = -FLT_MAX;
	float right = -FLT_MAX;
	float bottom = FLT_MAX;

	for ( int i = 0; i < ARRAYSIZE( pointList ); i++ ) {
		if ( left > transformed[ i ].x )
			left = transformed[ i ].x;
		if ( top < transformed[ i ].y )
			top = transformed[ i ].y;
		if ( right < transformed[ i ].x )
			right = transformed[ i ].x;
		if ( bottom > transformed[ i ].y )
			bottom = transformed[ i ].y;
	}

	box.x = left;
	box.y = bottom;
	box.w = right - left;
	box.h = top - bottom + 5;

	return true;
}