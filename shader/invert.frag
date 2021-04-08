uniform sampler2D sTex;

varying vec2 v_texCoord;

void main() {
    vec4 sPixel = texture2D(sTex, v_texCoord);
    gl_FragColor = vec4(1.0 - sPixel.r, 1.0 - sPixel.g, 1.0 - sPixel.b, sPixel.a);
}
