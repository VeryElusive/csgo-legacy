#pragma once
#include "../../../features/rage/autowall.h"

namespace Wrappers::Utils {
	PenetrationData FireBullet( CBasePlayer* const shooter, CBasePlayer* const target,
		const bool isTaser, const Vector src, const Vector dst, bool penetrate ) {
		if ( !shooter->GetWeapon( ) )
			return { };

		return Features::Autowall.FireBullet( shooter, target, shooter->GetWeapon( )->GetCSWeaponData( ), isTaser, src, dst, penetrate );
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
}