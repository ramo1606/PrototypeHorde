#pragma once

#include "UIPopup.h"

class InGameDialogue : public UIPopup
{
public:
	virtual void Create(UIPopupManager*);
	virtual void Update(float interval);
	virtual void Draw();
};

class SettingsDialogue : public UIPopup
{
public:
	virtual void Create(UIPopupManager*);
	virtual void Update(float interval);
	virtual void Draw();
};

class SoundDialogue : public UIPopup
{
public:
	virtual void Create(UIPopupManager*);
	virtual void Update(float interval);
	virtual void Draw();
	SoundDialogue();
protected:
	bool _toggleStatus;
};

class Demo2eUIManager : public UIPopupManager
{
public:
	void Create() override;
	UIPopup* GetPopup(const char* name);
protected:
	InGameDialogue Menu;
	SettingsDialogue Settings;
	SoundDialogue Sound;
};

