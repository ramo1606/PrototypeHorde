#include "UIPopup.h"

void UIPopup::Create(UIPopupManager* uim)
{
	ui_manager = uim;
}

void UIPopupManager::Show(UIPopup* pp)
{
	history.push(pp);
}

void UIPopupManager::Show(const char* popupName)
{
	UIPopup* uip = GetPopup(popupName);
	if (uip != NULL)
		Show(uip);
}

void UIPopupManager::Close()
{
	if (history.size() > 0) {
		history.pop();
	}
}

void UIPopupManager::Draw()
{
	if (history.size())
	{
		history.top()->Draw();
	}
}

void UIPopupManager::Update(float interval)
{
	if (history.size()) 
	{
		history.top()->Update(interval);
	}
}

bool UIPopupManager::IsAnyPopupShown()
{
	return (history.size() > 0);
}

