#pragma once

#define METERS_PER_INCH					(0.0254f)
#define CUBIC_METERS_PER_CUBIC_INCH		(METERS_PER_INCH*METERS_PER_INCH*METERS_PER_INCH)
// 2.2 lbs / kg
#define POUNDS_PER_KG	(2.2f)
#define KG_PER_POUND	(1.0f/POUNDS_PER_KG)

// convert from pounds to kg
#define lbs2kg(x)		((x)*KG_PER_POUND)
#define kg2lbs(x)		((x)*POUNDS_PER_KG)

const float VPHYSICS_MIN_MASS = 0.1f;
const float VPHYSICS_MAX_MASS = 5e4f;

class IPhysicsObject;
class IPhysicsEnvironment;
class IPhysicsSurfaceProps;
class IPhysicsConstraint;
class IPhysicsConstraintGroup;
class IPhysicsFluidController;
class IPhysicsSpring;
class IPhysicsVehicleController;
class IConvexInfo;
class IPhysicsObjectPairHash;
class IPhysicsCollisionSet;
class IPhysicsPlayerController;
class IPhysicsFrictionSnapshot;

struct CCSUsrMsg_ReportHit {
	char pad[ 0x8 ];
	float pos_x = 1;
	float pos_y = 2;
	float timestamp = 4;
	float pos_z = 3;
};

class IVPhysicsDebugOverlay
{
public:
	virtual void AddEntityTextOverlay( int ent_index, int line_offset, float duration, int r, int g, int b, int a, const char* format, ... ) = 0;
	virtual void AddBoxOverlay( const Vector& origin, const Vector& mins, const Vector& max, QAngle const& orientation, int r, int g, int b, int a, float duration ) = 0;
	virtual void AddTriangleOverlay( const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest, float duration ) = 0;
	virtual void AddLineOverlay( const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration ) = 0;
	virtual void AddTextOverlay( const Vector& origin, float duration, const char* format, ... ) = 0;
	virtual void AddTextOverlay( const Vector& origin, int line_offset, float duration, const char* format, ... ) = 0;
	virtual void AddScreenTextOverlay( float flXPos, float flYPos, float flDuration, int r, int g, int b, int a, const char* text ) = 0;
	virtual void AddSweptBoxOverlay( const Vector& start, const Vector& end, const Vector& mins, const Vector& max, const QAngle& angles, int r, int g, int b, int a, float flDuration ) = 0;
	virtual void AddTextOverlayRGB( const Vector& origin, int line_offset, float duration, float r, float g, float b, float alpha, const char* format, ... ) = 0;
};

class IPhysics : public IAppSystem
{
public:
	virtual	IPhysicsEnvironment* CreateEnvironment( void ) = 0;
	virtual void DestroyEnvironment( IPhysicsEnvironment* ) = 0;
	virtual IPhysicsEnvironment* GetActiveEnvironmentByIndex( int index ) = 0;

	// Creates a fast hash of pairs of objects
	// Useful for maintaining a table of object relationships like pairs that do not collide.
	virtual IPhysicsObjectPairHash* CreateObjectPairHash( ) = 0;
	virtual void						DestroyObjectPairHash( IPhysicsObjectPairHash* pHash ) = 0;

	// holds a cache of these by id.  So you can get by id to search for the previously created set
	// UNDONE: Sets are currently limited to 32 elements.  More elements will return NULL in create.
	// NOTE: id is not allowed to be zero.
	virtual IPhysicsCollisionSet* FindOrCreateCollisionSet( unsigned int id, int maxElementCount ) = 0;
	virtual IPhysicsCollisionSet* FindCollisionSet( unsigned int id ) = 0;
	virtual void						DestroyAllCollisionSets( ) = 0;
};

// CPhysConvex is a single convex solid
class CPhysConvex;
// CPhysPolysoup is an abstract triangle soup mesh
class CPhysPolysoup;
class ICollisionQuery;
struct convertconvexparams_t;
class CPackedPhysicsDescription;
class CPolyhedron;
struct virtualmeshparams_t;
struct objectparams_t
{
	Vector* massCenterOverride;
	float		mass;
	float		inertia;
	float		damping;
	float		rotdamping;
	float		rotInertiaLimit;
	const char* pName;				// used only for debugging
	void* pGameData;
	float		volume;
	float		dragCoefficient;
	bool		enableCollisions;
};
struct fluidparams_t
{
	Vector4D	surfacePlane;	// x,y,z normal, dist (plane constant) fluid surface
	Vector		currentVelocity; // velocity of the current in inches/second
	float		damping;		// damping factor for buoyancy (tweak)
	float		torqueFactor;
	float		viscosityFactor;
	void* pGameData;
	bool		useAerodynamics;// true if this controller should calculate surface pressure
	int			contents;

	fluidparams_t( ) {}
	fluidparams_t( fluidparams_t const& src )
	{
		surfacePlane = src.surfacePlane;
		currentVelocity = src.currentVelocity;
		damping = src.damping;
		torqueFactor = src.torqueFactor;
		viscosityFactor = src.viscosityFactor;
		contents = src.contents;
	}
};

struct solid_t
{
	int		index;
	char	name[ 512 ];
	char	parent[ 512 ];
	char	surfaceprop[ 512 ];
	Vector	massCenterOverride;
	objectparams_t params;
};

struct fluid_t
{
	int index;
	char surfaceprop[ 512 ];

	fluidparams_t params;

	fluid_t( ) {}
	fluid_t( fluid_t const& src ) : params( src.params )
	{
		index = src.index;
	}
};

//-----------------------------------------------------------------------------
// Purpose: Pass this into the parser to handle the keys that vphysics does not
// parse.
//-----------------------------------------------------------------------------
class IVPhysicsKeyHandler
{
public:
	virtual void ParseKeyValue( void* pData, const char* pKey, const char* pValue ) = 0;
	virtual void SetDefaults( void* pData ) = 0;
};


class IVPhysicsKeyParser
{
public:
	virtual ~IVPhysicsKeyParser( ) {}

	virtual const char* GetCurrentBlockName( void ) = 0;
	virtual bool		Finished( void ) = 0;
	virtual void		ParseSolid( solid_t* pSolid, IVPhysicsKeyHandler* unknownKeyHandler ) = 0;
	virtual void		ParseFluid( fluid_t* pFluid, IVPhysicsKeyHandler* unknownKeyHandler ) = 0;
	virtual void		ParseRagdollConstraint( void* pConstraint, IVPhysicsKeyHandler* unknownKeyHandler ) = 0;
	virtual void		ParseSurfaceTable( int* table, IVPhysicsKeyHandler* unknownKeyHandler ) = 0;
	virtual void		ParseCustom( void* pCustom, IVPhysicsKeyHandler* unknownKeyHandler ) = 0;
	virtual void		ParseVehicle( void* pVehicle, IVPhysicsKeyHandler* unknownKeyHandler ) = 0;
	virtual void		SkipBlock( void ) = 0;
};

// UNDONE: Find a better place for this?  Should be in collisionutils, but it's needs VPHYSICS' solver.
struct truncatedcone_t
{
	Vector	origin;
	Vector	normal;
	float	h;			// height of the cone (hl units)
	float	theta;		// cone angle (degrees)
};

class IPhysicsCollision
{
public:
	virtual ~IPhysicsCollision( void ) {}

	// produce a convex element from verts (convex hull around verts)
	virtual CPhysConvex* ConvexFromVerts( Vector** pVerts, int vertCount ) = 0;
	// produce a convex element from planes (csg of planes)
	virtual CPhysConvex* ConvexFromPlanes( float* pPlanes, int planeCount, float mergeDistance ) = 0;
	// calculate volume of a convex element
	virtual float			ConvexVolume( CPhysConvex* pConvex ) = 0;

	virtual float			ConvexSurfaceArea( CPhysConvex* pConvex ) = 0;
	// store game-specific data in a convex solid
	virtual void			SetConvexGameData( CPhysConvex* pConvex, unsigned int gameData ) = 0;
	// If not converted, free the convex elements with this call
	virtual void			ConvexFree( CPhysConvex* pConvex ) = 0;
	virtual CPhysConvex* BBoxToConvex( const Vector& mins, const Vector& maxs ) = 0;
	// produce a convex element from a convex polyhedron
	virtual CPhysConvex* ConvexFromConvexPolyhedron( const CPolyhedron& ConvexPolyhedron ) = 0;
	// produce a set of convex triangles from a convex polygon, normal is assumed to be on the side with forward point ordering, which should be clockwise, output will need to be able to hold exactly (iPointCount-2) convexes
	virtual void			ConvexesFromConvexPolygon( const Vector& vPolyNormal, const Vector* pPoints, int iPointCount, CPhysConvex** pOutput ) = 0;

	// concave objects
	// create a triangle soup
	virtual CPhysPolysoup* PolysoupCreate( void ) = 0;
	// destroy the container and memory
	virtual void			PolysoupDestroy( CPhysPolysoup* pSoup ) = 0;
	// add a triangle to the soup
	virtual void			PolysoupAddTriangle( CPhysPolysoup* pSoup, const Vector& a, const Vector& b, const Vector& c, int materialIndex7bits ) = 0;
	// convert the convex into a compiled collision model
	virtual CPhysCollide* ConvertPolysoupToCollide( CPhysPolysoup* pSoup, bool useMOPP ) = 0;

	// Convert an array of convex elements to a compiled collision model (this deletes the convex elements)
	virtual CPhysCollide* ConvertConvexToCollide( CPhysConvex** pConvex, int convexCount ) = 0;
	virtual CPhysCollide* ConvertConvexToCollideParams( CPhysConvex** pConvex, int convexCount, const convertconvexparams_t& convertParams ) = 0;
	// Free a collide that was created with ConvertConvexToCollide()
	virtual void			DestroyCollide( CPhysCollide* pCollide ) = 0;

	// Get the memory size in bytes of the collision model for serialization
	virtual int				CollideSize( CPhysCollide* pCollide ) = 0;
	// serialize the collide to a block of memory
	virtual int				CollideWrite( char* pDest, CPhysCollide* pCollide, bool bSwap = false ) = 0;
	// unserialize the collide from a block of memory
	virtual CPhysCollide* UnserializeCollide( char* pBuffer, int size, int index ) = 0;

	// compute the volume of a collide
	virtual float			CollideVolume( CPhysCollide* pCollide ) = 0;
	// compute surface area for tools
	virtual float			CollideSurfaceArea( CPhysCollide* pCollide ) = 0;

	// Get the support map for a collide in the given direction
	virtual Vector			CollideGetExtent( const CPhysCollide* pCollide, const Vector& collideOrigin, const QAngle& collideAngles, const Vector& direction ) = 0;

	// Get an AABB for an oriented collision model
	virtual void			CollideGetAABB( Vector* pMins, Vector* pMaxs, const CPhysCollide* pCollide, const Vector& collideOrigin, const QAngle& collideAngles ) = 0;

	virtual void			CollideGetMassCenter( CPhysCollide* pCollide, Vector* pOutMassCenter ) = 0;
	virtual void			CollideSetMassCenter( CPhysCollide* pCollide, const Vector& massCenter ) = 0;
	// get the approximate cross-sectional area projected orthographically on the bbox of the collide
	// NOTE: These are fractional areas - unitless.  Basically this is the fraction of the OBB on each axis that
	// would be visible if the object were rendered orthographically.
	// NOTE: This has been precomputed when the collide was built or this function will return 1,1,1
	virtual Vector			CollideGetOrthographicAreas( const CPhysCollide* pCollide ) = 0;
	virtual void			CollideSetOrthographicAreas( CPhysCollide* pCollide, const Vector& areas ) = 0;

	// query the vcollide index in the physics model for the instance
	virtual int				CollideIndex( const CPhysCollide* pCollide ) = 0;

	// Convert a bbox to a collide
	virtual CPhysCollide* BBoxToCollide( const Vector& mins, const Vector& maxs ) = 0;
	virtual int				GetConvexesUsedInCollideable( const CPhysCollide* pCollideable, CPhysConvex** pOutputArray, int iOutputArrayLimit ) = 0;


	// Trace an AABB against a collide
	virtual void TraceBox( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const CPhysCollide* pCollide, const Vector& collideOrigin, const QAngle& collideAngles, Trace_t* ptr ) = 0;
	virtual void TraceBox( const Ray_t& ray, const CPhysCollide* pCollide, const Vector& collideOrigin, const QAngle& collideAngles, Trace_t* ptr ) = 0;
	virtual void TraceBox( const Ray_t& ray, unsigned int contentsMask, IConvexInfo* pConvexInfo, const CPhysCollide* pCollide, const Vector& collideOrigin, const QAngle& collideAngles, Trace_t* ptr ) = 0;

	// Trace one collide against another
	virtual void TraceCollide( const Vector& start, const Vector& end, const CPhysCollide* pSweepCollide, const QAngle& sweepAngles, const CPhysCollide* pCollide, const Vector& collideOrigin, const QAngle& collideAngles, Trace_t* ptr ) = 0;

	// relatively slow test for box vs. truncated cone
	virtual bool			IsBoxIntersectingCone( const Vector& boxAbsMins, const Vector& boxAbsMaxs, const truncatedcone_t& cone ) = 0;

	// loads a set of solids into a vcollide_t
	virtual void			VCollideLoad( vcollide_t* pOutput, int solidCount, const char* pBuffer, int size, bool swap = false ) = 0;
	// destroyts the set of solids created by VCollideLoad
	virtual void			VCollideUnload( vcollide_t* pVCollide ) = 0;

	// begins parsing a vcollide.  NOTE: This keeps pointers to the text
	// If you free the text and call members of IVPhysicsKeyParser, it will crash
	virtual IVPhysicsKeyParser* VPhysicsKeyParserCreate( const char* pKeyData ) = 0;
	// Free the parser created by VPhysicsKeyParserCreate
	virtual void			VPhysicsKeyParserDestroy( IVPhysicsKeyParser* pParser ) = 0;

	// creates a list of verts from a collision mesh
	virtual int				CreateDebugMesh( CPhysCollide const* pCollisionModel, Vector** outVerts ) = 0;
	// destroy the list of verts created by CreateDebugMesh
	virtual void			DestroyDebugMesh( int vertCount, Vector* outVerts ) = 0;

	// create a queryable version of the collision model
	virtual ICollisionQuery* CreateQueryModel( CPhysCollide* pCollide ) = 0;
	// destroy the queryable version
	virtual void			DestroyQueryModel( ICollisionQuery* pQuery ) = 0;

	virtual IPhysicsCollision* ThreadContextCreate( void ) = 0;
	virtual void			ThreadContextDestroy( IPhysicsCollision* pThreadContex ) = 0;

	virtual CPhysCollide* CreateVirtualMesh( const virtualmeshparams_t& params ) = 0;
	virtual bool			SupportsVirtualMesh( ) = 0;


	virtual bool			GetBBoxCacheSize( int* pCachedSize, int* pCachedCount ) = 0;


	// extracts a polyhedron that defines a CPhysConvex's shape
	virtual CPolyhedron* PolyhedronFromConvex( CPhysConvex* const pConvex, bool bUseTempPolyhedron ) = 0;

	// dumps info about the collide to Msg()
	virtual void			OutputDebugInfo( const CPhysCollide* pCollide ) = 0;
	virtual unsigned int	ReadStat( int statID ) = 0;
};