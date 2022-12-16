#pragma once
#include "prop_manager.h"

struct signatures {
	uintptr_t LocalPlayer;

	uintptr_t uPredictionRandomSeed;
	uintptr_t pPredictionPlayer;

	uintptr_t InitKeyValues;
	uintptr_t DestructKeyValues;
	uintptr_t oFromString;
	uintptr_t oLoadFromBuffer;
	uintptr_t oLoadFromFile;
	uintptr_t oFindKey;
	uintptr_t oSetString;
	uintptr_t oGetString;

	uintptr_t oCreateAnimationState;
	uintptr_t oUpdateAnimationState;
	uintptr_t oResetAnimationState;

	uintptr_t uDisableRenderTargetAllocationForever;

	uintptr_t SetAbsOrigin;
	uintptr_t SetAbsAngles;
	uintptr_t InvalidatePhysicsRecursive;

	uintptr_t PostProcess;

	uintptr_t SmokeCount;
	uintptr_t TakeDamageOffset;
	uintptr_t LookupBone;

	uintptr_t SetAbsVelocity;

	uintptr_t LoadNamedSkys;
	uintptr_t CL_ReadPackets;
	uintptr_t ClanTag;

	uintptr_t ClearNotices;

	uintptr_t StartDrawing;
	uintptr_t FinishDrawing;

	uintptr_t ReturnToExtrapolate;
	uintptr_t ReturnToCl_ReadPackets;

	uintptr_t SetupVelocityReturn;

	uintptr_t uInsertIntoTree;

	uintptr_t uAllocKeyValuesEngine;
	uintptr_t uAllocKeyValuesClient;

	uintptr_t uCAM_ThinkReturn;

	uintptr_t InvalidateBoneCache;

	uintptr_t SetCollisionBounds;

	uintptr_t m_pStudioHdr;

	uintptr_t WriteUsercmd;

	uintptr_t AddBoxOverlayReturn;

	uintptr_t CL_SendMove;

	uintptr_t GetSequenceActivity;

	//uintptr_t SetupBones_AttachmentHelper;

	uintptr_t ClampBonesInBBox;
	uintptr_t C_BaseAnimating__BuildTransformations;

	uintptr_t CL_FireEvents;
	uintptr_t NET_ProcessSocket;

	uintptr_t TraceFilterSkipTwoEntities;
	uintptr_t numticks;

	uintptr_t ReturnToPerformPrediction;
	uintptr_t ReturnToInterpolateServerEntities;

	uintptr_t current_tickcount;
	uintptr_t host_currentframetick;


	uintptr_t IK_Context_Construct;
	uintptr_t IK_Context_Init;
	uintptr_t IK_Context_UpdateTargets;
	uintptr_t IK_Context_SolveDependencies;
	uintptr_t IK_Context_AddDependencies;
	uintptr_t IK_Context_CopyTo;


	uintptr_t BoneMergeCache_Delete;
	uintptr_t BoneMergeCache_Construct;
	uintptr_t BoneMergeCache_Init;
	uintptr_t BoneMergeCache_MergeMatchingPoseParams;
	uintptr_t BoneMergeCache_CopyFromFollow;
	uintptr_t BoneMergeCache_CopyToFollow;

	uintptr_t BoneSetup_AccumulatePose;
	uintptr_t BoneSetup_CalcAutoplaySequences;
	uintptr_t BoneSetup_CalcBoneAdj;
};

#include "../sdk/convar.h"

struct cvars {
	CConVar* mp_teammates_are_enemies;
	CConVar* cl_foot_contact_shadows;
	CConVar* weapon_recoil_scale;
	CConVar* view_recoil_tracking;
	CConVar* ff_damage_reduction_bullets;
	CConVar* ff_damage_bullet_penetration;
	CConVar* r_drawspecificstaticprop;
	CConVar* cl_updaterate;
	CConVar* r_modelAmbientMin;
	CConVar* r_jiggle_bones;
	CConVar* sv_maxunlag;
	CConVar* cl_interp;
	CConVar* cl_interp_ratio;
	CConVar* sv_clockcorrection_msecs;
	CConVar* sv_accelerate_use_weapon_speed;
	CConVar* sv_accelerate;
	CConVar* mat_ambient_light_r;
	CConVar* mat_ambient_light_g;
	CConVar* mat_ambient_light_b;
	CConVar* sv_gravity;
	CConVar* sv_stopspeed;
	CConVar* sv_maxvelocity;
	CConVar* weapon_molotov_maxdetonateslope;
	CConVar* molotov_throw_detonate_time;
	CConVar* sv_showimpacts;
	CConVar* cl_clock_correction;
	CConVar* r_drawmodelstatsoverlay;
	CConVar* cl_mouseenable;
	CConVar* weapon_debug_spread_show;
	CConVar* cl_csm_shadows;
	CConVar* sv_friction;
	CConVar* cl_ignorepackets;
	CConVar* sv_enablebunnyhopping;
	CConVar* sv_jump_impulse;
};

namespace Offsets {
	void Init( );

	std::uintptr_t FindInDataMap( DataMap_t* pMap, const char* name );


	inline int m_iClip1;
	inline int m_flNextPrimaryAttack;
	inline int m_flNextSecondaryAttack;
	inline int m_hWeaponWorldModel;
	inline int m_iItemDefinitionIndex;
	inline int m_hActiveWeapon;
	inline int m_hMyWeapons;
	inline int m_angEyeAngles;
	inline int m_vecMins;
	inline int m_vecMaxs;
	inline int m_vecOrigin;
	inline int m_iTeamNum;
	inline int m_CollisionGroup;
	inline int m_nRenderMode;
	inline int m_flSimulationTime;
	inline int m_lifeState;
	inline int m_iHealth;
	inline int m_fFlags;
	inline int m_aimPunchAngle;
	inline int m_viewPunchAngle;
	inline int pl;
	inline int m_hOwnerEntity;
	inline int m_nTickBase;
	inline int m_hViewEntity;
	inline int m_hViewModel;
	inline int m_vphysicsCollisionState;
	inline int m_aimPunchAngleVel;
	inline int m_vecVelocity;
	inline int m_flFlashDuration;
	inline int m_bIsScoped;
	inline int m_ArmorValue;
	inline int m_zoomLevel;
	inline int m_iBurstShotsRemaining;
	inline int m_fNextBurstShot;
	inline int m_fAccuracyPenalty;
	inline int m_flRecoilIndex;
	inline int m_flPostponeFireReadyTime;
	inline int m_bIsBroken;
	inline int m_nSequence;
	inline int m_nHitboxSet;
	inline int m_bClientSideAnimation;
	inline int m_flCycle;
	inline int m_flEncodedController;
	inline int m_bHasHelmet;
	inline int m_bHasHeavyArmor;
	inline int m_vecViewOffset;
	inline int m_flVelocityModifier;
	inline int m_bWaitForNoAttack;
	inline int m_iPlayerState;
	inline int m_bIsDefusing;
	inline int m_flNextAttack;
	inline int m_iMoveState;
	inline int m_iAddonBits;
	inline int m_flPoseParameter;
	inline int m_flDuckAmount;
	inline int m_flLowerBodyYawTarget;
	inline int m_bGunGameImmunity;
	inline int m_flThirdpersonRecoil;
	inline int m_bIsWalking;
	inline int deadflag;
	inline int m_flDuckSpeed;
	inline int m_flMaxSpeed;
	inline int m_hGroundEntity;
	inline int m_vecVelocityGRENADE;
	inline int m_nSmokeEffectTickBegin;
	inline int m_nExplodeEffectTickBegin;
	inline int m_bUseCustomBloomScale;
	inline int m_bUseCustomAutoExposureMax;
	inline int m_bUseCustomAutoExposureMin;
	inline int m_flCustomAutoExposureMin;
	inline int m_flCustomAutoExposureMax;
	inline int m_flCustomBloomScale;
	inline int m_iPing;
	inline int m_iPlayerC4;
	inline int m_fThrowTime;
	inline int m_flThrowStrength;
	inline int m_bPinPulled;
	inline int m_fog_enable;
	inline int m_bStrafing;
	inline int m_hWeapon;
	inline int m_nAnimationParity;
	inline int m_nModelIndex;

	inline signatures Sigs;
	inline cvars Cvars;
}