#include "visuals.h"
#include "../rage/autowall.h"

class CPredTraceFilter : public ITraceFilter {
public:
	CPredTraceFilter( ) = default;

	bool ShouldHitEntity( IHandleEntity* pEntityHandle, int /*contentsMask*/ ) {
		if ( !pEntityHandle || entities.empty( ) )
			return false;

		auto it = std::find( entities.begin( ), entities.end( ), pEntityHandle );
		if ( it != entities.end( ) )
			return false;

		CBaseClient* pEntCC = ( ( IClientEntity* )pEntityHandle )->GetClientClass( );
		if ( pEntCC && strcmp( ccIgnore, "" ) ) {
			if ( pEntCC->szNetworkName == ccIgnore )
				return false;
		}

		return true;
	}

	virtual ETraceType GetTraceType( ) const { return TRACE_EVERYTHING; }

	inline void SetIgnoreClass( const char* Class ) { ccIgnore = Class; }

	std::vector< CBaseEntity* > entities;
	const char* ccIgnore = "";
};

void dispatch_effect( const char* name, const CEffectData& data )
{
	// 55 8B EC 83 E4 F8 83 EC 20 56 57 8B F9 C7 44 24
	static auto dispatch_effect_fn
		= reinterpret_cast< int( __fastcall* )( const char* n, const CEffectData & d ) >(
			MEM::FindPattern( "client.dll", "55 8B EC 83 E4 F8 83 EC 20 56 57 8B F9 C7 44 24 ? ? ? ? ? 8D 4C 24 08 C7 44 24" )
			);

	dispatch_effect_fn( name, data );
}

void CGrenadePrediction::Paint( ) {
	if ( !Config::Get<bool>( Vars.VisGrenadePrediction ) )
		return;

	if ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) )
		return;

	if ( !ctx.m_pWeapon || !ctx.m_pWeapon->IsGrenade( ) || !ctx.m_pWeapon->m_bPinPulled( ) || ctx.m_pWeapon->m_fThrowTime( ) > 0 )
		return;

	if ( !ctx.m_pWeaponData )
		return;

	CTraceFilter filter{ ctx.m_pLocal };
	CGameTrace trace;

	const auto wpnIndex{ ctx.m_pWeapon->m_iItemDefinitionIndex( ) };

	// setup trace filter for later.
	if ( wpnIndex && vecPath.size( ) > 1 ) {
		// iterate all players.
		if ( wpnIndex == WEAPON_HEGRENADE ) {
			for ( int i{ 1 }; i <= 64; ++i ) {
				const auto player = static_cast< CBasePlayer* >( Interfaces::ClientEntityList->GetClientEntity( i ) );
				if ( !player )
					continue;

				if ( player->IsDead( ) || player->m_bGunGameImmunity( ) || player->IsTeammate( ) )
					continue;

				const auto hitboxSet{ player->m_pStudioHdr( )->pStudioHdr->GetHitboxSet( player->m_nHitboxSet( ) ) };
				const auto hitbox = hitboxSet->GetHitbox( HITBOX_PELVIS );
				if ( !hitbox )
					continue;

				const auto center = Math::VectorTransform( ( hitbox->vecBBMin + hitbox->vecBBMax ) / 2.f, player->m_CachedBoneData( ).Base( )[ HITBOX_CHEST ] );
				const auto delta = center - vecPath.back( );

				// is within damage radius?
				if ( delta.Length( ) > 475.f )
					continue;

				// check if our path was obstructed by anything
				Interfaces::EngineTrace->TraceRay( { vecPath.back( ), center, Vector( -2.f, -2.f, -2.f ), Vector( 2.f, 2.f, 2.f ) }, MASK_SHOT, ( ITraceFilter* )&filter, &trace );
				if ( !trace.pHitEntity || trace.pHitEntity != player )
					continue;

				// rather 'interesting' formula by valve to compute damage.
				const auto d = ( delta.Length( ) - 25.f ) / 140.f;
				auto damage = 105.f * std::exp( -d * d );

				Features::Autowall.ScaleDamage( player, damage, 1.f, HITGROUP_CHEST, 0 );

				// clamp max damage
				damage = std::floorf( std::min( damage, ( player->m_ArmorValue( ) > 0 ) ? 57.f : 98.f ) );

				auto& entry{ Features::Visuals.PlayerESP.Entries.at( player->Index( ) - 1 ) };

				// we got damage on this guy
				if ( damage > 0 )
					entry.m_iNadeDamage = damage;
			}
		}
		/*else if ( wpnIndex != WEAPON_MOLOTOV && wpnIndex != WEAPON_INCGRENADE ) {
			Vector screen;
			if ( Math::WorldToScreen( vecBounces.back( ), screen ) ) {
				Render::Rectangle( screen.x, screen.y, 10.f, 10.f, Menu::AccentCol );
			}
		}
		else {
			CEffectData data;
			data.hitBox = 3;// waaaaaaaaaaaaaaaaaaaaaaat
			data.origin = vecBounces.back( );
			data.otherEntIndex = 0;
			data.damageType = 2;

			dispatch_effect( "weapon_molotov_held", data );
		}

		else if ( wpnIndex != WEAPON_FLASHBANG && wpnIndex != WEAPON_DECOY )
			Render::WorldCircle( vecPath.back( ), 120.f, Config::Get<Color>( Vars.VisGrenadePredictionCol ), Config::Get<Color>( Vars.VisGrenadePredictionCol ).Set<COLOR_A>( Config::Get<Color>( Vars.VisGrenadePredictionCol ).Get<COLOR_A>( ) / 8.f ) );
		*/
			
		Vector2D ab, cd;
		auto prev = vecPath[ 0 ];

		for ( const auto& bounce : vecBounces ) {
			Vector screen;

			if ( Math::WorldToScreen( bounce, screen ) ) {
				//Render::FilledRectangle( screen.x - 2, screen.y - 2, 4, 4, Menu::Accent2Col.Set<COLOR_A>( 100 ) );
				//Render::Rectangle( screen.x - 3, screen.y - 3, 6, 6, Menu::AccentCol );

				Render::Circle( screen.x, screen.y, 3, 10, Menu::AccentCol );
			}
		}

		for ( auto it = vecPath.begin( ), end = vecPath.end( ); it != end; ++it ) {
			if ( Math::WorldToScreen( prev, ab ) && Math::WorldToScreen( *it, cd ) )
				Render::Line( ab, cd, Config::Get<Color>( Vars.VisGrenadePredictionCol ) );

			prev = *it;
		}
	}
}

void CGrenadePrediction::View( ) {
	vecIgnoredEntities.clear( );
	vecPath.clear( );
	vecBounces.clear( );

	flThrowStrength = 0.f;
	flThrowVelocity = 0.f;

	if ( !Config::Get<bool>( Vars.VisGrenadePrediction ) )
		return;

	if ( !ctx.m_pLocal || ctx.m_pLocal->IsDead( ) )
		return;

	if ( !ctx.m_pWeapon || !ctx.m_pWeapon->IsGrenade( ) || !ctx.m_pWeapon->m_bPinPulled( ) || ctx.m_pWeapon->m_fThrowTime( ) > 0 )
		return;

	if ( !ctx.m_pWeaponData )
		return;

	QAngle angThrow;
	Interfaces::Engine->GetViewAngles( angThrow );

	flThrowStrength = ctx.m_pWeapon->m_flThrowStrength( );
	flThrowVelocity = ctx.m_pWeaponData->flThrowVelocity;

	Simulate( angThrow );
}

void CGrenadePrediction::TraceHull( Vector& src, Vector& end, CGameTrace& tr ) {
	// Setup grenade hull
	static const Vector hull[ 2 ] = { Vector( -2.0f, -2.0f, -2.0f ), Vector( 2.0f, 2.0f, 2.0f ) };

	CPredTraceFilter filter;
	filter.SetIgnoreClass( _( "CBaseCSGrenadeProjectile" ) );
	filter.entities = vecIgnoredEntities;

	Ray_t ray{ src, end, hull[ 0 ], hull[ 1 ] };

	const unsigned int mask = 0x200400B;
	Interfaces::EngineTrace->TraceRay( ray, mask, &filter, &tr );
}

void CGrenadePrediction::Simulate( QAngle& Angles ) {
	Vector vecSrc, vecThrow;
	Setup( vecSrc, vecThrow, Angles );

	const float& interval = Interfaces::Globals->flIntervalPerTick;

	// Log positions 20 times per sec
	const int logstep = TIME_TO_TICKS( 0.05f );

	int logtimer = 0;

	vecIgnoredEntities.push_back( ctx.m_pLocal );

	for ( unsigned int i = 0; i < 2048; ++i ) {
		if ( !logtimer )
			vecPath.push_back( vecSrc );

		const int s = Step( vecSrc, vecThrow, i, interval );
		if ( ( s & 1 ) || vecThrow == Vector( 0, 0, 0 ) )
			break;

		// Reset the log timer every logstep OR we bounced
		if ( ( s & 2 ) || logtimer >= logstep ) logtimer = 0;
		else ++logtimer;

		if ( vecThrow == Vector( ) )
			break;
	}

	vecPath.push_back( vecSrc );
}

void CGrenadePrediction::Setup( Vector& vecSrc, Vector& vecThrow, const QAngle& angEyeAngles ) {
	QAngle angThrow = angEyeAngles;
	float pitch = angThrow.x;

	if ( pitch <= 90.0f ) {
		if ( pitch < -90.0f ) {
			pitch += 360.0f;
		}
	}
	else {
		pitch -= 360.0f;
	}
	float a = pitch - ( 90.0f - fabs( pitch ) ) * 10.0f / 90.0f;
	angThrow.x = a;

	// get ThrowVelocity from weapon files.
	float flVel = flThrowVelocity * 0.9f;

	// clipped to [ 15, 750 ]
	flVel = std::clamp( flVel, 15.f, 750.f );

	//clamp the throw strength ranges just to be sure
	float flClampedThrowStrength = flThrowStrength;
	flClampedThrowStrength = std::clamp( flClampedThrowStrength, 0.0f, 1.0f );

	flVel *= Math::Lerp( flClampedThrowStrength, 0.3f, 1.0f );

	Vector vForward, vRight, vUp;
	Math::AngleVectors( angThrow, &vForward, &vRight, &vUp );

	vecSrc = ctx.m_pLocal->GetAbsOrigin( ) + ctx.m_pLocal->m_vecViewOffset( );
	float off = Math::Lerp( flClampedThrowStrength, -12.f, 0.0f );
	vecSrc.z += off;

	// Game calls UTIL_TraceHull here with hull and assigns vecSrc tr.endpos
	CGameTrace tr;
	Vector vecDest = vecSrc;
	vecDest = ( vecDest + vForward * 22.0f );
	TraceHull( vecSrc, vecDest, tr );

	// After the hull trace it moves 6 units back along vForward
	// vecSrc = tr.endpos - vForward * 6
	Vector vecBack = vForward; vecBack *= 6.0f;
	vecSrc = tr.vecEnd;
	vecSrc -= vecBack;

	auto velocity = ctx.m_pLocal->m_vecAbsVelocity( );
	if ( velocity.Length2D( ) > 1.2f )
		vecThrow = velocity;
	else
		vecThrow = { 0,0,0 };

	vecThrow *= 1.25f;
	vecThrow += ( vForward * flVel );
}

void CGrenadePrediction::PushEntity( Vector& src, const Vector& move, CGameTrace& tr ) {
	Vector vecAbsEnd = src;
	vecAbsEnd += move;

	// Trace through world
	TraceHull( src, vecAbsEnd, tr );
};

int CGrenadePrediction::PhysicsClipVelocity( const Vector& in, const Vector& normal, Vector& out, float overbounce ) {
	static const float STOP_EPSILON = 0.1f;

	float    backoff;
	float    change;
	float    angle;
	int        i, blocked;

	blocked = 0;

	angle = normal[ 2 ];

	if ( angle > 0 ) {
		blocked |= 1;        // floor
	}
	if ( !angle ) {
		blocked |= 2;        // step
	}

	backoff = in.DotProduct( normal ) * overbounce;

	for ( i = 0; i < 3; i++ ) {
		change = normal[ i ] * backoff;
		out[ i ] = in[ i ] - change;
		if ( out[ i ] > -STOP_EPSILON && out[ i ] < STOP_EPSILON ) {
			out[ i ] = 0;
		}
	}

	return blocked;
}

void CGrenadePrediction::ResolveFlyCollisionCustom( CGameTrace& tr, Vector& vecVelocity, float interval ) {
	// Calculate elasticity
	float flSurfaceElasticity = 1.0;  // Assume all surfaces have the same elasticity

	 //Don't bounce off of players with perfect elasticity
	if ( tr.pHitEntity && tr.pHitEntity->IsPlayer( ) ) {
		flSurfaceElasticity = 0.3f;
	}

	// if its breakable glass and we kill it, don't bounce.
	// give some damage to the glass, and if it breaks, pass
	// through it.
	if ( tr.pHitEntity->IsBreakable( ) ) {
		const auto clientClass{ tr.pHitEntity->GetClientClass( ) };
		if ( !clientClass )
			return;

		const auto& networkName{ clientClass->szNetworkName };

		if ( strcmp( networkName, "CFuncBrush" ) && strcmp( networkName, "CBaseDoor" ) && strcmp( networkName, "CCSPlayer" ) && strcmp( networkName, "CBaseEntity" ) ) {
			vecIgnoredEntities.push_back( tr.pHitEntity );

			// slow our flight a little bit
			vecVelocity *= 0.4f;
			return;
		}
	}

	float flGrenadeElasticity = 0.45f;
	float flTotalElasticity = flGrenadeElasticity * flSurfaceElasticity;
	if ( flTotalElasticity > 0.9f )
		flTotalElasticity = 0.9f;

	if ( flTotalElasticity < 0.0f )
		flTotalElasticity = 0.0f;

	// Calculate bounce
	Vector vecAbsVelocity;
	PhysicsClipVelocity( vecVelocity, tr.plane.vecNormal, vecAbsVelocity, 2.0f );
	vecAbsVelocity *= flTotalElasticity;

	// Stop completely once we move too slow
	float flSpeedSqr = vecAbsVelocity.Length2DSqr( );
	static const float flMinSpeedSqr = 20.0f * 20.0f; // 30.0f * 30.0f in CSS
	if ( flSpeedSqr < flMinSpeedSqr ) {
		vecAbsVelocity.x = vecAbsVelocity.y = vecAbsVelocity.z = 0;
	}

	// Stop if on ground
	if ( tr.plane.vecNormal.z > 0.7f ) {
		vecVelocity = vecAbsVelocity;
		vecAbsVelocity *= ( ( 1.0f - tr.flFraction ) * interval );
		PushEntity( tr.vecEnd, vecAbsVelocity, tr );
	}
	else {
		vecVelocity = vecAbsVelocity;
	}
};

int CGrenadePrediction::Step( Vector& vecSrc, Vector& vecThrow, int tick, float interval ) {
	auto AddGravityMove = [ ]( Vector& move, Vector& vel ) {
		// gravity for grenades.
		float gravity{ Offsets::Cvars.sv_gravity->GetFloat( ) * 0.4f };

		// move one tick using current velocity.
		move.x = vel.x * Interfaces::Globals->flIntervalPerTick;
		move.y = vel.y * Interfaces::Globals->flIntervalPerTick;

		// apply linear acceleration due to gravity.
		// calculate new z velocity.
		float z = vel.z - ( gravity * Interfaces::Globals->flIntervalPerTick );

		// apply velocity to move, the average of the new and the old.
		move.z = ( ( vel.z + z ) / 2.f ) * Interfaces::Globals->flIntervalPerTick;

		// write back new gravity corrected z-velocity.
		vel.z = z;
	};

	auto CheckDetonate = [ ]( const Vector& vecThrow, const CGameTrace& tr, int tick, float interval ) -> bool {
		// convert current simulation tick to time.
		float time = TICKS_TO_TIME( tick );

		switch ( ctx.m_pWeapon->m_iItemDefinitionIndex( ) ) {
		case WEAPON_SMOKEGRENADE:
			return vecThrow.Length( ) <= 0.1f && !( tick % TIME_TO_TICKS( 0.2f ) );
		case WEAPON_DECOY:
			return vecThrow.Length( ) <= 0.2f && !( tick % TIME_TO_TICKS( 0.2f ) );
		case WEAPON_MOLOTOV:
		case WEAPON_INCGRENADE:
			// Detonate when hitting the floor
			if ( tr.flFraction != 1.0f && ( std::cos( DEG2RAD( Offsets::Cvars.weapon_molotov_maxdetonateslope->GetFloat( ) ) ) <= tr.plane.vecNormal.z ) )
				return true;

			// detonate if we have traveled for too long.
			// checked every 0.1s
			return time >= Offsets::Cvars.molotov_throw_detonate_time->GetFloat( ) && !( tick % TIME_TO_TICKS( 0.1f ) );

		case WEAPON_FLASHBANG:
		case WEAPON_HEGRENADE:
		{
			return time >= 1.5f && !( tick % TIME_TO_TICKS( 0.2f ) );
		}
		default:
			return false;
		}

		return false;
	};

	// Apply gravity
	Vector move;
	AddGravityMove( move, vecThrow );

	// Push entity
	CGameTrace tr;
	PushEntity( vecSrc, move, tr );

	int result = 0;
	// Check ending conditions
	if ( CheckDetonate( vecThrow, tr, tick, interval ) ) {
		result |= 1;
	}

	if ( ( result & 1 ) || vecThrow == Vector( 0, 0, 0 ) || tr.flFraction != 1.0f )
		vecBounces.push_back( tr.vecEnd );

	// Resolve collisions
	if ( tr.flFraction != 1.0f && !tr.plane.vecNormal.IsZero( ) ) {
		result |= 2; // Collision!
		ResolveFlyCollisionCustom( tr, vecThrow, interval );
	}


	// Set new position
	vecSrc = tr.vecEnd;

	return result;
}