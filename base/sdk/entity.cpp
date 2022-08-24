#include "entity.h"
#include "../utils/math.h"
#include "../core/interfaces.h"

bool CBaseEntity::IsBreakable( )
{
	// @ida isbreakableentity: client.dll @ 55 8B EC 51 56 8B F1 85 F6 74 68

	const int iHealth = this->m_iHealth( );

	// first check to see if it's already broken
	if ( iHealth < 0 && this->MaxHealth( ) > 0 )
		return true;

	if ( this->TakeDamage( ) != DAMAGE_YES ) {
		const EClassIndex nClassIndex = this->GetClientClass( )->nClassID;

		// force pass cfuncbrush
		if ( nClassIndex != EClassIndex::CFuncBrush )
			return false;
	}

	if ( const int nCollisionGroup = this->m_CollisionGroup( ); nCollisionGroup != COLLISION_GROUP_PUSHAWAY && nCollisionGroup != COLLISION_GROUP_BREAKABLE_GLASS && nCollisionGroup != COLLISION_GROUP_NONE )
		return false;

	if ( iHealth > 200 )
		return false;

	if ( IMultiplayerPhysics* pPhysicsInterface = dynamic_cast< IMultiplayerPhysics* >( this ); pPhysicsInterface != nullptr ) {
		if ( pPhysicsInterface->GetMultiplayerPhysicsMode( ) != PHYSICS_MULTIPLAYER_SOLID )
			return false;
	}
	else {
		if ( const char* szClassName = this->GetClassname( ); !strcmp( szClassName, _( "func_breakable" ) ) || !strcmp( szClassName, _( "func_breakable_surf" ) ) ) {
			if ( !strcmp( szClassName, _( "func_breakable_surf" ) ) ) {
				CBreakableSurface* pSurface = static_cast< CBreakableSurface* >( this );

				// don't try to break it if it has already been broken
				if ( pSurface->m_bIsBroken( ) )
					return false;
			}
		}
		else if ( this->PhysicsSolidMaskForEntity( ) & CONTENTS_PLAYERCLIP ) {
			// hostages and players use CONTENTS_PLAYERCLIP, so we can use it to ignore them
			return false;
		}
	}

	if ( IBreakableWithPropData* pBreakableInterface = dynamic_cast< IBreakableWithPropData* >( this ); pBreakableInterface != nullptr ) {
		// bullets don't damage it - ignore
		if ( pBreakableInterface->GetDmgModBullet( ) <= 0.0f )
			return false;
	}

	return true;
}