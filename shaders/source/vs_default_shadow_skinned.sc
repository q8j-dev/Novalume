$input a_position, a_texcoord4, a_texcoord5
$output v_shadowDepth

#include <bgfx_shader.sh>

uniform mat4 ViewProjection;
uniform vec4 WorldMatrixArray[216];

void main()
{
	int boneIndex0 = int(a_texcoord4.x);
	int boneIndex1 = int(a_texcoord4.y);
	int boneIndex2 = int(a_texcoord4.z);
	int boneIndex3 = int(a_texcoord4.w);

	vec4 worldRow0 =
		WorldMatrixArray[boneIndex0 * 3    ] * a_texcoord5.x
		+ WorldMatrixArray[boneIndex1 * 3    ] * a_texcoord5.y
		+ WorldMatrixArray[boneIndex2 * 3    ] * a_texcoord5.z
		+ WorldMatrixArray[boneIndex3 * 3    ] * a_texcoord5.w;
	vec4 worldRow1 =
		WorldMatrixArray[boneIndex0 * 3 + 1] * a_texcoord5.x
		+ WorldMatrixArray[boneIndex1 * 3 + 1] * a_texcoord5.y
		+ WorldMatrixArray[boneIndex2 * 3 + 1] * a_texcoord5.z
		+ WorldMatrixArray[boneIndex3 * 3 + 1] * a_texcoord5.w;
	vec4 worldRow2 =
		WorldMatrixArray[boneIndex0 * 3 + 2] * a_texcoord5.x
		+ WorldMatrixArray[boneIndex1 * 3 + 2] * a_texcoord5.y
		+ WorldMatrixArray[boneIndex2 * 3 + 2] * a_texcoord5.z
		+ WorldMatrixArray[boneIndex3 * 3 + 2] * a_texcoord5.w;

	vec3 posWorld = vec3(
		dot(worldRow0, a_position),
		dot(worldRow1, a_position),
		dot(worldRow2, a_position));
	gl_Position = mul(ViewProjection, vec4(posWorld, 1.0));
	v_shadowDepth = gl_Position.z / gl_Position.w;
}
