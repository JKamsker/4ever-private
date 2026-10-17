$input v_color0, v_color1, v_texcoord0, v_texcoord1, v_fog

// Direct3D 9 fixed-function pixel processing for the bgfx backend: four
// texture stages (D3DTOP operations on D3DTA arguments), specular add, fog and
// alpha test.

#include <bgfx_shader.sh>

SAMPLER2D(s_tex0, 0);
SAMPLER2D(s_tex1, 1);
SAMPLER2D(s_tex2, 2);
SAMPLER2D(s_tex3, 3);

// 4 vec4 per stage: color (op, arg1, arg2, arg0), color argument modifiers,
// alpha (op, arg1, arg2, arg0), alpha argument modifiers. A modifier is
// complement (1) + alpha replicate (2).
uniform vec4 u_tStage[16];

// [0] texture factor
// [1] alpha test enable, alpha function (D3DCMP), alpha reference (0-255), specular enable
// [2] fog color, fog enable
// [3] texture bound per stage
// [4] LOD bias per stage
// [5-8] D3DTSS_CONSTANT per stage
uniform vec4 u_tPS[9];

vec4 stageArg( float fSelect, float fModifier, vec4 vDiffuse, vec4 vCurrent, vec4 vTexture, vec4 vSpecular, vec4 vConstant)
{
	vec4 vArg;

	if( fSelect < 0.5 )
		vArg = vDiffuse;
	else if( fSelect < 1.5 )
		vArg = vCurrent;
	else if( fSelect < 2.5 )
		vArg = vTexture;
	else if( fSelect < 3.5 )
		vArg = u_tPS[0];
	else if( fSelect < 4.5 )
		vArg = vSpecular;
	else if( fSelect < 5.5 )
		vArg = vec4( 0.0, 0.0, 0.0, 0.0);
	else
		vArg = vConstant;

	if( fModifier > 1.5 )
	{
		vArg = vArg.wwww;
		fModifier -= 2.0;
	}

	if( fModifier > 0.5 )
		vArg = vec4( 1.0, 1.0, 1.0, 1.0) - vArg;

	return vArg;
}

vec4 stageOp( float fOp, vec4 a1, vec4 a2, vec4 a0, vec4 vDiffuse, vec4 vTexture, vec4 vCurrent)
{
	int nOp = int(fOp + 0.5);

	if( nOp == 2 ) return a1;
	if( nOp == 3 ) return a2;
	if( nOp == 4 ) return a1 * a2;
	if( nOp == 5 ) return a1 * a2 * 2.0;
	if( nOp == 6 ) return a1 * a2 * 4.0;
	if( nOp == 7 ) return a1 + a2;
	if( nOp == 8 ) return a1 + a2 - 0.5;
	if( nOp == 9 ) return (a1 + a2 - 0.5) * 2.0;
	if( nOp == 10 ) return a1 - a2;
	if( nOp == 11 ) return a1 + a2 - a1 * a2;
	if( nOp == 12 ) return mix( a2, a1, vDiffuse.w);
	if( nOp == 13 ) return mix( a2, a1, vTexture.w);
	if( nOp == 14 ) return mix( a2, a1, u_tPS[0].w);
	if( nOp == 15 ) return a1 + a2 * (1.0 - vTexture.w);
	if( nOp == 16 ) return mix( a2, a1, vCurrent.w);
	if( nOp == 18 ) return a1 + a1.wwww * a2;
	if( nOp == 19 ) return a1 * a2 + a1.wwww;
	if( nOp == 20 ) return (1.0 - a1.wwww) * a2 + a1;
	if( nOp == 21 ) return (vec4( 1.0, 1.0, 1.0, 1.0) - a1) * a2 + a1.wwww;
	if( nOp == 24 )
	{
		float fDot = clamp( 4.0 * dot( a1.xyz - 0.5, a2.xyz - 0.5), 0.0, 1.0);
		return vec4( fDot, fDot, fDot, fDot);
	}
	if( nOp == 25 ) return a0 + a1 * a2;
	if( nOp == 26 ) return mix( a2, a1, a0);

	// PREMODULATE and the bump mapping operations are not used by the engine.
	return a1;
}

// Whether an operation reads D3DTA_TEXTURE (2) through the arguments it uses, or the
// texture alpha. D3D9 passes the current color through such an operation when the
// stage has no texture: effects use MODULATE2X and weapons ADDSIGNED2X with a
// texture argument on an empty stage 1.
bool isTexture( float fSelect)
{
	return fSelect > 1.5 && fSelect < 2.5;
}

bool usesTexture( vec4 vOp)
{
	int nOp = int(vOp.x + 0.5);

	if( nOp == 13 || nOp == 15 )
		return true;
	if( nOp == 2 )
		return isTexture(vOp.y);
	if( nOp == 3 )
		return isTexture(vOp.z);
	if( nOp == 25 || nOp == 26 )
		return isTexture(vOp.y) || isTexture(vOp.z) || isTexture(vOp.w);

	return isTexture(vOp.y) || isTexture(vOp.z);
}

vec4 sampleStage( int nStage, vec4 vCoord01, vec4 vCoord23)
{
	vec4 vBound = u_tPS[3];
	vec4 vBias = u_tPS[4];

	// A stage without a texture is not sampled: usesTexture skips it.
	vec4 vWhite = vec4( 1.0, 1.0, 1.0, 1.0);

	if( nStage == 0 )
		return vBound.x > 0.5 ? texture2DBias( s_tex0, vCoord01.xy, vBias.x) : vWhite;
	if( nStage == 1 )
		return vBound.y > 0.5 ? texture2DBias( s_tex1, vCoord01.zw, vBias.y) : vWhite;
	if( nStage == 2 )
		return vBound.z > 0.5 ? texture2DBias( s_tex2, vCoord23.xy, vBias.z) : vWhite;

	return vBound.w > 0.5 ? texture2DBias( s_tex3, vCoord23.zw, vBias.w) : vWhite;
}

void main()
{
	vec4 vDiffuse = v_color0;
	vec4 vSpecular = v_color1;
	vec4 vCurrent = vDiffuse;

	for( int i=0; i<4; i++)
	{
		vec4 vColorOp = u_tStage[i * 4];
		vec4 vColorMod = u_tStage[i * 4 + 1];
		vec4 vAlphaOp = u_tStage[i * 4 + 2];
		vec4 vAlphaMod = u_tStage[i * 4 + 3];

		// D3DTOP_DISABLE ends the cascade.
		if( vColorOp.x < 1.5 )
			break;

		vec4 vTexture = sampleStage( i, v_texcoord0, v_texcoord1);
		vec4 vBound = u_tPS[3];
		bool bBound = (i == 0 ? vBound.x : i == 1 ? vBound.y : i == 2 ? vBound.z : vBound.w) > 0.5;
		vec4 vConstant = u_tPS[5 + i];

		vec4 c1 = stageArg( vColorOp.y, vColorMod.x, vDiffuse, vCurrent, vTexture, vSpecular, vConstant);
		vec4 c2 = stageArg( vColorOp.z, vColorMod.y, vDiffuse, vCurrent, vTexture, vSpecular, vConstant);
		vec4 c0 = stageArg( vColorOp.w, vColorMod.z, vDiffuse, vCurrent, vTexture, vSpecular, vConstant);
		vec4 vColor = stageOp( vColorOp.x, c1, c2, c0, vDiffuse, vTexture, vCurrent);
		float fAlpha = vCurrent.w;

		if( !bBound && usesTexture(vColorOp) )
			vColor = vCurrent;

		if( int(vColorOp.x + 0.5) == 24 )
			fAlpha = vColor.w;
		else if( vAlphaOp.x > 1.5 && (bBound || !usesTexture(vAlphaOp)) )
		{
			vec4 l1 = stageArg( vAlphaOp.y, vAlphaMod.x, vDiffuse, vCurrent, vTexture, vSpecular, vConstant);
			vec4 l2 = stageArg( vAlphaOp.z, vAlphaMod.y, vDiffuse, vCurrent, vTexture, vSpecular, vConstant);
			vec4 l0 = stageArg( vAlphaOp.w, vAlphaMod.z, vDiffuse, vCurrent, vTexture, vSpecular, vConstant);
			fAlpha = stageOp( vAlphaOp.x, l1, l2, l0, vDiffuse, vTexture, vCurrent).w;
		}

		vCurrent = clamp( vec4( vColor.xyz, fAlpha), 0.0, 1.0);
	}

	vec4 vParams = u_tPS[1];

	if( vParams.w > 0.5 )
		vCurrent.xyz = clamp( vCurrent.xyz + vSpecular.xyz, 0.0, 1.0);

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
