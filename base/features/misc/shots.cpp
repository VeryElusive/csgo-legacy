#include "shots.h"

void CShots::ProcessShots( ) {
	if ( m_vecShots.empty( ) )
		return;

	if ( !ctx.m_pLocal )
		return m_vecShots.clear( );

	m_vecShots.erase(
		std::remove_if(
			m_vecShots.begin( ), m_vecShots.end( ),
			[ & ]( const Shot_t& shot ) -> bool {
				const auto remove{ Interfaces::Globals->flRealTime - shot.m_flFireTime > 1.f };

				if ( !shot.m_iServerProcessTick
					&& remove ) {
					if ( ctx.m_pLocal->IsDead( ) )
						Features::Logger.Log( _( "local player died before server processed shos" ), true );
					else
						Features::Logger.Log( _( "shot did not register" ), true );
				}

				return remove;
			}
		),
		m_vecShots.end( )
				);

	if ( m_vecShots.empty( ) )
		return;

	ctx.m_bInCreatemove = true;


	// fixme!
	for ( auto it{ m_vecShots.begin( ) }; it != m_vecShots.end( ); it = std::next( it ) ) {
		auto& shot{ *it };
		if ( !shot.m_pPlayer ) {
		NONEXISTANT:
			Features::Logger.Log( _( "player no longer exists" ), true );
			it = m_vecShots.erase( it );
			if ( m_vecShots.empty( ) )
				break;

			continue;
		}

		if ( !shot.m_iServerProcessTick )
			continue;


		const auto playerInfo{ Interfaces::Engine->GetPlayerInfo( shot.m_pPlayer->Index( ) ) };
		if ( !playerInfo.has_value( ) )
			goto NONEXISTANT;

		if ( shot.m_pPlayer->IsDead( ) ) {
			if ( shot.m_bHitPlayer )
				Features::Logger.Log( ( _( "hit " ) + ( std::string ) playerInfo->szName
					+ _( " in " ) + Features::Ragebot.HitgroupToString( shot.m_iServerHitgroup )
					+ _( " for " ) + std::to_string( shot.m_iServerDamage ).c_str( ) ) + _( " damage" ), true );
			else
				Features::Logger.Log( _( "target died before server processed shot" ), true );

			it = m_vecShots.erase( it );
			if ( m_vecShots.empty( ) )
				break;

			continue;
		}

		LagBackup_t backup{ shot.m_pPlayer };
		CGameTrace trace{ };

		shot.m_pRecord->Apply( shot.m_pPlayer );// side doesnt matter

		Interfaces::EngineTrace->ClipRayToEntity(
			{ shot.m_vecStart, shot.m_vecServerEnd },
			MASK_SHOT_PLAYER, shot.m_pPlayer, &trace
		);

		if ( shot.m_bHitPlayer ) {
			if ( shot.m_iServerHitgroup != shot.m_iHitgroup ) {
				// we miss resolved but still hit the player
				if ( trace.iHitGroup == shot.m_iHitgroup
					&& trace.pHitEntity == shot.m_pPlayer ) {
					Interfaces::EngineTrace->ClipRayToEntity(
						{ shot.m_vecStart, shot.m_vecServerEnd },
						MASK_SHOT_PLAYER, shot.m_pPlayer, &trace
					);

					Features::AnimSys.m_arrEntries.at( shot.m_pPlayer->Index( ) - 1 ).m_iMissedShots++;
					Features::Logger.Log( _( "hit incorrect hitbox due to resolver" ), false );
				}
				else
					Features::Logger.Log( _( "hit incorrect hitbox due to spread" ), false );
			}

			Features::Logger.Log( ( _( "hit " ) + ( std::string ) playerInfo->szName
				+ _( " in " ) + Features::Ragebot.HitgroupToString( shot.m_iServerHitgroup )
				+ _( " for " ) + std::to_string( shot.m_iServerDamage ).c_str( ) ) + _( " damage" ), true );
		}
		else {

			if ( trace.pHitEntity == shot.m_pPlayer ) {
				Features::AnimSys.m_arrEntries.at( shot.m_pPlayer->Index( ) - 1 ).m_iMissedShots++;
				Features::Logger.Log( _( "missed shot due to resolver" ), true );
			}
			else {
				const auto dir{ ( shot.m_vecServerEnd - shot.m_vecStart ).Normalized( ) };

				const auto vecEnd{ shot.m_vecStart + ( dir * 8192.f ) };

				Interfaces::EngineTrace->ClipRayToEntity(
					{ shot.m_vecStart, vecEnd },
					MASK_SHOT_PLAYER, shot.m_pPlayer, &trace
				);

				if ( trace.pHitEntity == shot.m_pPlayer )
					Features::Logger.Log( _( "missed shot due to penetration" ), true );
				else
					Features::Logger.Log( _( "missed shot due to spread" ), true );
			}
		}

		backup.Apply( shot.m_pPlayer );

		it = m_vecShots.erase( it );

		if ( m_vecShots.empty( ) )
			break;
	}

	ctx.m_bInCreatemove = false;
}