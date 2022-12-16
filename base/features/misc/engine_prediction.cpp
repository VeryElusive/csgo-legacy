#include "engine_prediction.h"

void CEnginePrediction::PreStart( ) {
	if ( !ctx.m_pWeapon )
		return;

	if ( ctx.m_iLastFSNStage == FRAME_NET_UPDATE_END ) {
		ctx.m_bDontSavePredVars = true;

		Interfaces::Prediction->Update( Interfaces::ClientState->iDeltaTick, Interfaces::ClientState->iDeltaTick > 0, Interfaces::ClientState->iLastCommandAck,
			Interfaces::ClientState->iLastOutgoingCommand + Interfaces::ClientState->nChokedCommands );

		ctx.m_bDontSavePredVars = false;
	}

	CurTime = Interfaces::Globals->flCurTime;
	FrameTime = Interfaces::Globals->flFrameTime;

	InPrediction = Interfaces::Prediction->bInPrediction;
	FirstTimePrediction = Interfaces::Prediction->Split->bIsFirstTimePredicted;

	Interfaces::Globals->flCurTime = TICKS_TO_TIME( ctx.m_pLocal->m_nTickBase( ) );
	Interfaces::Globals->flFrameTime = Interfaces::Prediction->bEnginePaused ? 0.f : Interfaces::Globals->flIntervalPerTick;
}

void CEnginePrediction::RunCommand( CUserCmd& cmd ) {
	if ( !ctx.m_pWeapon )
		return;

	AccuracyPenalty = ctx.m_pWeapon->m_fAccuracyPenalty( );
	RecoilIndex = ctx.m_pWeapon->m_flRecoilIndex( );

	const auto backupVelocityModifier = ctx.m_pLocal->m_flVelocityModifier( );

	// we don't need to set randomseed because CInput::CreateMove already did it for us!

	ctx.m_pLocal->CurrentCommand( ) = &cmd;
	*( *reinterpret_cast< unsigned int** >( Offsets::Sigs.uPredictionRandomSeed ) ) = cmd.iRandomSeed;
	*( *reinterpret_cast< CBasePlayer*** >( Offsets::Sigs.pPredictionPlayer ) ) = ctx.m_pLocal;

	Interfaces::MoveHelper->SetHost( ctx.m_pLocal );

	Interfaces::Prediction->bInPrediction = true;
	Interfaces::Prediction->Split->bIsFirstTimePredicted = false;

	Interfaces::GameMovement->StartTrackPredictionErrors( ctx.m_pLocal );

	std::memset( &MoveData, 0, sizeof( CMoveData ) );

	Interfaces::Prediction->SetupMove( ctx.m_pLocal, &cmd, Interfaces::MoveHelper, &MoveData );
	Interfaces::GameMovement->ProcessMovement( ctx.m_pLocal, &MoveData );
	Interfaces::Prediction->FinishMove( ctx.m_pLocal, &cmd, &MoveData );
	Interfaces::GameMovement->FinishTrackPredictionErrors( ctx.m_pLocal );
	Interfaces::MoveHelper->SetHost( nullptr );

	ctx.m_pWeapon->UpdateAccuracyPenalty( );

	Spread = ctx.m_pWeapon->GetSpread( );
	Inaccuracy = ctx.m_pWeapon->GetInaccuracy( );

	ctx.m_pLocal->m_flVelocityModifier( ) = backupVelocityModifier;

	Interfaces::Prediction->bInPrediction = InPrediction;
	Interfaces::Prediction->Split->bIsFirstTimePredicted = FirstTimePrediction;
}

void CEnginePrediction::Finish( ) {
	if ( !ctx.m_pWeapon )
		return;

	ctx.m_pLocal->CurrentCommand( ) = nullptr;
	*( *reinterpret_cast< unsigned int** >( Offsets::Sigs.uPredictionRandomSeed ) ) = -1;
	*( *reinterpret_cast< CBasePlayer*** >( Offsets::Sigs.pPredictionPlayer ) ) = nullptr;

	ctx.m_pWeapon->m_flRecoilIndex( ) = RecoilIndex;
	ctx.m_pWeapon->m_fAccuracyPenalty( ) = AccuracyPenalty;

	Interfaces::Globals->flCurTime = CurTime;
	Interfaces::Globals->flFrameTime = FrameTime;

	/*if ( m_pOldWeapon == ctx.m_pWeapon )
		return;

	const auto datamap{ ctx.m_pWeapon->GetPredDescMap( ) };

	bool changed{ };

	const auto m_iIronSightMode{ get_typedescription( datamap, _( "m_iIronSightMode" ) ) };
	if ( m_iIronSightMode->fFlags != ( 0x0200 | 0x0080 | 0x0400 ) ) {
		m_iIronSightMode->fFlags = 0x0200 | 0x0080 | 0x0400;
		changed = true;
	}	
	
	if ( changed )
		datamap->pOptimizedDataMap = nullptr;

	m_pOldWeapon = ctx.m_pWeapon;*/
}

void CEnginePrediction::RestoreNetvars( int slot ) {
	const auto local = ctx.m_pLocal;
	if ( !local || local->IsDead( ) )
		return;

	const auto& data = m_cCompressionVars.at( slot % 150 );
	if ( data.m_iCommandNumber != slot )
		return;

	const auto m_aimPunchAngleVelDiff = local->m_aimPunchAngleVel( ) - data.m_aimPunchAngleVel;
	const auto m_aimPunchAngleDiff = local->m_aimPunchAngle( ) - data.m_aimPunchAngle;
	const auto m_vecViewOffsetDiff = local->m_vecViewOffset( ).z - data.m_vecViewOffsetZ;

	if ( std::abs( m_vecViewOffsetDiff ) <= 0.03125f )
		local->m_vecViewOffset( ).z = data.m_vecViewOffsetZ;

	if ( std::abs( m_aimPunchAngleDiff.x ) <= 0.03125f && std::abs( m_aimPunchAngleDiff.y ) <= 0.03125f && std::abs( m_aimPunchAngleDiff.z ) <= 0.03125f )
		local->m_aimPunchAngle( ) = data.m_aimPunchAngle;

	if ( std::abs( m_aimPunchAngleVelDiff.x ) <= 0.03125f && std::abs( m_aimPunchAngleVelDiff.y ) <= 0.03125f && std::abs( m_aimPunchAngleVelDiff.z ) <= 0.03125f )
		local->m_aimPunchAngleVel( ) = data.m_aimPunchAngleVel;

	auto& viewOffset{ local->m_vecViewOffset( ) };
	if ( viewOffset.z > 46.f && viewOffset.z < 46.045f )
		viewOffset.z = 46.f;
	else if ( viewOffset.z > 64.f )
		viewOffset.z = 64.f;
}

void CEnginePrediction::StoreNetvars( int slot ) {
	const auto local = ctx.m_pLocal;
	if ( !local || local->IsDead( ) )
		return;

	auto& data = m_cCompressionVars.at( slot % 150 );

	data.m_aimPunchAngleVel = local->m_aimPunchAngleVel( );
	data.m_aimPunchAngle = local->m_aimPunchAngle( );
	data.m_vecViewOffsetZ = local->m_vecViewOffset( ).z;
}

TypeDescription_t custom_datamap[ 1009 ]{ };
bool CEnginePrediction::AddToDataMap( ) {
	/*if ( ctx.m_pLocal && !ctx.m_pLocal->IsDead( ) ) {
		auto datamap{ ctx.m_pLocal->GetPredDescMap( ) };
		if ( datamap ) {
			// copy original datamap
			memcpy( custom_datamap, datamap->pDataDesc, 996 );

			// extend datamap
			datamap->pDataDesc = custom_datamap;
			datamap->nDataFields = 12;
			datamap->iPackedSize = 1009;

			// add m_flVelocityModifier
			TypeDescription_t velocityModifier{ };
			velocityModifier.szFieldName = _( "m_flVelocityModifier" );
			velocityModifier.fFlags = 0x100;
			velocityModifier.fieldTolerance = 0.00625f;
			velocityModifier.iFieldOffset = static_cast< int >( Offsets::m_flVelocityModifier );
			velocityModifier.uFieldSize = 0x1;
			velocityModifier.fieldSizeInBytes = 0x4;
			velocityModifier.iFieldType = 0x1;
			velocityModifier.flatOffset[ TD_OFFSET_NORMAL ] = static_cast< int >( Offsets::m_flVelocityModifier );
			datamap->pDataDesc[ 10 ] = velocityModifier;

			// add m_bWaitForNoAttack
			/*TypeDescription_t waitForNoAttack{ };
			waitForNoAttack.szFieldName = _( "m_bWaitForNoAttack" );
			waitForNoAttack.fFlags = 0x100;
			waitForNoAttack.fieldTolerance = 0x0;
			waitForNoAttack.iFieldOffset = static_cast< int >( Offsets::m_bWaitForNoAttack );
			waitForNoAttack.uFieldSize = 0x1;
			waitForNoAttack.fieldSizeInBytes = 0x1;
			waitForNoAttack.iFieldType = 0x6;
			waitForNoAttack.flatOffset[ TD_OFFSET_NORMAL ] = static_cast< int >( Offsets::m_bWaitForNoAttack );
			datamap->pDataDesc[ 11 ] = waitForNoAttack;

			// add m_iMoveState
			TypeDescription_t moveState{ };
			moveState.szFieldName = _( "m_iMoveState" );
			moveState.fFlags = 0x100;
			moveState.fieldTolerance = 0x0;
			moveState.iFieldOffset = static_cast< int >( Offsets::m_iMoveState );
			moveState.uFieldSize = 0x1;
			moveState.fieldSizeInBytes = 0x4;
			moveState.iFieldType = 0x5;
			moveState.flatOffset[ TD_OFFSET_NORMAL ] = static_cast< int >( Offsets::m_iMoveState );
			datamap->pDataDesc[ 11 ] = moveState;

			// FTYPEDESC_PRIVATE | FTYPEDESC_OVERRIDE | FTYPEDESC_NOERRORCHECK
			const auto m_vphysicsCollisionState = MEM::GetTypeDescription( datamap, _( "m_vphysicsCollisionState" ) );
			m_vphysicsCollisionState->fFlags = 0x0200 | 0x0080 | 0x0400;

			datamap->pOptimizedDataMap = nullptr;

			/*if ( ctx.m_pLocal->m_hViewModel( ) ) {
				if ( const auto viewModel{ static_cast< CBaseViewModel* >( Interfaces::ClientEntityList->GetClientEntityFromHandle( ctx.m_pLocal->m_hViewModel( ) ) ) }; viewModel ) {
					datamap = viewModel->GetPredDescMap( );

					const auto m_nAnimationParity{ get_typedescription( datamap, _( "m_nAnimationParity" ) ) };
					m_nAnimationParity->fFlags = 0x0200 | 0x0080 | 0x0400;

					const auto m_nSequence{ get_typedescription( datamap, _( "m_nSequence" ) ) };
					m_nSequence->fFlags = 0x0200 | 0x0080 | 0x0400;

					datamap->pOptimizedDataMap = nullptr;

					return true;
				}
			}
		}

	}*/

	return false;
}