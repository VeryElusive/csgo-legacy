#include "scripting.h"
#include "../../sdk/hash/fnv1a.h"
#include "wrappers/render.h"
#include "wrappers/entity.h"
#include "wrappers/interfaces.h"
#include "wrappers/util.h"


void Scripting::Unload( std::string name ) {
	const auto it = std::find_if( m_mapStates.begin( ), m_mapStates.end( ), [ &name ]( const auto& it ) -> bool {
		return it.first == name;
		} );

	if ( it != m_mapStates.end( ) )
		m_mapStates.erase( it );
}

void Scripting::AddCallback( std::string name, sol::protected_function callback, sol::this_state state ) {
	auto lua = sol::state_view( state.lua_state( ) );
	auto state_ptr = lua[ _( "__lua_state" ) ].get<std::shared_ptr<LuaState_t>>( );

	state_ptr->m_cCallbacks.insert_or_assign( FNV1A::Hash( name.c_str( ) ), callback );
}


void Scripting::Log( std::string text, bool onscreen ) {
	Features::Logger.Log( text, onscreen );
}

void Scripting::Load( std::string fileName ) {
	m_bLoadingScript = true;

	Unload( fileName );

	const auto state{ std::make_shared<LuaState_t>( ) };

	state->m_cState.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::string,
		sol::lib::math,
		sol::lib::table,
		sol::lib::bit32,
		sol::lib::utf8 );

	state->m_cState[ _( "__lua_state" ) ] = state;

	// logging
	state->m_cState[ _( "log" ) ] = sol::overload( &Log );

	/* tables */
	const auto callbackTable{ state->m_cState[ _( "callbacks" ) ].get_or_create<sol::table>( ) };
	const auto renderTable{ state->m_cState[ _( "render" ) ].get_or_create<sol::table>( ) };
	const auto netvarTable{ state->m_cState[ _( "netvars" ) ].get_or_create<sol::table>( ) };
	const auto utilsTable{ state->m_cState[ _( "utils" ) ].get_or_create<sol::table>( ) };

	const auto entityList{ state->m_cState[ _( "entity_list" ) ].get_or_create<sol::table>( ) };
	const auto globalsList{ state->m_cState[ _( "globals" ) ].get_or_create<sol::table>( ) };

	// callbacks
	callbackTable[ _( "register" ) ] = sol::overload( &AddCallback );

	// netvars
	netvarTable[ _( "get_offset" ) ] = sol::overload( &Wrappers::Entity::GetOffset );

	// render
	renderTable[ _( "rectangle" ) ] = sol::overload( &Wrappers::Renderer::Rect );
	renderTable[ _( "filled_rectangle" ) ] = sol::overload( &Wrappers::Renderer::RectFilled );
	renderTable[ _( "rounded_rectangle" ) ] = sol::overload( &Wrappers::Renderer::RoundedBox );
	renderTable[ _( "filled_rounded_rectangle" ) ] = sol::overload( &Wrappers::Renderer::FilledRoundedBox );
	renderTable[ _( "line" ) ] = sol::overload( &Wrappers::Renderer::Line );

	utilsTable[ _( "trace_bullet" ) ] = sol::overload( &Wrappers::Utils::FireBullet );
	utilsTable[ _( "trace_line" ) ] = sol::overload( &Wrappers::Utils::TraceLine );

	// INTERFACES:
	entityList[ _( "get_local_index" ) ] = sol::overload( &Wrappers::Interface::EntityList::GetLocalIndex );
	entityList[ _( "get_entity" ) ] = sol::overload( &Wrappers::Interface::EntityList::GetClientEntity );
	entityList[ _( "get_max_clients" ) ] = sol::overload( &Wrappers::Interface::EntityList::GetMaxClients );
	entityList[ _( "get_highest_entity_index" ) ] = sol::overload( &Wrappers::Interface::EntityList::GetHighestEntityIndex );

	globalsList[ _( "real_time" ) ] = sol::overload( &Wrappers::Interface::Globals::RealTime );


	/* usertypes */
	auto color_ut{ state->m_cState.new_usertype<Color>( _( "color" ) ) };
	color_ut[ sol::meta_function::construct ] = sol::constructors<Color( ), Color( int, int, int ), Color( int, int, int, int )>( );
	color_ut[ _( "r" ) ] = sol::overload( &Color::r );
	color_ut[ _( "g" ) ] = sol::overload( &Color::g );
	color_ut[ _( "b" ) ] = sol::overload( &Color::b );
	color_ut[ _( "a" ) ] = sol::overload( &Color::a );

	auto vector_ut = state->m_cState.new_usertype<Vector>( _( "vector" ) );
	vector_ut[ sol::meta_function::construct ] = sol::constructors<Vector( float, float, float )>( );
	vector_ut[ _( "x" ) ] = &Vector::x;
	vector_ut[ _( "y" ) ] = &Vector::y;
	vector_ut[ _( "z" ) ] = &Vector::z;

	auto vector2d_ut = state->m_cState.new_usertype<Vector2D>( _( "vector2d" ) );
	vector2d_ut[ sol::meta_function::construct ] = sol::constructors<Vector2D( float, float )>( );
	vector2d_ut[ _( "x" ) ] = &Vector2D::x;
	vector2d_ut[ _( "y" ) ] = &Vector2D::y;

	auto qangle_ut = state->m_cState.new_usertype<QAngle>( _( "qangle" ) );
	qangle_ut[ sol::meta_function::construct ] = sol::constructors<QAngle( float, float, float )>( );
	qangle_ut[ _( "x" ) ] = &QAngle::x;
	qangle_ut[ _( "y" ) ] = &QAngle::y;
	qangle_ut[ _( "z" ) ] = &QAngle::z;	
	
	auto penetration_data_t_ut{ state->m_cState.new_usertype<PenetrationData>( _( "penetration_data_t" ) ) };
	penetration_data_t_ut[ sol::meta_function::construct ] = sol::constructors<PenetrationData( )>( );
	penetration_data_t_ut[ _( "damage" ) ] = &PenetrationData::dmg;
	penetration_data_t_ut[ _( "hitbox" ) ] = &PenetrationData::hitbox;
	penetration_data_t_ut[ _( "hitgroup" ) ] = &PenetrationData::hitgroup;
	penetration_data_t_ut[ _( "penetration_count" ) ] = &PenetrationData::penetrationCount;
	penetration_data_t_ut[ _( "target" ) ] = &PenetrationData::target;
	
	auto trace_t_ut{ state->m_cState.new_usertype<CGameTrace>( _( "trace_t" ) ) };
	trace_t_ut[ sol::meta_function::construct ] = sol::constructors<CGameTrace( )>( );
	trace_t_ut[ _( "end" ) ] = &CGameTrace::vecEnd;
	trace_t_ut[ _( "start" ) ] = &CGameTrace::vecStart;
	trace_t_ut[ _( "fraction" ) ] = &CGameTrace::flFraction;
	trace_t_ut[ _( "hitbox" ) ] = &CGameTrace::iHitbox;
	trace_t_ut[ _( "hitgroup" ) ] = &CGameTrace::iHitGroup;
	trace_t_ut[ _( "hit_entity" ) ] = &CGameTrace::pHitEntity;
	trace_t_ut[ _( "all_solid" ) ] = &CGameTrace::bAllSolid;
	trace_t_ut[ _( "start_solid" ) ] = &CGameTrace::bStartSolid;
	trace_t_ut[ _( "contents" ) ] = &CGameTrace::iContents;
	trace_t_ut[ _( "plane" ) ] = &CGameTrace::plane;

	auto entity_ut{ state->m_cState.new_usertype<Wrappers::Entity::CPlayer>( _( "player_t" ), sol::no_constructor ) };
	entity_ut[ _( "get_int" ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetInt );
	entity_ut[ _( "get_bool" ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetBool );
	entity_ut[ _( "get_float" ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetFloat );
	entity_ut[ _( "get_vector" ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetVector );
	entity_ut[ _( "set_int" ) ] = sol::overload( &Wrappers::Entity::CPlayer::SetInt );
	entity_ut[ _( "set_bool" ) ] = sol::overload( &Wrappers::Entity::CPlayer::SetBool );
	entity_ut[ _( "set_float" ) ] = sol::overload( &Wrappers::Entity::CPlayer::SetFloat );
	entity_ut[ _( "set_vector" ) ] = sol::overload( &Wrappers::Entity::CPlayer::SetVector );

	entity_ut[ _( "valid" ) ] = sol::overload( &Wrappers::Entity::CPlayer::Valid );
	entity_ut[ _( "alive" ) ] = sol::overload( &Wrappers::Entity::CPlayer::IsAlive );
	entity_ut[ _( "dormant" ) ] = sol::overload( &Wrappers::Entity::CPlayer::IsDormant );
	entity_ut[ _( "is_player" ) ] = sol::overload( &Wrappers::Entity::CPlayer::IsPlayer );

	entity_ut[ _( "get_layer_sequence_activity" ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetLayerSequenceActivity );
	entity_ut[ _( "animate" ) ] = sol::overload( &Wrappers::Entity::CPlayer::AnimatePlayer );
	entity_ut[ _( "setupbones" ) ] = sol::overload( &Wrappers::Entity::CPlayer::SetupBones );

	auto player_entry_t_ut{ state->m_cState.new_usertype<PlayerEntry>( _( "player_entry_t" ), sol::no_constructor ) };
	player_entry_t_ut[ _( "get_previous_anim_data" ) ] = sol::overload( &PlayerEntry::GetPreviousAnimData );
	player_entry_t_ut[ _( "player_index" ) ] = &PlayerEntry::m_iPlayerIndex;
	player_entry_t_ut[ _( "bot" ) ] = &PlayerEntry::m_bBot;
	player_entry_t_ut[ _( "broke_lagcompensation" ) ] = &PlayerEntry::m_bBrokeLC;
	player_entry_t_ut[ _( "lag_exploit" ) ] = &PlayerEntry::m_bRecordAdded;
	player_entry_t_ut[ _( "first_shot_yaw" ) ] = &PlayerEntry::m_flFirstShotEyeYaw;
	player_entry_t_ut[ _( "first_shot_time" ) ] = &PlayerEntry::m_flFirstShotTime;
	player_entry_t_ut[ _( "lower_body_realign_timer" ) ] = &PlayerEntry::m_flLowerBodyRealignTimer;
	player_entry_t_ut[ _( "lower_body_yaw_target" ) ] = &PlayerEntry::m_flLowerBodyYawTarget;
	player_entry_t_ut[ _( "spawntime" ) ] = &PlayerEntry::m_flSpawnTime;
	player_entry_t_ut[ _( "previous_new_commands" ) ] = &PlayerEntry::m_iLastNewCmds;
	player_entry_t_ut[ _( "last_recieved_tick" ) ] = &PlayerEntry::m_iLastRecievedTick;
	player_entry_t_ut[ _( "missed_shots" ) ] = &PlayerEntry::m_iMissedShots;

	auto animation_data_t_ut{ state->m_cState.new_usertype<AnimData_t>( _( "animation_data_t" ), sol::no_constructor ) };
	//animation_data_t_ut[ sol::meta_function::construct ] = sol::constructors<AnimData_t( )>( );
	animation_data_t_ut[ _( "velocity" ) ] = &AnimData_t::m_vecVelocity;
	animation_data_t_ut[ _( "origin" ) ] = &AnimData_t::m_vecOrigin;
	animation_data_t_ut[ _( "mins" ) ] = &AnimData_t::m_vecMins;
	animation_data_t_ut[ _( "maxs" ) ] = &AnimData_t::m_vecMaxs;
	animation_data_t_ut[ _( "weapon" ) ] = &AnimData_t::m_pWeapon;
	animation_data_t_ut[ _( "anim_layers" ) ] = &AnimData_t::m_pLayers;
	animation_data_t_ut[ _( "flags" ) ] = &AnimData_t::m_iFlags;
	animation_data_t_ut[ _( "simulation_time" ) ] = &AnimData_t::m_flSimulationTime;
	animation_data_t_ut[ _( "old_simulation_time" ) ] = &AnimData_t::m_flOldSimulationTime;
	animation_data_t_ut[ _( "duck_amount" ) ] = &AnimData_t::m_flDuckAmount;


	auto pathToFile = "C:\\Users\\Admin\\Documents\\Havoc\\Scripts\\test.lua";

	auto result = state->m_cState.load_file( pathToFile );

	// oo, no bueno :/
	if ( !result.valid( ) ) {
		sol::error err = result;
		Features::Logger.Log( err.what( ), true );
	}
	else {
		auto call_result = result.call( );

		if ( !call_result.valid( ) ) {
			sol::error err = call_result;
			Features::Logger.Log( err.what( ), true );

			m_bLoadingScript = false;
			return;
		}

		m_mapStates.insert_or_assign( "test.lua", state );
	}

	m_bLoadingScript = false;
}