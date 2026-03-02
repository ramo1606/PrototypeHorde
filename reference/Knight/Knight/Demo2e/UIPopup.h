#pragma once

#include <vector>
#include <stack>

class UIPopupManager;

class UIPopup
{
public:
	virtual void Create(UIPopupManager *);
	virtual void Update(float interval) = 0;
	virtual void Draw() = 0;

protected:
	static UIPopupManager* ui_manager;
};

class UIPopupManager
{
public:
	virtual void Create() = 0;
	virtual void Update(float interval);
	virtual void Draw();

	void Show(UIPopup *);
	void Show(const char *popupName);
	void Close();

	bool IsAnyPopupShown();

	virtual UIPopup* GetPopup(const char *name) = 0;

protected:
	std::stack<UIPopup*> history;
};


