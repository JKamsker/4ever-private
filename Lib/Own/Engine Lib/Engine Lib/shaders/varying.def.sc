vec4 v_color0    : COLOR0    = vec4(1.0, 1.0, 1.0, 1.0);
vec4 v_color1    : COLOR1    = vec4(0.0, 0.0, 0.0, 0.0);
vec4 v_texcoord0 : TEXCOORD0 = vec4(0.0, 0.0, 0.0, 0.0);
vec4 v_texcoord1 : TEXCOORD1 = vec4(0.0, 0.0, 0.0, 0.0);
float v_fog      : TEXCOORD2 = 1.0;

vec4 a_position  : POSITION;
vec4 a_weight    : BLENDWEIGHT;
vec4 a_indices   : BLENDINDICES;
vec3 a_normal    : NORMAL;
vec4 a_color0    : COLOR0;
vec2 a_texcoord0 : TEXCOORD0;
vec2 a_texcoord1 : TEXCOORD1;
