#include "animation.h"

#define SWAP_RESIK_SIDE( side ) ( side == 2 ? 1 : 2 )
void CAnimationSys::GetSide( PlayerEntry& entry ) {
	if ( !entry.m_iMissedShots )
		return;

	switch ( entry.m_iMissedShots % 5 ) {
	case 0: { // normal angle
		if ( entry.m_iFirstResolverSide )
			entry.m_iResolverSide = entry.m_iFirstResolverSide;
		else
			entry.m_iResolverSide = 1;

		break;
	}
	case 1: { // opposite
		entry.m_iResolverSide = SWAP_RESIK_SIDE( entry.m_iFirstResolverSide );
		break;
	}
	case 2: { // half opposite
		entry.m_iResolverSide = SWAP_RESIK_SIDE( entry.m_iFirstResolverSide );
		break;
	}
	case 3: { // half normal
		entry.m_iResolverSide = entry.m_iFirstResolverSide;
		break;
	}
	case 4: { // no
		break;
	}
	default: { // no
		break;
	}
	}
}