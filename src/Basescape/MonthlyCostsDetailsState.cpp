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

#include "MonthlyCostsDetailsState.h"
#include <sstream>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the costs category breakdown subwindow.
 *
 * @param base Pointer to the base to get info from.
 */
MonthlyCostsDetailsState::MonthlyCostsDetailsState(Base *base, CostCategory currentCategory) : _base(base), _currentCategory(currentCategory)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 320, 184, 0, 8, POPUP_BOTH); // TransferState Style
	//_window = new Window(this, 320, 162, 0, 28, POPUP_BOTH); // ManufactureStart Style with 'use title from previous screen' GUI trickery.
	_btnOk = new TextButton(288, 16, 16, 166);
	_btnPrev = new TextButton(28, 14, 8, 18);
	_btnNext = new TextButton(28, 14, 284, 18);
	_txtTitle = new Text(278, 17, 21, 18);
	_txtSource = new Text(114, 9, 30, 35);
	_txtQuantity = new Text(34, 9, 178, 35);
	_txtResult = new Text(76, 9, 218, 35);
	_lstDetails = new TextList(272, 104, 23, 46); // Height = 13*8 (8 due to rowheight overlap using default rules).
	_lstTotal = new TextList(133, 9, 171, 154);

	// Set palette
	setInterface("costDetailsInfo");

	add(_window, "window", "costDetailsInfo");
	add(_btnOk, "button", "costDetailsInfo");
	add(_btnPrev, "button", "costDetailsInfo");
	add(_btnNext, "button", "costDetailsInfo");
	add(_txtTitle, "text", "costDetailsInfo");
	add(_txtSource, "text", "costDetailsInfo");
	add(_txtQuantity, "text", "costDetailsInfo");
	add(_txtResult, "text", "costDetailsInfo");
	add(_lstDetails, "list", "costDetailsInfo");
	add(_lstTotal, "text", "costDetailsInfo");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "costDetailsInfo");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&MonthlyCostsDetailsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&MonthlyCostsDetailsState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&MonthlyCostsDetailsState::btnOkClick, Options::keyCancel);
	_btnPrev->setText("<<");
	_btnPrev->onMouseClick((ActionHandler)&MonthlyCostsDetailsState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&MonthlyCostsDetailsState::btnPrevClick, Options::keyBattlePrevUnit);
	_btnNext->setText(">>");
	_btnNext->onMouseClick((ActionHandler)&MonthlyCostsDetailsState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&MonthlyCostsDetailsState::btnNextClick, Options::keyBattleNextUnit);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	/// TODO: Make translatable!
	/// TODO: Move to specific 'body...' functions?
	_txtSource->setText("Source");
	_txtQuantity->setText("Amount");
	_txtResult->setText("Result");

	_lstDetails->setColumns(3, 155, 32, 83); // Note, list starts indented 2px due to align?
	_lstDetails->setSelectable(true);        // Needed for collapse/fold functionality.
	_lstDetails->setBackground(_window);
	_lstDetails->setScrolling(true);
	_lstDetails->setMargin(2);
	_lstDetails->setDot(true);
	_lstDetails->onMousePress((ActionHandler)&MonthlyCostsDetailsState::lstDetailsMousePress);

	_lstTotal->setColumns(2, 57, 76); // Allow column 2 to display  $999,999,999,999 (3px overflow)
	_lstTotal->setDot(true);
	_lstTotal->setColor(_lstTotal->getSecondaryColor());

	drawBody();
}

/**
 *
 */
MonthlyCostsDetailsState::~MonthlyCostsDetailsState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void MonthlyCostsDetailsState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Goes to the next 'cost' category.
 * @param action Pointer to an action.
 */
void MonthlyCostsDetailsState::btnNextClick(Action *)
{
	if (_currentCategory == CC_GLOBAL_RESULT)
		_currentCategory = CC_CRAFTS_ALL;
	else
		_currentCategory = (CostCategory)(_currentCategory + 1);

	drawBody();
}

/**
 * Goes to the previous 'cost' category.
 * @param action Pointer to an action.
 */
void MonthlyCostsDetailsState::btnPrevClick(Action *)
{
	if (_currentCategory == CC_CRAFTS_ALL)
		_currentCategory = CC_GLOBAL_RESULT;
	else
		_currentCategory = (CostCategory)(_currentCategory - 1);

	drawBody();
}

/**
 * Handles the mouse-click on the list rows.
 * @param action Pointer to an action.
 */
void MonthlyCostsDetailsState::lstDetailsMousePress(Action *action)
{
	if (action->getDetails()->button.button != SDL_BUTTON_RIGHT) return;

	_sel = _lstDetails->getSelectedRow();
	if (getRow().id == getRow().parentId && !(_details[_rows[_sel] + 1].isVisible))
	{
		// Show all elements contributing to parent.
		for (size_t i = 0; i < _details.size(); ++i)
		{
			if (_details[i].parentId == getRow().id)
			{
				_details[i].isVisible = true;
			}
		}
	}
	else
	{
		// Collapse all elements contributing to parent.
		for (size_t i = 0; i < _details.size(); ++i)
		{
			if (_details[i].parentId == getRow().parentId && (_details[i].id != _details[i].parentId))
			{
				_details[i].isVisible = false;
			}
		}
	}

	updateList();
}

/**
 * Setup and draw the screen's body.
 *  * Screen title
 *  * listDetails
 *  * listTotal
 */
void MonthlyCostsDetailsState::drawBody()
{
	_details.clear();
	_lstTotal->clearList();
	std::ostringstream ssTitle;

	switch (_currentCategory)
	{
	case CC_FACILITIES:
		ssTitle << tr("MCDS_TITEL_BASE_FACILITIES");
		categoryFacilityMaintenance();
		break;
	case CC_GLOBAL_RESULT:
		ssTitle << tr("MCDS_TITEL_GLOBAL_RESULT");
		categoryGlobalResult();
		break;
	default:
		ssTitle << "Cost Category " << _currentCategory << " not implemented yet";

		BeanCounter row;
		int parent, id = 0; // I know: parent is not initialized yet, will happen in the loop.
		for (auto i = 0; i < 5; i++)
		{
			parent = id;
			row = {id, parent, true, "Long text explaining the source", 999, 999999999999};
			_details.push_back(row);
			id++;
			
			for (auto j = 0; j < 5; j++)
			{
				row = {id, parent, false, "Normally collapsed (moaar details)", 99, 999999999999};
				_details.push_back(row);
				id++;
			}

		}
		_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(999999999999).c_str());
		break;
	}

	_txtTitle->setText(ssTitle.str().c_str());
	updateList();
}

/**
 * Setup screen that displays facility maintenance
 */
void MonthlyCostsDetailsState::categoryFacilityMaintenance()
{
	bool baseHasRevenueFacilities = false;
	// Keep both of those running numbers positive, correct when casting into row,
	int totalMaintenance = 0, totalRevenue = 0;

	BeanCounter row;
	// Use common scenario as start (facilities contribute to maintenance costs).
	int id = 1; // Offset since vector is build up using elements and subtotal will be inserted later.
	int idParent = 0;
	for (auto *facility : *_base->getFacilities())
	{
		// Facilities under construction do not cost/generate funds.
		if (facility->getBuildTime() > 0) continue;

		// Exclude revenue generating facilities.
		if (facility->getRules()->getMonthlyCost() < 0)
		{
			baseHasRevenueFacilities = true;
			continue;
		}

		std::string facilityName = tr(facility->getRules()->getType());
		// Is this facility already listed?
		bool facilityAlreadyAccountedFor = false;
		for (auto &listedFacility : _details)
		{
			if (listedFacility.description == facilityName)
			{
				listedFacility.amount += 1;
				facilityAlreadyAccountedFor = true;
				break;
			}
		}
		if (!facilityAlreadyAccountedFor)
		{
			// row.value is always positive for details
			row = {id, idParent, false, tr(facility->getRules()->getType()), 1, facility->getRules()->getMonthlyCost()};
			_details.push_back(row);
			id++;
		}

		totalMaintenance += facility->getRules()->getMonthlyCost();
	}
	// Prefer alphabetical listing.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return Unicode::naturalCompare(a.description, b.description);
		}
	);
	// Insert subtotal
	row = {idParent, idParent, true, tr("MCDS_SUBTOTAL_FACILITY_MAINTENANCE"), 0, -1 * totalMaintenance};
	_details.insert(_details.begin(), row);

	// Facilities generating revenue.
	if (baseHasRevenueFacilities)
	{
		idParent = id;
		id++; // Offset since vector is build up using elements and subtotal will be inserted later.
		std::vector<BeanCounter> detailsRevenueTmp;
		for (auto *facility : *_base->getFacilities())
		{
			if (facility->getBuildTime() > 0 || facility->getRules()->getMonthlyCost() >= 0) continue;

			std::string facilityName = tr(facility->getRules()->getType());
			// Is this facility already listed?
			bool facilityAlreadyAccountedFor = false;
			for (auto &listedFacility : detailsRevenueTmp)
			{
				if (listedFacility.description == facilityName)
				{
					listedFacility.amount++;
					facilityAlreadyAccountedFor = true;
					break;
				}
			}
			if (!facilityAlreadyAccountedFor)
			{
				row = {id, idParent, false, tr(facility->getRules()->getType()), 1, -1 * facility->getRules()->getMonthlyCost()};
				detailsRevenueTmp.push_back(row);
				id++;
			}

			// -= leads to += since all costs are negative in this loop.
			totalRevenue -= facility->getRules()->getMonthlyCost();
		}
		// Prefer alphabetical listing.
		std::stable_sort(detailsRevenueTmp.begin(), detailsRevenueTmp.end(),
			[](const BeanCounter a, const BeanCounter b)
			{
				return Unicode::naturalCompare(a.description, b.description);
			}
		);
		// Insert subtotal
		row = {idParent, idParent, true, tr("MCDS_SUBTOTAL_FACILITY_REVENUE"), 0, totalRevenue};
		detailsRevenueTmp.insert(detailsRevenueTmp.begin(), row);

		// Merge vectors, start with income
		_details.insert(_details.begin(), detailsRevenueTmp.begin(), detailsRevenueTmp.end());
	}

	// Screen Total
	_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(totalRevenue - totalMaintenance).c_str());
}


/**
 * Setup the global income overview.
 *
 * NOTE:
 *  Other possibility is to split income categories (or even make 2 separate screens).
 *  - See comments further down on "can be broken down further".
 *  - For now not chosen (maintenance one would lead to a pretty empty screen).
 *  - Might help a player where to put it's next focus, although geoscape's GRAPHS and FUNDING screens are better suited for that.
 */
void MonthlyCostsDetailsState::categoryGlobalResult()
{
	int countryFunding = _game->getSavedGame()->getCountryFunding();
	// Depends on 'getPerformanceBonusFactor() == 0' when not defined.
	int performanceFunding = std::max(0, _game->getSavedGame()->getCurrentScore(_game->getSavedGame()->getMonthsPassed()) * _game->getMod()->getPerformanceBonusFactor());
	int allBasesMaintenance = _game->getSavedGame()->getBaseMaintenance();
	BeanCounter row;

	// Global Income subtotal
	row = {0, 0, true, tr("MCDS_SUBTOTAL_GEO_INCOME"), 0, countryFunding + performanceFunding};
	_details.push_back(row);
	// Global Income elements. Can theoretically be broken down further (per country funding).
	row = {1, 0, false, tr("MCDS_DETAIL_COUNTRIES_INCOME"), 0, countryFunding};
	_details.push_back(row);
	if (_game->getMod()->getPerformanceBonusFactor())
	{
		// Can theoretically be broken down further (score per region, council protection scheme, research scores).
		// - Those can be misleading. Could add-up to negative income, which is not allowed (must be corrected for via an extra row).
		// - Would need to adapt screen to [description][score][value]
		// - Impossible to break down research scores (it is a running number, no concept of topics researched *this* month).
		row = {2, 0, false, tr("MCDS_DETAIL_PERFORMANCE_INCOME"), 0, performanceFunding};
		_details.push_back(row);
	}

	// Global maintenance subtotal
	row = {1, 1, true, tr("MCDS_SUBTOTAL_BASES_MAINTENANCE"), 0, -1 * allBasesMaintenance};
	_details.push_back(row);
	// Global maintenance elements (row.value is always positive)
	int id = 2;
	for (auto *base : *_game->getSavedGame()->getBases())
	{
		row = {id, 1, false, base->getName(), 0, base->getMonthlyMaintenace()};
		_details.push_back(row);
		id++;
	}

	// Screen Total
	_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(countryFunding + performanceFunding - allBasesMaintenance).c_str());
}

/**
 * Draw (en filter) the current list of cost details.
 */
void MonthlyCostsDetailsState::updateList()
{
	_lstDetails->clearList();
	_rows.clear();

	for (size_t i = 0; i < _details.size(); ++i)
	{
		// Filter
		if (!_details[i].isVisible) continue;

		std::string description = _details[i].description;
		std::ostringstream ssAmount, ssValue;
		// Let subtotals show signs, but elements not (aesthetics).
		bool unconditionallyShowSign = true;
		if (_details[i].parentId != _details[i].id) // Not a subtotal.
		{
			description.insert(0, " "); // Do not use dots for description indentation.
			ssAmount << tr("MCDS_DOTTED_INDENTATION");
			ssValue << tr("MCDS_DOTTED_INDENTATION") << tr("MCDS_DOTTED_INDENTATION");
			unconditionallyShowSign = false;
		}
		ssAmount << _details[i].amount;
		ssValue << Unicode::formatFunding(_details[i].value * std::max(1,_details[i].amount), unconditionallyShowSign);

		if (_details[i].amount > 0)
		{
			_lstDetails->addRow(3, description.c_str(), ssAmount.str().c_str(), ssValue.str().c_str());
		}
		else
		{
			_lstDetails->addRow(3, description.c_str(), "", ssValue.str().c_str());
		}
		_rows.push_back(i);

		if(_details[i].parentId == _details[i].id)
		{
			_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), _lstDetails->getSecondaryColor());
		}
	}
}
}
