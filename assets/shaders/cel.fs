#version 330

/*
 * cel.fs — Cel Shading Fragment Shader
 *
 * Computes a quantized (banded) diffuse lighting model:
 *   1. Dot product of world-space normal with light direction → intensity
 *   2. Clamp to [0, 1]
 *   3. Quantize into discrete bands (floor)
 *   4. Mix with ambient to avoid pitch-black shadows
 *   5. Multiply by base color (texture × vertex color × material tint)
 *
 * Custom uniforms (set by the renderer each frame):
 *   lightDir    — normalized direction TOWARD the light (world space)
 *   ambient     — minimum light level (0.0 = pitch black shadows, 1.0 = no shading)
 *   numBands    — number of discrete light bands (3-4 is typical cel look)
 *
 * Raylib-managed uniforms:
 *   colDiffuse  — material diffuse tint color (RGBA, 0-1 range)
 *   texture0    — diffuse texture (1x1 white if model has no texture)
 */

/* ── Inputs from vertex shader ─────────────────────────────────────────── */
in vec3 fragWorldNormal;
in vec2 fragTexCoord;
in vec4 fragVertColor;

/* ── Uniforms (raylib-managed) ─────────────────────────────────────────── */
uniform sampler2D texture0;     /* Diffuse texture map                    */
uniform vec4 colDiffuse;        /* Material diffuse color tint            */

/* ── Uniforms (custom, set by renderer) ────────────────────────────────── */
uniform vec3  lightDir;         /* Normalized direction toward the light  */
uniform float ambient;          /* Minimum light level [0..1]             */
uniform float numBands;         /* Number of cel-shading bands (e.g. 3)  */

/* ── Output ────────────────────────────────────────────────────────────── */
out vec4 finalColor;

void main()
{
    /* ── 1. Base color ────────────────────────────────────────────────── */
    /*
     * Combine all color sources:
     *   texture — the diffuse map (white if none)
     *   vertex color — KayKit models store color here (palette-style)
     *   colDiffuse — raylib's material tint (usually white)
     *
     * Multiplying them together means any one can tint the result,
     * and if any is "neutral" (white/1.0) it has no effect.
     */
    vec4 texColor = texture(texture0, fragTexCoord);
    vec4 baseColor = texColor * fragVertColor * colDiffuse;

    /* ── 2. Diffuse lighting ──────────────────────────────────────────── */
    /*
     * Classic Lambert: NdotL = dot(normal, lightDir).
     * We normalize fragWorldNormal because interpolation across the
     * triangle can denormalize it slightly.
     * Clamp to [0,1]: negative values mean the surface faces away
     * from the light (in shadow).
     */
    vec3 N = normalize(fragWorldNormal);
    float NdotL = max(dot(N, lightDir), 0.0);

    /* ── 3. Quantize to bands ─────────────────────────────────────────── */
    /*
     * floor(NdotL * numBands) gives a stepped integer (0, 1, 2, ...),
     * dividing by numBands maps it back to [0, 1) in discrete steps.
     *
     * With numBands=3:  NdotL 0.0..0.33 → 0.0
     *                   NdotL 0.33..0.66 → 0.33
     *                   NdotL 0.66..1.0  → 0.66
     *
     * We add a small bump (+0.5/numBands) to the result so bands are
     * centered rather than starting at 0. This avoids the darkest band
     * being pure black (which looks flat and loses detail).
     * With the bump, band values become ~0.17, 0.5, 0.83 for 3 bands.
     */
    float band = floor(NdotL * numBands) / numBands;
    band += 0.5 / numBands;

    /* ── 4. Combine with ambient ──────────────────────────────────────── */
    /*
     * Mix: at minimum the fragment gets 'ambient' light,
     * and the banded lighting adds on top, scaled by (1 - ambient)
     * so total never exceeds 1.0.
     *
     * ambient=0.2, band=0.83 → light = 0.2 + 0.83*0.8 = 0.86
     * ambient=0.2, band=0.17 → light = 0.2 + 0.17*0.8 = 0.34
     */
    float light = ambient + band * (1.0 - ambient);

    /* ── 5. Final output ──────────────────────────────────────────────── */
    finalColor = vec4(baseColor.rgb * light, baseColor.a);
}
