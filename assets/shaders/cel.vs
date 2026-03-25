#version 330

/*
 * cel.vs — Cel Shading Vertex Shader
 *
 * Transforms vertex position to clip space (standard) and the normal
 * to world space (for lighting in the fragment shader).
 *
 * Raylib sets these automatically when the proper SHADER_LOC is configured:
 *   mvp      — model × view × projection (always set by raylib)
 *   matModel — model matrix (set if SHADER_LOC_MATRIX_MODEL is registered)
 *
 * Why we need matModel separately from mvp:
 *   mvp transforms positions to clip space, but normals need to stay in
 *   world space for the lighting calculation (dot product with a world-space
 *   light direction). We use mat3(matModel) to transform normals.
 *
 *   Technically, normals should be transformed by the inverse-transpose of
 *   the model matrix. For uniform scale (which is all we use with KayKit
 *   assets), mat3(matModel) is equivalent and much cheaper. If we ever
 *   use non-uniform scale, this needs to change.
 */

/* ── Vertex attributes (raylib standard locations) ─────────────────────── */
in vec3 vertexPosition;     /* location 0 */
in vec2 vertexTexCoord;     /* location 1 */
in vec3 vertexNormal;       /* location 2 */
in vec4 vertexColor;        /* location 3 */

/* ── Uniforms (set by raylib) ──────────────────────────────────────────── */
uniform mat4 mvp;           /* model-view-projection matrix               */
uniform mat4 matModel;      /* model matrix (world transform)             */

/* ── Outputs to fragment shader ────────────────────────────────────────── */
out vec3 fragWorldNormal;   /* Normal in world space (for lighting)       */
out vec2 fragTexCoord;      /* Pass-through UV coordinates                */
out vec4 fragVertColor;     /* Pass-through vertex color (KayKit uses it) */

void main()
{
    /* Transform normal to world space.
     * mat3(matModel) strips the translation column, leaving only
     * rotation + scale. For uniform scale this is correct. */
    fragWorldNormal = mat3(matModel) * vertexNormal;

    fragTexCoord = vertexTexCoord;
    fragVertColor = vertexColor;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
