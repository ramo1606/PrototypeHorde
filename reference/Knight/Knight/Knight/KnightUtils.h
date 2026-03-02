#pragma once

#include "Knight.h"

struct BoundingRect {
    Vector2 min;
    Vector2 max;
};

struct Vector3Compare {
	bool operator()(const Vector3& a, const Vector3& b) const
	{
		if (a.x != b.x) return a.x < b.x;
		if (a.y != b.y) return a.y < b.y;
		return a.z < b.z;
	}
};

extern Rectangle& CenterRectangle(Rectangle& r, int width, int height);
extern float DegreesToRadians(float degrees);
extern BoundingRect Get2DBoundingRectOfCube(const Vector3& cubePosition, float cubeSize, const Camera3D& camera);
extern void RecalculateSmoothNormals(const Model& model);
extern void ConvertMeshToIndexed(Mesh* mesh);
extern bool IsPointInTriangle2D(Vector2 p, Vector2 v0, Vector2 v1, Vector2 v2);

// Structure to hold the result of the check
struct PointInTriangleResult {
	bool isInTriangle;
	float calculatedPy;
};

extern PointInTriangleResult DetectPointInTriangleAndCalcY(Vector3 v0, Vector3 v1, Vector3 v2,
	Vector3 testPoint);

extern bool __cdecl Camera3DEquals(const Camera3D& a, const Camera3D& b, float epsilon = 0.0001f);

extern BoundingBox __cdecl GetBoundingBoxUnion(BoundingBox box1, BoundingBox box2);

extern float PointToBoxDistanceSqr(Vector3 p, BoundingBox box);

extern bool IsBoundingBoxValid(BoundingBox box);

//End of KnightUtils.h