// Direct3D 9 fixed-function vertex processing for the bgfx backend.
// The variant defines give the vertex format: POS_XYZ, POS_RHW (pretransformed)
// or POS_B4 (XYZ + 3 blend weights + UBYTE4 indices), HAS_NORMAL, HAS_COLOR and
// TEXCOUNT (0, 1 or 2). CompileShaders.cmake adds the $input/$output lines.
// Matrices are D3D (row vectors); each one is uploaded as its transpose rows,
// so p * M is a dot product with each row.

#include <bgfx_shader.sh>

// World matrices of the blend palette, 3 rows each (the last column is 0 0 0 1).
#define MAX_WORLD 256
uniform vec4 u_tWorld[MAX_WORLD * 3];
uniform vec4 u_tViewProj[4];
uniform vec4 u_tView[4];

// [0] blend mode (0 world 0, 1 matrices 0-3, 2 indexed), weights, lighting, normalize normals
// [1] material sources of diffuse, specular, ambient, emissive (0 material, 1 diffuse, 2 specular)
// [2] color vertex, specular enable, -, material power
// [3] global ambient
// [4] viewport x, y, width, height
// [5] viewport min z, max z, homogeneous depth (OpenGL), depth bias
// [6] fog mode (0 none, 1 exp, 2 exp2, 3 linear), start, end, density
// [7] range fog, fog enable, flip y (render targets on OpenGL), -
// [8] camera position
uniform vec4 u_tVS[9];

// Diffuse, ambient, specular, emissive.
uniform vec4 u_tMaterial[4];

// 8 lights, 7 vec4 each: (type, enabled, range, -), position, direction,
// diffuse, specular, ambient, (attenuation 0, 1, 2, -). Type: 1 point, 2 spot, 3 directional.
#define MAX_LIGHTS 8
uniform vec4 u_tLights[MAX_LIGHTS * 7];

// 4 stages, 4 vec4 each: transform rows 0 and 1, (texcoord index, transform enabled, -, -), -.
uniform vec4 u_tTexXform[16];

vec3 transformPoint( int nIndex, vec4 vPos)
{
	return vec3(
		dot( vPos, u_tWorld[nIndex * 3]),
		dot( vPos, u_tWorld[nIndex * 3 + 1]),
		dot( vPos, u_tWorld[nIndex * 3 + 2]));
}

// Like D3D9, normals go through the inverse transpose of the world matrix: a
// scaled mesh gets longer or shorter normals (and brighter or darker lighting)
// unless NORMALIZENORMALS is on. The uploaded rows are the matrix columns, so
// the rows of the inverse are their cross products over the determinant.
vec3 transformNormal( int nIndex, vec3 vNormal)
{
	vec3 c0 = u_tWorld[nIndex * 3].xyz;
	vec3 c1 = u_tWorld[nIndex * 3 + 1].xyz;
	vec3 c2 = u_tWorld[nIndex * 3 + 2].xyz;
	vec3 r0 = cross( c1, c2);
	float fDet = dot( c0, r0);

	if( abs(fDet) < 1e-12 )
		return vec3( 0.0, 0.0, 0.0);

	return vec3(
		dot( r0, vNormal),
		dot( cross( c2, c0), vNormal),
		dot( cross( c0, c1), vNormal)) / fDet;
}

vec4 materialColor( float fSource, vec4 vMaterial, vec4 vDiffuse, vec4 vSpecular, bool bDiffuse)
{
	if( u_tVS[2].x < 0.5 )
		return vMaterial;

	if( fSource > 1.5 )
		return vSpecular;

	if( fSource > 0.5 && bDiffuse )
		return vDiffuse;

	return vMaterial;
}

vec2 stageCoord( int nStage, vec2 vSet0, vec2 vSet1)
{
	vec4 vParam = u_tTexXform[nStage * 4 + 2];
	vec2 vCoord = vParam.x < 0.5 ? vSet0 : vSet1;

	if( vParam.y > 0.5 )
	{
		vec4 vIn = vec4( vCoord, 1.0, 0.0);
		vCoord = vec2( dot( vIn, u_tTexXform[nStage * 4]), dot( vIn, u_tTexXform[nStage * 4 + 1]));
	}

	return vCoord;
}

void main()
{
	vec4 vDiffuse = vec4( 1.0, 1.0, 1.0, 1.0);
	vec4 vSpecular = vec4( 0.0, 0.0, 0.0, 0.0);
	bool bDiffuse = false;

#if HAS_COLOR
	// D3DCOLOR bytes are B, G, R, A.
	vDiffuse = a_color0.zyxw;
	bDiffuse = true;
#endif

	vec2 vSet0 = vec2( 0.0, 0.0);
	vec2 vSet1 = vec2( 0.0, 0.0);

#if TEXCOUNT >= 1
	vSet0 = a_texcoord0;
#endif
#if TEXCOUNT >= 2
	vSet1 = a_texcoord1;
#endif

	vec4 vViewport = u_tVS[4];
	vec4 vDepth = u_tVS[5];
	float fFog = 1.0;

#if POS_RHW
	// Pretransformed: screen position, depth and 1/w. D3D9 pixel centers are at
	// integer coordinates, bgfx's (D3D11, OpenGL, Vulkan) at +0.5.
	float fW = a_position.w > 0.0 ? 1.0 / a_position.w : 1.0;
	vec2 vNDC = vec2(
		(a_position.x + 0.5 - vViewport.x) / vViewport.z * 2.0 - 1.0,
		1.0 - (a_position.y + 0.5 - vViewport.y) / vViewport.w * 2.0);
	float fZ = a_position.z;

	if( vDepth.z > 0.5 )
		fZ = fZ * 2.0 - 1.0;

	gl_Position = vec4( vNDC * fW, fZ * fW, fW);
	v_color0 = vDiffuse;
	v_color1 = vSpecular;
#else
	vec4 vPos = vec4( a_position.xyz, 1.0);
	vec3 vNormalIn = vec3( 0.0, 0.0, 0.0);

#if HAS_NORMAL
	vNormalIn = a_normal;
#endif

	vec3 vWorld = vec3( 0.0, 0.0, 0.0);
	vec3 vNormal = vec3( 0.0, 0.0, 0.0);

#if POS_B4
	float fMode = u_tVS[0].x;

	if( fMode < 0.5 )
	{
		vWorld = transformPoint( 0, vPos);
		vNormal = transformNormal( 0, vNormalIn);
	}
	else
	{
		float vWeight[4];
		int vIndex[4];
		int nCount = int(u_tVS[0].y + 0.5);

		vWeight[0] = a_weight.x;
		vWeight[1] = a_weight.y;
		vWeight[2] = a_weight.z;
		vWeight[3] = 0.0;

		// The last weight makes the sum 1.
		float fSum = 0.0;
		for( int i=0; i<3; i++)
		{
			if( i >= nCount )
				vWeight[i] = 0.0;
			fSum += vWeight[i];
		}
		vWeight[nCount < 3 ? nCount : 3] = 1.0 - fSum;

		if( fMode > 1.5 )
		{
			vIndex[0] = int(a_indices.x * 255.0 + 0.5);
			vIndex[1] = int(a_indices.y * 255.0 + 0.5);
			vIndex[2] = int(a_indices.z * 255.0 + 0.5);
			vIndex[3] = int(a_indices.w * 255.0 + 0.5);
		}
		else
		{
			vIndex[0] = 0;
			vIndex[1] = 1;
			vIndex[2] = 2;
			vIndex[3] = 3;
		}

		for( int i=0; i<4; i++)
		{
			vWorld += vWeight[i] * transformPoint( vIndex[i], vPos);
			vNormal += vWeight[i] * transformNormal( vIndex[i], vNormalIn);
		}
	}
#else
	vWorld = transformPoint( 0, vPos);
	vNormal = transformNormal( 0, vNormalIn);
#endif

	vec4 vWorld4 = vec4( vWorld, 1.0);
	vec4 vClip = vec4(
		dot( vWorld4, u_tViewProj[0]),
		dot( vWorld4, u_tViewProj[1]),
		dot( vWorld4, u_tViewProj[2]),
		dot( vWorld4, u_tViewProj[3]));
	vec3 vView = vec3(
		dot( vWorld4, u_tView[0]),
		dot( vWorld4, u_tView[1]),
		dot( vWorld4, u_tView[2]));

	// Depth bias is in depth units, like D3D9's.
	vClip.z += vDepth.w * vClip.w;
	vClip.x += vClip.w / vViewport.z;
	vClip.y -= vClip.w / vViewport.w;

	if( vDepth.z > 0.5 )
		vClip.z = vClip.z * 2.0 - vClip.w;
	gl_Position = vClip;

	if( u_tVS[0].z > 0.5 )
	{
		vec4 vSources = u_tVS[1];
		vec4 vMatDiffuse = materialColor( vSources.x, u_tMaterial[0], vDiffuse, vSpecular, bDiffuse);
		vec4 vMatSpecular = materialColor( vSources.y, u_tMaterial[2], vDiffuse, vSpecular, bDiffuse);
		vec4 vMatAmbient = materialColor( vSources.z, u_tMaterial[1], vDiffuse, vSpecular, bDiffuse);
		vec4 vMatEmissive = materialColor( vSources.w, u_tMaterial[3], vDiffuse, vSpecular, bDiffuse);

		if( u_tVS[0].w > 0.5 && dot( vNormal, vNormal) > 0.0 )
			vNormal = normalize(vNormal);

		vec3 vColor = vMatEmissive.xyz + u_tVS[3].xyz * vMatAmbient.xyz;
		vec3 vSpec = vec3( 0.0, 0.0, 0.0);
		vec3 vEye = normalize( u_tVS[8].xyz - vWorld);

		for( int i=0; i<MAX_LIGHTS; i++)
		{
			vec4 vType = u_tLights[i * 7];

			if( vType.y < 0.5 )
				continue;

			vec3 vLight;
			float fAtten = 1.0;

			if( vType.x > 2.5 )
				vLight = -normalize(u_tLights[i * 7 + 2].xyz);
			else
			{
				vec3 vDelta = u_tLights[i * 7 + 1].xyz - vWorld;
				float fDist = length(vDelta);
				vec4 vAtt = u_tLights[i * 7 + 6];

				vLight = fDist > 0.0 ? vDelta / fDist : vec3( 0.0, 1.0, 0.0);
				fAtten = fDist > vType.z ? 0.0 : 1.0 / max( vAtt.x + vAtt.y * fDist + vAtt.z * fDist * fDist, 0.0001);
			}

			float fDot = max( dot( vNormal, vLight), 0.0);

			vColor += fAtten * (
				u_tLights[i * 7 + 5].xyz * vMatAmbient.xyz +
				u_tLights[i * 7 + 3].xyz * vMatDiffuse.xyz * fDot);

			if( u_tVS[2].y > 0.5 && fDot > 0.0 )
			{
				vec3 vHalf = normalize( vLight + vEye);
				vSpec += fAtten * u_tLights[i * 7 + 4].xyz * vMatSpecular.xyz * pow( max( dot( vNormal, vHalf), 0.0), u_tVS[2].w);
			}
		}

		v_color0 = clamp( vec4( vColor, vMatDiffuse.w), 0.0, 1.0);
		v_color1 = clamp( vec4( vSpec, vMatSpecular.w), 0.0, 1.0);
	}
	else
	{
		v_color0 = vDiffuse;
		v_color1 = vSpecular;
	}

	vec4 vFog = u_tVS[6];
	if( u_tVS[7].y > 0.5 && vFog.x > 0.5 )
	{
		float fDist = u_tVS[7].x > 0.5 ? length(vView) : vView.z;

		if( vFog.x > 2.5 )
			fFog = (vFog.z - fDist) / max( vFog.z - vFog.y, 0.0001);
		else if( vFog.x > 1.5 )
			fFog = exp( -(vFog.w * fDist) * (vFog.w * fDist));
		else
			fFog = exp( -vFog.w * fDist);
	}
#endif

	v_texcoord0 = vec4( stageCoord( 0, vSet0, vSet1), stageCoord( 1, vSet0, vSet1));
	v_texcoord1 = vec4( stageCoord( 2, vSet0, vSet1), stageCoord( 3, vSet0, vSet1));
	v_fog = clamp( fFog, 0.0, 1.0);

	if( u_tVS[7].z > 0.5 )
		gl_Position.y = -gl_Position.y;
}
