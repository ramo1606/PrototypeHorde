
#pragma once
#include "Component.h"
#include "raylib.h"

struct Model;
struct Color;

class SphereComponent : public Component
{
public:
	float Radius;
	int Rings;
	int Slices;

	SphereComponent();
	~SphereComponent();

	void Update(float ElapsedSeconds, RenderHints* pRH = nullptr) override;
	void Draw(RenderHints* pRH = nullptr) override;

	void SetColor(Color Color);
	Color GetColor();

protected:
	Mesh _Mesh;
	Material _Material;
	float _Radius;
	int _Rings;
	int _Slices;
};