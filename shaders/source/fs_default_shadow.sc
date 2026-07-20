$input v_shadowDepth

#include <bgfx_shader.sh>

uniform vec4 ShadowParams;

void main()
{
	float slope = max(abs(dFdx(v_shadowDepth)), abs(dFdy(v_shadowDepth)));
	float depth = clamp(
		v_shadowDepth + ShadowParams.x
		+ min(ShadowParams.z, slope * ShadowParams.y),
		0.0,
		1.0);
	gl_FragColor = vec4(depth, 1.0, 0.0, 0.0);
}
