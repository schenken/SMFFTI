#include "pch.h"
#include "CConsoleUI.h"
#include "Common.h"
#include <iomanip> // For setw and setfill


//-----------------------------------------------------------------------------
CConsoleUI::CConsoleUI() : bQuit (false)
{
	bQuit;
}

void CConsoleUI::Run()
{
	InitUI();
	UserInterface (0);
}

void CConsoleUI::UserInterface (uint32_t _menuId)
{
	uint32_t curMenuId = _menuId;

	// Lambda
	auto UI = [&](uint32_t menuId)
	{
		int result = 0;

		std::map<uint32_t, MenuDescriptor>::iterator itMenu;
		itMenu = mMenus.find (menuId);
		ASSERT (itMenu != mMenus.end());
		MenuDescriptor& menu = itMenu->second;

		bool bValid = false;
		while (!bValid)
		{
			system ("cls");
			uint32_t nIndex = 0;

			for (auto s : menu.vMenuItems)
			{
				if (nIndex == 0)
				{
					// Menu heading
					std::cout << std::endl;
					std::cout << s.sChoice << std::endl << std::endl;
					nIndex = 1;
					continue;
				}

				std::cout << std::setw (2);
				std::cout << std::setfill(' ');
				std::cout << "   " << nIndex++ << ". " << s.sChoice << std::endl;
			}

			std::cout << std::endl;
			std::cout << "Choose a menu option (/q to quit): ";
			std::string sInput;
			getline (std::cin, sInput);

			if (sInput == "/Q" || sInput == "/q")
			{
				return 1;
			}

			try
			{
				menu.nChoice = std::stoi (sInput);

				if (menu.nChoice >= 0 && menu.nChoice < (int)menu.vMenuItems.size())
					bValid = true;
			}
			catch (...) {}

			if (!bValid)
			{
				std::cout << "Error! Invalid option." << std::endl;
				Sleep (1000);
				continue;
			}

			MenuItemDescriptor& mid = menu.vMenuItems[menu.nChoice];

			// Execute the action
			if (bValid)
			{
				if (mid.nType == 0)
				{
					mid.fnAction();
				}
				else if (mid.nType == 1)
				{
					curMenuId = mid.iMenu;
				}
			}

			if (bQuit)
				return 1;
		}

		return result;
	};

	int r = 0;
	while (r != 1)
		r = UI (curMenuId);

}

void CConsoleUI::AddMenu(uint32_t menuId, MenuDescriptor& menu)
{
	mMenus.insert (std::pair<uint32_t, MenuDescriptor>(menuId, menu));
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
void MenuDescriptor::AddMenuLink (const std::string& title, uint32_t menuId)
{
	vMenuItems.push_back (MenuItemDescriptor (title, menuId));
}

void MenuDescriptor::AddActionLink (const std::string& name, std::function<void ()> fn)
{
	vMenuItems.push_back (MenuItemDescriptor (name, fn));
}

void MenuDescriptor::AddFormLink(const std::string& formName, Form& form)
{
	vMenuItems.push_back (MenuItemDescriptor (formName, [&](){ Form::FormInput (form); }));
}
//-----------------------------------------------------------------------------





//-----------------------------------------------------------------------------
void Form::FormInput (Form& form)
{
	system ("cls");

	form.ResetForm();
	std::cout << std::endl << form.vPrompts[0].sPrompt << std::endl << std::endl;

	std::cout << "      (To quit the form enter /q)" << std::endl << std::endl;

	std::string sInput;
	for (size_t i = 1; i < form.vPrompts.size(); i++)
	{
		uint32_t nIndex = i;
		bool bField = false;
		bool bLatest = false;
		Prompt& pr = form.vPrompts[i];

		auto Ask = [&]()
		{
			std::string s;
			std::cout << std::setw (2);
			std::cout << std::setfill(' ');
			std::cout << "   " << nIndex << ". " << pr.sPrompt;
			if (pr.sAnswer.length())
				std::cout << " <" << pr.sAnswer << ">";
			std::cout << "? ";
			getline (std::cin, sInput);
		};

		// Validate the input...
		bool bValid = false;
		while (!bValid)
		{
			Ask();

			sInput = akl::RemoveWhitespace (sInput, 3);		// strip leading/trailing spaces.

			// If input is blank, use existing answer if present.
			if (!sInput.length())
				sInput = pr.sAnswer;

			// Control input
			if (sInput.length() == 2 && sInput[0] == '/')
			{
				if (sInput[1] == 'q')
				{
					std::cout << "      Are you sure you want to quit (Y/N)? ";
					getline (std::cin, sInput);
					std::transform (sInput.begin(), sInput.end(), sInput.begin(), ::toupper);
					if (sInput == "Y" || sInput == "YES")
						return;
					continue;	// anything other than Y: repeat the prompt
				}
				else if (sInput[1] == 'b')	// Previous prompt
				{
					bField = true;
					i -= (i <= 1) ? 1 : 2;
					break;
				}
				else if (sInput[1] == 'l')	// latest uncompleted field.
				{
					bLatest = true;
					i = form.nLastFieldCompleted;
					break;
				}
			}

			// Go back to specific prompt
			if (sInput.length() >= 2 && sInput[0] == '/')
			{
				std::string s = sInput.substr (1);
				uint32_t n;
				bool bNum = false;
				bool bOutOfRange = false;
				try
				{
					n = std::stoi (s);
					bNum = true;

					if (n < 1 || n >= form.vPrompts.size())
					{
						bOutOfRange = true;
					}
					else if (n >= 1 && n <= (form.nLastFieldCompleted + 1))
					{
						bField = true;
						i = n - 1;
						break;
					}
				}
				catch (...) {}

				if (bOutOfRange)
				{
					std::cout << "      Invalid option." << std::endl;
				}
				else if (bNum)
				{
					std::cout << "      You cannot jump to this question yet." << std::endl;
				}
				else
				{
					std::cout << "      Invalid input." << std::endl;
				}
				continue;
			}
	
			if (pr.nType == 0)		// Text
			{
				int32_t nLen = sInput.length();
				if (nLen < pr.nTextMinLen || nLen > pr.nTextMaxLen)
				{
					std::cout << "      Input must be " << pr.nTextMinLen << " to " << pr.nTextMaxLen << " characters." << std::endl;
					continue;
				}
			}
			else if (pr.nType == 1)		// Number
			{
				int32_t n;
				if (!akl::VerifyTextInteger (sInput, n, pr.nIntRangeFrom, pr.nIntRangeTo))
				{
					std::cout << "      Input must be in range " << pr.nIntRangeFrom << " - " << pr.nIntRangeTo << "." << std::endl;
					continue;
				}
			}
			else if (pr.nType == 2)		// Decimal
			{
				double n;
				if (!akl::VerifyDoubleInteger (sInput, n, pr.nFloatRangeFrom, pr.nFloatRangeTo))
				{
					std::cout << "      Input must be in range " << pr.nFloatRangeFrom << " - " << pr.nFloatRangeTo << "." << std::endl;
					continue;
				}
			}
			else if (pr.nType == 3)		// Y or N
			{
				std::transform (sInput.begin(), sInput.end(), sInput.begin(), ::toupper);
				if (sInput != "Y" && sInput != "N")
				{
					std::cout << "      Please specify Y or N." << std::endl;
					continue;
				}
			}
			else if (pr.nType == 4)		// Form completed - confirm
			{
				std::transform (sInput.begin(), sInput.end(), sInput.begin(), ::toupper);
				if (sInput != "Y")
				{
					std::cout << "      Please enter Y to proceed." << std::endl;
					continue;
				}
				sInput = "";	// don't save - user must be explicit each time
			}

			bValid = true;

			// Additional, custom validation
			if (form.fnValidation)
			{
				if (pr.nType != 4)
					bValid = form.fnValidation (pr, sInput);
			}
		}

		if (bField)
			continue;

		if (bLatest)
			continue;

		pr.sAnswer = sInput;
		if (i > form.nLastFieldCompleted)
			form.nLastFieldCompleted = i;
	}

	form.fnAction();

}

void Form::AddTextPrompt (uint32_t nSeq, const std::string& prompt, int32_t minLen, int32_t maxLen, const std::string& default)
{
	vPrompts.push_back (Prompt (nSeq, 0, prompt, minLen, maxLen, default));
}

void Form::AddIntegerPrompt (uint32_t nSeq, const std::string& prompt, int32_t min, int32_t max, const std::string& default)
{
	vPrompts.push_back (Prompt (nSeq, 1, prompt, min, max, default));
}

void Form::AddDecimalPrompt (uint32_t nSeq, const std::string& prompt, double min, double max, const std::string& default)
{
	vPrompts.push_back (Prompt (nSeq, 2, prompt, min, max, default));
}

void Form::AddYNPrompt (uint32_t nSeq, const std::string& prompt, const std::string& default)
{
	vPrompts.push_back (Prompt (nSeq, 3, prompt, default));
}

void Form::FormCompletedPrompt (uint32_t nSeq, const std::string& prompt)
{
	vPrompts.push_back (Prompt (nSeq, 4, prompt));
}

void Form::FormAction (std::function<void()> f)
{
	fnAction = f;
}

void Form::Validation (std::function<bool(Prompt& pr, std::string& sInput)> f)
{
	fnValidation = f;
}

void Form::ResetForm()
{
	for (auto& p : vPrompts)
		p.sAnswer = p.sDefault;
	nLastFieldCompleted = 0;
}
//-----------------------------------------------------------------------------







//-----------------------------------------------------------------------------
Prompt::Prompt (uint32_t a_nSeq, uint8_t nt, const std::string& p, int32_t min, int32_t max, std::string a_sDefault)
{
	nSeq = a_nSeq;
	nType = nt;

	if (nType == 0)
	{
		nTextMinLen = min;
		nTextMaxLen = max;
	}
	else // assumed to be 1
	{
		nIntRangeFrom = min;
		nIntRangeTo = max;
	}

	sPrompt = p;
	sDefault = a_sDefault;
}

Prompt::Prompt (uint32_t a_nSeq, uint8_t nt, const std::string& p, double min, double max, std::string a_sDefault)
{
	nSeq = a_nSeq;
	nType = 2;
	sPrompt = p;

	nFloatRangeFrom = min;
	nFloatRangeTo = max;

	sDefault = a_sDefault;
}

Prompt::Prompt (uint32_t a_nSeq, uint8_t nt, const std::string& p, std::string a_sDefault)
{
	nSeq = a_nSeq;
	nType = 3;
	sPrompt = p;
	sDefault = a_sDefault;
}

Prompt::Prompt (uint32_t a_nSeq, uint8_t nt, const std::string& p)
{
	nSeq = a_nSeq;
	nType = 4;
	sPrompt = p;
}
//-----------------------------------------------------------------------------

