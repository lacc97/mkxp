uniform sampler2D sTex;
uniform sampler2D dTex;

uniform vec4 posData;

uniform lowp float opacity;

varying vec2 v_texCoord;

#define BLEND_NORMAL    0x00
#define BLEND_ADD       0x01
#define BLEND_SUB       0x02
#define BLEND_MUL       0x03
#define BLEND_DODGE     0x04
#define BLEND_BURN      0x05
#define BLEND_SCREEN    0x06
#define BLEND_OVERLAY   0x07

vec4 blend_alpha(vec4 d, vec4 s) {
    vec4 p;
    p.rgb = (s.rgb * opacity) + (d.rgb * (1.0 - opacity));
    p.a = d.a;

    return p;
}

vec4 blend_add(vec4 d, vec4 s) {
    s.a *= opacity;
    if(s.a == 0.0)
        return d;
    if(d.a == 0.0)
        return s;

    d.rgb *= min(d.a/s.a, 1.0);
    s.rgb *= min(s.a/d.a, 1.0);

    vec4 p;
    p.rgb = min(d.rgb + s.rgb, vec3(1.0));
    p.a = max(d.a, s.a);

    return p;
}

vec4 blend_sub(vec4 d, vec4 s) {
    s.a *= opacity;
    if(s.a == 0.0)
        return d;
    if(d.a == 0.0)
        return vec4(0.0, 0.0, 0.0, s.a);

    d.rgb *= min(d.a/s.a, 1.0);
    s.rgb *= min(s.a/d.a, 1.0);

    vec4 p;
    p.rgb = max(d.rgb - s.rgb, vec3(0.0));
    p.a = max(d.a, s.a);

    return p;
}

vec4 blend_mul(vec4 d, vec4 s) {
    d.rgb *= d.a;
    s.rgb *= s.a;

    vec4 p;
    p.rgb = d.rgb*s.rgb;
    p.a = 1.0;

    return p;
}

float blend_dodge(float d, float s) {
    return (s == 1.0) ? s : min(d / (1.0 - s), 1.0);
}
vec4 blend_dodge(vec4 d, vec4 s) {
    d.rgb *= d.a;
    s.rgb *= s.a;

    vec4 p;
    p.rgb = vec3(blend_dodge(d.r, s.r), blend_dodge(d.g, s.g), blend_dodge(d.b, s.b));
    p.a = 1.0;

    return p;
}

float blend_burn(float d, float s) {
    return (s == 0.0) ? s : max((1.0 - ((1.0 - d) / s)), 0.0);
}
vec4 blend_burn(vec4 d, vec4 s) {
    d.rgb *= d.a;
    s.rgb *= s.a;

    vec4 p;
    p.rgb = vec3(blend_burn(d.r, s.r), blend_burn(d.g, s.g), blend_burn(d.b, s.b));
    p.a = 1.0;

    return p;
}

float blend_screen(float d, float s) {
    return 1.0 - ((1.0 - d)*(1.0 - s));
}
vec4 blend_screen(vec4 d, vec4 s) {
    d.rgb *= d.a;
    s.rgb *= s.a;

    vec4 p;
    p.rgb = vec3(blend_screen(d.r, s.r), blend_screen(d.g, s.g), blend_screen(d.b, s.b));
    p.a = 1.0;

    return p;
}

float blend_overlay(float d, float s) {
    return d < 0.5 ? (2.0 * d * s) : (1.0 - 2.0 * (1.0 - d)*(1.0 - s));
}
vec4 blend_overlay(vec4 d, vec4 s) {
    d.rgb *= d.a;
    s.rgb *= s.a;

    vec4 p;
    p.rgb = vec3(blend_overlay(d.r, s.r), blend_overlay(d.g, s.g), blend_overlay(d.b, s.b));
    p.a = 1.0;

    return p;
}

void main() {
    vec2 sCoord = v_texCoord;
    vec2 dCoord = (sCoord - posData.xy) * posData.zw;

    vec4 sFrag = texture2D(sTex, sCoord);
    vec4 dFrag = texture2D(dTex, dCoord);

#if BLEND_TYPE == BLEND_NORMAL
    gl_FragColor = blend_alpha(dFrag, sFrag);
#elif BLEND_TYPE == BLEND_ADD
    gl_FragColor = blend_add(dFrag, sFrag);
#elif BLEND_TYPE == BLEND_SUB
    gl_FragColor = blend_sub(dFrag, sFrag);
#elif BLEND_TYPE == BLEND_MUL
    gl_FragColor = blend_mul(dFrag, sFrag);
#elif BLEND_TYPE == BLEND_DODGE
    gl_FragColor = blend_dodge(dFrag, sFrag);
#elif BLEND_TYPE == BLEND_BURN
    gl_FragColor = blend_burn(dFrag, sFrag);
#elif BLEND_TYPE == BLEND_SCREEN
    gl_FragColor = blend_screen(dFrag, sFrag);
#elif BLEND_TYPE == BLEND_OVERLAY
    gl_FragColor = blend_overlay(dFrag, sFrag);
#else
    gl_FragColor = blend_alpha(dFrag, sFrag);
#endif
}