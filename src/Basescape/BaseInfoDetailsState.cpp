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
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
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
	_btnOk = new TextButton(148, 16, 164, 169);
	_btnQueuedFacilities = new ToggleTextButton(148, 16, 9, 169);
	_btnPrev = new TextButton(28, 14, 8, 18);
	_btnNext = new TextButton(28, 14, 284, 18);
	_txtTitle = new Text(278, 17, 21, 18);
	_txtSource = new Text(114, 9, 30, 35);
	_txtQuantity = new Text(34, 9, 178, 35);
	_txtResult = new Text(76, 9, 218, 35);
	_lstDetails = new TextList(272, 104, 23, 46); // Height = 13*8 (8 due to rowheight overlap using default rules).
	_txtTotal = new Text(133, 9, 171, 154);

	// Set palette
	setInterface("baseInfoDetails");

	add(_window, "window", "baseInfoDetails");
	add(_btnOk, "button", "baseInfoDetails");
	add(_btnQueuedFacilities, "button", "baseInfoDetails");
	add(_btnPrev, "button", "baseInfoDetails");
	add(_btnNext, "button", "baseInfoDetails");
	add(_txtTitle, "text", "baseInfoDetails");
	add(_txtSource, "text", "baseInfoDetails");
	add(_txtQuantity, "text", "baseInfoDetails");
	add(_txtResult, "text", "baseInfoDetails");
	add(_lstDetails, "list", "baseInfoDetails");
	add(_txtTotal, "text", "baseInfoDetails");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "baseInfoDetails");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnOkClick, Options::keyCancel);
	_btnQueuedFacilities->setText(tr("STR_INCLUDE_QUEUED_FACILITIES"));
	_btnQueuedFacilities->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnToggleQueuedFacilities); // LMB only
	_btnPrev->setText("<<");
	_btnPrev->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnPrevClick, Options::keyGeoLeft);
	_btnNext->setText(">>");
	_btnNext->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnNextClick, Options::keyGeoRight);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtSource->setText(tr("STR_SOURCE"));
	_txtQuantity->setText(tr("STR_AMOUNT"));
	_txtResult->setText(tr("STR_VALUE"));

	_lstDetails->setColumns(3, 155, 45, 70);
	_lstDetails->setSelectable(true); // Required for collapse/fold functionality.
	_lstDetails->setBackground(_window);
	_lstDetails->setScrolling(true);
	_lstDetails->setMargin(2);        // Shifts **all** columns 2px to the right.
	_lstDetails->onMousePress((ActionHandler)&BaseInfoDetailsState::lstDetailsMousePress, SDL_BUTTON_RIGHT);
	_lstDetails->setDot(true);

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
void BaseInfoDetailsState::btnToggleQueuedFacilities(Action *)
{
	drawBody();
}

/**
 * Goes to the next details category.
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
 * Goes to the previous details category.
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
* Handles mouse-clicks on the list rows.
*/
void BaseInfoDetailsState::lstDetailsMousePress(Action *)
{
	_sel = _lstDetails->getSelectedRow();

	// Flip visibility of child elements.
	for (size_t i = 0; i < _details.size(); ++i)
	{
		if (_details[i].parentId == _details[i].childId) continue;
		if (_details[i].parentId != getRow().parentId) continue;

		_details[i].isRowVisible ^= true;
	}
	updateList();
}

/**
 * Setup and draw the screen's body.
 *  * Screen title
 *  * list details
 *  * (optional) Grand total
 */
void BaseInfoDetailsState::drawBody()
{
	_details.clear();

	switch (_category)
	{
	// case BaseInfoDetailsCategory::SOLDIERS:
	//	break;
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
		setupPlaceholders();
		break;
	}
}

/**
 * Creates all placeholder elements.
*/
void BaseInfoDetailsState::setupPlaceholders()
{
	std::ostringstream ssTitle, screenTotal;
	// Do not end sentence with '.'
	// Automatic font scaling "setText()" does not like that (Cause: b1b6f9ae).
	ssTitle << "Category " << enum2string(_category) << " not implemented yet";
	screenTotal << tr("STR_TOTAL") << ">\t" << Unicode::formatFunding(999'999'999'999);
	_txtTitle->setText(ssTitle.str());
	_txtTotal->setText(screenTotal.str());

	BeanCounter row;
	size_t parent, childId = 0;
	for (auto i = 0; i < 5; i++)
	{
		parent = childId;
		row = {childId, parent, "Long text explaining the source", 999, 999'999'999, true};
		_details.push_back(row);
		childId++;
		for (auto j = 0; j < 5; j++)
		{
			row = {childId, parent, "Normally collapsed (moaar details)", 99, 999'999'999, false};
			_details.push_back(row);
			childId++;
		}
	}
	updateList();

}

/**
* Draw (en filter) the current details list.
*/
void BaseInfoDetailsState::updateList()
{
	_lstDetails->clearList();
	_rows.clear();

	for (size_t i = 0; i < _details.size(); ++i)
	{
		// Filter
		if (!_details[i].isRowVisible) continue;

		std::string description = _details[i].description;
		std::ostringstream ssAmount, ssValue;
		//bool unconditionallyShowSign = true;
		if (_details[i].parentId != _details[i].childId) // A child row.
		{
			description.insert(0, " "); // Do not use dots for description indentation.
			ssAmount << tr("STR_DOTTED_INDENTATION");
			ssValue << tr("STR_DOTTED_INDENTATION");
			//unconditionallyShowSign = false;
		}

		if (_details[i].valueOverride != "")
		{
			ssValue << _details[i].valueOverride;
		}
		else
		{
			ssValue << _details[i].baseValue;
		}
		ssAmount << _details[i].amount;

		if (_details[i].amount > 0)
		{
			_lstDetails->addRow(3, description.c_str(), ssAmount.str().c_str(), ssValue.str().c_str());
		}
		else
		{
			_lstDetails->addRow(3, description.c_str(), "", ssValue.str().c_str());
		}
		_rows.push_back(i);

		if(_details[i].parentId == _details[i].childId)
		{
			_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), _lstDetails->getSecondaryColor());
		}
	}
}

/**
* Adds a unique row to the given (temporal) subcategory vector.
*
* Creates a new entry if needed, updates existing one where possible.
*
* @note
* Row uniqueness is defined by the combination of the fields:
*`parentId`, `description` and `baseValue`.
*
* @param subCategory Vector to operate on (`_details` is allowed but not recommend).
* @param row         The contents of the row we want to insert (or update).
*/
void BaseInfoDetailsState::add2vector(std::vector<BeanCounter> &subCategory, BeanCounter row)
{
	for (auto &bean : subCategory)
	{
		// Could probably get away with check on description only.
		// Others are for the (unlikely) case method is called on `_details`.
		if (bean.parentId == row.parentId &&
			bean.baseValue == row.baseValue &&
			bean.description == row.description)
		{
			bean.amount += row.amount;
			return;
		}
	}
	subCategory.push_back(row);
	return;
}

/**
* Alphabetical sort of children (by description)
*
* @param subCategory The method's local `_details` slice copy to work on.
* @param skipChildren Number of starting child rows to be left untouched.
*/
void BaseInfoDetailsState::sortChildrenByDescription(std::vector<BeanCounter> &subCategory, size_t skipChildren)
{
	// No children
	if (subCategory.size() < 2 + skipChildren) return;

	std::stable_sort(std::next(subCategory.begin(), 1 + skipChildren), subCategory.end(),
		[](const BeanCounter a, const BeanCounter b)
		{ return Unicode::naturalCompare(a.description, b.description); }
	);
}

/**
* Calculate the probability of getting a success for the given subCategory.
*
* @remark
* For detection we only need one success.
* Assuming independent checks (which seem to be true):
* - P_success = 1 - P_all_failed
* - P_all_failed = P_check1failed * P_check2failed * ... * P_checkNfailed.
* - P_check#failed = 1 - P_check_success
* - P_check_success = chance_value/100
*
* @param subCategory The method's local `_details` slice copy to work on.
* @return The probability of success (0 < Psucces < 1).
*/
double BaseInfoDetailsState::calcProbabilityAtLeastOne(std::vector<BeanCounter> subCategory)
{

	double detectionFail = 1.0;
	for (auto bean : subCategory)
	{
		// By default the first element is meant to display the result of this method.
		//* Hence we skip this first one by default.
		if (bean.parentId == bean.childId) continue; // not a child

		detectionFail *= (1.0 - calcProbabilityAtLeastOne(bean.baseValue, bean.amount));
	}
	// Always round down (even if that results in non probability "0").
	return 1.0 - detectionFail;
}

/**
* Calculate the probability of getting a success.
*
* @remark
* P_success = 1 - P_all_failed
*           = 1 - (P_facility1fails * P_facility1fails + ... + PfacilityNfails)
* Since all tries have the same baseChange the formula becomes:
* P_success = 1 - (1 - P_baseChance)^tries
*
* @param baseChance The success chance for each try (e.g. detection chance).
* @param tries      Number of facilities contributing to this calculation.
* @return The probability of success (0 < Psucces < 1).
*/
double BaseInfoDetailsState::calcProbabilityAtLeastOne(int baseChance, int tries)
{
	if (baseChance <= 0) return 0.0;

	return 1.0 - std::pow(1.0 - baseChance/100.0, tries);
}

}
