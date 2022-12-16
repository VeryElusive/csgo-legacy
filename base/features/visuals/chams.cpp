#include "visuals.h"
#include "../../core/hooks.h"
#include "../animations/animation.h"
#include "../../context.h"

void CChams::InitMaterials( ) {
	RegularMat = CreateMaterial(
		_( "mb_regular.vmt" ),
		_( "VertexLitGeneric" ),
		_(
			R"#("VertexLitGeneric" {
					"$basetexture" "vgui/white_additive"
					"$ignorez"      "0"
					"$model"		"1"
					"$flat"			"0"
					"$nocull"		"1"
					"$halflambert"	"1"
					"$nofog"		"1"
					"$wireframe"	"0"
				})#"
		)
	);

	FlatMat = CreateMaterial(
		_( "vuln_solid.vmt" ),
		_( "UnlitGeneric" ),
		_(
			R"#("UnlitGeneric" {
					"$basetexture" "vgui/white_additive"
					"$ignorez"      "0"
					"$model"		"1"
					"$flat"			"1"
					"$nocull"		"1"
					"$selfillum"	"1"
					"$halflambert"	"1"
					"$nofog"		"1"
					"$wireframe"	"0"
				})#"
		)
	);

	GlowMat = Interfaces::MaterialSystem->FindMaterial( _( "dev/glow_armsrace" ), nullptr );

	MetallicMat = CreateMaterial(
		_( "mb_metallic.vmt" ),
		_( "VertexLitGeneric" ),
		_(
			R"#("VertexLitGeneric" {
					 "$basetexture"					"vgui/white_additive"
					 "$ignorez"						"0"
					 "$envmap"						"env_cubemap"
					 "$normalmapalphaenvmapmask"	"1"
					 "$envmapcontrast"				"1"
					 "$nofog"						"1"
					 "$model"						"1"
					 "$nocull"						"0"
					 "$selfillum"					"1"
					 "$halflambert"					"1"
					 "$znearer"						"0"
					 "$flat"						"1" 
					 "$flat"						"1" 
				})#"
		)
	);

	GalaxyMat = CreateMaterial(
		_( "mb_star.vmt" ),
		_( "VertexLitGeneric" ),
		_( R"#("VertexLitGeneric" {
			"$basetexture" "dev\snowfield"
			"$additive" "1"

			"Proxies"
			{
				"TextureScroll"
				{
					"textureScrollVar" "$baseTextureTransform"
					"textureScrollRate" "0.05"
					"textureScrollAngle" "0.0"
				}
			}
		}
		)#" )
	);

	if ( !RegularMat || RegularMat->IsErrorMaterial( ) )
		return;

	if ( !FlatMat || FlatMat->IsErrorMaterial( ) )
		return;

	if ( !GlowMat || GlowMat->IsErrorMaterial( ) )
		return;

	if ( !MetallicMat || MetallicMat->IsErrorMaterial( ) )
		return;

	if ( !GalaxyMat || GalaxyMat->IsErrorMaterial( ) )
		return;

	init = true;
}

IMaterial* CChams::CreateMaterial( 
	const std::string_view name, const std::string_view shader, const std::string_view material 
) const {
	//CKeyValues* pKeyValues = new CKeyValues( shader.data( ) );
	//pKeyValues->LoadFromBuffer( name.data( ), material.data( ) );

	//return Interfaces::MaterialSystem->CreateMaterial( name.data( ), pKeyValues );
	return nullptr;
}

void CChams::OverrideMaterial(
	const int type, const bool ignore_z,
	const float r, const float g, const float b, const float a, const int glow, const bool wireframe
) const {
	IMaterial* material{ };

	switch ( type ) {
		case 0: material = RegularMat; break;
		case 1: material = FlatMat; break;
		case 2: material = GlowMat; break;
		case 3: material = MetallicMat; break;
		case 4: material = GalaxyMat; break;
		default: break;
	}
	material->AlphaModulate( a / 255.f );
	material->ColorModulate( r / 255.f, g / 255.f, b / 255.f );
	material->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, ignore_z );
	material->SetMaterialVarFlag( MATERIAL_VAR_WIREFRAME, wireframe );

	if ( type > 1 ) {
		if ( const auto $envmaptint = material->FindVar( _( "$envmaptint" ), nullptr, false ) )
			$envmaptint->SetVector( r / 255, g / 255, b / 255 );
	}

	if ( type == 2 ) {
		if ( const auto envmap = material->FindVar( _( "$envmapfresnelminmaxexp" ), nullptr ) )
			envmap->SetVector( 0.f, 1.f, std::clamp<float>( ( 100.0f - glow ) * 0.2f, 1.f, 20.f ) );
	}

	Interfaces::StudioRender->ForcedMaterialOverride( material );
}

void CChams::Main( DrawModelResults_t* pResults, const DrawModelInfo_t& info, matrix3x4_t* pBoneToWorld, float* flFlexWeights, float* flFlexDelayedWeights, const Vector& vecModelOrigin, int nFlags ) {
	static auto oDrawModel = DTR::DrawModel.GetOriginal<decltype( &Hooks::hkDrawModel )>( );

	if ( !init )
		InitMaterials( );

	IClientRenderable* pRenderable{ info.pClientEntity };
	if ( !pRenderable )
		return oDrawModel( Interfaces::StudioRender, 0, pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );

	CBasePlayer* pPlayer{ static_cast< CBasePlayer* >( pRenderable->GetIClientUnknown( )->GetBaseEntity( ) ) };
	if ( !pPlayer )
		return oDrawModel( Interfaces::StudioRender, 0, pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );

	// check for players
	if ( pPlayer->IsPlayer( ) && !pPlayer->IsDead( ) ) {
		// skip glow models
		if ( nFlags & ( STUDIO_RENDER | STUDIO_SKIP_FLEXES | STUDIO_DONOTMODIFYSTENCILSTATE | STUDIO_NOLIGHTING_OR_CUBEMAP | STUDIO_SKIP_DECALS ) )
			return oDrawModel( Interfaces::StudioRender, 0, pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );

		int type{ ENEMY };
		if ( pPlayer == ctx.m_pLocal )
			type = LOCAL;
		else if ( pPlayer->IsTeammate( ) )
			type = TEAM;

		int Mat{ };
		GetPlayerIntFig( type, ChamMat, Mat );

		int GlowStrength{ };
		GetPlayerIntFig( type, ChamGlowStrength, GlowStrength );

		// this order is important 2 me
		if ( type == ENEMY && Config::Get<bool>( Vars.ChamBacktrack ) ) {
			Color Col{ Config::Get<Color>( Vars.ChamBacktrackCol ) };

			const auto& records = Features::AnimSys.m_arrEntries.at( pPlayer->Index( ) - 1 ).m_pRecords;
			matrix3x4_t matrix[ 256 ];
			bool valid{ };
			if ( !records.empty( ) ) {
				for ( auto it = records.rbegin( ); it != records.rend( ); it = std::next( it ) ) {
					const auto& record{ *it };
					if ( !record )
						continue;

					if ( record->m_bBrokeLC )
						break;

					if ( !record->IsValid( )
						|| !record->m_bAnimated )
						continue;

					if ( ( record->m_cAnimData.m_vecOrigin - pPlayer->GetAbsOrigin( ) ).Length( ) < 1.f )
						continue;

					std::memcpy(
						matrix, record->m_cAnimData.m_pMatrix,
						record->m_iBonesCount * sizeof( matrix3x4_t )
					);

					valid = true;
				}
			}

			if ( valid ) {
				OverrideMaterial(
					Config::Get<int>( Vars.ChamBacktrackMat ), true,
					Col.Get<COLOR_R>( ),
					Col.Get<COLOR_G>( ),
					Col.Get<COLOR_B>( ),
					Col.Get<COLOR_A>( ),
					GlowStrength,
					false
				);

				oDrawModel( Interfaces::StudioRender, 0, pResults, info, matrix, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );
				Interfaces::StudioRender->ForcedMaterialOverride( nullptr );
			}
		}

		if ( type == LOCAL && Config::Get<bool>( Vars.ChamDesync ) ) {
			Color Col{ Config::Get<Color>( Vars.ChamDesyncCol ) };

			int Mat{ Config::Get<int>( Vars.ChamDesyncMat ) };

			OverrideMaterial(
				Mat, false,
				Col.Get<COLOR_R>( ),
				Col.Get<COLOR_G>( ),
				Col.Get<COLOR_B>( ),
				Col.Get<COLOR_A>( ),
				GlowStrength,
				false
			);

			oDrawModel( Interfaces::StudioRender, 0, pResults, info, ctx.m_cFakeData.m_matMatrix, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );

			Interfaces::StudioRender->ForcedMaterialOverride( nullptr );
		}

		CheckIfPlayer( ChamHid, type ) {
			Color Col{ };
			GetPlayerColorFig( type, ChamHidCol, Col );

			OverrideMaterial(
				Mat, true,
				Col.Get<COLOR_R>( ),
				Col.Get<COLOR_G>( ),
				Col.Get<COLOR_B>( ),
				Col.Get<COLOR_A>( ),
				GlowStrength,
				false
			);

			oDrawModel( Interfaces::StudioRender, 0, pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );

			Interfaces::StudioRender->ForcedMaterialOverride( nullptr );
			//ret = false;
		}

		CheckIfPlayer( ChamVis, type ) {
			Color Col{ };
			GetPlayerColorFig( type, ChamVisCol, Col );

			OverrideMaterial(
				Mat, false,
				Col.Get<COLOR_R>( ),
				Col.Get<COLOR_G>( ),
				Col.Get<COLOR_B>( ),
				Col.Get<COLOR_A>( ),
				GlowStrength,
				false
			);

			oDrawModel( Interfaces::StudioRender, 0, pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );

			Interfaces::StudioRender->ForcedMaterialOverride( nullptr );
		}
		// else we render regular model
		else
			oDrawModel( Interfaces::StudioRender, 0, pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );

		CheckIfPlayer( ChamDouble, type ) {
			Color Col{ };
			GetPlayerColorFig( type, ChamDoubleCol, Col );

			int Mat{ };
			GetPlayerIntFig( type, ChamDoubleMat, Mat );

			OverrideMaterial(
				Mat, false,
				Col.Get<COLOR_R>( ),
				Col.Get<COLOR_G>( ),
				Col.Get<COLOR_B>( ),
				Col.Get<COLOR_A>( ),
				GlowStrength,
				false
			);

			oDrawModel( Interfaces::StudioRender, 0, pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );

			Interfaces::StudioRender->ForcedMaterialOverride( nullptr );
		}

		return;
	}

	oDrawModel( Interfaces::StudioRender, 0, pResults, info, pBoneToWorld, flFlexWeights, flFlexDelayedWeights, vecModelOrigin, nFlags );
}

void CChams::OnPostScreenEffects( ) {
	if ( !ctx.m_pLocal ) {
		if ( !m_Hitmatrix.empty( ) )
			m_Hitmatrix.clear( );
		return;
	}

	if ( m_Hitmatrix.empty( ) )
		return;

	const auto ctx{ Interfaces::MaterialSystem->GetRenderContext( ) };
	if ( !ctx )
		return;

	m_bFakeModel = true;

	for ( auto i = m_Hitmatrix.begin( ); i != m_Hitmatrix.end( ); ) {
		const auto delta{ i->time - Interfaces::Globals->flRealTime };
		if ( delta <= 0 ) {
			i = m_Hitmatrix.erase( i );
			continue;
		}

		auto alpha{ 1.f };

		if ( delta < 0.5f )
			alpha = delta * 2;

		auto color{ Config::Get<Color>( Vars.MiscHitMatrixCol ) };
		color = color.Set<COLOR_A>( color.Get<COLOR_A>( ) * alpha );

		int Mat{ Config::Get<int>( Vars.MiscHitMatrixMat ) };

		OverrideMaterial(
			Mat, true,
			color.Get<COLOR_R>( ),
			color.Get<COLOR_G>( ),
			color.Get<COLOR_B>( ),
			color.Get<COLOR_A>( ),
			Config::Get<int>( Vars.ChamGlowStrengthEnemy ),
			false
		);

		Interfaces::ModelRender->DrawModelExecute( ctx, i->state, i->info, i->pBoneToWorld->Base( ) );
		Interfaces::StudioRender->ForcedMaterialOverride( nullptr );

		i = std::next( i );
	}
	m_bFakeModel = false;
}

void CChams::AddHitmatrix( CBasePlayer* player, matrix3x4_t* bones ) {
	if ( !player || !bones )
		return;

	const auto mdlData = player->m_pStudioHdr( );
	if ( !mdlData
		|| !mdlData->pStudioHdr )
		return;

	auto& hit = m_Hitmatrix.emplace_back( );

	const auto model{ player->GetModel( ) };

	hit.time = Interfaces::Globals->flRealTime + 3.f;
	hit.state.pStudioHdr = mdlData->pStudioHdr;
	hit.state.pStudioHWData = Interfaces::MDLCache->GetHardwareData( model->hStudio );
	hit.state.pRenderable = player->GetClientRenderable( );

	hit.info.pRenderable = player->GetClientRenderable( );
	hit.info.pModel = model;
	hit.info.iHitboxSet = player->m_nHitboxSet( );
	hit.info.iSkin = player->GetSkin( );
	hit.info.iBody = player->GetBody( );
	hit.info.nEntityIndex = player->Index( );
	hit.info.vecOrigin = player->m_vecOrigin( );
	hit.info.angAngles.y = player->GetAbsAngles( ).y;

	hit.info.hInstance = player->GetModelInstance( );
	hit.info.iFlags = 1;

	std::memcpy( hit.pBoneToWorld, bones, player->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	Math::AngleMatrix( hit.info.angAngles, hit.model_to_world );

	hit.model_to_world[ 0 ][ 3 ] = hit.info.vecOrigin.x;
	hit.model_to_world[ 1 ][ 3 ] = hit.info.vecOrigin.y;
	hit.model_to_world[ 2 ][ 3 ] = hit.info.vecOrigin.z;

	hit.info.pModelToWorld = hit.state.pModelToWorld = &hit.model_to_world;
}