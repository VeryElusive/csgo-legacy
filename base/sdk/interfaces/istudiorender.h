#pragma once
// @credits: https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/istudiorender.h

#pragma region studiorender_enumerations
enum
{
	STUDIORENDER_DRAW_ENTIRE_MODEL = 0,
	STUDIORENDER_DRAW_OPAQUE_ONLY = 0x01,
	STUDIORENDER_DRAW_TRANSLUCENT_ONLY = 0x02,
	STUDIORENDER_DRAW_GROUP_MASK = 0x03,
	STUDIORENDER_DRAW_NO_FLEXES = 0x04,
	STUDIORENDER_DRAW_STATIC_LIGHTING = 0x08,
	STUDIORENDER_DRAW_ACCURATETIME = 0x10,
	STUDIORENDER_DRAW_NO_SHADOWS = 0x20,
	STUDIORENDER_DRAW_GET_PERF_STATS = 0x40,
	STUDIORENDER_DRAW_WIREFRAME = 0x80,
	STUDIORENDER_DRAW_ITEM_BLINK = 0x100,
	STUDIORENDER_SHADOWDEPTHTEXTURE = 0x200,
	STUDIORENDER_SSAODEPTHTEXTURE = 0x1000,
	STUDIORENDER_GENERATE_STATS = 0x8000,
};

enum EOverrideType : int
{
	OVERRIDE_NORMAL = 0,
	OVERRIDE_BUILD_SHADOWS,
	OVERRIDE_DEPTH_WRITE,
	OVERRIDE_SSAO_DEPTH_WRITE
};
#pragma endregion

struct MaterialLightingState_t
{
	Vector			vecAmbientCube[6];
	Vector			vecLightingOrigin;
	int				nLocalLightCount;
	LightDesc_t		localLightDesc[4];
};

class CCycleCount { public: long long m_Int64; };

class CFastTimer {
public:

private:
	CCycleCount	m_Duration;
	bool m_bRunning;		// Are we currently running?
};

struct DrawModelResults_t {
	int m_ActualTriCount;
	int m_TextureMemoryBytes;
	int m_NumHardwareBones;
	int m_NumBatches;
	int m_NumMaterials;
	int m_nLODUsed;
	int m_flLODMetric;
	CFastTimer m_RenderTime;
	char m_Materials[ 0x20 ];
};

struct ColorMeshInfo_t;
//struct StudioDecalHandle_t { int iUnused; };
using StudioDecalHandle_t = void*;
struct DrawModelInfo_t
{
	studiohdr_t*			pStudioHdr;
	studiohwdata_t*			pHardwareData;
	StudioDecalHandle_t	hDecals;
	int						iSkin;
	int						iBody;
	int						iHitboxSet;
	IClientRenderable*		pClientEntity;
	int						iLOD;
	ColorMeshInfo_t*		pColorMeshes;
	bool					bStaticLighting;
	MaterialLightingState_t	lightingState;
};

class lightpos_t {
public:
	Vector delta;  //0x0000
	float falloff; //0x000C
	float dot;     //0x0010

}; //Size=0x0014

class IStudioRender
{
public:
	void SetColorModulation(float const* arrColor)
	{
		MEM::CallVFunc<void>(this, 27, arrColor);
	}

	void SetAlphaModulation(float flAlpha)
	{
		MEM::CallVFunc<void>(this, 28, flAlpha);
	}

	void DrawModel( void* pResults, DrawModelInfo_t* pInfo, matrix3x4_t* pBoneToWorld, float* flpFlexWeights, float* flpFlexDelayedWeights, Vector& vrModelOrigin, int32_t iFlags ) {
		using DrawModelFn = void( __thiscall* )( void*, void*, DrawModelInfo_t*, const matrix3x4_t*, float*, float*, Vector&, int );
		MEM::GetVFunc< DrawModelFn >( this, 29 ) ( this, pResults, pInfo, pBoneToWorld, flpFlexWeights, flpFlexDelayedWeights, vrModelOrigin, iFlags );
	}

	void ForcedMaterialOverride(IMaterial* pMaterial, EOverrideType nOverrideType = OVERRIDE_NORMAL, int nOverrides = 0)
	{
		MEM::CallVFunc<void>(this, 33, pMaterial, nOverrideType, nOverrides);
	}
};
