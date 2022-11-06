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
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the costs category breakdown subwindow.
 *
 * @param base Pointer to the base to get info from.
 */
BaseInfoDetailsState::BaseInfoDetailsState(Base *base, DetailsCategory currentCategory) : _base(base), _currentCategory(currentCategory)
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
	_txtQuantity = new Text(54, 9, 165, 35);
	_txtResult = new Text(54, 9, 220, 35);
	_lstDetails = new TextList(272, 104, 23, 46); // Height = 13*8 (8 due to rowheight overlap using default rules).
	_lstTotal = new TextList(133, 9, 171, 154);

	// Set palette
	setInterface("baseInfoDetails");

	add(_window, "window", "baseInfoDetails");
	add(_btnOk, "button", "baseInfoDetails");
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
	_btnPrev->setText("<<");
	_btnPrev->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnPrevClick, Options::keyGeoLeft);
	_btnNext->setText(">>");
	_btnNext->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnNextClick, Options::keyGeoRight);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtSource->setText(tr("STR_SOURCE"));
	_txtQuantity->setText(tr("MCDS_TITEL_BASE_FACILITIES"));
	_txtResult->setText(tr("STR_VALUE"));

	_lstDetails->setColumns(3, 155, 45, 70); // Note, list starts indented 2px due to align?
	_lstDetails->setSelectable(true);        // Needed for collapse/fold functionality.
	_lstDetails->setBackground(_window);
	_lstDetails->setScrolling(true);
	_lstDetails->setMargin(2);
	_lstDetails->setDot(true);
	_lstDetails->onMousePress((ActionHandler)&BaseInfoDetailsState::lstDetailsMousePress);

	_lstTotal->setColumns(2, 57, 76); // Allow column 2 to display  $999,999,999,999 (3px overflow)
	_lstTotal->setDot(true);
	_lstTotal->setColor(_lstTotal->getSecondaryColor());

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
 * Goes to the next 'cost' category.
 * @param action Pointer to an action.
 */
void BaseInfoDetailsState::btnNextClick(Action *)
{
	switch (_currentCategory)
	{
	case DC_DETECTION:
		_currentCategory = DC_SOLDIERS;
		break;
	default:
		_currentCategory = (DetailsCategory)(_currentCategory + 1);
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
	switch (_currentCategory)
	{
	case DC_SOLDIERS:
		_currentCategory = DC_DETECTION;
		break;
	default:
		_currentCategory = (DetailsCategory)(_currentCategory - 1);
		break;
	}

	drawBody();
}

/**
* Handles mouse-clicks on the list rows.
* @param action Pointer to an action.
*/
void BaseInfoDetailsState::lstDetailsMousePress(Action *action)
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
* Adds another contribution to the '_details' vector.
*
* Creates a new entry if needed, updates if an entry already exist.
* An entry is defined by the unique combination of 'parentID','description' and 'colResultOverride'.
*
* @param row The contents of the row we want to insert.
* @param updateValueField Whether or not we want to update 'value' on existing entries.
* @return Unique identifier for the next element in the list (!not the vector's rowid!).
*/
int BaseInfoDetailsState::addToDetailsVector(BeanCounter row, bool updateValueField)
{
	for (auto &bean : _details)
	{
		if (bean.parentId == row.parentId && bean.description == row.description && bean.colResultOverride == row.colResultOverride)
		{
			bean.value += row.value * updateValueField; // Branchless programming trick.
			// No checking if bean.amount > -1.
			// It is callers responsibility to supply correct values.
			// To ensure any implementation faults become a bit more visible (weird numbers on screen).
			bean.amount += row.amount;

			return row.id;
		}
	}
	_details.push_back(row);
	return ++row.id;
}

/**
* Check if list contains details for given parentID.
*
* @param parentId Id of subtotal to check.
*/
bool BaseInfoDetailsState::isSubtotalNeeded(int parentId)
{
	auto bean = std::find_if(_details.begin(), _details.end(),
		[&](const BeanCounter row) {return row.parentId == parentId;});
	if (bean == _details.end())
	{
		return false;
	}
	return true;
}

/**
* Calculate specific subtotal amount of entries.
*
* @param parentId Id of subtotal to check.
* @return The sum of all element amounts for this subtotal.
*/
int BaseInfoDetailsState::calculateSubtotalAmount(int parentId)
{
	int64_t amount = 0;
	for (auto element : _details)
	{
		if (element.parentId == parentId && element.id != element.parentId)
		{
			amount += element.amount;
		}
	}
	return amount;
};

/**
* Calculate specific subtotal result.
*
* @param parentId Id of subtotal to check.
* @return The total value for this subtotal.
*/
int BaseInfoDetailsState::calculateSubtotalValue(int parentId)
{
	int total = 0;
	for (auto element : _details)
	{
		if (element.parentId == parentId && element.id != element.parentId)
		{
			total += element.value;
		}
	}
	return total;
};

/**
 * Returns the max value for this specific subtotal.
 *
 * @param parentId Id of subtotal to check.
 * @return The max value for this subtotal.
 */

int BaseInfoDetailsState::getSubtotalValueMax(int parentId)
{
	int total = 0;
	for (auto element : _details)
	{
		if (element.parentId == parentId && element.id != element.parentId)
		{
			total = std::max(total, element.value);
		}
	}
	return total;
};


/**
 * Setup and draw the screen's body.
 *  * Screen title
 *  * listDetails
 *  * listTotal
 */
void BaseInfoDetailsState::drawBody()
{
	// How did we manage to get here?
	// Not in init so player still has an empty screen to wonder what went wrong.
	if (_base->getFacilities()->size() == 0) return;

	_details.clear();
	_lstTotal->clearList();
	std::ostringstream ssTitle;

	switch (_currentCategory)
	{
	case DC_DETECTION:
		ssTitle << tr("BIDS_TITEL_DETECTION");
		categoryDetection();
		break;
	default:
		ssTitle << "Cost Category " << _currentCategory << " not implemented yet";

		break;
	}

	_txtTitle->setText(ssTitle.str().c_str());
	updateList();
}

/**
 * Setup base detection abilities and camouflage screen.
 *
 * Facilities contributing to the following subcategories:
 *  - Base Camouflage: Chance of staying undetected.
 *  - UFO detection per range, including hyperwave abilities.
 *  - Alien Base detection per range.
 */
void BaseInfoDetailsState::categoryDetection()
{
	int idItem = 100; // Ensure details use id's > than theoretical maximum subcategories of 74 (2*36 + 2).
	int idParent = 0; // Unique subcategories.
	int itemValue;    // Prefer positive values only for details (subtotals are allowed to be negative)
	std::vector<BeanCounter> subCategories;
	BeanCounter row;  // List's workhorse.

	// Calculate detection probability for multiple radars
	// For detection we only need one success, assuming independent checks (which seem to be true):
	// - P(detection) = 1 - P(all radars failed to detect)
	// - P(all radars failed) = P(radar 1 failed) * P(radar 2 failed) * ... P(radar N failed).
	// - P(radar X failed) = 1 - P(detection of radar)
	// - P(detection of radar) = %/100
	auto detectionResult = [&](int parentId) -> int
	{
		double detectionFail = 1.0;
		for (auto element : _details)
		{
			if (element.parentId == parentId && element.id != element.parentId)
			{
				for (int i = 0 ; i < element.amount ; i++ )
				{
					detectionFail *= (100 - element.value)/100.0;
				}
			}
		}
		return (int) std::round(100 * (1.0 - detectionFail));
	};

	// Recognize facilities might have different radar/sight ranges.
	// We want to show detection chance per unique range.
	std::set<int> radarRanges {0}, sightRanges {0};
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0) continue;

		radarRanges.insert(facility->getRules()->getRadarRange());
		sightRanges.insert(facility->getRules()->getSightRange());
	}

	// Base camouflage.
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0) continue;

		// Base size in grid units.
		itemValue = facility->getRules()->getSize() * facility->getRules()->getSize();
		row = {idItem, idParent, false, tr("BIDS_DETAIL_CAMOUFLAGE_BASE_SIZE") , -1, itemValue, ""};
		idItem = addToDetailsVector(row, true);

		if (!facility->getRules()->isMindShield()) continue;

		// Mind shields & their strength
		itemValue = facility->getRules()->getMindShieldPower();
		std::ostringstream facilityName;
		facilityName << tr(facility->getRules()->getType());
		if (facility->getDisabled())
		{
			facilityName << " (" << tr("STR_DISABLED") << ")";
		}
		row = {idItem, idParent, false, facilityName.str().c_str() , 1, itemValue, ""};
		idItem = addToDetailsVector(row, false);
	}
	// SubCategory for base camouflage
	{
		// Lowest possible value is 79% (pretty high).
		// Could lead to a false sense of security though since check is done every 10 minutes.
		// One can argue value/subcategory is not that important and hence should not be shown.
		int subTotal = 100 - _base->getDetectionChance();

		row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_CAMOUFLAGE"), -1, subTotal, ""};
		row.colResultOverride = Unicode::formatPercentage(subTotal);
		subCategories.push_back(row);

		idParent++;

		// TRIVIA: If 'mindpower' >= 21 base cannot be found by UFO's (due to integer math).
	}

	// Ufo detection per range limit.
	for (auto detectionRange : radarRanges)
	{
		if (detectionRange == 0) continue;

		bool hasHyperwaveFunctionality = false;
		for (auto *facility : *_base->getFacilities())
		{
			// Skip buildings under construction & ranges that are too small.
			// Inclusion of bigger ranges is desired since they contribute to this detection.
			if (facility->getBuildTime() > 0 || facility->getRules()->getRadarRange() < detectionRange)
				continue;

			// Facility detection chance
			itemValue = facility->getRules()->getRadarChance();
			row = {idItem, idParent, false, tr(facility->getRules()->getType()), 1, itemValue, ""};
			row.colResultOverride = Unicode::formatPercentage(itemValue);
			idItem = addToDetailsVector(row, false);

			if (!facility->getRules()->isHyperwave())
				continue;

			// Facility provides hyperwave ability for this range.
			hasHyperwaveFunctionality = true;
			itemValue = 0; // This entry should not influence subtotal chance calculation.
			row = {idItem, idParent, false, tr(facility->getRules()->getType()) , 1, itemValue, ""};
			row.colResultOverride = tr("BIDS_DETAIL_DETECTION_HYPERWAVE");
			idItem = addToDetailsVector(row, false);
		}

		// SubCategory
		std::string description;
		if (hasHyperwaveFunctionality)
		{
			description = tr("BIDS_SUBTOTAL_UFO_DETECTION_HYPERWAVE").arg(detectionRange);
		}
		else
		{
			description = tr("BIDS_SUBTOTAL_UFO_DETECTION_RADAR").arg(detectionRange);
		}
		int subAmount = -1;
		int subTotal = detectionResult(idParent);
		row = {idParent, idParent, true, description.c_str(), subAmount, subTotal, ""};
		row.colResultOverride = Unicode::formatPercentage(subTotal);
		subCategories.push_back(row);

		idParent++;
	}

	// Alien Base detection
	// Not sure about this: One could argue it should be a hidden stat.
	for (auto detectionRange : sightRanges)
	{
		if (detectionRange == 0) continue;

		for (auto *facility : *_base->getFacilities())
		{
			// Skip buildings under construction & ranges that are too small.
			// Inclusion of bigger ranges is desired since they contribute to this detection.
			if (facility->getBuildTime() > 0 || facility->getRules()->getSightRange() < detectionRange)
				continue;

			// Facility detection chance
			itemValue = facility->getRules()->getSightChance();
			if (itemValue == 0)
			{
				// Dynamic detection (0-50% based on distance).
				// Formula from `GeoscapeState::time1Day()`:
				//  chanceToDetect = 50 - (distance * 50 / facility->getRules()->getSightRange())
				// Distance to alien base is unknown but the increase is linear.
				// In order to work with a single value we assume it is ok to use an average distance.
				// In this case: half of the current detectionRange.
				itemValue = 50 - (detectionRange/2 * 50) / facility->getRules()->getSightRange();
			}
			row = {idItem, idParent, false, tr(facility->getRules()->getType()), 1, itemValue, ""};
			row.colResultOverride = Unicode::formatPercentage(itemValue);
			idItem = addToDetailsVector(row, false);
		}

		// SubCategory
		int subAmount = -1;
		int subTotal = detectionResult(idParent);
		row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_ALIEN_BASE_DETECTION").arg(detectionRange), subAmount, subTotal, ""};
		row.colResultOverride = Unicode::formatPercentage(subTotal);
		subCategories.push_back(row);

		idParent++;
	}

	// Prefer alphabetical listing of detailed rows.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return Unicode::naturalCompare(a.description, b.description);
		}
	);

	// Add subtotals to list vector
	_details.insert(_details.begin(), subCategories.begin(), subCategories.end());

	// Ensure elements are shown below appropriate subtotal.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return a.parentId < b.parentId;
		}
	);
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
		if (!_details[i].isVisible) continue;


		std::string description = _details[i].description;
		std::ostringstream ssAmount, ssValue;
		//bool unconditionallyShowSign = true;
		if (_details[i].parentId != _details[i].id) // Not a subtotal.
		{
			description.insert(0, " "); // Do not use dots for description indentation.
			ssAmount << tr("MCDS_DOTTED_INDENTATION");
			ssValue << tr("MCDS_DOTTED_INDENTATION") << tr("MCDS_DOTTED_INDENTATION");
			//unconditionallyShowSign = false;
		}

		if (_details[i].colResultOverride != "")
		{
			ssValue << _details[i].colResultOverride;
		}
		else
		{
			ssValue << _details[i].value;
		}
		ssAmount << _details[i].amount;

		if (_details[i].amount > -1)
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
