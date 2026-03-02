#pragma once

#include <string>

class Entity
{
	public:
		// method to render the entire level
		virtual void Create() = 0;
		virtual void Update(float elaspedTime) {};
		virtual void Draw2D() = 0;
		virtual void Release() = 0;

		bool isReady;
		string title;
		string description;

		Entity();
};


class SimpleDrawTextureDemo : public Entity
{
public:

	void Create() override;
	void Draw2D() override;
	void Release() override;

	SimpleDrawTextureDemo();

private:
	Texture2D texture = { 0 };
};

class CompressTextureDemo : public Entity
{
public:

	void Create() override;
	void Draw2D() override;
	void Release() override;

	CompressTextureDemo();

private:
	Texture2D original = { 0 };
	Texture2D compressed = { 0 };
};


class DrawPartialRotateDemo : public Entity
{
public:

	void Create() override;
	void Update(float elapsed) override;
	void Draw2D() override;
	void Release() override;

	DrawPartialRotateDemo();

private:
	Texture2D texture = { 0 };
	float currentAngle = 0.0f;
	float timeDiff = 0.0f;
};

class ColorBlendingDemo : public Entity
{
public:

	void Create() override;
	void Update(float elapsed) override;
	void Draw2D() override;
	void Release() override;

	ColorBlendingDemo();

protected:
	Texture2D texture = { 0 };
	int currentIdx = -1;
	float timeDiff = 0.0f;
};

class SmoothColorBlendingDemo : public ColorBlendingDemo
{
public:
	void Draw2D() override;

	SmoothColorBlendingDemo();
};

class SceneTransitionDemo : public Entity
{
public:

	void Create() override;
	void Update(float elapsed) override;
	void Draw2D() override;
	void Release() override;

	SceneTransitionDemo();

private:
	Texture2D scene1 = { 0 };
	Texture2D scene2 = { 0 };
	float currentProgress = 0.0f;
	float elapsed_time = 0.0f;
};

class AnimatedTexDemo : public Entity
{
public:

	void Create() override;
	void Update(float elapsed) override;
	void Draw2D() override;
	void Release() override;

	AnimatedTexDemo();

protected:
	Texture2D _spritesSheet = { 0 };
	float _currentIdx = -1;
	float _anim_time = 0.0f;
	float _anim_length = 1.0f;
};


class GlowDemo : public Entity
{
public:

	void Create() override;
	void Update(float elapsed) override;
	void Draw2D() override;
	void Release() override;

	GlowDemo();

private:
	Texture2D background = {0};
	Texture2D glow = { 0 };
	float currentAngle = 0.0f;
};

class NPatchDemo : public Entity
{
public:

	void Create() override;
	void Update(float elapsed) override;
	void Draw2D() override;
	void Release() override;

	NPatchDemo();

private:
	Texture2D _npatchTex = { 0 };
	NPatchInfo _ninePatchInfo = {0};
	Rectangle _dest = { 0 };
	float elapsed_time = 0.0f;
};
