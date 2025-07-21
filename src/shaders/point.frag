#version 450

layout(location = 0) out vec4 outColor;

void main() {
    // Normalize coordinates from center
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float dist = dot(coord, coord);

    // Hard cutoff (keep this for shape)
    if (dist > 1.0) {
        discard;
    }

    // Smoothing the edge for anti-aliasing
    float radius = 1.0;
    float feather = 0.6; // tweak for softer or harder edge

    // Distance from edge (0 at edge, negative inside, positive outside)
    float edgeDist = sqrt(dist) - radius;
    float alpha = 1.0 - smoothstep(0.0, feather, edgeDist);

    outColor = vec4(1.0, 0.2, 0.6, alpha);
}
