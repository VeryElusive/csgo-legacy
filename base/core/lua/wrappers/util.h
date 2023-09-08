#pragma once
#include "../../../features/rage/autowall.h"

namespace Wrappers::Utils {
	PenetrationData FireBullet( Wrappers::Entity::CPlayer shooter, Wrappers::Entity::CPlayer target,
		bool isTaser, Vector src, Vector dst, bool penetrate ) {
		if ( !shooter.m_pPlayer->GetWeapon( ) || !shooter.m_pPlayer->GetWeapon( )->GetCSWeaponData( ) )
			return { };

		return Features::Autowall.FireBullet( shooter.m_pPlayer, target.m_pPlayer, shooter.m_pPlayer->GetWeapon( )->GetCSWeaponData( ), isTaser, src, dst, penetrate );
	}

	CGameTrace TraceLine( const Vector src, const Vector end, int mask, ETraceType traceType = TRACE_EVERYTHING, IHandleEntity* skip = nullptr ) {
		CGameTrace trace{ };
		CTraceFilter traceFilter{ skip, traceType };

		Interfaces::EngineTrace->TraceRay(
			{ src, end }, mask,
			&traceFilter, &trace
		);

		return trace;
	}

	Vector GetLocalEyePosition( ) {
		return ctx.m_vecEyePos;
	}
}