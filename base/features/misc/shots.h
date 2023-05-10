#pragma once
#include "../../context.h"
#include "../animations/animation.h"
#include "logger.h"

struct Shot_t {
	Shot_t( CBasePlayer* player,
		std::shared_ptr<LagRecord_t> record,
		int hitgroup,
		Vector start,
		Vector end ) : m_pPlayer( player ), m_pRecord( record ), m_iHitgroup( hitgroup ),  m_vecStart( start ), m_flFireTime( Interfaces::Globals->flRealTime ), m_vecPredEnd( end )
	{
	}

	/* our inital data */
	CBasePlayer* m_pPlayer{ };
	std::shared_ptr<LagRecord_t> m_pRecord{ };
	int m_iHitgroup{ };
	Vector m_vecStart{ };
	Vector m_vecPredEnd{ };
	float m_flFireTime{ };
	float m_flYaw{ };

	/* data given by server */
	bool m_bHitPlayer{ };
	int m_iServerProcessTick{ };
	int m_iServerHitgroup{ };
	int m_iServerDamage{ };
	Vector m_vecServerEnd{ };

	/* set after added */
	//int m_iDelayTime{ };
};

class CShots {
public:
	void AddShot( CBasePlayer* player,
		std::shared_ptr<LagRecord_t> record,
		int hitgroup,
		Vector start, Vector end ) {
		m_vecShots.emplace_back( player, record, hitgroup, start, end );
	}

	void ProcessShots( );

	FORCEINLINE Shot_t* LastUnprocessed( ) {
		if ( m_vecShots.empty( ) )
			return nullptr;

		const auto shot = std::find_if(
			m_vecShots.rbegin( ), m_vecShots.rend( ),
			[ ]( const Shot_t& shot ) {
				return /*!shot.m_iDelayTime
					&& */shot.m_iServerProcessTick
					&& shot.m_iServerProcessTick == Interfaces::ClientState->iServerTick;
			}
		);

		return shot == m_vecShots.rend( ) ? nullptr : &*shot;
	}

	std::vector<Shot_t> m_vecShots{ };
};

namespace Features { inline CShots Shots; };