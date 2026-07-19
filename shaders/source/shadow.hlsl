#include "common.h"

struct Appdata
{
    float4 Position     : POSITION;

    ATTR_INT4 Extra : COLOR1;
	ATTR_INT4 SkinIndices : TEXCOORD4;
	float4 SkinWeights : TEXCOORD5;
};

struct VertexOutput
{
	float4 HPosition: POSITION;
	float Depth: TEXCOORD0;
};

#ifdef PIN_SKINNED
    WORLD_MATRIX_ARRAY(WorldMatrixArray, MAX_BONE_COUNT * 3);
#endif

VertexOutput ShadowVS(Appdata IN)
{
    VertexOutput OUT = (VertexOutput)0;

    // Transform position to world space
#ifdef PIN_SKINNED
	int boneIndex0 = int(IN.SkinIndices.x);
	int boneIndex1 = int(IN.SkinIndices.y);
	int boneIndex2 = int(IN.SkinIndices.z);
	int boneIndex3 = int(IN.SkinIndices.w);
	float4 worldRow0 =
		WorldMatrixArray[boneIndex0 * 3 + 0] * IN.SkinWeights.x +
		WorldMatrixArray[boneIndex1 * 3 + 0] * IN.SkinWeights.y +
		WorldMatrixArray[boneIndex2 * 3 + 0] * IN.SkinWeights.z +
		WorldMatrixArray[boneIndex3 * 3 + 0] * IN.SkinWeights.w;
	float4 worldRow1 =
		WorldMatrixArray[boneIndex0 * 3 + 1] * IN.SkinWeights.x +
		WorldMatrixArray[boneIndex1 * 3 + 1] * IN.SkinWeights.y +
		WorldMatrixArray[boneIndex2 * 3 + 1] * IN.SkinWeights.z +
		WorldMatrixArray[boneIndex3 * 3 + 1] * IN.SkinWeights.w;
	float4 worldRow2 =
		WorldMatrixArray[boneIndex0 * 3 + 2] * IN.SkinWeights.x +
		WorldMatrixArray[boneIndex1 * 3 + 2] * IN.SkinWeights.y +
		WorldMatrixArray[boneIndex2 * 3 + 2] * IN.SkinWeights.z +
		WorldMatrixArray[boneIndex3 * 3 + 2] * IN.SkinWeights.w;
		
	float3 posWorld = float3(dot(worldRow0, IN.Position), dot(worldRow1, IN.Position), dot(worldRow2, IN.Position));
#else
	float3 posWorld = IN.Position.xyz;
#endif

	OUT.HPosition = mul(G(ViewProjection), float4(posWorld, 1));
	OUT.Depth = OUT.HPosition.z / OUT.HPosition.w;

	return OUT;
}

float4 ShadowPS(VertexOutput IN): COLOR0
{
	// Store depth from the cascade currently being rendered.  Receiver-side
	// cascade selection is invalid here: it can select another atlas tile and,
	// while early cascades render, matrices that have not been populated yet.
	float slope = max(abs(ddx(IN.Depth)), abs(ddy(IN.Depth)));
	float depth = saturate(IN.Depth + G(ShadowParams).x + min(G(ShadowParams).z, slope * G(ShadowParams).y));

	return float4(depth, 1, 0, 0);
}
