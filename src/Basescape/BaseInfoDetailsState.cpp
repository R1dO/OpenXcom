/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "BaseInfoDetailsState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include <utility>

namespace OpenXcom
{

/**
 * Initializes all elements in the base info category breakdown subwindow.
 *
 * @param base Pointer to the base to get info from.
 * @param currentCategory Category to open
 */
BaseInfoDetailsState::BaseInfoDetailsState(Base *base, BaseInfoDetailsCategory currentCategory) : _base(base), _category(currentCategory)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 320, 184, 0, 8, POPUP_BOTH); // TransferState Style
	//_window = new Window(this, 320, 162, 0, 28, POPUP_BOTH); // ManufactureStart Style with 'use title from previous screen' GUI trickery.
	_btnOk = new TextButton(148, 16, 164, 169);
	_btnAllBases = new ToggleTextButton(148, 16, 9, 169);
	_btnPrev = new TextButton(28, 14, 8, 18);
	_btnNext = new TextButton(28, 14, 284, 18);
	_txtTitle = new Text(278, 17, 21, 18);
	_txtSource = new Text(114, 9, 30, 35);
	_txtQuantity = new Text(34, 9, 178, 35);
	_txtResult = new Text(76, 9, 218, 35);
	_lstDetails = new TextList(272, 104, 23, 46); // Height = 13*8 (8 due to rowheight overlap using default rules).
	_lstTotal = new TextList(133, 9, 171, 154);

	// Set palette
	setInterface("baseInfoDetails");

	add(_window, "window", "baseInfoDetails");
	add(_btnOk, "button", "baseInfoDetails");
	add(_btnAllBases, "button", "baseInfoDetails");
	add(_btnPrev, "button", "baseInfoDetails");
	add(_btnNext, "button", "baseInfoDetails");
	add(_txtTitle, "text", "baseInfoDetails");
	add(_txtSource, "text", "baseInfoDetails");
	add(_txtQuantity, "text", "baseInfoDetails");
	add(_txtResult, "text", "baseInfoDetails");
	add(_lstDetails, "list", "baseInfoDetails");
	add(_lstTotal, "text", "baseInfoDetails");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "baseInfoDetails");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnOkClick, Options::keyCancel);
	_btnAllBases->setText(tr("STR_ALL_BASES"));
	_btnAllBases->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnAllBasesClick);
	_btnPrev->setText("<<");
	_btnPrev->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnPrevClick, Options::keyGeoLeft);
	_btnNext->setText(">>");
	_btnNext->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnNextClick, Options::keyGeoRight);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	drawBody();
}

/**
 *
 */
BaseInfoDetailsState::~BaseInfoDetailsState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void BaseInfoDetailsState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Show details from all bases.
 */
void BaseInfoDetailsState::btnAllBasesClick(Action *)
{
	drawBody();
}

/**
 * Goes to the next 'cost' category.
 * @param action Pointer to an action.
 */
void BaseInfoDetailsState::btnNextClick(Action *)
{
	switch (_category)
	{
	case BaseInfoDetailsCategory::DETECTION:
		_category = BaseInfoDetailsCategory::SOLDIERS;
		break;
	case BaseInfoDetailsCategory::SOLDIERS:
	case BaseInfoDetailsCategory::ENGINEERS:
	case BaseInfoDetailsCategory::SCIENTISTS:
		// Skip categories for which I could not devise meaningful screen content.
		_category = BaseInfoDetailsCategory::QUARTERS;
		break;
	default:
		_category = static_cast<BaseInfoDetailsCategory>(static_cast<int>(_category) + 1);
		break;
	}

	drawBody();
}

/**
 * Goes to the previous 'cost' category.
 * @param action Pointer to an action.
 */
void BaseInfoDetailsState::btnPrevClick(Action *)
{
	switch (_category)
	{
	case BaseInfoDetailsCategory::SOLDIERS:
		_category = BaseInfoDetailsCategory::DETECTION;
		break;
	case BaseInfoDetailsCategory::QUARTERS:
	case BaseInfoDetailsCategory::SCIENTISTS:
	case BaseInfoDetailsCategory::ENGINEERS:
		// Skip categories for which I could not devise meaningful screen content.
		_category = BaseInfoDetailsCategory::SOLDIERS;
		break;
	default:
		_category = static_cast<BaseInfoDetailsCategory>(static_cast<int>(_category) - 1);
		break;
	}

	drawBody();
}


/**
 * Setup and draw the screen's body.
 *  * Screen title
 *  * listDetails
 *  * listTotal
 */
void BaseInfoDetailsState::drawBody()
{
	bool allBases = _btnAllBases->getPressed();

	//_details.clear();
	//_lstTotal->clearList();
	std::ostringstream ssTitle;
	if (allBases)
	{
		ssTitle << "[all bases] ";
	}

	switch (_category)
	{
	case BaseInfoDetailsCategory::SOLDIERS:
		ssTitle << "whoops";
		break;
	// case BaseInfoDetailsCategory::ENGINEERS:
	// 	break;
	// case BaseInfoDetailsCategory::SCIENTISTS:
	// 	break;
	// case BaseInfoDetailsCategory::QUARTERS:
	// 	break;
	// case BaseInfoDetailsCategory::STORES:
	// 	break;
	// case BaseInfoDetailsCategory::LABORATORIES:
	// 	break;
	// case BaseInfoDetailsCategory::WORKSHOPS:
	// 	break;
	// case BaseInfoDetailsCategory::CONTAINMENT:
	// 	break;
	// case BaseInfoDetailsCategory::HANGARS:
	// 	break;
	// case BaseInfoDetailsCategory::DEFENSE:
	// 	break;
	// case BaseInfoDetailsCategory::DETECTION:
	// 	break;
	default:
		// Do not end sentence with '.', automatic font scaling "setText()" does not like that.
		// Cause: b1b6f9ae
		ssTitle << "Category " << enum2string(_category) << " not implemented yet";
		break;
	}

	_txtTitle->setText(ssTitle.str());
}

}
