#include "shot_info.h"
#include "../visuals/visuals.h"
#include "../rage/ragebot.h"

void CShots::OnNetUpdate( ) {
	if ( m_elements.empty( ) )
		return;

	if ( !ctx.m_pLocal
		|| !Interfaces::Engine->IsInGame( ) )
		return m_elements.clear( );

	if ( ctx.m_pLocal->IsDead( ) ) {
		m_elements.erase(
			std::remove_if(
				m_elements.begin( ), m_elements.end( ),
				[ & ]( const shot_t& shot ) -> bool {
					return !shot.m_target 
						&& shot.m_cmd_number + TIME_TO_TICKS( ctx.m_flInLatency + ctx.m_flRealOutLatency ) + 1 < Interfaces::ClientState->iCommandAck
						&& !shot.m_process_tick;
				}
			),
			m_elements.end( )
					);

		if ( m_elements.empty( ) ) {
			Features::Logger.Log( _( "local player died before server processed shots" ), true );

			return;
		}
	}

	for ( auto& shot : m_elements ) {
		if ( shot.m_processed
			|| !shot.m_process_tick
			|| Interfaces::Globals->iTickCount < shot.m_process_tick )
			continue;

		if ( shot.m_target ) {
			LagBackup_t backup{ shot.m_target };

			const auto p_info = Interfaces::Engine->GetPlayerInfo( shot.m_target->Index( ) );

			if ( shot.m_server_info.m_hurt_tick ) {
				shot.m_lag_record->Apply( shot.m_target, true );
				CGameTrace trace{ };

				Interfaces::EngineTrace->ClipRayToEntity(
					{ shot.m_src, shot.m_server_info.m_impact_pos },
					MASK_SHOT_PLAYER, shot.m_target, &trace
				);

				// we miss resolved lol
				if ( trace.iHitGroup == shot.m_shot_hitgroup &&
					shot.m_server_info.m_hitgroup != shot.m_shot_hitgroup ) {
					Features::Logger.Log( _( "mis-resolved target" ), false );
					Features::AnimSys.m_arrEntries.at( shot.m_target->Index( ) - 1 ).m_iMissedShots++;
				}

				Features::Logger.Log( ( _( "hit " ) + ( std::string )p_info->szName + _( " in " ) + Features::Ragebot.Hitgroup2Str( shot.m_server_info.m_hitgroup ) + _( " for " ) + std::to_string( shot.m_server_info.m_damage ).c_str( ) ) + _( " damage" ), true );
			}
			else if ( shot.m_target->IsDead( ) )
				Features::Logger.Log( _( "target died before server processed shot" ), true );
			else {
				shot.m_lag_record->Apply( shot.m_target, true );

				CGameTrace trace{ };

				Interfaces::EngineTrace->ClipRayToEntity(
					{ shot.m_src, shot.m_server_info.m_impact_pos },
					MASK_SHOT_PLAYER, shot.m_target, &trace
				);

				if ( trace.pHitEntity == shot.m_target ) {
					Features::AnimSys.m_arrEntries.at( shot.m_target->Index( ) - 1 ).m_iMissedShots++;
					Features::Logger.Log( _( "missed shot due to hitbox mismatch" ), true );
				}
				else
					Features::Logger.Log( _( "missed shot due to spread" ), true );
			}

			backup.Apply( shot.m_target );
		}

		shot.m_processed = true;
	}

	m_elements.erase(
		std::remove_if(
			m_elements.begin( ), m_elements.end( ),
			[ & ]( const shot_t& shot ) -> bool {
				return shot.m_processed 
					|| ( shot.m_cmd_number + TIME_TO_TICKS( ctx.m_flInLatency + ctx.m_flRealOutLatency ) + 1 < Interfaces::ClientState->iCommandAck && !shot.m_process_tick );
			}
		),
		m_elements.end( )
				);
}