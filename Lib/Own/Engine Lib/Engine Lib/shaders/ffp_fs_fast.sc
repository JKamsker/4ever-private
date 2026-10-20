$input v_color0, v_color1, v_texcoord0, v_texcoord1, v_fog

// The common case of ffp_fs.sc, without its branches: stage 0 only, a SELECTARG or
// MODULATE of the diffuse color, the texture and the texture factor, for the color and
// the alpha. The CPU turns the stage into weights: each argument is
// diffuse * x + texture * y + factor * z + w, so a SELECTARG has a second argument of 1.

#include <bgfx_shader.sh>

SAMPLER2D(s_tex0, 0);

// Color argument 1, color argument 2, alpha argument 1, alpha argument 2.
uniform vec4 u_tFast[4];

// As in ffp_fs.sc: [0] texture factor, [1] alpha test enable, function, reference,
// specular enable, [2] fog color, fog enable, [4] LOD bias per stage.
uniform vec4 u_tPS[9];

void main()
{
	vec4 vDiffuse = v_color0;
	vec4 vTexture = texture2DBias( s_tex0, v_texcoord0.xy, u_tPS[4].x);
	vec4 vFactor = u_tPS[0];

	vec4 c1 = vDiffuse * u_tFast[0].x + vTexture * u_tFast[0].y + vFactor * u_tFast[0].z + u_tFast[0].w;
	vec4 c2 = vDiffuse * u_tFast[1].x + vTexture * u_tFast[1].y + vFactor * u_tFast[1].z + u_tFast[1].w;
	float a1 = vDiffuse.w * u_tFast[2].x + vTexture.w * u_tFast[2].y + vFactor.w * u_tFast[2].z + u_tFast[2].w;
	float a2 = vDiffuse.w * u_tFast[3].x + vTexture.w * u_tFast[3].y + vFactor.w * u_tFast[3].z + u_tFast[3].w;
	vec4 vCurrent = clamp( vec4( c1.xyz * c2.xyz, a1 * a2), 0.0, 1.0);

	vec4 vParams = u_tPS[1];

	if( vParams.w > 0.5 )
		vCurrent.xyz = clamp( vCurrent.xyz + v_color1.xyz, 0.0, 1.0);

	if( u_tPS[2].w > 0.5 )
		vCurrent.xyz = mix( u_tPS[2].xyz, vCurrent.xyz, v_fog);

	if( vParams.x > 0.5 )
	{
		int nFunc = int(vParams.y + 0.5);
		float fAlpha = floor( vCurrent.w * 255.0 + 0.5);
		float fRef = vParams.z;
		bool bPass = true;

		if( nFunc == 1 ) bPass = false;
		else if( nFunc == 2 ) bPass = fAlpha < fRef;
		else if( nFunc == 3 ) bPass = fAlpha == fRef;
		else if( nFunc == 4 ) bPass = fAlpha <= fRef;
		else if( nFunc == 5 ) bPass = fAlpha > fRef;
		else if( nFunc == 6 ) bPass = fAlpha != fRef;
		else if( nFunc == 7 ) bPass = fAlpha >= fRef;

		if(!bPass)
			discard;
	}

	gl_FragColor = vCurrent;
}
