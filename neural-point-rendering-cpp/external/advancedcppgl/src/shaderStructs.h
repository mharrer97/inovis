#ifndef __SHADERSTRUCTS_H__
#define __SHADERSTRUCTS_H__


/*
		STRUCTS TO BE USED IN C++ AND GLSL CODE
		!!!!
		BE VERY CAREFUL WITH VEC3 TYPES, COMPILERS ALIGN THEM WIERDLY EVEN WITH PACKING
		https://stackoverflow.com/a/38172697

		!ONLY! USE VEC3 IF YOU FOLLOW IT WITH A SCALAR (INT/FLOAT) TYPE [ALIGN AT 16 BYTES], 
		PADDING IF YOU HAVE TO,	AND HOPE THE C++ COMPILER DOESN'T CHANGE (see static assert below)
		(AND USE std430 LAYOUT IN GLSL, AT LEAST THIS SHOULD BE FIXED)
*/

#ifdef __cplusplus

//packing is important to ensure gpu and cpu compatability between types
#ifdef __GNUC__
#define PACK( __struct__ ) __struct__ __attribute__((__packed__))
#elif defined _MSC_VER
#define PACK( __struct__ ) __pragma( pack(push, 1) ) __struct__ __pragma( pack(pop))
#else
#define PACK( __struct__ ) __struct__
#endif

using vec3 = glm::vec3;
using vec2 = glm::vec2;
using uint = unsigned int;
using mat4 = glm::mat4;
#else
//glsl
#define PACK( __struct__ ) __struct__

#endif

	//////////////////////////////////////////////////////////
	// BindPoints go here
#define MATERIAL_DATA_BINDPOINT 1
#define MODEL_DATA_BINDPOINT 2
#define DYNAMIC_MODEL_DATA_BINDPOINT 3
#define LIGHTING_DATA_BINDPOINT 4

	//////////////////////////////////////////////////////////
	// Structs go here

	PACK(
	struct PointLight {
		vec3 pointLightPos;
		float pointLightPower;

		vec3 pointLightColor;
		float pointLightFalloffFactor;

	});

	PACK(
	struct LightingData {
		vec3 directionalLightDir;
		float luminance;
		vec3 directionalLightColor;
		float directionalLightStrength;

		vec3 padding;
		int numPointLights;

	});

	PACK(
	struct DynamicModelData {
		mat4 modelMatrix;
	});

	PACK(
	//Model Data (scales models back from compression and links material)
	struct ModelData
	{
		vec3 b_min;
		int materialID;

		vec3 axisLengths;
		int subModelID;

		vec3 subModel_b_min;
		float tcMin;
		
		vec3 subModel_axisLengths;
		float tcExtend;

		vec3 padding;
		int dynamicModelID; //original modelID, used to link with dynamic data
	});
	//\Model Data 

	#ifdef __cplusplus
	//material data
		PACK(
			struct MaterialData
		{
			vec3 kDiffuse;
			float opacity;
			vec3 kSpecular;
			float roughness;
			uint64_t texDiff;
			uint64_t texNormal;
			uint64_t texSpec;
			uint64_t texAlpha;
		});
		//\material data
	#else
			//material data
		PACK(
			struct MaterialData
		{
			vec3 kDiffuse;
			float opacity;
			vec3 kSpecular;
			float roughness;
			uvec2 texDiff;
			uvec2 texNormal;
			uvec2 texSpec;
			uvec2 texAlpha;
		});
		//\material data
	#endif



	//////////////////////////////////////////////////////////
	// Structs end here


	//////////////////////////////////////////////////////
	// Structs with constructers

	//command buffer
	struct DrawElementsIndirectCommand {
		uint count;
		uint instanceCount;
		uint first;
		uint baseVertex;
		uint baseInstance;
	#ifdef __cplusplus
		DrawElementsIndirectCommand()
			:first(0), count(0), instanceCount(1), baseVertex(0), baseInstance(0)
		{}
		DrawElementsIndirectCommand(GLuint first, GLuint count, GLuint baseVertex)
			: first(first), count(count), instanceCount(1), baseVertex(baseVertex), baseInstance(0)
		{}
	#endif
	};
//\command buffer


#ifdef __cplusplus
struct vec3_alignment_test_struct {
	glm::vec3 t1;
	float t2;
};
static_assert(sizeof(vec3_alignment_test_struct) == 16,
	"Alignment with vec3 and floats not correct (most likely because the compiler has changed), \
so the shader storage buffer structs might not be the same between c++ and glsl code. \
Expect errors or other wierdness :("
);
#endif


#endif // SHADERSTRUCTS_VARS__