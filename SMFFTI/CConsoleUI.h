#pragma once
#include <functional>
#include <conio.h>
#include "Common.h"

//-----------------------------------------------------------------------------
class Prompt
{
public:
	uint32_t nSeq;
	std::string sPrompt;
	std::string sAnswer;
	std::string sDefault;

	uint8_t nType = 0; 
		// 0 = Text
		// 1 = Integer
		// 2 = Float
		// 3 = Y/N
		// 4 = Form Completed confirmation

	int32_t nTextMinLen = 0;
	int32_t nTextMaxLen = 0;

	int32_t nIntRangeFrom = 0;
	int32_t nIntRangeTo = 0;

	double nFloatRangeFrom = 0.0;
	double nFloatRangeTo = 0.0;

	// Constructor for text and integers.
	Prompt (uint32_t a_nSeq, uint8_t nt, const std::string& p, int32_t min, int32_t max, std::string a_sDefault = "");

	// Constructor for decimals.
	Prompt (uint32_t a_nSeq, uint8_t nt, const std::string& p, double min, double max, std::string a_sDefault = "");

	// Constructor for Y/N prompt
	Prompt (uint32_t a_nSeq, uint8_t nt, const std::string& p, std::string a_sDefault);

	// Constructor for Form Completed prompt
	Prompt (uint32_t a_nSeq, uint8_t nt, const std::string& p);

};
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
struct Form
{
	std::vector<Prompt> vPrompts;
	std::function<void()> fnAction;
	std::function<bool(Prompt& pr, std::string& sInput)> fnValidation;
	uint32_t nLastFieldCompleted = 0;

	Form() {}

	static void FormInput (Form& form);

	void AddTextPrompt (uint32_t nSeq, const std::string& prompt, int32_t minLen, int32_t maxLen, const std::string& default);
	void AddIntegerPrompt (uint32_t nSeq, const std::string& prompt, int32_t min, int32_t max, const std::string& default);
	void AddDecimalPrompt (uint32_t nSeq, const std::string& prompt, double min, double max, const std::string& default);
	void AddYNPrompt (uint32_t nSeq, const std::string& prompt, const std::string& default);
	void FormCompletedPrompt (uint32_t nSeq, const std::string& prompt);
	void FormAction (std::function<void()> f);
	void Validation (std::function<bool (Prompt& pr, std::string& sInput)> f);

	void ResetForm();
};
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
class MenuItemDescriptor
{
public:
	MenuItemDescriptor() {}

	// Constructor to specifiy an action.
	MenuItemDescriptor (const std::string& s, std::function<void()> f) :
		sChoice (s), fnAction (f), nType (0) {}

	// Constructor to specify another menu.
	MenuItemDescriptor (const std::string& s, uint32_t im) :
		sChoice (s), iMenu (im), nType (1) {}

	std::string sChoice;

	std::function<void ()> fnAction;
	int nType = 0;	// 0 = Action, 1 = Another Menu Page.
	uint32_t iMenu = 0;


};
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// Everything pertaining to a given menu.
struct MenuDescriptor
{
	uint32_t nChoice = 0;

	void AddMenuLink (const std::string& title, uint32_t menuId);

	void AddActionLink (const std::string& name, std::function<void ()>);
	void AddFormLink (const std::string& formName, Form& form);

	std::vector<MenuItemDescriptor> vMenuItems;
};


//-----------------------------------------------------------------------------
class CConsoleUI
{
public:
	CConsoleUI();
	void Run();

protected:
	void UserInterface (uint32_t iMenu);

	std::map<uint32_t, MenuDescriptor> mMenus;
	void AddMenu (uint32_t menuId, MenuDescriptor& menu);

	void Quit() { bQuit = true; }
	bool bQuit;

	// Sub class must implement this to init menus/pages.
	virtual void InitUI() = 0;

};

