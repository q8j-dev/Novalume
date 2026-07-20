$input a_position
$output v_shadowDepth

#include <bgfx_shader.sh>

uniform mat4 ViewProjection;
uniform mat4 WorldMatrix;

void main()
{
	vec3 posWorld = mul(WorldMatrix, a_position).xyz;
	gl_Position = mul(ViewProjection, vec4(posWorld, 1.0));
	v_shadowDepth = gl_Position.z / gl_Position.w;
}
