#pragma once
#include "../../core/displacement.h"
#include "../entity.h"

class IGameResources
{
public:
	int GetPing( int playerIndex ) {
		return *( int* )( uintptr_t( this ) + Displacement::Netvars->m_iPing + playerIndex * 4 );
	}	
	
	int GetC4Carrier( ) {
		return *( bool* )( uintptr_t( this ) + Displacement::Netvars->m_iPlayerC4 );
	}
};