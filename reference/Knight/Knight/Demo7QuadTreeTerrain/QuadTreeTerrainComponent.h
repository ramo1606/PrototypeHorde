#pragma once

#include <vector>
#include "Component.h"
#include "SceneActor.h"

#include "rlgl.h"

// Simple Quadtree Node
struct QuadTreeNode {
    BoundingBox bounds;       // Axis-Aligned Bounding Box for this node's area
    QuadTreeNode* children[4]; // Pointers to child nodes (null if leaf)
    Vector2 center;           // Center XZ coordinate of the node
    float size;               // Size (width/depth) of the node's square area
    int depth;                // Depth in the tree (0 = root)
    bool isLeaf;              // Is this node a leaf?

    QuadTreeNode(BoundingBox b, int d) : bounds(b), depth(d), isLeaf(true) {
        for (int i = 0; i < 4; ++i) {
            children[i] = nullptr;
        }
        // Calculate center and size from bounds
        center.x = bounds.min.x + (bounds.max.x - bounds.min.x) / 2.0f;
        center.y = bounds.min.z + (bounds.max.z - bounds.min.z) / 2.0f; // Using y for Z map coordinate
        size = bounds.max.x - bounds.min.x; // Assuming square nodes for simplicity in quadtree division
    }

    // Recursive destructor
    ~QuadTreeNode() {
        for (int i = 0; i < 4; ++i) {
            delete children[i]; // This will recursively delete children
            children[i] = nullptr;
        }
    }
};

class QuadTreeTerrainComponent : public Component
{
public:

    // These will be set by the loaded image
    int HeightMapWidth = 256; // Default, will be overridden
    int HeightMapDepth = 256; // Default, will be overridden
    int NumTriangles = 0;
    bool DebugShowBounds = false; // Toggle for drawing bounding boxes

    const int MaxQuadTreeDepth = 7; // Max depth of the quadtree (adjust as needed for map size)
    const float LevelOfDetailDistance = 4.5f; // Lower value = subdivide sooner (higher detail)

    QuadTreeTerrainComponent();
	~QuadTreeTerrainComponent();

    virtual bool CreateFromFile(Vector3 terrainDimension, Vector2 texTileSize, const char* pHightmapFilePath, const char* pTerrainTexurePath);

	void Update(float ElapsedSeconds, RenderHints* pRH = nullptr) override;
	void Draw(RenderHints *pRH = nullptr) override;

    float GetHeightmapValue(int x, int z);
    float GetTerrainY(float x, float y);
    Vector3 GetHeightmapNormal(int x, int y);
    Vector3 GetSmoothedNormal(float x, float z);

    int NumMaterial = 0;
	Material* materials = nullptr; // Array of materials for the terrain

	friend SceneActor;

protected:

    QuadTreeNode* rootNode = nullptr; // Root of the quadtree
    vector<float> heightmap; // Stores normalized height values (0.0 to 1.0)
    vector<Vector3> heightMapNormals; // store normal of each heightmap pixel 
    Texture2D terrainTexture = { 0 };
    Vector2 tilingFactor = { 1.0f, 1.0f }; // How many times the texture repeats across the terrain
    Vector3 terrainScale = { 2.0f, 35.0f, 2.0f }; // Scale for X, Z and Y axes

    bool LoadHeightmapFromImage(const char* fileName);
    void BuildQuadtreeNode(QuadTreeNode* node);
    void DrawQuadtreeNode(QuadTreeNode* node, SceneCamera *pCam, bool drawBounds, const FrustumPlane frustumPlanes[6]);
    void DrawTerrainChunk(QuadTreeNode* node);
};





