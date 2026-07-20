$input a_position
$output v_shadowDepth

#include <bgfx_shader.sh>

uniform mat4 ViewProjection;

void main()
{
	gl_Position = mul(ViewProjection, vec4(a_position.xyz, 1.0));
	v_shadowDepth = gl_Position.z / gl_Position.w;
}
