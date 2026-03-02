
#pragma once
#include "Component.h"
#include "raylib.h"

struct Model;
struct Color;

class CylinderComponent : public Component
{
public:
	float Radius;
	float Height;
	int Slices;

	CylinderComponent();
	~CylinderComponent();

	void Update(float ElapsedSeconds, RenderHints* pRH = nullptr) override;
	void Draw(RenderHints* pRH = nullptr) override;

	void SetColor(Color Color);
	Color GetColor();

protected:
	Mesh _Mesh;
	Material _Material;
	float _Radius;
	float _Height;
	int _Slices;
};