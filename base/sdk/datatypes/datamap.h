#pragma once

#pragma region datamap_enumerations
enum EFieldTypes : int
{
	FIELD_VOID = 0,			// No type or value
	FIELD_FLOAT,			// Any floating point value
	FIELD_STRING,			// A string ID (return from ALLOC_STRING)
	FIELD_VECTOR,			// Any vector, QAngle, or AngularImpulse
	FIELD_QUATERNION,		// A quaternion
	FIELD_INTEGER,			// Any integer or enum
	FIELD_BOOLEAN,			// boolean, implemented as an int, I may use this as a hint for compression
	FIELD_SHORT,			// 2 byte integer
	FIELD_CHARACTER,		// a byte
	FIELD_COLOR32,			// 8-bit per channel r,g,b,a (32bit color)
	FIELD_EMBEDDED,			// an embedded object with a datadesc, recursively traverse and embedded class/structure based on an additional typedescription
	FIELD_CUSTOM,			// special type that contains function pointers to it's read/write/parse functions
	FIELD_CLASSPTR,			// CBaseEntity*
	FIELD_EHANDLE,			// Entity handle
	FIELD_EDICT,			// edict_t*
	FIELD_POSITION_VECTOR,	// A world coordinate (these are fixed up across level transitions automagically)
	FIELD_TIME,				// a floating point time (these are fixed up automatically too!)
	FIELD_TICK,				// an integer tick count( fixed up similarly to time)
	FIELD_MODELNAME,		// Engine string that is a model name (needs precache)
	FIELD_SOUNDNAME,		// Engine string that is a sound name (needs precache)
	FIELD_INPUT,			// a list of inputed data fields (all derived from CMultiInputVar)
	FIELD_FUNCTION,			// A class function pointer (Think, Use, etc)
	FIELD_VMATRIX,			// a vmatrix (output coords are NOT worldspace)
	FIELD_VMATRIX_WORLDSPACE,// A VMatrix that maps some local space to world space (translation is fixed up on level transitions)
	FIELD_MATRIX3X4_WORLDSPACE,	// matrix3x4_t that maps some local space to world space (translation is fixed up on level transitions)
	FIELD_INTERVAL,			// a start and range floating point interval ( e.g., 3.2->3.6 == 3.2 and 0.4 )
	FIELD_MODELINDEX,		// a model index
	FIELD_MATERIALINDEX,	// a material index (using the material precache string table)
	FIELD_VECTOR2D,			// 2 floats
	FIELD_INTEGER64,		// 64bit integer
	FIELD_VECTOR4D,			// 4 floats
	FIELD_TYPECOUNT
};

enum
{
	TD_OFFSET_NORMAL = 0,
	TD_OFFSET_PACKED = 1,
	TD_OFFSET_COUNT
};
#pragma endregion

struct DataMap_t;

class TypeDescription_t
{
public:
	int			iFieldType;
	const char* szFieldName;
	int					iFieldOffset; // Local offset value
	unsigned short		uFieldSize;
	short				fFlags;
	// the name of the variable in the map/fgd data, or the name of the action
	const char* externalName;
	// pointer to the function set for save/restoring of custom data types
	void* pSaveRestoreOps;
	// for associating function with string names
	void* inputFunc;
	// For embedding additional datatables inside this one
	DataMap_t* pTypeDescription;

	// Stores the actual member variable size in bytes
	int					fieldSizeInBytes;

	// FTYPEDESC_OVERRIDE point to first baseclass instance if chains_validated has occurred
	class TypeDescription_t* pOverrideField;

	// Used to track exclusion of baseclass fields
	int					overrideCount;

	// Tolerance for field errors for float fields
	float				fieldTolerance;

	// For raw fields (including children of embedded stuff) this is the flattened offset
	int					flatOffset[ TD_OFFSET_COUNT ];
	unsigned short		flatGroup;
}; //Size: 0x003C

struct DataMap_t
{
	TypeDescription_t* pDataDesc;
	int nDataFields;
	const char* szDataClassName;
	DataMap_t* pBaseMap;
	int iPackedSize;
	void* pOptimizedDataMap;
};
