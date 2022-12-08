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
#include "../Mod/Mod.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Production.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Transfer.h"

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
	_txtQuantity->setText(tr("STR_AMOUNT"));
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
	if (getRow().childId == getRow().parentId && !(_details[_rows[_sel] + 1].isVisible))
	{
		// Show all elements contributing to parent.
		for (size_t i = 0; i < _details.size(); ++i)
		{
			if (_details[i].parentId == getRow().childId)
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
			if (_details[i].parentId == getRow().parentId && (_details[i].childId != _details[i].parentId))
			{
				_details[i].isVisible = false;
			}
		}
	}

	drawList();
}

/**
* Adds another contribution to the '_details' vector.
*
* Creates a new entry if needed, updates if an entry already exist.
* An entry is defined by the unique combination of 'parentID','description' and 'valueOverride'.
*
* @param row The contents of the row we want to insert.
* @param updateValueField Whether or not we want to update 'value' on existing entries.
* @return Unique identifier for the next element in the list (!not the vector's rowid!).
*/
int BaseInfoDetailsState::addToDetailsVector(BeanCounter row, bool updateValueField)
{
	for (auto &bean : _details)
	{
		if (bean.parentId == row.parentId && bean.description == row.description && bean.valueOverride == row.valueOverride)
		{
			bean.value += row.value * updateValueField; // Branchless programming trick.
			// No checking if bean.amount > -1.
			// It is callers responsibility to supply correct values.
			// To ensure any implementation faults become a bit more visible (weird numbers on screen).
			bean.amount += row.amount;

			return row.childId;
		}
	}
	_details.push_back(row);
	return ++row.childId;
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
		if (element.parentId == parentId && element.childId != element.parentId && element.amount != -1)
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
		if (element.parentId == parentId && element.childId != element.parentId && element.amount >= -1)
		{
			total += element.value * std::abs(element.amount); // If '-1' we probably want to add a single instance of corresponding value.
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
		if (element.parentId == parentId && element.childId != element.parentId)
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
	case DC_SOLDIERS:
		categorySoldiers();
		return; // Temporal untill other cases uses new scheme.
		//break;
	case DC_QUARTERS:
		ssTitle << tr("personnel"); // common/language/Technical
		categoryQuarters();
		break;
	case DC_STORES:
		ssTitle << tr("STR_STORES");
		categoryStorage();
		break;
	case DC_LABORATORIES:
		ssTitle << tr("STR_LABORATORIES");
		categoryLabs();
		break;
	case DC_WORKSHOPS:
		ssTitle << tr("STR_WORKSHOP");
		categoryWorkshops();
		break;
	case DC_CONTAINMENT:
		ssTitle << tr("STR_ALIEN_CONTAINMENT");
		categoryAlienContainment();
		break;
	case DC_HANGARS:
		ssTitle << tr("BIDS_TITEL_HANGARS");
		categoryHangars();
		break;
	case DC_DEFENSE:
		ssTitle << tr("BIDS_TITEL_DEFENSE");
		categoryDefense();
		break;
	case DC_DETECTION:
		ssTitle << tr("BIDS_TITEL_DETECTION");
		categoryDetection();
		break;
	default:
		ssTitle << "Cost Category " << _currentCategory << " not implemented yet";

		BeanCounter row;
		int parent, childId = 0;
		for (auto i = 0; i < 5; i++)
		{
			parent = childId;
			row = {childId, parent, true, "Long text explaining the source", 999, 999999999, {}};
			_details.push_back(row);
			childId++;
			for (auto j = 0; j < 5; j++)
			{
				row = {childId, parent, false, "Normally collapsed (moaar details)", 99, 999999999, {}};
				_details.push_back(row);
				childId++;
			}
		}
		_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(999999999999).c_str());
		break;
	}

	_txtTitle->setText(ssTitle.str().c_str());
	drawList();
}

/**
 * Setup soldier benefits from facilities functionality screen.
 *
 * Facilities contributing to the following subcategories:
 *  + Psionic training.
 *  + Physical/combat training.
 *  + Wound regeneration.
 *  + Health regeneration
 *  + Mana regeneration
 */
void BaseInfoDetailsState::categorySoldiers()
{
	_txtTitle->setText(tr("STR_SOLDIERS"));

	int idParent = 0;
	addSubCategoryPsionicTraining(idParent);
	addSubCategoryPhysicalTraining(idParent);
	addSubCategoryWoundRecovery(idParent);
	addSubCategoryHealthRecovery(idParent);
	addSubCategoryManaRecovery(idParent);

	drawList();
}

/**
 * Add health recovery overview to _details vector.
 *
 * Child elements include:
 * - List of base facilities contributing to recovery.
 * - List of affected soldiers (since not visible on other screens).
 *
 * @note
 * Hp recovery after wounds are healed but soldier not yet at full HP.
 * Can occur due to health loss from battle or scripts.
 *
 * @note
 * Logic based on: `Soldier::replenishStats`.
 *
 * @param parentId  Identifier for subcategory (will be updated).
 */
void BaseInfoDetailsState::addSubCategoryHealthRecovery(int& parentId)
{
	size_t parentIndex = _details.size();
	int idItem = parentId;
	BeanCounter row;

	auto recoveryRates = _base ? _base->getSumRecoveryPerDay() : BaseSumDailyRecovery();
	if (recoveryRates.HealthRecovery == 0) return;

	// Subcategory header
	std::string valueOverride = toStringHp(recoveryRates.HealthRecovery);
	row = {parentId, parentId, true, tr("STR_BIDS_SUBTOTAL_HEALTH_RECOVERY"), 0, 0, "", valueOverride};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		int itemValue = facility->getRules()->getHealthRecoveryPerDay();
		if (facility->getBuildTime() > 0 || itemValue == 0) continue;

		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, "", toStringHp(itemValue)};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		// Prefer alphabetical sort of facilities
		int startOffset = parentIndex + 1; // 1 <-- Subtotal only.
		std::sort(std::next(_details.begin(), startOffset), _details.end(),
			[](const BeanCounter a, const BeanCounter b)
			{ return Unicode::naturalCompare(a.description, b.description); }
		);

		_details[parentIndex].amount = facilities;
	}

	// Might as well list soldiers harassing nurses.
	size_t soldierIndex = _details.size();
	for (auto soldier : *_base->getSoldiers())
	{
		// Soldiers in sickbay are listed under a different subtotal.
		if (soldier->getWoundRecoveryInt() >= 0) continue;

		int itemValue = soldier->getHealthMissing();
		if ( itemValue == 0) continue;

		row = {idItem, parentId, false, soldier->getName(), 1, itemValue, ".", toStringHp(itemValue)};
		idItem = addToDetailsVector(row);
	}
	if (_details.size() > soldierIndex)
	{
		// Prefer alphabetical sort of names
		std::sort(std::next(_details.begin(), soldierIndex), _details.end(),
			[](const BeanCounter a, const BeanCounter b)
			{ return Unicode::naturalCompare(a.description, b.description); }
		);
	}

	parentId++;
}

/**
 * Add mana recovery overview to _details vector.
 *
 * Child elements include:
 * - List of base facilities contributing to recovery.
 * - List of affected soldiers (since not visible on other screens).
 *
 * @note
 * For positive recoveryRates mana recovery occurs after all wounds are healed.
 * For negative 'recovery' it always occurs.
 *
 * @note
 * Logic based on: `Soldier::replenishStats`.
 *
 * @param parentId  Identifier for subcategory (will be updated).
 */
void BaseInfoDetailsState::addSubCategoryManaRecovery(int& parentId)
{
	if (!_game->getMod()->isManaFeatureEnabled()) return;
	if (!_game->getSavedGame()->isManaUnlocked(_game->getMod())) return;

	size_t parentIndex = _details.size();
	int idItem = parentId;
	BeanCounter row;

	auto recoveryRates = _base ? _base->getSumRecoveryPerDay() : BaseSumDailyRecovery();
	// No check if manaRecoverRate is 0, in case mod defines facilities
	// with both positive and negative recovery.

	// Subcategory header
	std::string valueOverride = toStringMana(recoveryRates.ManaRecovery);
	row = {parentId, parentId, true, tr("STR_BIDS_SUBTOTAL_MANA_RECOVERY"), 0, 0, "", valueOverride};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		int itemValue = facility->getRules()->getManaRecoveryPerDay();
		if (facility->getBuildTime() > 0 || itemValue == 0) continue;

		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, "", toStringMana(itemValue)};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		// Prefer alphabetical sort of facilities
		int startOffset = parentIndex + 1; // 1 <-- Subtotal only.
		std::sort(std::next(_details.begin(), startOffset), _details.end(),
			[](const BeanCounter a, const BeanCounter b)
			{ return Unicode::naturalCompare(a.description, b.description); }
		);

		_details[parentIndex].amount = facilities;
	}

	// Might as well list soldiers experimenting with substances.
	size_t soldierIndex = _details.size();
	for (auto soldier : *_base->getSoldiers())
	{
		// Soldiers in sickbay are listed under a different subtotal.
		if (soldier->getWoundRecoveryInt() > 0) continue;

		int itemValue = soldier->getManaMissing();
		if (itemValue == 0) continue;

		row = {idItem, parentId, false, soldier->getName(), 1, itemValue, ".", toStringMana(itemValue)};
		idItem = addToDetailsVector(row);
	}
	if (_details.size() > soldierIndex)
	{
		// Prefer alphabetical sort of names
		std::sort(std::next(_details.begin(), soldierIndex), _details.end(),
			[](const BeanCounter a, const BeanCounter b)
			{ return Unicode::naturalCompare(a.description, b.description); }
		);
	}

	parentId++;
}

/**
 * Add physical training overview to _details vector.
 *
 * Child elements include:
 * - List of base facilities adding space.
 * - Grand total of used space.
 *   + See `AllocatePsiTrainingState` for per soldier overview.
 *
 * @param parentId  Identifier for subcategory (will be updated).
 */
void BaseInfoDetailsState::addSubCategoryPhysicalTraining(int& parentId)
{
	size_t parentIndex = _details.size();
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header
	row = {parentId, parentId, false, tr("STR_PHYSICAL_TRAINING"), {}}; // STR from 'common/Language/OXCE'
	idItem = addToDetailsVector(row, false);

	// Prefer to list usage before any facilities.
	// This row can only become visible (via RMB) if parent is visible.
	row = {idItem, parentId, false, tr("STR_TRAINING"), 0, _base->getUsedTraining(), ".", ""}; // STR from 'common/Language/OXCE'
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		// Allow display of gyms with negative space.
		if (facility->getBuildTime() > 0 || facility->getRules()->getTrainingFacilities() == 0)
			continue;

		int itemValue = facility->getRules()->getTrainingFacilities();
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		// Prefer alphabetical sort of facilities
		int startOffset = parentIndex + 2; // 2 <-- Subtotal and usage rows.
		std::sort(std::next(_details.begin(), startOffset), _details.end(),
			[](const BeanCounter a, const BeanCounter b)
			{ return Unicode::naturalCompare(a.description, b.description); }
		);

		_details[parentIndex].isVisible = true;
		_details[parentIndex].amount = facilities;
		_details[parentIndex].valueOverride =
			tr("STR_BIDS_ASSIGNED_VS_TOTAL").arg(_base->getUsedTraining()).arg(_base->getAvailableTraining());
	}

	parentId++;
}

/**
 * Add psionic training overview to _details vector.
 *
 * Child elements include:
 * - List of base facilities adding space.
 * - Grand total of used space.
 *   + See `AllocatePsiTrainingState` for per soldier overview.
 *
 * @param parentId  Identifier for subcategory (will be updated).
*/
void BaseInfoDetailsState::addSubCategoryPsionicTraining(int& parentId)
{
	if (!_game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements()))
		return;

	size_t parentIndex = _details.size();
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header
	row = {parentId, parentId, false, tr("STR_BIDS_SUBTOTAL_PSI_TRAINING"), {}};
	idItem = addToDetailsVector(row, false);

	// Prefer to list usage before any facilities.
	// This row can only become visible (via RMB) if parent is visible.
	row = {idItem, parentId, false, tr("STR_TRAINING"), 0, _base->getUsedPsiLabs(), ".", ""}; // STR from 'common/Language/OXCE'
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		// Allow display of psi lab's with negative space.
		if (facility->getBuildTime() > 0 || facility->getRules()->getPsiLaboratories() == 0)
			continue;

		int itemValue = facility->getRules()->getPsiLaboratories();
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		// Prefer alphabetical sort of facilities
		int startOffset = parentIndex + 2; // 2 <-- Subtotal and usage rows.
		std::sort(std::next(_details.begin(), startOffset), _details.end(),
			[](const BeanCounter a, const BeanCounter b)
			{ return Unicode::naturalCompare(a.description, b.description); }
		);

		_details[parentIndex].isVisible = true;
		_details[parentIndex].amount = facilities;
		_details[parentIndex].valueOverride =
			tr("STR_BIDS_ASSIGNED_VS_TOTAL").arg(_base->getUsedPsiLabs()).arg(_base->getAvailablePsiLabs());
	}

	parentId++;
}

/**
 * Add wound recovery overview to _details vector.
 *
 * Child elements include:
 * - List of base facilities contributing to recovery.
 * - List of affected soldiers (since not visible on other screens).
 *
 * @note
 * Normally between 1/2 and 3/2 of health loss from battle.
 * Can also occur due to transformations or scripts.
 *
 * @note
 * Logic based on: `BattleUnit::postMissionProcedures`.
 *
 * @param parentId  Identifier for subcategory (will be updated).
 */
void BaseInfoDetailsState::addSubCategoryWoundRecovery(int& parentId)
{
	size_t parentIndex = _details.size();
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header
	// Can I safely assume this one is always in effect?
	auto recoveryRates = _base ? _base->getSumRecoveryPerDay() : BaseSumDailyRecovery();
	std::string valueOverride = toStringHp(recoveryRates.SickBayAbsoluteBonus + 1.0f)
		 + " + " + toStringPercent(recoveryRates.SickBayRelativeBonus);

	row = {parentId, parentId, true, tr("STR_BIDS_SUBTOTAL_WOUND_RECOVERY"), 0, 0, "", valueOverride};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0) continue;

		float absBonus = facility->getRules()->getSickBayAbsoluteBonus();
		float relBonus = facility->getRules()->getSickBayRelativeBonus();
		if (absBonus == 0.0f && relBonus == 0.0f) continue;

		int itemValue = 0; // In case value sorting becomes desired.
		// Allow display of negative space regeneration.
		if (absBonus != 0.0f)
		{
			itemValue = std::round(absBonus);
			row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, "", toStringHp(absBonus)};
			idItem = addToDetailsVector(row, false);
		}
		if (relBonus != 0.0f)
		{
			itemValue = std::round(relBonus);
			row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, "", toStringPercent(relBonus)};
			idItem = addToDetailsVector(row, false);
		}
		facilities++;
	}
	if (facilities > 0)
	{
		// Prefer alphabetical sort of facilities
		int startOffset = parentIndex + 1; // 1 <-- Subtotal only.
		std::sort(std::next(_details.begin(), startOffset), _details.end(),
			[](const BeanCounter a, const BeanCounter b)
			{ return Unicode::naturalCompare(a.description, b.description); }
		);

		_details[parentIndex].amount = facilities;
	}

	// Might as well list soldiers in sickbay.
	size_t soldierIndex = _details.size();
	for (auto soldier : *_base->getSoldiers())
	{
		// Integer time as returned from 'Soldier::getWoundRecoveryInt()'
		// equals the amount of wound HP.
		int woundHP = soldier->getWoundRecoveryInt();
		if (woundHP <= 0) continue;

		row = {idItem, parentId, false, soldier->getName(), 1, woundHP, ".", toStringHp(woundHP)};
		idItem = addToDetailsVector(row);
	}
	if (_details.size() > soldierIndex)
	{
		// Prefer alphabetical sort of names
		std::sort(std::next(_details.begin(), soldierIndex), _details.end(),
			[](const BeanCounter a, const BeanCounter b)
			{ return Unicode::naturalCompare(a.description, b.description); }
		);
	}

	parentId++;
}

/**
 * Setup living space & hiring functionality screen.
 *
 * Recognize multiple subcategories:
 *  - Facilities contributing to living space.
 *  - Overview of claimed living space
 *    + Show grand total per type of soldier/scientist/engineer.
 *    + Can also include facilities with negative living space.
 *  - Services needed to acquire soldiers/scientists/engineers
 *    + Just list them, no need to show what they provide
 *    + This includes services required for transformations.
 */
void BaseInfoDetailsState::categoryQuarters()
{
	// Recognize there might be dependencies on base services.
	// We want to show facilities providing those.
	// Based on altered version of SavedGame::getAvailableProduction()
	RuleBaseFacilityFunctions requiredServices, providedServices;
	// Scientist & Engineers
	requiredServices |= _game->getMod()->getHireScientistsRequiresBaseFunc();
	requiredServices |= _game->getMod()->getHireEngineersRequiresBaseFunc();
	// Soldiers (per type)
	for (auto soldierType : _game->getMod()->getSoldiersList())
	{
		RuleSoldier *rule = _game->getMod()->getSoldier(soldierType);
		requiredServices |= rule->getRequiresBuyBaseFunc();
	}
	// Transformations
	// Based on: SavedGame::getAvailableTransformations()
	for (auto transformer : _game->getMod()->getSoldierTransformationList())
	{
		RuleSoldierTransformation *ruleTransform = _game->getMod()->getSoldierTransformation(transformer);
		if (!_game->getSavedGame()->isResearched(ruleTransform->getRequiredResearch()))
			continue;
		requiredServices |= ruleTransform->getRequiredBaseFuncs();
	}
	// Manufacture of personnel
	for (auto manufactureProject : _game->getMod()->getManufactureList())
	{
		RuleManufacture *ruleManufacture = _game->getMod()->getManufacture(manufactureProject);
		if (ruleManufacture->getSpawnedPersonType() == "" || !_game->getSavedGame()->isResearched(ruleManufacture->getRequirements()))
		{
			continue;
		}
		requiredServices |= ruleManufacture->getRequireBaseFunc();
	}

	// Check if we are allowed to know this service based on facility knowledge.
	// Based on: BuildFacilitiesState::populateBuildList()
	for (auto facilityType : _game->getMod()->getBaseFacilitiesList())
	{
		RuleBaseFacility *rule = _game->getMod()->getBaseFacility(facilityType);
		if (!rule->isAllowedForBaseType(_base->isFakeUnderwater()))
		{
			continue;
		}
		// Check if we can see facility in ufopaedia (less strict than check if we can build).
		ArticleDefinition *article =  _game->getMod()->getUfopaediaArticle(rule->getType(), false);
		if (article && !Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
		{
			continue;
		}
		providedServices |= rule->getProvidedBaseFunc();
	}
	requiredServices &= providedServices;

	int idItem = requiredServices.count() + 2; // Offset based on expected subtotal entries.
	int idParent = 0;
	int itemValue;
	std::vector<BeanCounter> subCategories;
	BeanCounter row;  // Workhorse

	// Living space
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0 || facility->getRules()->getPersonnel() == 0) continue;

		itemValue = facility->getRules()->getPersonnel();
		idParent = itemValue >= 0 ? 0 : 1;
		row = {idItem, idParent, false, tr(facility->getRules()->getType()) , 1, std::abs(itemValue), {}};
		idItem = addToDetailsVector(row, false);
	}
	// Personnel claiming living space.
	idParent = 1;
	itemValue = _base->getTotalScientists(); // Includes transfers
	row = {idItem, idParent, false, tr("STR_SCIENTISTS"), 0, itemValue, ".", ""};
	idItem = addToDetailsVector(row, false);
	itemValue = _base->getTotalEngineers(); // Includes transfers
	row = {idItem, idParent, false, tr("STR_ENGINEERS"), 0, itemValue, ".", ""};
	idItem = addToDetailsVector(row, false);
	// Soldiers
	for (auto soldier : *_base->getSoldiers())
	{
		row = {idItem, idParent, false, tr(soldier->getRules()->getType()), 1, 1, ".", ""};
		idItem = addToDetailsVector(row);
	}
	for (auto transfer : *_base->getTransfers())
	{
		if (transfer->getType() != TRANSFER_SOLDIER) continue;
		// Soldiers and all transformers.

		row = {idItem, idParent, false, tr(transfer->getSoldier()->getRules()->getType()), 0, itemValue, ".", ""};
		idItem = addToDetailsVector(row);
	}
	// Any person being 'produced'.
	for (auto conceived : _base->getProductions())
	{
		if (conceived->getRules()->getSpawnedPersonType() == "")
			continue;

		// Assume it is not possible to produce multiple persons with a single project.
		// Seems correct looking at Base::getUsedQuarters()
		row = {idItem, idParent, false, tr(conceived->getRules()->getSpawnedPersonType()), 0, itemValue, ".", ""};
		idItem = addToDetailsVector(row);
	}
	idParent++;

	// Facilities per required service
	// Based on 'Mod::getBaseFunctionNames()' (want to test my bit field skills).
	for (size_t bitPosition = 0; bitPosition < requiredServices.size(); ++bitPosition)
	{
		if (requiredServices.test(bitPosition))
		{
			bool providesService = false;
			RuleBaseFacilityFunctions currentService{};
			currentService.set(bitPosition);

			for (auto *facility : *_base->getFacilities())
			{
				if (facility->getBuildTime() > 0) continue;

				auto facilityServices = facility->getRules()->getProvidedBaseFunc();
				if ((facilityServices & currentService).none()) continue;

				providesService = true;
				row = {idItem, idParent, false, tr(facility->getRules()->getType()), 1, 1, ""};
				row.valueOverride = ".";
				idItem = addToDetailsVector(row, false);
			}
			//if (providesService)
			{
				std::string serviceName = tr(_game->getMod()->getBaseFunctionNames(currentService).front());
				int subAmount = calculateSubtotalAmount(idParent) > 0 ? calculateSubtotalAmount(idParent) : -1;

				row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_SERVICE").arg(serviceName), subAmount, 1, ""};
				row.valueOverride = providesService ? tr("STR_YES") : tr("STR_NO");
				subCategories.push_back(row);

				idParent++;
			}
		}
	}

	if (isSubtotalNeeded(0)) // Providers
	{
		int subTotal = calculateSubtotalValue(0);
		int subAmount = calculateSubtotalAmount(0);
		row = {0, 0, true, tr("STR_BIDS_SUBTOTAL_LIVING_SPACE_PROVIDERS"), subAmount == 0 ? -1 : subAmount, subTotal, ""};
		subCategories.push_back(row);
	}
	if (isSubtotalNeeded(1)) // Users
	{
		int subTotal = calculateSubtotalValue(1);
		int subAmount = calculateSubtotalAmount(1);
		row = {1, 1, true, tr("STR_BIDS_SUBTOTAL_LIVING_SPACE_USERS"), subAmount == 0 ? -1 : subAmount, subTotal, ""};
		subCategories.push_back(row);
	}

	// Prefer alphabetical listing.
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
 * Setup storage providers (and usage) screen.
 *
 * Recognize 2 subtotals may exist:
 * (0) Facilities (and items) providing storage space
 * (1) Items (and facilities) taking up storage space
 *
 * Deliberately not shown:
 * - List of space usage per item: use storestate for that.
 */
void BaseInfoDetailsState::categoryStorage()
{
	int idItem = 2; // Offset based on expected subtotal entries.
	int idParent = 0;
	int itemValue;  // Always positive, unless a subtotal.
	std::vector<BeanCounter> subCategories;
	BeanCounter row;  // Workhorse

	// Facility store space
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0 || facility->getRules()->getStorage() == 0) continue;

		itemValue = facility->getRules()->getStorage();
		idParent = itemValue >= 0 ? 0 : 1;
		row = {idItem, idParent, false, tr(facility->getRules()->getType()) , 1, std::abs(itemValue), ""};
		idItem = addToDetailsVector(row, false);
	}
	// Item store space (only interested in total)
	double itemValPos = 0, itemValNeg = 0;
	// Based on StoreState::initList() & Base::getUsedStores().
	for (auto& item : _game->getMod()->getItemsList())
	{
		auto rule = _game->getMod()->getItem(item, true);
		int qty = _base->getStorageItems()->getItem(item);
		// From crafts on base
		for (auto* craft : *_base->getCrafts())
		{
			qty += craft->getTotalItemCount(rule);
		}
		// From transfers
		for (auto* transfer : *_base->getTransfers())
		{
			if (transfer->getCraft())
			{
				qty += transfer->getCraft()->getTotalItemCount(rule);
			}
			else if (transfer->getItems() == item)
			{
				qty += transfer->getQuantity();
			}
		}

		double size = rule->getSize();
		if (size > 0)
		{
			itemValPos += qty * size;
		}
		else if (size < 0)
		{
			itemValNeg -= qty * size;
		}

	}
	if (itemValPos > 0)
	{
		itemValue = (int)std::round(itemValPos);
		idParent = 1;
		row = {idItem, idParent, false, tr("STR_ITEMS_UC") , -1, itemValue, ""};
		idItem = addToDetailsVector(row, false);
	}
	if (itemValNeg > 0)
	{
		itemValue = (int)std::round(itemValNeg);
		idParent = 0;
		row = {idItem, idParent, false, tr("STR_ITEMS_UC") , -1, itemValue, ""};
		idItem = addToDetailsVector(row, false);
	}

	// Subtotals
	if (isSubtotalNeeded(0)) // Available space
	{
		int subTotal = calculateSubtotalValue(0);
		int subAmount = calculateSubtotalAmount(0);
		row = {0, 0, true, tr("STR_BIDS_SUBTOTAL_STORAGE_PROVIDERS"), subAmount == 0 ? -1 : subAmount, subTotal, ""};
		subCategories.push_back(row);
	}
	if (isSubtotalNeeded(1)) // Available space
	{
		int subTotal = calculateSubtotalValue(1);
		int subAmount = calculateSubtotalAmount(1);
		row = {1, 1, true, tr("STR_BIDS_SUBTOTAL_STORAGE_USERS"), subAmount == 0 ? -1 : subAmount, subTotal, ""};
		subCategories.push_back(row);
	}

	// Prefer alphabetical listing.
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
 * Setup laboratory functionality screen.
 *
 * Recognize multiple subcategories:
 *  - Facilities contributing to laboratory space.
 *  - Overview of assigned laboratory space
 *    + Project base space total
 *    + Lump sum of assigned scientists
 *  - Services needed for (known) manufacture projects.
 *
 * Deliberately not shown:
 *  - Overview of details per project.
 *    + One can use the research screen for that.
 */
void BaseInfoDetailsState::categoryLabs()
{
	// Recognize research projects might depend on base services.
	// We want to show facilities providing those.
	// Based on altered version of SavedGame::getAvailableProduction()
	RuleBaseFacilityFunctions requiredServices, providedServices;

	// Available research independent from base (yup ...part of that method is no longer save converter only).
	std::vector<RuleResearch *> availableResearch;
	_game->getSavedGame()->getAvailableResearchProjects(availableResearch, _game->getMod(), 0);
	for (auto unlockedProject : availableResearch)
	{
		requiredServices |= unlockedProject->getRequireBaseFunc();
	}
	// Finished research
	for (auto finishedProject : _game->getSavedGame()->getDiscoveredResearch())
	{
		requiredServices |= finishedProject->getRequireBaseFunc();
	}
	// Check if we are allowed to know this service based on facility knowledge.
	// Based on: BuildFacilitiesState::populateBuildList()
	for (auto facilityType : _game->getMod()->getBaseFacilitiesList())
	{
		RuleBaseFacility *rule = _game->getMod()->getBaseFacility(facilityType);
		if (!rule->isAllowedForBaseType(_base->isFakeUnderwater()))
		{
			continue;
		}
		// Check if we can see facility in ufopaedia (less strict than check if we can build).
		ArticleDefinition *article =  _game->getMod()->getUfopaediaArticle(rule->getType(), false);
		if (article && !Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
		{
			continue;
		}
		providedServices |= rule->getProvidedBaseFunc();
	}
	requiredServices &= providedServices;

	int idItem = requiredServices.count() + 2; // Offset based on expected subtotal entries.
	int idParent = 0;
	int itemValue;
	std::vector<BeanCounter> subCategories;
	BeanCounter row;  // Workhorse

	// Lab space
	bool hasLabs = false;
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0 || facility->getRules()->getLaboratories() == 0) continue;

		hasLabs = true;
		itemValue = facility->getRules()->getLaboratories();
		row = {idItem, idParent, false, tr(facility->getRules()->getType()) , 1, itemValue, ""};
		idItem = addToDetailsVector(row, false);
	}
	if (hasLabs)
	{
		// Subtotal
		int subTotal = calculateSubtotalValue(idParent);
		int subAmount = calculateSubtotalAmount(idParent);
		row = {idParent, idParent, true, tr("STR_LABORATORIES"), subAmount, subTotal, ""};
		subCategories.push_back(row);

		idParent++;

		// Usage is a separate category (in case we want more details)
		for (auto research : _base->getResearch())
		{
			// In case future changes allow research projects to have space requirements
			itemValue = 0; // research->getRules()->getRequiredSpace();
			if (itemValue > 0)
			{
				row = {idItem, idParent, false, tr("BIDS_DETAIL_LABS_PROJECT_SPACE") , 1, itemValue, ""};
				idItem = addToDetailsVector(row, true);
			}
			itemValue = research->getAssigned();
			if (itemValue > 0)
			{
				row = {idItem, idParent, false, tr("BIDS_DETAIL_LABS_ENGINEERS") , 1, itemValue, ""};
				idItem = addToDetailsVector(row, true);
			}
		}
		subTotal = _base->getUsedLaboratories();
		subAmount = _base->getResearch().size();
		row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_LABS_USED"), subAmount, subTotal, ""};
		subCategories.push_back(row);

		idParent++;
	}

	// Facilities per required services
	// Based on 'Mod::getBaseFunctionNames()' (want to test my bit field skills).
	for (size_t bitPosition = 0; bitPosition < requiredServices.size(); ++bitPosition)
	{
		if (requiredServices.test(bitPosition))
		{
			bool providesService = false;
			RuleBaseFacilityFunctions currentService{};
			currentService.set(bitPosition);

			for (auto *facility : *_base->getFacilities())
			{
				if (facility->getBuildTime() > 0) continue;

				auto facilityServices = facility->getRules()->getProvidedBaseFunc();
				if ((facilityServices & currentService).none()) continue;

				providesService = true;
				row = {idItem, idParent, false, tr(facility->getRules()->getType()), 1, 1, ""};
				row.valueOverride = ".";
				idItem = addToDetailsVector(row, false);
			}
			//if (providesService)
			{
				std::string serviceName = tr(_game->getMod()->getBaseFunctionNames(currentService).front());
				int subAmount = calculateSubtotalAmount(idParent) > 0 ? calculateSubtotalAmount(idParent) : -1;

				row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_SERVICE").arg(serviceName), subAmount, 1, ""};
				row.valueOverride = providesService ? tr("STR_YES") : tr("STR_NO");
				subCategories.push_back(row);

				idParent++;
			}
		}
	}

	// Prefer alphabetical listing.
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
 * Setup workshop functionality screen.
 *
 * Recognize multiple subcategories:
 *  - Facilities contributing to Workshop space.
 *  - Overview of assigned workshop space
 *    + Project base space total
 *    + Lump sum of assigned engineers
 *  - Services needed for (known) manufacture projects.
 *
 * Deliberately not shown:
 *  - Overview of details per project.
 *    + One can use the manufacture screen for that.
 */
void BaseInfoDetailsState::categoryWorkshops()
{
	// Recognize workshop projects might depend on base services.
	// We want to show facilities providing those.
	// Based on SavedGame::getAvailableProduction()
	RuleBaseFacilityFunctions requiredServices, providedServices;
	for (auto manufactureProject : _game->getMod()->getManufactureList())
	{
		RuleManufacture *ruleManufacture = _game->getMod()->getManufacture(manufactureProject);
		if (!_game->getSavedGame()->isResearched(ruleManufacture->getRequirements()))
		{
			continue;
		}
		requiredServices |= ruleManufacture->getRequireBaseFunc();
	}
	// Check if we are allowed to know this service based on facility knowledge.
	// Based on: BuildFacilitiesState::populateBuildList()
	for (auto facilityType : _game->getMod()->getBaseFacilitiesList())
	{
		RuleBaseFacility *rule = _game->getMod()->getBaseFacility(facilityType);
		if (!rule->isAllowedForBaseType(_base->isFakeUnderwater()))
		{
			continue;
		}
		// Check if we can see facility in ufopaedia (less strict than check if we can build).
		ArticleDefinition *article =  _game->getMod()->getUfopaediaArticle(rule->getType(), false);
		if (article && !Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
		{
			continue;
		}
		providedServices |= rule->getProvidedBaseFunc();
	}
	requiredServices &= providedServices;

	int idItem = requiredServices.count() + 2; // Offset based on expected subtotal entries.
	int idParent = 0;
	int itemValue;
	std::vector<BeanCounter> subCategories;
	BeanCounter row;  // Workhorse

	// Workshop space
	bool hasWorkshops = false;
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0 || facility->getRules()->getWorkshops() == 0) continue;

		hasWorkshops = true;
		itemValue = facility->getRules()->getWorkshops();
		row = {idItem, idParent, false, tr(facility->getRules()->getType()) , 1, itemValue, ""};
		idItem = addToDetailsVector(row, false);
	}
	if (hasWorkshops)
	{
		// Subtotal
		int subTotal = calculateSubtotalValue(idParent);
		int subAmount = calculateSubtotalAmount(idParent);
		row = {idParent, idParent, true, tr("STR_WORKSHOP"), subAmount, subTotal, ""};
		subCategories.push_back(row);

		idParent++;

		// Usage is a separate category (in case we want more details)
		for (auto production : _base->getProductions())
		{
			itemValue = production->getRules()->getRequiredSpace();
			if (itemValue > 0)
			{
				row = {idItem, idParent, false, tr("BIDS_DETAIL_WORKSHOP_PROJECT_SPACE") , 1, itemValue, ""};
				idItem = addToDetailsVector(row, true);
			}
			itemValue = production->getAssignedEngineers();
			if (itemValue > 0)
			{
				row = {idItem, idParent, false, tr("BIDS_DETAIL_WORKSHOP_ENGINEERS") , 1, itemValue, ""};
				idItem = addToDetailsVector(row, true);
			}
		}
		subTotal = _base->getUsedWorkshops();
		subAmount = _base->getProductions().size();
		row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_WORKSHOP_USED"), subAmount, subTotal, ""};
		subCategories.push_back(row);

		idParent++;
	}

	// Facilities per required services
	// Based on 'Mod::getBaseFunctionNames()' (want to test my bit field skills).
	for (size_t bitPosition = 0; bitPosition < requiredServices.size(); ++bitPosition)
	{
		if (requiredServices.test(bitPosition))
		{
			bool providesService = false;
			RuleBaseFacilityFunctions currentService{};
			currentService.set(bitPosition);

			for (auto *facility : *_base->getFacilities())
			{
				if (facility->getBuildTime() > 0) continue;

				auto facilityServices = facility->getRules()->getProvidedBaseFunc();
				if ((facilityServices & currentService).none()) continue;

				providesService = true;
				row = {idItem, idParent, false, tr(facility->getRules()->getType()), 1, 1, ""};
				row.valueOverride = ".";
				idItem = addToDetailsVector(row, false);
			}
			//if (providesService)
			{
				std::string serviceName = tr(_game->getMod()->getBaseFunctionNames(currentService).front());
				int subAmount = calculateSubtotalAmount(idParent) > 0 ? calculateSubtotalAmount(idParent) : -1;

				row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_SERVICE").arg(serviceName), subAmount, 1, ""};
				row.valueOverride = providesService ? tr("STR_YES") : tr("STR_NO");
				subCategories.push_back(row);

				idParent++;
			}
		}
	}

	// Prefer alphabetical listing.
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
 * Setup alien containment space providers screen.
 *
 * Recognizes multiple subtotals may exist due to different 'prison' types.
 */
void BaseInfoDetailsState::categoryAlienContainment()
{
	int idItem = 100; // Ensure details use childId's > than theoretical maximum subcategories of 36.
	int idParent = 0;
	int itemValue;
	std::vector<BeanCounter> subCategories;
	BeanCounter row;  // List's workhorse.

	// Recognize multiple containment types might exist.
	std::set<int> containmentTypes {0}; // Default alien containment.
	for (auto *facility : *_base->getFacilities())
	{
		// No guardian: We want to recognize containments under construction.
		containmentTypes.insert(facility->getRules()->getPrisonType());
	}

	// Facilities per Prison type
	for (auto prisonType : containmentTypes)
	{
		bool hasPrisonType = false;
		for (auto *facility : *_base->getFacilities())
		{
			// Skip non prison buildings or prisons of the wrong type.
			// Allow 'under construction'.
			if (facility->getRules()->getPrisonType() != prisonType || facility->getRules()->getAliens() == 0)
				continue;

			hasPrisonType = true;
			std::ostringstream facilityName;

			facilityName << tr(facility->getRules()->getType());
			if (facility->getBuildTime() > 0)
			{
				facilityName << " " << tr("STR_UNDER_CONSTRUCTION");
			}
			itemValue = facility->getRules()->getAliens();
			row = {idItem, idParent, false, facilityName.str().c_str(), 1, itemValue, ""};
			idItem = addToDetailsVector(row, false);
		}
		if (hasPrisonType)
		{
			int inUse = _base->getUsedContainment(prisonType);
			// Total usage detail row.
			row = {idItem, idParent, false, tr("BIDS_DETAIL_CONTAINMENT_USAGE"), inUse, 1, ""}; // or use "trAlt() so modder can show a specified description."
			row.valueOverride = trAlt("STR_ALIEN", prisonType);
			idItem = addToDetailsVector(row, false);

			// Subtotal
			std::ostringstream description, usage;
			description << tr("STR_ALIEN_CONTAINMENT") << " - " << trAlt("STR_ALIEN", prisonType);
			int subTotal = _base->getAvailableContainment(prisonType);
			row = {idParent, idParent, true, description.str().c_str(), -1, subTotal, ""};
			usage << inUse << "/" << subTotal;
			row.valueOverride = usage.str();
			subCategories.push_back(row);
		}
		idParent++;
	}

	// Prefer alphabetical listing.
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
 * Setup hangar providers (and usage) screen.
 *
 * Recognize 2 subtotals may exist:
 * (0) Facilities proving hangar space
 * (1) Crafts demanding hangar space
*/
void BaseInfoDetailsState::categoryHangars()
{
	int idItem = 2; // Offset based on expected subtotal entries.
	int idParent = 0;   // Let parentId represent the numbers as described in method description.
	int itemValue;  // Always positive, unless a subtotal.
	std::vector<BeanCounter> subCategories;
	BeanCounter row;

	// Hangar space provided
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction and non-hangars.
		if (facility->getBuildTime() > 0 || facility->getRules()->getCrafts() == 0) continue;

		// Hangar space per facility type
		itemValue = facility->getRules()->getCrafts();
		row = {idItem, idParent, false, tr(facility->getRules()->getType()) , 1, itemValue, ""};
		idItem = addToDetailsVector(row, false);
	}
	// Unconditionally show subCategory.
	{
		int subTotal = calculateSubtotalValue(idParent);
		int subAmount = calculateSubtotalAmount(idParent);

		row = {idParent, idParent, true, tr("STR_HANGARS"), subAmount, subTotal, ""};
		subCategories.push_back(row);

		idParent++;
	}

	// Hangar space claimed
	// Not sure:
	//  Does not add info that is not visible in the same way elsewhere
	//  Unless the option of craft requiring specific hangars (or craft
	//  requiring more hangar space) comes into play.
	bool hasCrafts = false;
	for (auto craft : *_base->getCrafts())
	{
		hasCrafts = true;
		// itemValue = 1; // Unless that becomes a mod variable.
		row = {idItem, idParent, false, craft->getName(_game->getLanguage()), 1, 1, ""};
		idItem = addToDetailsVector(row, false);
	}
	for (auto transfer : *_base->getTransfers())
	{
		if (transfer->getType() == TRANSFER_CRAFT)
		{
			hasCrafts = true;
			std::ostringstream craftName;
			craftName << transfer->getName(_game->getLanguage());
			craftName << " (" << tr("STR_TRANSFER") << ")";
			row = {idItem, idParent, false, craftName.str().c_str(), 1, 1, ""};
			idItem = addToDetailsVector(row, false);
		}
	}
	if (hasCrafts)
	{
		int subTotal = calculateSubtotalValue(idParent);
		int subAmount = calculateSubtotalAmount(idParent);

		row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_HANGARS_USED"), subAmount, subTotal, ""};
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
 * Setup base defense abilities screen.
 *
 * Facilities contributing to the following subcategories:
 *  - Defense strength
 *  - Defense hitchances.
 *  - Gravitational shields (in oxc they stack).
 *
 * Will not show the following subcategories:
 * + Missile attraction of a facility.
 *   - Seems like it should be a hidden stat.
 * + Hitchance per combined power output (or a selected subset).
 *   - Has to account for 36 defenses and shields on a base (through modding).
 *     Meaning: 2^36 ~= 6.8*10^10 permutations, not even taking grav shields into account.
 *   - Unless one comes up with a clever mathematical solution this is way
 *     too computational expensive (for too little gain).
*/
void BaseInfoDetailsState::categoryDefense()
{
	int idItem = 100; // Just to ensure details use childId's > idParents
	int idParent = 0; // Unique subcategories.
	int itemValue;    // Prefer positive values only for details (subtotals are allowed to be negative)
	std::vector<BeanCounter> subCategories;
	BeanCounter row;  // List's workhorse.

	// Calculate probability of landing at least one hit
	auto atLeastOneHit = [&](int parentId) -> int
	{
		double detectionFail = 1.0;
		for (auto element : _details)
		{
			if (element.parentId == parentId && element.childId != element.parentId)
			{
				for (int i = 0 ; i < element.amount ; i++ )
				{
					detectionFail *= (100 - element.value)/100.0;
				}
			}
		}
		return (int) std::round(100 * (1.0 - detectionFail));
	};

	// Defense strength & ratio
	bool hasDefenses = false;
	for (auto *facility : *_base->getFacilities())
	{
		// Categories are fixed and 'idItem' only needs to be unique.
		// Hence it is ok to do both main defense subcategories at once.
		if (facility->getBuildTime() > 0 || facility->getRules()->getDefenseValue() == 0) continue;

		hasDefenses = true;

		// Strength
		itemValue = facility->getRules()->getDefenseValue();
		row = {idItem, idParent, false, tr(facility->getRules()->getType()), 1, itemValue, ""};
		idItem = addToDetailsVector(row, false);

		// Hit ratio
		itemValue = facility->getRules()->getHitRatio();
		row = {idItem, idParent + 1, false, tr(facility->getRules()->getType()), 1, itemValue, ""};
		row.valueOverride = Unicode::formatPercentage(itemValue);
		idItem = addToDetailsVector(row, false);
	}
	if (hasDefenses)
	{
		int subAmount = -1;
		int subTotal = calculateSubtotalValue(idParent);
		row = {idParent, idParent, true, tr("STR_DEFENSE_STRENGTH"), subAmount, subTotal, ""};
		subCategories.push_back(row);

		subTotal = atLeastOneHit(idParent + 1);
		row = {idParent + 1, idParent + 1 , true, tr("STR_HIT_RATIO"), subAmount, subTotal, ""};
		row.valueOverride = Unicode::formatPercentage(subTotal);
		subCategories.push_back(row);

		idParent += 2;
	}

	// Defensive shields
	bool hasShields = false;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0 || !facility->getRules()->isGravShield()) continue;

		hasShields = true;

		row = {idItem, idParent, false, tr(facility->getRules()->getType()), 1, 1, ""};
		idItem = addToDetailsVector(row, false);
	}
	if (hasShields)
	{
		int subAmount = -1;
		int subTotal = calculateSubtotalValue(idParent);
		row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_GRAV_SHIELD"), subAmount, subTotal, ""};
		subCategories.push_back(row);

		//idParent++;
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
 * Setup base detection abilities and camouflage screen.
 *
 * Facilities contributing to the following subcategories:
 *  - Base Camouflage: Chance of staying undetected.
 *  - UFO detection per range, including hyperwave abilities.
 *  - Alien Base detection per range.
 */
void BaseInfoDetailsState::categoryDetection()
{
	int idItem = 100; // Ensure details use childId's > than theoretical maximum subcategories of 74 (2*36 + 2).
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
			if (element.parentId == parentId && element.childId != element.parentId)
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
		row.valueOverride = Unicode::formatPercentage(subTotal);
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
			row.valueOverride = Unicode::formatPercentage(itemValue);
			idItem = addToDetailsVector(row, false);

			if (!facility->getRules()->isHyperwave())
				continue;

			// Facility provides hyperwave ability for this range.
			hasHyperwaveFunctionality = true;
			itemValue = 0; // This entry should not influence subtotal chance calculation.
			row = {idItem, idParent, false, tr(facility->getRules()->getType()) , 1, itemValue, ""};
			row.valueOverride = tr("BIDS_DETAIL_DETECTION_HYPERWAVE");
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
		row.valueOverride = Unicode::formatPercentage(subTotal);
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
			row.valueOverride = Unicode::formatPercentage(itemValue);
			idItem = addToDetailsVector(row, false);
		}

		// SubCategory
		int subAmount = -1;
		int subTotal = detectionResult(idParent);
		row = {idParent, idParent, true, tr("BIDS_SUBTOTAL_ALIEN_BASE_DETECTION").arg(detectionRange), subAmount, subTotal, ""};
		row.valueOverride = Unicode::formatPercentage(subTotal);
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
void BaseInfoDetailsState::drawList()
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
		if (_details[i].parentId != _details[i].childId) // Not a subtotal.
		{
			description.insert(0, " "); // Do not use dots for description indentation.
			ssAmount << tr("MCDS_DOTTED_INDENTATION");
			ssValue << tr("MCDS_DOTTED_INDENTATION");
			//unconditionallyShowSign = false;
		}

		if (_details[i].amountOverride != "")
		{
			ssAmount << _details[i].amountOverride;
		}
		else
		{
			ssAmount << _details[i].amount;
		}

		if (_details[i].valueOverride != "")
		{
			ssValue << _details[i].valueOverride;
		}
		else
		{
			ssValue << _details[i].value;
		}


		if (_details[i].amount > -1)
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
* Convert value to string representation (appended with "Hp").
*/
std::string BaseInfoDetailsState::toStringHp(float value)
{
	std::ostringstream ssValue;
	//ssValue << std::fixed << std::setprecision(2);
	ssValue << value << " " << tr("STR_HEALTH_ABBREVIATION");
	return ssValue.str();
}
std::string BaseInfoDetailsState::toStringHp(int value)
{
	return std::to_string(value) + " " + tr("STR_HEALTH_ABBREVIATION").c_str();
}

/**
* Convert value to string representation (appended with "Man").
*/
std::string BaseInfoDetailsState::toStringMana(int value)
{
	return std::to_string(value) + " " + tr("STR_MANA_ABBREVIATION").c_str();
}
/**
* Convert value to string representation (appended with "%").
*/
std::string BaseInfoDetailsState::toStringPercent(float value)
{
	std::ostringstream ssValue;
	//ssValue << std::fixed << std::setprecision(2);
	ssValue << value << "%";
	return ssValue.str();
}

}
