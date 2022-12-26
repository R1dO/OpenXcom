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

#include <iomanip>
#include <climits>
#include "BaseInfoDetailsState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleCountry.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
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
	_txtTotal = new Text(133, 9, 171, 154);

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
	add(_txtTotal, "text", "baseInfoDetails");

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

	// Check which services are known for this base type.
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
		_unlockedServicesBaseType |= rule->getProvidedBaseFunc();
	}

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
	case DC_ENGINEERS:
	case DC_SCIENTISTS:
		_currentCategory = DC_QUARTERS;
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
	case DC_ENGINEERS:
	case DC_SCIENTISTS:
		_currentCategory = DC_SOLDIERS;
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

	// Flip visibility of child elements.
	for (size_t i = 0; i < _details.size(); ++i)
	{
		if (_details[i].parentId == _details[i].childId) continue;
		if (_details[i].parentId != getRow().parentId) continue;

		_details[i].isVisible ^= true;
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
	_txtTotal->clear();
	std::ostringstream ssTitle;

	switch (_currentCategory)
	{
	case DC_SOLDIERS:
		categorySoldiers();
		return; // Temporal until other cases uses new scheme.
		//break;
	case DC_QUARTERS:
		categoryQuarters();
		return; // Temporal until other cases uses new scheme.
		//break;
	case DC_STORES:
		categoryStorage();
		return; // Temporal until other cases uses new scheme.
		//break;
	case DC_LABORATORIES:
		categoryLabs();
		return; // Temporal until other cases uses new scheme.
		//break;
	case DC_WORKSHOPS:
		categoryWorkshops();
		return; // Temporal until other cases uses new scheme.
		//break;
	case DC_CONTAINMENT:
		categoryAlienContainment();
		return; // Temporal until other cases uses new scheme.
		//break;
	case DC_HANGARS:
		categoryHangars();
		return; // Temporal until other cases uses new scheme.
		//break;
	case DC_DEFENSE:
		categoryDefense();
		return; // Temporal until other cases uses new scheme.
		//break;
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
			row.valueOverride = toStringHp(99.9f) + " + " + toStringPercent(99.9f);
			_details.push_back(row);
			childId++;
			for (auto j = 0; j < 5; j++)
			{
				row = {childId, parent, false, "Normally collapsed (moaar details)", 99, 999999999, {}};
				_details.push_back(row);
				childId++;
			}
		}
		std::ostringstream ss;
		ss << tr("STR_TOTAL") << ">\t" <<  Unicode::formatFunding(999999999999);
		_txtTotal->setText(ss.str());
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
 *  + Transformations related (it only acts on existing soldiers)
 */
void BaseInfoDetailsState::categorySoldiers()
{
	_txtTitle->setText(tr("STR_SOLDIERS"));
	_txtQuantity->setText(tr("STR_BIDS_FACILITIES"));
	// Total does **not** include soldiers being produced (via manufacture).
	// Could lead to 'bug-reports' from number enthusiasts.
	std::ostringstream ss;
	ss << tr("STR_SOLDIERS") << ">\t" << Unicode::TOK_COLOR_FLIP <<_base->getTotalSoldiers();
	_txtTotal->setText(ss.str());

	addSubCategoryPsionicTraining();
	addSubCategoryPhysicalTraining();
	addSubCategoryWoundRecovery();
	addSubCategoryWoundRecoveryInProgress();
	addSubCategoryHealthRecovery();
	addSubCategoryHealthRecoveryInProgress();
	addSubCategoryManaRecovery();
	addSubCategoryManaRecoveryInProgress();
	addSubCategoryTransformations();

	drawList();
}

/**
 * Add health recovery overview to _details vector.
 *
 * A list of base facilities contributing to recovery.
 */
void BaseInfoDetailsState::addSubCategoryHealthRecovery()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	auto recoveryRates = _base ? _base->getSumRecoveryPerDay() : BaseSumDailyRecovery();
	// Negative health regeneration not possible, 0 is default.
	if (recoveryRates.HealthRecovery == 0) return;

	// Subcategory header (show unconditionally)
	std::string valueOverride = toStringHp(recoveryRates.HealthRecovery);
	std::string description = tr("STR_BIDS_SUBTOTAL_RECOVERY_FACILITIES").arg(tr("STR_HEALTH"));
	row = {parentId, parentId, true, description, 0, 0, "", valueOverride};
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
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
	}
}

/**
 * Add health recovery usage overview to _details vector.
 *
 * A list of affected soldiers (since not visible on other screens).
 *
 * @note
 * Hp recovers only after all wounds are healed but soldier is not yet
 * at full HP. Can occur due to health loss from battle or scripts.
 *
 * @note
 * Logic based on: `Soldier::replenishStats`.
 */
void BaseInfoDetailsState::addSubCategoryHealthRecoveryInProgress()
{

	// Might as well list soldiers harassing nurses.
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header
	std::string description = tr("STR_BIDS_SUBTOTAL_RECOVERY_USAGE").arg(tr("STR_HEALTH")).arg(tr("STR_SOLDIERS"));
	row = {parentId, parentId, false, description, 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	for (auto soldier : *_base->getSoldiers())
	{
		// Soldiers in sickbay are listed under a different subtotal.
		if (soldier->getWoundRecoveryInt() >= 0) continue;

		int itemValue = soldier->getHealthMissing();
		if (itemValue == 0) continue;

		row = {idItem, parentId, false, soldier->getName(), 1, itemValue, ".", toStringHp(itemValue)};
		idItem = addToDetailsVector(row);
	}
	if (_details.size() > parentIndex + 1)
	{
		sortChildren(parentIndex);
		_details[parentIndex].isVisible = true;
		_details[parentIndex].value = _details.size() - parentIndex - 1;
	}
}

/**
 * Add mana recovery overview to _details vector.
 *
 * A list of base facilities contributing to recovery.
 */
void BaseInfoDetailsState::addSubCategoryManaRecovery()
{
	if (!_game->getMod()->isManaFeatureEnabled()) return;
	if (!_game->getSavedGame()->isManaUnlocked(_game->getMod())) return;

	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	auto recoveryRates = _base ? _base->getSumRecoveryPerDay() : BaseSumDailyRecovery();
	// No check if manaRecoverRate is 0
	// In case mod defines facilities with both positive and negative recovery.

	// Subcategory header (show unconditionally)
	std::string valueOverride = toStringMana(recoveryRates.ManaRecovery);
	std::string description = tr("STR_BIDS_SUBTOTAL_RECOVERY_FACILITIES").arg(tr("STR_MANA"));
	row = {parentId, parentId, true, description, 0, 0, "", valueOverride};
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
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
	}
}

/**
 * Add mana recovery usage overview to _details vector.
 *
 * A list of affected soldiers (since not visible on other screens).
 *
 * @note
 * For positive recoveryRates mana recovery occurs after all wounds are healed.
 * For negative 'recovery' it always occurs.
 *
 * @note
 * Logic based on: `Soldier::replenishStats`.
 */
void BaseInfoDetailsState::addSubCategoryManaRecoveryInProgress()
{
	if (!_game->getMod()->isManaFeatureEnabled()) return;
	if (!_game->getSavedGame()->isManaUnlocked(_game->getMod())) return;

	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header
	std::string description = tr("STR_BIDS_SUBTOTAL_RECOVERY_USAGE").arg(tr("STR_MANA")).arg(tr("STR_SOLDIERS"));
	row = {parentId, parentId, false, description, 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	for (auto soldier : *_base->getSoldiers())
	{
		// Soldiers in sickbay are listed under a different subtotal.
		if (soldier->getWoundRecoveryInt() > 0) continue;

		int itemValue = soldier->getManaMissing();
		if (itemValue == 0) continue;

		row = {idItem, parentId, false, soldier->getName(), 1, itemValue, ".", toStringMana(itemValue)};
		idItem = addToDetailsVector(row);
	}
	if (_details.size() > parentIndex + 1)
	{
		sortChildren(parentIndex);
		_details[parentIndex].isVisible = true;
		_details[parentIndex].value = _details.size() - parentIndex - 1;
	}
}

/**
 * Add physical training overview to _details vector.
 *
 * Child elements include:
 * - List of base facilities adding space.
 * - Grand total of used space.
 *   + See `AllocatePsiTrainingState` for per soldier overview.
 */
void BaseInfoDetailsState::addSubCategoryPhysicalTraining()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
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
		sortChildren(parentIndex, 1);
		_details[parentIndex].isVisible = true;
		_details[parentIndex].amount = facilities;
		_details[parentIndex].valueOverride =
			tr("STR_BIDS_ASSIGNED_VS_TOTAL").arg(_base->getUsedTraining()).arg(_base->getAvailableTraining());
	}
}

/**
 * Add psionic training overview to _details vector.
 *
 * Child elements include:
 * - List of base facilities adding space.
 * - Grand total of used space.
 *   + See `AllocatePsiTrainingState` for per soldier overview.
*/
void BaseInfoDetailsState::addSubCategoryPsionicTraining()
{
	if (!_game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements()))
		return;

	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
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
		sortChildren(parentIndex, 1);
		_details[parentIndex].isVisible = true;
		_details[parentIndex].amount = facilities;
		_details[parentIndex].valueOverride =
			tr("STR_BIDS_ASSIGNED_VS_TOTAL").arg(_base->getUsedPsiLabs()).arg(_base->getAvailablePsiLabs());
	}
}

/**
 * Add transformation related facilities to _details vector.
 *
 * A list of facilities providing required services for transformations
 * appended with a list of missing services.
 */
void BaseInfoDetailsState::addSubCategoryTransformations()
{
	// Required services for known transformations.
	// Based on: SavedGame::getAvailableTransformations()
	RuleBaseFacilityFunctions requiredServices;
	for (auto transformer : _game->getMod()->getSoldierTransformationList())
	{
		RuleSoldierTransformation *ruleTransform = _game->getMod()->getSoldierTransformation(transformer);
		if (!_game->getSavedGame()->isResearched(ruleTransform->getRequiredResearch()))
			continue;
		requiredServices |= ruleTransform->getRequiredBaseFuncs();
	}
	// Determine dependencies on unlocked (potential) base services.
	// We want to include missing services (but only if player can solve that problem).
	requiredServices &= _unlockedServicesBaseType;
	if (requiredServices.none()) return;

	addServices(requiredServices, tr("STR_BIDS_SUBTOTAL_TRANSFORMATION_SERVICES"));
}

/**
 * Add wound recovery overview to _details vector.
 *
 * A list of base facilities contributing to recovery.
 */
void BaseInfoDetailsState::addSubCategoryWoundRecovery()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	auto recoveryRates = _base ? _base->getSumRecoveryPerDay() : BaseSumDailyRecovery();

	// Subcategory header (show unconditionally)
	std::string valueOverride = toStringHp(recoveryRates.SickBayAbsoluteBonus + 1.0f)
		+ " + " + toStringPercent(recoveryRates.SickBayRelativeBonus);
	std::string description = tr("STR_BIDS_SUBTOTAL_RECOVERY_FACILITIES").arg(tr("STR_WOUND"));
	row = {parentId, parentId, true, description, 0, 0, "", valueOverride};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0) continue;

		float absBonus = facility->getRules()->getSickBayAbsoluteBonus();
		float relBonus = facility->getRules()->getSickBayRelativeBonus();
		if (absBonus == 0.0f && relBonus == 0.0f) continue;

		int itemValue = 0; // In case value sorting becomes desired.
		// Allow display of negative wound regeneration.
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
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
	}
}

/**
 * Add wound recovery usage overview to _details vector.
 *
 * A list of affected soldiers (since not visible on other screens).
 *
 * @note
 * Normally between 1/2 and 3/2 of health loss from battle.
 * Can also occur due to transformations or scripts.
 *
 * @note
 * Logic based on: `BattleUnit::postMissionProcedures`.
 */
void BaseInfoDetailsState::addSubCategoryWoundRecoveryInProgress()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header
	std::string description = tr("STR_BIDS_SUBTOTAL_RECOVERY_USAGE").arg(tr("STR_WOUND")).arg(tr("STR_SOLDIERS"));
	row = {parentId, parentId, false, description, 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	for (auto soldier : *_base->getSoldiers())
	{
		// Integer time as returned from 'Soldier::getWoundRecoveryInt()'
		// equals the amount of wound HP.
		int woundHP = soldier->getWoundRecoveryInt();
		if (woundHP <= 0) continue;

		row = {idItem, parentId, false, soldier->getName(), 1, woundHP, ".", toStringHp(woundHP)};
		idItem = addToDetailsVector(row);
	}
	if (_details.size() > parentIndex + 1)
	{
		sortChildren(parentIndex);
		_details[parentIndex].isVisible = true;
		_details[parentIndex].value = _details.size() - parentIndex - 1;
	}
}

/**
 * Setup living space & hiring functionality screen.
 *
 * Recognize multiple subcategories:
 *  + Facilities contributing to living space.
 *  + Overview of claimed living space
 *  + Services needed to acquire soldiers/scientists/engineers
 */
void BaseInfoDetailsState::categoryQuarters()
{
	_txtTitle->setText(tr("personnel")); // common/language/Technical
	_txtQuantity->setText(tr("STR_BIDS_FACILITIES"));
	_txtTotal->setText(tr("STR_SPACE_AVAILABLE").arg(_base->getAvailableQuarters() - _base->getUsedQuarters()));

	addSubCategoryQuarterProviders();
	addSubCategoryQuarterUsage();
	addSubCategoryHiringServices();

	drawList();
	// Note: Does include soldiers being produced (via manufacture).
}

/**
 * Add living space providers to _details vector.
 *
 * A list of base facilities providing living space.
 */
void BaseInfoDetailsState::addSubCategoryQuarterProviders()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	row = {parentId, parentId, true, tr("STR_BIDS_SUBTOTAL_LIVING_SPACE_PROVIDERS"), 0, _base->getAvailableQuarters(), {}};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		int itemValue = facility->getRules()->getPersonnel();
		if (facility->getBuildTime() > 0 || itemValue <= 0) continue;

		row = {idItem, parentId, false, tr(facility->getRules()->getType()) , 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
	}
}

/**
 * Add overview of claimed living space to _details vector.
 *
 * A list including the following elements:
 * + Grand total per scientist/engineer/soldier type.
 * + Facilities providing negative living space.
 */
void BaseInfoDetailsState::addSubCategoryQuarterUsage()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	int itemValue = 0;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_BIDS_SUBTOTAL_LIVING_SPACE_USERS");
	row = {parentId, parentId, true, description, 0, _base->getUsedQuarters(), ".", ""};
	idItem = addToDetailsVector(row, false);

	// Facilities providing negative space.
	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		itemValue = facility->getRules()->getPersonnel();
		if (facility->getBuildTime() > 0 || itemValue >= 0) continue;

		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, std::abs(itemValue), {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
	}

	// Always show personnel claiming living space (including transfers).
	row = {idItem, parentId, false, tr("STR_SCIENTISTS"), 0, _base->getTotalScientists(), ".", ""};
	idItem = addToDetailsVector(row, false);
	row = {idItem, parentId, false, tr("STR_ENGINEERS"), 0, _base->getTotalEngineers(), ".", ""};
	idItem = addToDetailsVector(row, false);
	// Soldier type
	for (auto soldier : *_base->getSoldiers())
	{
		row = {idItem, parentId, false, tr(soldier->getRules()->getType()), 0, 1, ".", ""};
		idItem = addToDetailsVector(row);
	}
	for (auto transfer : *_base->getTransfers())
	{
		if (transfer->getType() != TRANSFER_SOLDIER) continue;

		// Soldiers and all transformers.
		row = {idItem, parentId, false, tr(transfer->getSoldier()->getRules()->getType()), 0, 1, ".", ""};
		idItem = addToDetailsVector(row);
	}
	// Any person being 'produced'.
	for (auto conceived : _base->getProductions())
	{
		if (conceived->getRules()->getSpawnedPersonType() == "") continue;

		// Assume it is not possible to produce multiple persons with a single project.
		// Seems correct looking at Base::getUsedQuarters()
		row = {idItem, parentId, false, tr(conceived->getRules()->getSpawnedPersonType()), 0, 1, ".", ""};
		idItem = addToDetailsVector(row);
	}
	if (_details.size() > parentIndex + 3 + facilities) // Subtotal + engineers + scientists + facilities
	{
		sortChildren(parentIndex, 2 + facilities);
	}
}

/**
 * Add facilities related to recruiting to _details vector.
 *
 * A list of facilities providing required services for recruitment
 * appended with a list of known missing services.
 */
void BaseInfoDetailsState::addSubCategoryHiringServices()
{
	// Required services for hiring/producing personnel.
	// Based on: SavedGame::getAvailableTransformations()
	RuleBaseFacilityFunctions requiredServices;
	// Scientist & Engineers
	requiredServices |= _game->getMod()->getHireScientistsRequiresBaseFunc();
	requiredServices |= _game->getMod()->getHireEngineersRequiresBaseFunc();
	// Soldiers (per type)
	for (auto soldierType : _game->getMod()->getSoldiersList())
	{
		RuleSoldier *rule = _game->getMod()->getSoldier(soldierType);
		requiredServices |= rule->getRequiresBuyBaseFunc();
	}
	// So ... Frankenstein's lab might exist after all
	for (auto manufactureProject : _game->getMod()->getManufactureList())
	{
		RuleManufacture *ruleManufacture = _game->getMod()->getManufacture(manufactureProject);
		if (ruleManufacture->getSpawnedPersonType() == "") continue;
		if (!_game->getSavedGame()->isResearched(ruleManufacture->getRequirements())) continue;
		requiredServices |= ruleManufacture->getRequireBaseFunc();
	}
	// Determine dependencies on unlocked (potential) base services.
	// We want to include missing services (but only if player can solve that problem).
	requiredServices &= _unlockedServicesBaseType;
	if (requiredServices.none()) return;

	addServices(requiredServices, tr("STR_BIDS_SUBTOTAL_LIVING_SPACE_SERVICES"));
}

/**
 * Setup storage providers (and usage) screen.
 *
 * Facilities (and items) contributing to the following subcategories:
 *  + Providers of storage space.
 *  + Users of storage space.
 *  + Services needed to purchase some items.
 *  + Limits imposed on purchase of items (amount and countries)
 */
void BaseInfoDetailsState::categoryStorage()
{
	_txtTitle->setText(tr("STR_STORES"));
	_txtQuantity->setText(tr("STR_BIDS_FACILITIES"));
	_txtTotal->setText(tr("STR_SPACE_AVAILABLE").arg(_base->getAvailableStores()-_base->getUsedStores()));

	addSubCategoryStorageProviders();
	addSubCategoryStoragesUsage();
	addSubCategoryPurchaseServices();
	addSubCategoryPurchaseLimits();
	addSubCategoryPurchaseCountries();

	drawList();
}

/**
 * Add storage providers to _details vector.
 *
 * The list includes:
 *  + Base facilities providing storage space.
 *  + Grand total of items providing storage space.
 *
 * @note
 * List of space usage per item is deliberately not shown.
 * We have storeState for that functionality.
 */
void BaseInfoDetailsState::addSubCategoryStorageProviders()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	double spaceProvided = 0.0;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_BIDS_SUBTOTAL_STORAGE_PROVIDERS");
	row = {parentId, parentId, true, description, 0, 0, {}};
	idItem = addToDetailsVector(row, false);

	// Start with grand total for items, to keep it at the top.
	bool hasItems = false;
	for (auto& item : _game->getMod()->getItemsList())
	{
		auto rule = _game->getMod()->getItem(item, true);
		double size = rule->getSize();
		if (size >= 0) continue;

		int qty = _base->getStorageItems()->getItem(item)
			+ _base->getItemClaimByCrafts(rule)   // No transfers yet.
			+ _base->getItemCountTransfers(rule); // Includes items from craft transfers.

		spaceProvided += std::abs(size) * qty;
		hasItems |= qty > 0;
	}
	if (hasItems)
	{
		std::ostringstream ssValue;
		ssValue << std::fixed << std::setprecision(3) << spaceProvided;
		row = {idItem, parentId, false, tr("STR_ITEMS_UC"), 0, 0, ".", ssValue.str()};
		idItem = addToDetailsVector(row, false);
	}

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0) continue;
		if (facility->getRules()->getStorage() <= 0) continue;

		int itemValue = facility->getRules()->getStorage();
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		// Update header row (prevent tracking another variable)
		spaceProvided += itemValue;
		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex, hasItems);
		_details[parentIndex].amount = facilities;
	}

	std::ostringstream ssVal;
	ssVal << std::fixed << std::setprecision(3) << spaceProvided;
	_details[parentIndex].valueOverride = ssVal.str();
}

/**
 * Add storage users to _details vector.
 *
 * The list includes:
 *  + Base facilities taking up storage space.
 *  + Grand total of items taking up storage space.
 *
 * @note
 * List of space usage per item is deliberately not shown.
 * We have storeState for that functionality.
 */
void BaseInfoDetailsState::addSubCategoryStoragesUsage()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	double spaceUsage = 0.0;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_BIDS_SUBTOTAL_STORAGE_USERS");
	row = {parentId, parentId, true, description, 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	// Start with grand total for items, to keep it at the top.
	bool hasItems = false;
	for (auto& item : _game->getMod()->getItemsList())
	{
		auto rule = _game->getMod()->getItem(item, true);
		double size = rule->getSize();
		if (size <= 0) continue;

		int qty = _base->getStorageItems()->getItem(item)
			+ _base->getItemClaimByCrafts(rule)   // No transfers yet.
			+ _base->getItemCountTransfers(rule); // Includes items from craft transfers.

		spaceUsage += size * qty;
		hasItems |= qty > 0;
	}
	if (hasItems)
	{
		std::ostringstream ssValue;
		ssValue << std::fixed << std::setprecision(3) << spaceUsage;
		row = {idItem, parentId, false, tr("STR_ITEMS_UC"), 0, 0, ".", ssValue.str()};
		idItem = addToDetailsVector(row, false);
	}

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0) continue;
		if (facility->getRules()->getStorage() >= 0) continue;

		int itemValue = std::abs(facility->getRules()->getStorage());
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		spaceUsage += itemValue;
		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex, hasItems);
		_details[parentIndex].amount = facilities;
	}

	std::ostringstream ssVal;
	ssVal << std::fixed << std::setprecision(3) << spaceUsage;
	_details[parentIndex].valueOverride = ssVal.str();
}

/**
 * Add facilities related to purchase services to _details vector.
 *
 * A list of facilities providing required services for purchase
 * appended with a list of known missing services.
 */
void BaseInfoDetailsState::addSubCategoryPurchaseServices()
{
	// Required services for purchase of items (**no** crafts, those are under hangars)
	// Based on: PurchaseState()
	RuleBaseFacilityFunctions requiredServices;
	for (auto& item : _game->getMod()->getItemsList())
	{
		auto rule = _game->getMod()->getItem(item);
		if (rule->getBuyCost() == 0) continue;
		if (!_game->getSavedGame()->isResearched(rule->getBuyRequirements()))
			continue;
		if (!_game->getSavedGame()->isResearched(rule->getRequirements()))
			continue;

		requiredServices |= rule->getRequiresBuyBaseFunc();
	}
	// Dependencies on unlocked (potential) base services.
	// We want to include missing services (but only if player can solve that problem).
	requiredServices &= _unlockedServicesBaseType;
	if (requiredServices.none()) return;

	addServices(requiredServices, tr("STR_BIDS_SUBTOTAL_PURCHASE_SERVICES"));
}

/**
 * Add a list of items that have a purchase limit.
 *
 * And show how many are still available.
 */
void BaseInfoDetailsState::addSubCategoryPurchaseLimits()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header
	row = {parentId, parentId, false, tr("STR_BIDS_SUBTOTAL_PURCHASE_LIMITS"), 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	auto& purchaseLimitLog = _game->getSavedGame()->getMonthlyPurchaseLimitLog();

	// List of (known) items with limits
	int itemsWithLimits = 0;
	for (auto& item : _game->getMod()->getItemsList())
	{
		auto rule = _game->getMod()->getItem(item);
		if (rule->getBuyCost() == 0) continue;

		int limit = rule->getMonthlyBuyLimit();
		if (limit <= 0) continue; // Negative limit has no usage in codebase.
		if (!_game->getSavedGame()->isResearched(rule->getBuyRequirements()))
			continue;
		if (!_game->getSavedGame()->isResearched(rule->getRequirements()))
			continue;

		std::string valueOverride = tr("STR_BIDS_ASSIGNED_VS_TOTAL").arg(purchaseLimitLog[rule->getType()]).arg(limit);
		row = {idItem, parentId, false, tr(rule->getType()), 0, 0, ".", valueOverride};
		idItem = addToDetailsVector(row, false);
		itemsWithLimits++;
	}
	if (itemsWithLimits > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].isVisible = true;
		_details[parentIndex].value = itemsWithLimits;
	}
}

/**
 * Add a lists of items depending on good country relations.
 *
 * Only show if item can be bought. No naming of specific countries
 * (item details screen is better suited for that kind of info).
 */
void BaseInfoDetailsState::addSubCategoryPurchaseCountries()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header
	row = {parentId, parentId, false, tr("STR_BIDS_SUBTOTAL_PURCHASE_COUNTRY"), 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	// List of (known) items which depend on country relations.
	int itemsDependingOnCountry = 0;
	for (auto& item : _game->getMod()->getItemsList())
	{
		auto rule = _game->getMod()->getItem(item);
		if (rule->getBuyCost() == 0) continue;
		if (rule->getRequiresBuyCountry().empty()) continue;
		if (!_game->getSavedGame()->isResearched(rule->getBuyRequirements()))
			continue;
		if (!_game->getSavedGame()->isResearched(rule->getRequirements()))
			continue;

		auto* countries = _game->getSavedGame()->getCountries();
		for (auto* country : *countries)
		{
			if (country->getRules()->getType() != rule->getRequiresBuyCountry())
				continue;

			std::string valueOverride = country->getPact() ? tr("STR_NO") : tr("STR_YES");
			row = {idItem, parentId, false, tr(rule->getType()), 0, 0, ".", valueOverride};
			idItem = addToDetailsVector(row, false);
			itemsDependingOnCountry++;
			break; // Item can only depend on one country.
		}
	}
	if (itemsDependingOnCountry > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].isVisible = true;
		_details[parentIndex].value = itemsDependingOnCountry;
	}
}

/**
 * Setup laboratory functionality screen.
 *
 * Recognize multiple subcategories:
 *  - Facilities contributing to laboratory space.
 *  - Overview of assigned laboratory space
 *  - Services needed for (known) research projects.
 */
void BaseInfoDetailsState::categoryLabs()
{
	_txtTitle->setText(tr("STR_LABORATORIES"));
	_txtQuantity->setText(tr("STR_BIDS_FACILITIES"));
	_txtTotal->setText(tr("STR_SPACE_AVAILABLE").arg(_base->getFreeLaboratories()));

	addSubCategoryLabProviders();
	addSubCategoryLabUsage();
	addSubCategoryLabServices();

	drawList();
}

/**
 * Add lab space providers to _details vector.
 *
 * A list of base facilities providing lab space.
 */
void BaseInfoDetailsState::addSubCategoryLabProviders()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_LABORATORIES"); //standard/xcom#/Language
	row = {parentId, parentId, true, description, 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	int providedSpace = 0;
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0) continue;
		if (facility->getRules()->getLaboratories() <= 0) continue;

		int itemValue = facility->getRules()->getLaboratories();
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
		providedSpace += itemValue;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
		_details[parentIndex].value = providedSpace;
	}
}

/**
 * Add lab space users to _details vector.
 *
 * The list includes:
 *  + Grand total of assigned scientists.
 *  + Grand total of queued projects base space (pure hypothetical).
 *  + Base facilities taking up lab space.
 *
 * @note
 * Lab usage per research project is deliberately not shown.
 * We have researchState for that functionality.
 */
void BaseInfoDetailsState::addSubCategoryLabUsage()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_BIDS_SUBTOTAL_LABS_USERS");
	row = {parentId, parentId, true, description, 0, _base->getUsedLaboratories(), ".", ""};
	idItem = addToDetailsVector(row, false);

	// Start with grand totals.
	int scientists = 0;
	int projectSpace = 0;
	for (auto research : _base->getResearch())
	{
		scientists += research->getAssigned();
		// Example in case future research gets support for (manufacture like) space parameter.
		//projectSpace += research->getRules()->getRequiredSpace();
	}
	if (scientists > 0)
	{
		row = {idItem, parentId, false, tr("STR_BIDS_DETAIL_LABS_SCIENTISTS") , 0, scientists, ".", ""};
		idItem = addToDetailsVector(row, true);
	}
	if (projectSpace > 0)
	{
		row = {idItem, parentId, false, tr("STR_BIDS_DETAIL_LABS_PROJECT_SPACE") , 0, projectSpace, ".", ""};
		idItem = addToDetailsVector(row, true);
	}

	// List of facilities taking away lab space.
	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0) continue;
		if (facility->getRules()->getLaboratories() >= 0) continue;

		int itemValue = std::abs(facility->getRules()->getLaboratories());
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex, (scientists > 0) + (projectSpace > 0));
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
	}
}

/**
 * Add facilities related to research services to _details vector.
 *
 * A list of facilities providing required services for research
 * appended with a list of known missing services.
 */
void BaseInfoDetailsState::addSubCategoryLabServices()
{
	// Required services for research.
	RuleBaseFacilityFunctions requiredServices;

	std::vector<RuleResearch *> availableResearch;
	// Available research independent from base (yup ...part of that method is no longer save converter only).
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
	requiredServices &= _unlockedServicesBaseType;
	if (requiredServices.none()) return;

	addServices(requiredServices, tr("STR_BIDS_SUBTOTAL_LABS_SERVICES"));
}

/**
 * Setup workshop functionality screen.
 *
 * Recognize multiple subcategories:
 *  - Facilities contributing to workshop space.
 *  - Overview of assigned workshop space
 *  - Services needed for (known) workshop projects.
 */
void BaseInfoDetailsState::categoryWorkshops()
{
	_txtTitle->setText(tr("STR_WORKSHOP"));
	_txtQuantity->setText(tr("STR_BIDS_FACILITIES"));
	_txtTotal->setText(tr("STR_SPACE_AVAILABLE").arg(_base->getFreeWorkshops()));

	addSubCategoryWorkshopProviders();
	addSubCategoryWorkshopUsage();
	addSubCategoryWorkshopServices();

	drawList();
}

/**
 * Add lab space providers to _details vector.
 *
 * A list of base facilities providing workshop space.
 */
void BaseInfoDetailsState::addSubCategoryWorkshopProviders()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_WORKSHOP"); //standard/xcom#/Language
	row = {parentId, parentId, true, description, 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	int providedSpace = 0;
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0) continue;
		if (facility->getRules()->getWorkshops() <= 0) continue;

		int itemValue = facility->getRules()->getWorkshops();
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
		providedSpace += itemValue;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
		_details[parentIndex].value = providedSpace;
	}
}

/**
 * Add lab space users to _details vector.
 *
 * The list includes:
 *  + Grand total of assigned engineers.
 *  + Grand total of queued projects base space.
 *  + Base facilities taking up workshop space.
 *
 * @note
 * Workshop usage per engineering project is deliberately not shown.
 * We have ManufactureState for that functionality.
 */
void BaseInfoDetailsState::addSubCategoryWorkshopUsage()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_BIDS_SUBTOTAL_WORKSHOP_USERS");
	row = {parentId, parentId, true, description, 0, _base->getUsedWorkshops(), ".", ""};
	idItem = addToDetailsVector(row, false);

	// Start with grand totals.
	int engineers = 0;
	int projectSpace = 0;
	for (auto production : _base->getProductions())
	{
		engineers += production->getAssignedEngineers();
		projectSpace += production->getRules()->getRequiredSpace();
	}
	if (engineers > 0)
	{
		row = {idItem, parentId, false, tr("STR_BIDS_DETAIL_WORKSHOP_ENGINEERS") , 0, engineers, ".", ""};
		idItem = addToDetailsVector(row, true);
	}
	if (projectSpace > 0)
	{
		row = {idItem, parentId, false, tr("STR_BIDS_DETAIL_WORKSHOP_PROJECT_SPACE") , 0, projectSpace, ".", ""};
		idItem = addToDetailsVector(row, true);
	}

	// List of facilities taking away lab space.
	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		// Skip buildings under construction.
		if (facility->getBuildTime() > 0) continue;
		if (facility->getRules()->getWorkshops() >= 0) continue;

		int itemValue = std::abs(facility->getRules()->getWorkshops());
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex, (engineers > 0) + (projectSpace > 0));
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
	}
}

/**
 * Add facilities related to workshop services to _details vector.
 *
 * List of facilities providing required services for workshop projects
 * appended with a list of known missing services.
 */
void BaseInfoDetailsState::addSubCategoryWorkshopServices()
{
	// Required services for research.
	RuleBaseFacilityFunctions requiredServices;

	std::vector<RuleResearch *> availableResearch;
	for (auto manufactureProject : _game->getMod()->getManufactureList())
	{
		RuleManufacture *ruleManufacture = _game->getMod()->getManufacture(manufactureProject);
		if (!_game->getSavedGame()->isResearched(ruleManufacture->getRequirements()))
		{
			continue;
		}
		requiredServices |= ruleManufacture->getRequireBaseFunc();
	}
	requiredServices &= _unlockedServicesBaseType;
	if (requiredServices.none()) return;

	addServices(requiredServices, tr("STR_BIDS_SUBTOTAL_WORKSHOP_SERVICES"));
}

/**
 * Setup alien containment space providers screen.
 *
 * Recognizes multiple subtotals may exist due to different 'prison' types.
 */
void BaseInfoDetailsState::categoryAlienContainment()
{
	_txtTitle->setText(tr("STR_ALIEN_CONTAINMENT"));
	_txtQuantity->setText(tr("STR_BIDS_FACILITIES"));
	//_txtTotal->setText(tr("STR_SPACE_AVAILABLE").arg(_base->getAvailableContainment()))

	// Recognize multiple containment types might exist.
	// But only if player has knowledge about them.
	std::set<int> containmentTypes {0}; // Default alien containment.
	for (auto& facility : _game->getMod()->getBaseFacilitiesList())
	{
		RuleBaseFacility *rule = _game->getMod()->getBaseFacility(facility);
		if (!(rule->getPrisonType() > 0)) continue;
		if (!_game->getSavedGame()->isResearched(rule->getRequirements()))
			continue;
		// No need to check if base allows facility.

		containmentTypes.insert(rule->getPrisonType());
	}

	// Facilities per Prison type
	for (auto prisonType : containmentTypes)
	{
		addSubCategoryContainmentType(prisonType);
	}

	drawList();
}

/**
 * Add containment type to _details vector.
 *
 * The list includes:
 *  + Defined type usage (e.g. amount of captives).
 *  + Facilities providing containment space for specified type
 *
 * @param type Integer denoting the containment type.
 */
void BaseInfoDetailsState::addSubCategoryContainmentType(int type)
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_BIDS_SUBTOTAL_CONTAINMENT_TYPE").arg(trAlt("STR_ALIEN", type));
	row = {parentId, parentId, true, description, 0, 0, ".", {}};
	idItem = addToDetailsVector(row, false);

	// We want usage to be the first details row.
	int inUse = _base->getUsedContainment(type);
	row = {idItem, parentId, false, tr("STR_BIDS_DETAIL_CONTAINMENT_USAGE"), 0, inUse, ".", {}}; // or use "trAlt() so modder can show a specified description."
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	int allowedAliens = 0;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0) continue;
		if (facility->getRules()->getPrisonType() != type) continue;
		if (facility->getRules()->getAliens() == 0) continue; // Allow display of negative space.

		int cells = facility->getRules()->getAliens();
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, cells, {}};
		idItem = addToDetailsVector(row, false);

		allowedAliens += cells;
		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex, 1);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
		_details[parentIndex].valueOverride = tr("STR_BIDS_ASSIGNED_VS_TOTAL").arg(inUse).arg(allowedAliens);
	}

	parentId++;
}

/**
 * Setup hangar providers (and usage) screen.
 *
 * Recognize 2 subtotals:
 * + Providers of hangar space
 * + Users of hangar space.
 * + Ammo needed for armament.
*/
void BaseInfoDetailsState::categoryHangars()
{
	_txtTitle->setText(tr("STR_HANGARS")); // common/language/Technical
	_txtQuantity->setText(tr("STR_BIDS_FACILITIES"));
	//_txtTotal->setText(tr("STR_SPACE_AVAILABLE").arg(_base->getAvailableHangars() - _base->getUsedHangars()));

	// Recognise crafts might carry armament requiring ammo.
	std::set<const RuleItem*> ammoItems;
	for (auto craft : *_base->getCrafts())
	{
		for (auto weapon : *craft->getWeapons())
		{
			if (weapon == nullptr) continue;
			if (weapon->getRules() == nullptr) continue;
			if (weapon->getRules()->getClipItem() == nullptr) continue;

			ammoItems.insert(weapon->getRules()->getClipItem());
		}
	}

	addSubCategoryHangarProviders();
	addSubCategoryHangarUsage();
	for (auto ammoItem : ammoItems)
	{
		addSubCategoryCraftArmamentAmmo(ammoItem);
	}

	drawList();
}

/**
 * Add hangar space providers to _details vector.
 *
 * A list of base facilities providing hangar space.
 */
void BaseInfoDetailsState::addSubCategoryHangarProviders()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	row = {parentId, parentId, true, tr("STR_BIDS_SUBTOTAL_HANGARS_PROVIDERS"), 0, _base->getAvailableHangars(), {}};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		int itemValue = facility->getRules()->getCrafts();
		if (facility->getBuildTime() > 0 || itemValue <= 0) continue;

		row = {idItem, parentId, false, tr(facility->getRules()->getType()) , 1, itemValue, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
	}
}

/**
 * Add overview of claimed hangar space to _details vector.
 *
 * A list including the following elements:
 * + Crafts demanding hangar space.
 * + Facilities providing negative hangar space.
 */
void BaseInfoDetailsState::addSubCategoryHangarUsage()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	int itemValue = 0;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_BIDS_SUBTOTAL_HANGARS_USERS");
	row = {parentId, parentId, true, description, 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	// Start with crafts.
	// Does not add that much info that isn't visible elsewhere already.
	// Included anyway since category view would be kinda empty otherwise.
	// This is the place to adapt if crafts start to require specific hangars
	// or more than 1 space.
	int spaceUsage = 0;
	for (auto craft : *_base->getCrafts())
	{
		itemValue = 1; // in case it becomes a mod variable.
		row = {idItem, parentId, false, craft->getName(_game->getLanguage()), 0, itemValue, ".", {}};
		idItem = addToDetailsVector(row, false);
		spaceUsage++;
	}
	for (auto transfer : *_base->getTransfers())
	{
		if (transfer->getType() == TRANSFER_CRAFT)
		{
			itemValue = 1; // in case it becomes a mod variable.
			description = tr("STR_BIDS_DETAIL_HANGARS_CRAFT_TRANSFER").arg(transfer->getName(_game->getLanguage()));
			row = {idItem, parentId, false, description, 0, itemValue, ".", {}};
			idItem = addToDetailsVector(row, false);
			spaceUsage += itemValue;
		}
	}

	// Facilities providing negative space.
	// For display consistency, not sure what those facilities would break in other parts of the code.
	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		itemValue = facility->getRules()->getCrafts();
		if (facility->getBuildTime() > 0 || itemValue >= 0) continue;

		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, std::abs(itemValue), {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
		spaceUsage += itemValue;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
	}
	if (spaceUsage > 0)
	{
		_details[parentIndex].value = spaceUsage;
	}

}

/**
 * Add craft ammo usage to  _details vector.
 *
 * The amount required for armament of all crafts.
 * Irrespectively if launcher is active or on hold.
 *
 * @param ammo Pointer to ruleset of required ammo.
 */
void BaseInfoDetailsState::addSubCategoryCraftArmamentAmmo(const RuleItem* ammo)
{
	if (!ammo) return;

	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_BIDS_SUBTOTAL_AMMO_REQUIRED").arg(tr(ammo->getName()));
	row = {parentId, parentId, true, description, 0, 0, ".", {}};
	idItem = addToDetailsVector(row, false);

	// Let first row show amount available on base.
	int ammoAvailable = _base->getStorageItems()->getItem(ammo);
	row = {idItem, parentId, false, tr("STR_AMMUNITION_AVAILABLE"), 0, 0, ".", {}};  // From: standard/xcom#/Language
	idItem = addToDetailsVector(row, false);

	int amountNeeded = 0;
	for (auto craft : *_base->getCrafts())
	{
		if (craft->getWeapons()->empty()) continue;
		int craftUsage = 0;
		for (auto weapon : *craft->getWeapons())
		{
			if (weapon == nullptr) continue;
			if (weapon->getRules() == nullptr) continue;
			if (weapon->getRules()->getClipItem() != ammo) continue;

			auto clipSize = ammo->getClipSize();
			if (clipSize == 0) continue;
			craftUsage += weapon->getRules()->getAmmoMax() / clipSize;
			// Correction for missing ammo.
			ammoAvailable += weapon->getClipsLoaded() - weapon->getRules()->getAmmoMax() / clipSize;
		}
		if (craftUsage > 0)
		{
			row = {idItem, parentId, false, craft->getName(_game->getLanguage()), 0, craftUsage, ".", {}};
			idItem = addToDetailsVector(row, false);
			amountNeeded += craftUsage;
		}
	}

	sortChildren(parentIndex, 1);
	_details[parentIndex].value = amountNeeded;
	_details[parentIndex].valueOverride = tr("STR_BIDS_ASSIGNED_VS_TOTAL").arg(ammoAvailable + amountNeeded).arg(amountNeeded);
	// 1st child is always the ammo available one.
	_details[parentIndex + 1].value = ammoAvailable;
}

/**
 * Setup base defense abilities screen.
 *
 * Facilities contributing to the following subcategories:
 *  - Defense strength
 *  - Defense hitchances.
 *  - Gravitational shields (in oxc they stack).
 *  - Ammunition usage.
 *
 * Will not show the following:
 * + Missile attraction of a facility.
 *   - Seems like it should be a hidden stat.
 * + Hitchance per combined power output (or a selected subset).
 *   - Has to account for 36 defenses and shields on a base.
 *     Defenses alone already mean: 2^36 ~= 6.8*10^10 permutations.
 *   - Way too computational expensive (for too little gain).
 *     Unless one comes up with a clever (mathematical) solution.
*/
void BaseInfoDetailsState::categoryDefense()
{
	_txtTitle->setText(tr("STR_DEFENSE_STRENGTH")); // common/language/Technical
	_txtQuantity->setText(tr("STR_BIDS_FACILITIES"));
	_txtTotal->setText("");

	// Recognise defense facilities might require ammo.
	// But only if player has knowledge about them.
	std::set<const RuleItem*> ammoItems; // Default alien containment.
	for (auto& facility : _game->getMod()->getBaseFacilitiesList())
	{
		RuleBaseFacility *rule = _game->getMod()->getBaseFacility(facility);
		if (!rule->getAmmoItem()) continue;
		if (!_game->getSavedGame()->isResearched(rule->getRequirements()))
			continue;
		// No need to check if base allows facility.

		ammoItems.insert(rule->getAmmoItem());
	}

	addSubCategoryDefenseStrength();
	addSubCategoryDefenseChance();
	addSubCategoryGravShield();
	for (auto ammoItem : ammoItems)
	{
		addSubCategoryDefenseAmmo(ammoItem);
	}

	drawList();
}

/**
 * Add defense strength overview to _details vector.
 *
 * A list of base facilities contributing to defense strength.
 */
void BaseInfoDetailsState::addSubCategoryDefenseStrength()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	row = {parentId, parentId, true, tr("STR_DEFENSE_STRENGTH"), 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	int totalStrength = 0;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0) continue;

		int strength = facility->getRules()->getDefenseValue();
		// Allow negative defense strength (facility adding HP to ufo).
		if (strength == 0) continue;

		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, strength, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
		totalStrength += strength;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
		_details[parentIndex].value = totalStrength;
	}
}

/**
 * Add defense hit ratio overview to _details vector.
 *
 * A list of base facilities contributing to defense hit ratio.
 */
void BaseInfoDetailsState::addSubCategoryDefenseChance()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	row = {parentId, parentId, true, tr("STR_HIT_RATIO"), 0, 0, ".", ""};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	double detectionFail = 1.0;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0) continue;

		int hitRatio = facility->getRules()->getHitRatio();
		// Allow display negative hit ratio.
		if (hitRatio == 0) continue;

		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, hitRatio, {}};
		idItem = addToDetailsVector(row, false);

		facilities++;
		if (hitRatio <= 0) continue; // Negative percentage are effectively 0.

		detectionFail *= (100 - hitRatio)/100.0;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
		_details[parentIndex].value = (int)std::round(100 * (1.0 - detectionFail));
		_details[parentIndex].valueOverride = toStringPercent(_details[parentIndex].value);
	}
}

/**
 * Add grav shields to _details vector.
 *
 * A list of base facilities contributing to extra defense rounds.
 */
void BaseInfoDetailsState::addSubCategoryGravShield()
{
	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header
	row = {parentId, parentId, false, tr("STR_BIDS_SUBTOTAL_GRAV_SHIELD"), 0, 0, {}};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0) continue;
		if (!facility->getRules()->isGravShield()) continue;

		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, 1, "", tr("STR_TRUE")};
		idItem = addToDetailsVector(row, false);

		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].value = facilities;
		_details[parentIndex].isVisible = true;
	}
}

/**
 * Add specific ammo usage to  _details vector.
 *
 * The amount required for a single defense cycle. Using the listed
 * gravitational shields a player can deduce an upper limit themselves.
 *
 * @param ammo Pointer to ruleset of required ammo.
 */
void BaseInfoDetailsState::addSubCategoryDefenseAmmo(const RuleItem* ammo)
{
	if (!ammo) return;

	size_t parentIndex = _details.size();
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (show unconditionally)
	std::string description = tr("STR_BIDS_SUBTOTAL_AMMO_REQUIRED").arg(tr(ammo->getName()));
	row = {parentId, parentId, true, description, 0, 0, ".", {}};
	idItem = addToDetailsVector(row, false);

	// Let first row show amount available on base.
	int ammoAvailable = _base->getStorageItems()->getItem(ammo);
	row = {idItem, parentId, false, tr("STR_AMMUNITION_AVAILABLE"), 0, ammoAvailable, ".", {}};  // From: standard/xcom#/Language
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	int roundsPerCycle = 0;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0) continue;
		if (facility->getRules()->getAmmoItem() != ammo) continue;

		int rounds = facility->getRules()->getAmmoNeeded();
		row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, rounds, {}};
		idItem = addToDetailsVector(row, false);

		roundsPerCycle += rounds;
		facilities++;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex, 1);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].amountOverride = "";
		_details[parentIndex].value = roundsPerCycle;
		_details[parentIndex].valueOverride = tr("STR_BIDS_ASSIGNED_VS_TOTAL").arg(ammoAvailable).arg(roundsPerCycle);
	}
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
*
* @param value Value to use in string representation.
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
*
* @param value Value to use in string representation.
*/
std::string BaseInfoDetailsState::toStringMana(int value)
{
	return std::to_string(value) + " " + tr("STR_MANA_ABBREVIATION").c_str();
}

/**
* Convert value to string representation (appended with "%").
*
* @param value Value to use in string representation.
*/
std::string BaseInfoDetailsState::toStringPercent(float value)
{
	std::ostringstream ssValue;
	//ssValue << std::fixed << std::setprecision(2);
	ssValue << value << "%";
	return ssValue.str();
}

/**
* Alphabetical sort of children
*
* Sorts the tail of the _details vector.
*
* @param parentIndex Index of subtotal row.
* @param skipChildren Amount of child rows to be left untouched.
*/
void BaseInfoDetailsState::sortChildren(size_t parentIndex, int skipChildren)
{
	size_t offset = 1 + skipChildren; // 1 >>> The header row
	std::sort(std::next(_details.begin(), parentIndex + offset), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{ return Unicode::naturalCompare(a.description, b.description); }
	);
}

/**
* Add known services to _details vector.
*
* @param services List of services to add.
* @param subTotalDescription Description for the parent (subtotal) row.
*/
void BaseInfoDetailsState::addServices(RuleBaseFacilityFunctions services, std::string subTotalDescription)
{
	size_t parentIndex = _details.size();
	// This list has lost it's purpose long before we reach INT_MAX
	int parentId = (int)parentIndex;
	int idItem = parentId;
	BeanCounter row;

	// Subcategory header (only show if we have facilities or missing services).
	row = {parentId, parentId, false, subTotalDescription, {}};
	idItem = addToDetailsVector(row, false);

	int facilities = 0;
	auto missingServices = services;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0) continue;

		auto facilityServices = facility->getRules()->getProvidedBaseFunc();
		facilityServices &= services; // Only Hiring services
		if (facilityServices.count() == 0) continue;

		auto servicesNames = _game->getMod()->getBaseFunctionNames(facilityServices);
		for (auto service : servicesNames)
		{
			row = {idItem, parentId, false, tr(facility->getRules()->getType()), 1, 0, "", service};
			idItem = addToDetailsVector(row, false);
		}
		facilities++;
		missingServices ^= facilityServices;
	}
	if (facilities > 0)
	{
		sortChildren(parentIndex);
		_details[parentIndex].amount = facilities;
		_details[parentIndex].isVisible = true;
	}
	int totalServices = services.count();
	int activeServices = totalServices;
	if (missingServices.count() > 0)
	{
		auto servicesMissing = _game->getMod()->getBaseFunctionNames(missingServices);
		for (auto service : servicesMissing)
		{
			row = {idItem, parentId, false, tr("STR_SERVICES_MISSING"), 0, 0, ".", service};
			idItem = addToDetailsVector(row, false);
		}

		activeServices -= missingServices.count();
		_details[parentIndex].isVisible = true;
	}
	_details[parentIndex].valueOverride = tr("STR_BIDS_ASSIGNED_VS_TOTAL").arg(activeServices).arg(totalServices);
}

}
