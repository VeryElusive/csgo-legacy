#include "entity.h"
#include "../utils/math.h"
#include "../core/interfaces.h"

void* CIKContext::operator new( size_t size )
{
	CIKContext* ptr = ( CIKContext* )Interfaces::MemAlloc->Alloc( size );
	Construct( ptr );

	return ptr;
}

void CIKContext::operator delete( void* ptr ) {
	Interfaces::MemAlloc->Free( ptr );
}

bool CBaseEntity::IsBreakable( ) {
	if ( !this || !this->Index( ) ) 
		return false;

	const auto cc{ this->GetClientClass( ) };
	if ( !cc ) 
		return false;

	// The member variable "m_takedamage" isn't properly set in the original function, causing it to return false prematurely.
	// set it to DAMAGE_YES
	// Reference: https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/mp/src/game/shared/obstacle_pushaway.cpp#L110
	int backupTakeDamage{ this->TakeDamage( ) };

	const auto name{ cc->szNetworkName };
	if ( ( name[ 1 ] != 'B' || name[ 5 ] != 'D' ) || ( name[ 1 ] == 'B' && name[ 9 ] == 'e' && name[ 10 ] == 'S' && name[ 16 ] == 'e' ) )
		this->TakeDamage( ) = 2;

	const auto ret{ reinterpret_cast< bool( __thiscall* )( void* ) >( Displacement::Sigs.IsBreakable )( this ) };
	this->TakeDamage( ) = backupTakeDamage;
	return ret;

	/*if ( reinterpret_cast< fn_t >( Displacement::Sigs.IsBreakable )( this ) )
		return true;

	const auto cc = this->GetClientClass( );
	if ( !cc )
		return false;

	return ( *reinterpret_cast< const std::uint32_t* >( cc->szNetworkName ) == 'erBC'
		&& *reinterpret_cast< const std::uint32_t* >( cc->szNetworkName + 7 ) == 'Selb' )
		|| ( *reinterpret_cast< const std::uint32_t* >( cc->szNetworkName ) == 'saBC'
			&& *reinterpret_cast< const std::uint32_t* >( cc->szNetworkName + 7 ) == 'ytit' );
	*/
}