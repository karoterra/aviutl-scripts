precision mediump float;
uniform float time;
uniform float totalTime;
uniform int frame;
uniform int totalFrame;
uniform vec2 resolution;
uniform vec4 track;

void main(void) {
    vec2 p = (gl_FragCoord.xy * 2.0 - resolution) / min(resolution.x, resolution.y);
    float size = sin(time) * 0.5 + 0.5;
    if (length(p) < size) {
        gl_FragColor = track / 100.0;
    } else {
        gl_FragColor = vec4(vec3(0.0), 0.0);
    }
}
