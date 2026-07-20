$input a_position
$output v_shadowDepth

#include <bgfx_shader.sh>

uniform mat4 ViewProjection;
uniform vec4 WorldMatrixArray[72];

void main()
{
	vec3 posWorld =
		a_position.xyz * WorldMatrixArray[0].w + WorldMatrixArray[0].xyz;
	gl_Position = mul(ViewProjection, vec4(posWorld, 1.0));
	v_shadowDepth = gl_Position.z / gl_Position.w;
}
