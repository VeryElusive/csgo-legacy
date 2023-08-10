#include "entity.h"
#include "../context.h"

bool CWeaponCSBase::IsGrenade( )
{
	if ( !this )
		return false;

	switch ( m_iItemDefinitionIndex( ) ) {
	case WEAPON_FLASHBANG:
	case WEAPON_HEGRENADE:
	case WEAPON_SMOKEGRENADE:
	case WEAPON_MOLOTOV:
	case WEAPON_DECOY:
	case WEAPON_INCGRENADE:
		return true;
	default: return false;
	}
}

bool CWeaponCSBase::IsKnife( )
{
	if ( !this )
		return false;

	switch ( m_iItemDefinitionIndex( ) ) {
	case WEAPON_KNIFE_T:
	case WEAPON_KNIFE_CT:
	case WEAPON_KNIFE_FLIP:
	case WEAPON_KNIFE_BAYONET:
	case WEAPON_KNIFE_GUT:
	case WEAPON_KNIFE_BUTTERFLY:
	case WEAPON_KNIFE_KARAMBIT:
	case WEAPON_KNIFE_FALCHION:
	case WEAPON_KNIFE_M9_BAYONET:
	case WEAPON_KNIFE_SHADOW_DAGGERS:
	case WEAPON_KNIFE_BOWIE:
	case WEAPON_KNIFE_HUNTSMAN:
		return true;
	default: return false;
	}
}

std::string CBaseCombatWeapon::GetIcon( )
{
	if ( !this )
		return "";

	if ( ( ( CWeaponCSBase* ) this )->IsKnife( ) )
		return _( "]" );

	switch ( this->m_iItemDefinitionIndex( ) )
	{
	case WEAPON_DEAGLE:
		return _( "A" );
	case WEAPON_ELITE:
		return _( "B" );
	case WEAPON_FIVESEVEN:
		return _( "C" );
	case WEAPON_GLOCK:
		return _( "D" );
	case WEAPON_AK47:
		return _( "W" );
	case WEAPON_AUG:
		return _( "U" );
	case WEAPON_AWP:
		return _( "Z" );
	case WEAPON_FAMAS:
		return _( "R" );
	case WEAPON_G3SG1:
		return _( "X" );
	case WEAPON_GALILAR:
		return _( "Q" );
	case WEAPON_M249:
		return _( "g" );
	case WEAPON_M4A1:
		return _( "S" );
	case WEAPON_MAC10:
		return _( "K" );
	case WEAPON_P90:
		return _( "P" );
	case WEAPON_MP5SD:
		return _( "K" );
	case WEAPON_UMP45:
		return _( "L" );
	case WEAPON_XM1014:
		return _( "b" );
	case WEAPON_BIZON:
		return _( "M" );
	case WEAPON_MAG7:
		return _( "d" );
	case WEAPON_NEGEV:
		return _( "f" );
	case WEAPON_SAWEDOFF:
		return _( "c" );
	case WEAPON_TEC9:
		return _( "H" );
	case WEAPON_TASER:
		return _( "h" );
	case WEAPON_HKP2000:
		return _( "E" );
	case WEAPON_MP7:
		return _( "N" );
	case WEAPON_MP9:
		return _( "O" );
	case WEAPON_NOVA:
		return _( "e" );
	case WEAPON_P250:
		return _( "F" );
	case WEAPON_SCAR20:
		return _( "Y" );
	case WEAPON_SG556:
		return _( "V" );
	case WEAPON_SSG08:
		return _( "a" );
	case WEAPON_FLASHBANG:
		return _( "i" );
	case WEAPON_HEGRENADE:
		return _( "j" );
	case WEAPON_SMOKEGRENADE:
		return _( "k" );
	case WEAPON_MOLOTOV:
		return _( "l" );
	case WEAPON_DECOY:
		return _( "m" );
	case WEAPON_INCGRENADE:
		return _( "n" );
	case WEAPON_C4:
		return _( "o" );
	case WEAPON_KNIFE_T:
		return _( "[" );
	case WEAPON_USPS:
		return _( "G" );
	case WEAPON_CZ75A:
		return _( "I" );
	case WEAPON_REVOLVER:
		return _( "J" );
	default:
		return _( "]" );
	}

	return "";
}

std::string CBaseCombatWeapon::GetGunName( ) {
	if ( !this )
		return "";

	if ( ( ( CWeaponCSBase* ) this )->IsKnife( ) )
		return _( "KNIFE" );

	switch ( m_iItemDefinitionIndex( ) )
	{
	case WEAPON_DEAGLE:
		return _( "DEAGLE" );
	case WEAPON_ELITE:
		return _( "DUAL BERETTAS" );
	case WEAPON_FIVESEVEN:
		return _( "FIVE-SEVEN" );
	case WEAPON_GLOCK:
		return _( "GLOCK 18" );
	case WEAPON_HKP2000:
		return _( "P2000" );
	case WEAPON_P250:
		return _( "P250" );
	case WEAPON_USPS:
		return _( "USP-S" );
	case WEAPON_TEC9:
		return _( "TEC-9" );
	case WEAPON_REVOLVER:
		return _( "REVOLVER" );
	case WEAPON_MAC10:
		return _( "MAC-10" );
	case WEAPON_UMP45:
		return _( "UMP-45" );
	case WEAPON_BIZON:
		return _( "PP-BIZON" );
	case WEAPON_MP7:
		return _( "MP7" );
	case WEAPON_MP9:
		return _( "MP9" );
	case WEAPON_P90:
		return _( "P90" );
	case WEAPON_GALILAR:
		return _( "GALIL AR" );
	case WEAPON_FAMAS:
		return _( "FAMAS" );
	case WEAPON_M4A1:
		return _( "M4A4" );
	case WEAPON_AUG:
		return _( "AUG" );
	case WEAPON_SG556:
		return _( "SG-553" );
	case WEAPON_AK47:
		return _( "AK-47" );
	case WEAPON_G3SG1:
		return _( "G3SG1" );
	case WEAPON_SCAR20:
		return _( "SCAR-20" );
	case WEAPON_AWP:
		return _( "AWP" );
	case WEAPON_SSG08:
		return _( "SSG 08" );
	case WEAPON_XM1014:
		return _( "XM1014" );
	case WEAPON_SAWEDOFF:
		return _( "SAWED-OFF" );
	case WEAPON_MAG7:
		return _( "MAG-7" );
	case WEAPON_NOVA:
		return _( "NOVA" );
	case WEAPON_NEGEV:
		return _( "NEGEV" );
	case WEAPON_M249:
		return _( "M249" );
	case WEAPON_TASER:
		return _( "ZEUS X27" );
	case WEAPON_FLASHBANG:
		return _( "FLASHBANG" );
	case WEAPON_HEGRENADE:
		return _( "HE GRENADE" );
	case WEAPON_SMOKEGRENADE:
		return _( "SMOKE" );
	case WEAPON_MOLOTOV:
		return _( "MOLOTOV" );
	case WEAPON_DECOY:
		return _( "DECOY" );
	case WEAPON_INCGRENADE:
		return _( "INCENDIARY" );
	case WEAPON_C4:
		return _( "C4" );
	case WEAPON_CZ75A:
		return _( "CZ75-AUTO" );
	default:
		return "";
	}

	return "";
}