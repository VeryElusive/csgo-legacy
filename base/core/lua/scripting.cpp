#include "scripting.h"
#include "../../sdk/hash/fnv1a.h"
#include "wrappers/render.h"
#include "wrappers/entity.h"


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

	utilsTable[ _( "penetrate" ) ] = sol::overload( &Wrappers::Renderer::Line );
	utilsTable[ _( "trace_line" ) ] = sol::overload( &Wrappers::Renderer::Line );

	/* usertypes */
	auto color_ut{ state->m_cState.new_usertype<Color>( _( "color" ) ) };
	color_ut[ sol::meta_function::construct ] = sol::constructors<Color( ), Color( int, int, int ), Color( int, int, int, int )>( );
	color_ut[ std::string( _( "r" ) ) ] = sol::overload( &Color::r );
	color_ut[ std::string( _( "g" ) ) ] = sol::overload( &Color::g );
	color_ut[ std::string( _( "b" ) ) ] = sol::overload( &Color::b );
	color_ut[ std::string( _( "a" ) ) ] = sol::overload( &Color::a );

	auto vector_ut = state->m_cState.new_usertype<Vector>( _( "vector" ) );
	vector_ut[ sol::meta_function::construct ] = sol::constructors<Vector( float, float, float )>( );
	vector_ut[ std::string( _( "x" ) ) ] = &Vector::x;
	vector_ut[ std::string( _( "y" ) ) ] = &Vector::y;
	vector_ut[ std::string( _( "z" ) ) ] = &Vector::z;

	auto vector2d_ut = state->m_cState.new_usertype<Vector2D>( _( "vector2d" ) );
	vector2d_ut[ sol::meta_function::construct ] = sol::constructors<Vector2D( float, float )>( );
	vector2d_ut[ std::string( _( "x" ) ) ] = &Vector2D::x;
	vector2d_ut[ std::string( _( "y" ) ) ] = &Vector2D::y;

	auto qangle_ut = state->m_cState.new_usertype<QAngle>( _( "qangle" ) );
	qangle_ut[ sol::meta_function::construct ] = sol::constructors<QAngle( float, float, float )>( );
	qangle_ut[ std::string( _( "x" ) ) ] = &QAngle::x;
	qangle_ut[ std::string( _( "y" ) ) ] = &QAngle::y;
	qangle_ut[ std::string( _( "z" ) ) ] = &QAngle::z;	
	
	auto trace_t_ut{ state->m_cState.new_usertype<CGameTrace>( _( "trace_t" ) ) };
	trace_t_ut[ sol::meta_function::construct ] = sol::constructors<CGameTrace( )>( );
	trace_t_ut[ std::string( _( "end" ) ) ] = &CGameTrace::vecEnd;
	trace_t_ut[ std::string( _( "start" ) ) ] = &CGameTrace::vecStart;
	trace_t_ut[ std::string( _( "fraction" ) ) ] = &CGameTrace::flFraction;
	trace_t_ut[ std::string( _( "hitbox" ) ) ] = &CGameTrace::iHitbox;
	trace_t_ut[ std::string( _( "hitgroup" ) ) ] = &CGameTrace::iHitGroup;
	trace_t_ut[ std::string( _( "hit_entity" ) ) ] = &CGameTrace::pHitEntity;
	trace_t_ut[ std::string( _( "all_solid" ) ) ] = &CGameTrace::bAllSolid;
	trace_t_ut[ std::string( _( "start_solid" ) ) ] = &CGameTrace::bStartSolid;
	trace_t_ut[ std::string( _( "contents" ) ) ] = &CGameTrace::iContents;
	trace_t_ut[ std::string( _( "plane" ) ) ] = &CGameTrace::plane;

	auto entity_ut{ state->m_cState.new_usertype<Wrappers::Entity::CPlayer>( _( "player" ), sol::no_constructor ) };
	entity_ut[ std::string( _( "get_int" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetInt );
	entity_ut[ std::string( _( "get_bool" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetBool );
	entity_ut[ std::string( _( "get_float" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetFloat );
	entity_ut[ std::string( _( "get_vector" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetVector );
	entity_ut[ std::string( _( "set_int" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::SetInt );
	entity_ut[ std::string( _( "set_bool" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::SetBool );
	entity_ut[ std::string( _( "set_float" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::SetFloat );
	entity_ut[ std::string( _( "set_vector" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::SetVector );

	entity_ut[ std::string( _( "get_layer_sequence_activity" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::GetLayerSequenceActivity );
	entity_ut[ std::string( _( "animate" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::AnimatePlayer );
	entity_ut[ std::string( _( "setupbones" ) ) ] = sol::overload( &Wrappers::Entity::CPlayer::AnimatePlayer );


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