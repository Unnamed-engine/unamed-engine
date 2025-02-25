#version 450

layout(set = 0, binding = 0) uniform ViewUniforms {
    mat4 viewproj;
    vec3 pos;
} view;

layout(location = 0) out float near;     // Added for fragment shader
layout(location = 1) out float far;      // Added for fragment shader
layout(location = 2) out vec3 nearPoint; // Existing
layout(location = 3) out vec3 farPoint;  // Existing
layout(location = 4) out mat4 fragView;  // Added for fragment shader
layout(location = 8) out mat4 fragProj;  // Added for fragment shader

const vec3 pos[4] = vec3[4](
    vec3(-1.0, 0.0, -1.0),
    vec3(1.0, 0.0, -1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(-1.0, 0.0, 1.0)
);

const int indices[6] = int[6](0, 2, 1, 2, 0, 3);

void main() {
    int idx = indices[gl_VertexIndex];
    vec3 vertPos = pos[idx];
    vec4 vertPos4 = vec4(vertPos, 1.0);
    gl_Position = view.viewproj * vertPos4; 
}
