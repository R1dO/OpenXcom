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
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Mod/RuleSoldier.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Country.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"

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
	switch (_currentCategory)
	{
	case CC_GLOBAL_RESULT:
		_currentCategory = CC_CRAFTS;
		break;
	case CC_SOLDIERS:
	case CC_SCIENTISTS:
	case CC_ENGINEERS:
		_currentCategory = CC_ITEMS;
		break;
	default:
		_currentCategory = (CostCategory)(_currentCategory + 1);
		break;
	}

	drawBody();
}

/**
 * Goes to the previous 'cost' category.
 * @param action Pointer to an action.
 */
void MonthlyCostsDetailsState::btnPrevClick(Action *)
{
	switch (_currentCategory)
	{
	case CC_CRAFTS:
		_currentCategory = CC_GLOBAL_RESULT;
		break;
	case CC_ITEMS:
	case CC_ENGINEERS:
	case CC_SCIENTISTS:
		_currentCategory = CC_SOLDIERS;
		break;
	default:
		_currentCategory = (CostCategory)(_currentCategory - 1);
		break;
	}

	drawBody();
}

/**
 * Handles mouse-clicks on the list rows.
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
 * Adds another contribution to the '_details' vector.
 *
 * Creates a new entry if needed, updates if an entry already exist.
 * An entry is defined by the unique combination of 'parentID' and 'description'.
 *
 * @param row The contents of the row we want to insert.
 * @return Unique identifier for the next element in the list (!not the vector's rowid!).
 */
int MonthlyCostsDetailsState::addToDetailsVector(BeanCounter row)
{
	for (auto &bean : _details)
	{
		// There are 2 ways to assign a cost to elements (positive & negative).
		// Therefore it is possible an element belongs to 2 parents.
		if (bean.parentId == row.parentId && bean.description == row.description)
		{
			bean.totalValue += row.totalValue;
			// No checking if bean.amount > -1.
			// It is callers responsibility to supply correct values.
			// By not checking any implementation faults become a bit more
			// visible (weird amount numbers on screen)
			bean.amount += row.amount;

			return row.id;
		}
	}
	_details.push_back(row);
	return ++row.id;
}

/**
 * Check if the details list need a specific subtotal
 *
 * @param parentId Id of subtotal to check.
 */
bool MonthlyCostsDetailsState::isSubtotalNeeded(int parentId)
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
int MonthlyCostsDetailsState::calculateSubtotalAmount(int parentId)
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
int64_t MonthlyCostsDetailsState::calculateSubtotalValue(int parentId)
{
	int64_t total = 0;
	for (auto element : _details)
	{
		if (element.parentId == parentId && element.id != element.parentId)
		{
			total += element.totalValue;
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
void MonthlyCostsDetailsState::drawBody()
{
	_details.clear();
	_lstTotal->clearList();
	std::ostringstream ssTitle;

	switch (_currentCategory)
	{
	case CC_CRAFTS:
		ssTitle << tr("MCDS_TITLE_CRAFT_MAINTENANCE");
		categoryCraftMaintenance();
		break;
	case CC_SOLDIERS:
		ssTitle << tr("STR_SOLDIERS");
		categorySoldierSalaries();
		break;
	case CC_ITEMS:
		ssTitle << tr("STR_OTHER_EMPLOYEES");
		categoryItemMaintenance();
		break;
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
		int parent, id = 0;
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
 * Setup screen that displays craft maintenance.
 *
 * Only takes into account crafts that are assigned to the base.
 *
 * Recognize 3 subtotals may exist:
 * (0) Fighter crafts
 * (1) Transporter crafts
 * (2) Mixed crafts (fighter/transporter)
 *
 * @note
 * No research check is performed. If a craft is on the base we have to pay
 * for maintenance anyway, meaning a player can deduce already.
 * @note
 * Crafts without maintenance costs are allowed on this screen.
 *
 * Logic based on: 'MonthlyCostState.cpp'
 */
void MonthlyCostsDetailsState::categoryCraftMaintenance()
{
	int idItem = 3;       // Offset based on expected subtotal entries.
	int idParent;         // Can be as much as there are soldier types -1 (0-based counting).
	int craftMaintenance; // Always positive, unless a subtotal.

	auto getSubTotalParentId = [&](const RuleCraft* craft) -> int
	{
		if (craft->getWeapons() == 0)
			return 1; // Transporter
		if (craft->getPilots() == 0  && craft->getMaxUnits() == 0)
			return 0; // Vanilla style Fighter
		if (craft->getPilots() == 0 && craft->getMaxUnits() != 0)
			return 2; // Vanilla style Mixed

		int flightCrewExtra = 0; // Adjust this for more relaxing fighter check.
		if (craft->getMaxUnits() > (craft->getPilots() + flightCrewExtra))
			return 2; // OXCE style Mixed

		// If none of above, assume fighter.
		return 0;
	};

	BeanCounter row;
	for (auto craft : *_base->getCrafts())
	{
		idParent = getSubTotalParentId(craft->getRules());
		craftMaintenance = craft->getRules()->getRentCost();
		row = {idItem, idParent, false, craft->getName(_game->getLanguage()), 1, craftMaintenance};

		idItem = addToDetailsVector(row);
	}

	int subTotal, subAmount, screenTotal = 0;
	if (isSubtotalNeeded(0)) // Fighter
	{
		subTotal = -1 * calculateSubtotalValue(0);
		subAmount = calculateSubtotalAmount(0);
		screenTotal += subTotal;
		row = {0, 0, true, tr("MCDS_SUBTOTAL_FIGHTER"), subAmount, subTotal};
		_details.insert(_details.begin(), row);
	}
	if (isSubtotalNeeded(1)) // Transporter
	{
		subTotal = -1 * calculateSubtotalValue(1);
		subAmount = calculateSubtotalAmount(1);
		screenTotal += subTotal;
		row = {1, 1, true, tr("MCDS_SUBTOTAL_TANSPORTER"), subAmount, subTotal};
		_details.insert(_details.begin(), row);
	}
	if (isSubtotalNeeded(2)) // Mixed
	{
		subTotal = -1 * calculateSubtotalValue(2);
		subAmount = calculateSubtotalAmount(2);
		screenTotal += subTotal;
		row = {2, 2, true, tr("MCDS_SUBTOTAL_MIXED"), subAmount, subTotal};
		_details.insert(_details.begin(), row);
	}

	// Ensure elements are shown below appropriate subtotal.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return a.parentId < b.parentId;
		}
	);

	// Allow for double checking, use a different method (w.r.t. previous screen) for total.
	_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(screenTotal, true).c_str());

}


/**
 * Setup screen that displays soldier salaries.
 *
 * Each soldier type gets their own subtotal.
 * Fanatics are included as well (those fighting for the divine case need no salary).
 *
 * @note
 * No research check is performed. Once a soldier is on the base we have
 * to pay them anyway, meaning a player can deduce already.
 *
 * Logic based on: 'Base::getSoldierCountAndSalary()'.
 */
void MonthlyCostsDetailsState::categorySoldierSalaries()
{
	int idItem = _game->getMod()->getSoldiersList().size(); // Offset based on expected subtotal entries.
	int idParent;      // Can be as much as there are soldier types -1 (0-based counting).
	int soldierSalary; // Always positive, unless a subtotal.

	// Keep track of soldier types and their corresponding parentId.
	std::vector<std::pair<int, std::string> > subTotals;
	// Returns parentID for selected soldierType, creates one if does not exist yet.
	auto getSubTotalParentId = [&](std::string soldierType) -> int
	{
		auto it = std::find_if(subTotals.begin(), subTotals.end(),
			[&](const std::pair<int, std::string>& sType) { return sType.second == soldierType; });
		if (it == subTotals.end())
		{
			subTotals.push_back(std::make_pair(subTotals.size(), soldierType));
			return subTotals.size() - 1;
		}
		return (*it).first;
	};

	BeanCounter row;
	for (auto soldier : *_base->getSoldiers())
	{
		idParent = getSubTotalParentId(soldier->getRules()->getType());
		soldierSalary = soldier->getRules()->getSalaryCost(soldier->getRank());
		row = {idItem, idParent, false, soldier->getName(), 1, soldierSalary};

		idItem = addToDetailsVector(row);
	}
	for (auto transfer : *_base->getTransfers())
	{
		if (transfer->getType() != TRANSFER_SOLDIER) continue;

		idParent = getSubTotalParentId(transfer->getSoldier()->getRules()->getType());
		soldierSalary = transfer->getSoldier()->getRules()->getSalaryCost(transfer->getSoldier()->getRank());
		row = {idItem, idParent, false, transfer->getSoldier()->getName(), 1, soldierSalary};

		idItem = addToDetailsVector(row);
	}
	// Prefer alphabetical listing.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return Unicode::naturalCompare(a.description, b.description);
		}
	);
	// But I want them grouped by rank (inferred from salary)
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return a.totalValue > b.totalValue;
		}
	);

	// Insert Subtotals
	int subTotal, subAmount, screenTotal = 0;
	for (auto soldierType : subTotals)
	{
		subTotal = calculateSubtotalValue(soldierType.first);
		subAmount = calculateSubtotalAmount(soldierType.first);
		screenTotal += subTotal;
		row = {soldierType.first, soldierType.first, true, tr(soldierType.second), subAmount, -1 * subTotal};
		_details.insert(_details.begin(), row);
	}
	// Ensure elements are shown below appropriate subtotal.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return a.parentId < b.parentId;
		}
	);

	// Allow for double checking: use a different method (w.r.t. previous screen) for total.
	_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(-1 * screenTotal, true).c_str());
}

/**
 * Setup screen that displays items maintenance and item salary.
 *
 * Recognize 4 subtotals may exist:
 * (0) Item Salary:      getMonthlySalary() > 0
 * (1) Item Consulting:  getMonthlySalary() < 0
 * (2) Item Maintenance: getMonthlyMaintenance() > 0.
 * (3) Item Services     getMonthlyMaintenance() < 0.
 *
 * Logic based on: 'Base::getTotalOtherStaffAndInventoryCost()'.
 */
void MonthlyCostsDetailsState::categoryItemMaintenance()
{
	int idItem = 4; // Offset based on expected subtotal entries.
	int idParent;   // Let parentId represent the numbers as described in method description.
	int totalValue; // Always positive, unless a subtotal.
	BeanCounter row;

	for (auto transfer : *_base->getTransfers())
	{
		if (transfer->getType() == TRANSFER_ITEM)
		{
			auto ruleItem = _game->getMod()->getItem(transfer->getItems(), true);
			if (ruleItem->getMonthlySalary() != 0)
			{
				idParent = ruleItem->getMonthlySalary() > 0 ? 0 : 1;
				totalValue = transfer->getQuantity() * abs(ruleItem->getMonthlySalary());
				row = {idItem, idParent, false, tr(ruleItem->getName()), transfer->getQuantity(), totalValue};

				idItem = addToDetailsVector(row);
			}
			if (ruleItem->getMonthlyMaintenance() != 0)
			{
				idParent = ruleItem->getMonthlyMaintenance() > 0 ? 2 : 3;
				totalValue = transfer->getQuantity() * abs(ruleItem->getMonthlyMaintenance());
				row = {idItem, idParent, false, tr(ruleItem->getName()), transfer->getQuantity(), totalValue};

				idItem = addToDetailsVector(row);
			}
		}
		else if (transfer->getType() == TRANSFER_SOLDIER)
		{
			auto ruleItem = transfer->getSoldier()->getArmor()->getStoreItem();
			if (ruleItem && ruleItem->getMonthlySalary() != 0)
			{
				idParent = ruleItem->getMonthlySalary() > 0 ? 0 : 1;
				totalValue = transfer->getQuantity() * abs(ruleItem->getMonthlySalary());
				row = {idItem, idParent, false, tr(ruleItem->getName()), transfer->getQuantity(), totalValue};

				idItem = addToDetailsVector(row);
			}
			if (ruleItem && ruleItem->getMonthlyMaintenance() != 0)
			{
				idParent = ruleItem->getMonthlyMaintenance() > 0 ? 2 : 3;
				totalValue = transfer->getQuantity() * abs(ruleItem->getMonthlyMaintenance());
				row = {idItem, idParent, false, tr(ruleItem->getName()), transfer->getQuantity(), totalValue};

				idItem = addToDetailsVector(row);
			}
		}
	}
	for (const auto& storeItem : *_base->getStorageItems()->getContents())
	{
		auto ruleItem = _game->getMod()->getItem(storeItem.first, true);
		if (ruleItem->getMonthlySalary() != 0)
		{
			idParent = ruleItem->getMonthlySalary() > 0 ? 0 : 1;
			totalValue = storeItem.second * abs(ruleItem->getMonthlySalary());
			row = {idItem, idParent, false, tr(ruleItem->getName()), storeItem.second, totalValue};

			idItem = addToDetailsVector(row);
		}
		if (ruleItem->getMonthlyMaintenance() != 0)
		{
			idParent = ruleItem->getMonthlyMaintenance() > 0 ? 2 : 3;
			totalValue = storeItem.second * abs(ruleItem->getMonthlyMaintenance());
			row = {idItem, idParent, false, tr(ruleItem->getName()), storeItem.second, totalValue};

			idItem = addToDetailsVector(row);
		}
	}
	for (auto craft : *_base->getCrafts())
	{
		for (const auto &craftItem : *craft->getItems()->getContents())
		{
			auto ruleItem = _game->getMod()->getItem(craftItem.first, true);
			if (ruleItem->getMonthlySalary() != 0)
			{
				idParent = ruleItem->getMonthlySalary() > 0 ? 0 : 1;
				totalValue = craftItem.second * abs(ruleItem->getMonthlySalary());
				row = {idItem, idParent, false, tr(ruleItem->getName()), craftItem.second, totalValue};

				idItem = addToDetailsVector(row);
			}
			if (ruleItem->getMonthlyMaintenance() != 0)
			{
				idParent = ruleItem->getMonthlyMaintenance() > 0 ? 2 : 3;
				totalValue = craftItem.second * abs(ruleItem->getMonthlyMaintenance());
				row = {idItem, idParent, false, tr(ruleItem->getName()), craftItem.second, totalValue};

				idItem = addToDetailsVector(row);
			}
		}
		for (auto vehicle : *craft->getVehicles())
		{
			auto ruleItem = vehicle->getRules();
			if (ruleItem->getMonthlySalary() != 0)
			{
				idParent = ruleItem->getMonthlySalary() > 0 ? 0 : 1;
				totalValue = 1 * abs(ruleItem->getMonthlySalary());
				row = {idItem, idParent, false, tr(ruleItem->getName()), 1, totalValue};

				idItem = addToDetailsVector(row);
			}
			if (ruleItem->getMonthlyMaintenance() != 0)
			{
				idParent = ruleItem->getMonthlyMaintenance() > 0 ? 2 : 3;
				totalValue = 1 * abs(ruleItem->getMonthlyMaintenance());
				row = {idItem, idParent, false, tr(ruleItem->getName()), 1, totalValue};

				idItem = addToDetailsVector(row);
			}
		}
	}
	for (auto soldier : *_base->getSoldiers())
	{
		auto ruleItem = soldier->getArmor()->getStoreItem();
		if (ruleItem && ruleItem->getMonthlySalary() != 0)
		{
			idParent = ruleItem->getMonthlySalary() > 0 ? 0 : 1;
			totalValue = 1 * abs(ruleItem->getMonthlySalary());
			row = {idItem, idParent, false, tr(ruleItem->getName()), 1, totalValue};

			idItem = addToDetailsVector(row);
		}
		if (ruleItem && ruleItem->getMonthlyMaintenance() != 0)
		{
			idParent = ruleItem->getMonthlyMaintenance() > 0 ? 2 : 3;
			totalValue = 1 * abs(ruleItem->getMonthlyMaintenance());
			row = {idItem, idParent, false, tr(ruleItem->getName()), 1, totalValue};

			idItem = addToDetailsVector(row);
		}
	}
	// Prefer alphabetical listing of detailed rows.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return Unicode::naturalCompare(a.description, b.description);
		}
	);

	int subTotal, subAmount, screenTotal = 0;
	if (isSubtotalNeeded(0)) // Salary
	{
		subTotal = -1 * calculateSubtotalValue(0);
		subAmount = calculateSubtotalAmount(0);
		screenTotal += subTotal;
		row = {0, 0, true, tr("MCDS_SUBTOTAL_ITEM_SALARY"), subAmount, subTotal};
		_details.insert(_details.begin(), row);
	}
	if (isSubtotalNeeded(1)) // Consulting
	{
		subTotal = calculateSubtotalValue(1);
		subAmount = calculateSubtotalAmount(1);
		screenTotal += subTotal;
		row = {1, 1, true, tr("MCDS_SUBTOTAL_ITEM_SALARY_INCOME"), subAmount, subTotal};
		_details.insert(_details.begin(), row);
	}
	if (isSubtotalNeeded(2)) // Maintenance
	{
		subTotal = -1 * calculateSubtotalValue(2);
		subAmount = calculateSubtotalAmount(2);
		screenTotal += subTotal;
		row = {2, 2, true, tr("MCDS_SUBTOTAL_ITEM_MAINTENANCE"), subAmount, subTotal};
		_details.insert(_details.begin(), row);
	}
	if (isSubtotalNeeded(3)) // Services
	{
		subTotal = calculateSubtotalValue(3);
		subAmount = calculateSubtotalAmount(3);
		screenTotal += subTotal;
		row = {3, 3, true, tr("MCDS_SUBTOTAL_ITEM_MAINTENANCE_INCOME"), subAmount, subTotal};
		_details.insert(_details.begin(), row);
	}
	// Ensure elements are shown below appropriate subtotal.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return a.parentId < b.parentId;
		}
	);

	// Allow for double checking, use a different method (w.r.t. previous screen) for total.
	_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(screenTotal, true).c_str());
}

/**
 * Setup screen that displays facility maintenance
 *
 * Recognize 2 subtotals may exist:
 * (0) Facility Maintenance: getMonthlyCost() > 0
 * (1) Facility Revenue:     getMonthlyCost() < 0
 */
void MonthlyCostsDetailsState::categoryFacilityMaintenance()
{
	int idItem = 2; // Offset based on expected subtotal entries.
	int idParent;   // Let parentId represent the numbers as described in method description.
	int itemValue;  // Always positive, unless a subtotal.

	BeanCounter row;
	for (auto *facility : *_base->getFacilities())
	{
		// Buildings under construction won't cost (or generate) money.
		if (facility->getBuildTime() > 0) continue;

		idParent = facility->getRules()->getMonthlyCost() >= 0 ? 0 : 1;
		itemValue = abs(facility->getRules()->getMonthlyCost());
		row = {idItem, idParent, false, tr(facility->getRules()->getType()), 1, itemValue};

		idItem = addToDetailsVector(row);
	}
	// Prefer alphabetical listing of detailed rows.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return Unicode::naturalCompare(a.description, b.description);
		}
	);

	int subTotal, subAmount, screenTotal = 0;
	if (isSubtotalNeeded(0)) // Maintenance
	{
		subTotal = -1 * calculateSubtotalValue(0);
		subAmount = calculateSubtotalAmount(0);
		screenTotal += subTotal;
		row = {0, 0, true, tr("MCDS_SUBTOTAL_FACILITY_MAINTENANCE"), subAmount, subTotal};
		_details.insert(_details.begin(), row);
	}
	if (isSubtotalNeeded(1)) // Revenue
	{
		subTotal = calculateSubtotalValue(1);
		subAmount = calculateSubtotalAmount(1);
		screenTotal += subTotal;
		row = {1, 1, true, tr("MCDS_SUBTOTAL_FACILITY_REVENUE"), subAmount, subTotal};
		_details.insert(_details.begin(), row);
	}
	// Ensure elements are shown below appropriate subtotal.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return a.parentId < b.parentId;
		}
	);

	// Allow for double checking: use a different method (w.r.t. previous screen) for total.
	_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(screenTotal, true).c_str());
}


/**
 * Setup the global income overview.
 *
 * Recognize 4 subtotals may exist:
 * (0) Geoscape Income: Country funding and performance funding
 * (1) Geoscape PayBack: Negative Country funding
 * (2) Bases Maintenance
 * (3) Bases Revenue
 */
void MonthlyCostsDetailsState::categoryGlobalResult()
{
	int idItem = 4; // Offset based on expected subtotal entries.
	int idParent;   // Let parentId represent the numbers as described in method description.
	int itemValue;  // Always positive, unless a subtotal.
	BeanCounter row;

	// Country funding.
	for (auto country : *_game->getSavedGame()->getCountries())
	{
		int funding = country->getFunding().back();
		std::string description = funding >= 0 ? tr("MCDS_DETAIL_COUNTRIES_INCOME") : tr("MCDS_DETAIL_COUNTRIES_PAYBACK");
		idParent = funding >= 0 ? 0 : 1;
		row = {idItem, idParent, false, description, 1, std::abs(funding)};

		idItem = addToDetailsVector(row);
		///NOTE:
		// Can theoretically be broken down further to display 'per country funding'.
		// Since that info is already visible (and presented better) at the geoscape's
		// GRAPHS screen, I do not believe it adds value here.
		///NOTE:
		// Even though we recognize possibility of negative funding that mod tactic will
		// probably not work as intended.
		// For example: 'Country::newMonth()' can reset funding to 0 if a player performs BAD.
	}

	// Score based income is special since it is not supposed to create negative income.
	// Recognize theoretical possibility, just for completeness.
	if (_game->getMod()->getPerformanceBonusFactor() != 0)
	{
		int currentScore = _game->getSavedGame()->getCurrentScore(_game->getSavedGame()->getMonthsPassed());
		int performanceFunding = currentScore * _game->getMod()->getPerformanceBonusFactor();
		// Currently negative boni is not allowed, remove next line if that changes.
		performanceFunding = std::max(0, performanceFunding);
		std::string description = performanceFunding >= 0 ? tr("MCDS_DETAIL_PERFORMANCE_INCOME") : tr("MCDS_DETAIL_PERFORMANCE_PAYBACK");
		idParent = performanceFunding >= 0 ? 0 : 1;
		row = {idItem, idParent, false, description, 1, std::abs(performanceFunding)};

		idItem = addToDetailsVector(row);
		///NOTE:
		// Can theoretically be broken down further:
		// - council protection scheme
		// - score per region (geoscape's GRAPHS screen is better suited for this info)
		// - research scores
		// - bookkeeping correction to prevent negative income?
		// But that can easily become misleading.
		// It would benefit from an adapted spreadsheet header column:
		//  [description][score][totalValue]
		// It is also impossible to break down research scores since that is a
		//  running number with no concept of topics researched *this* month.
	}
	// Contribution of bases
	for (auto *base : *_game->getSavedGame()->getBases())
	{
		idParent = base->getMonthlyMaintenace() >= 0 ? 2 : 3;
		itemValue = abs(base->getMonthlyMaintenace());
		row = {idItem, idParent, false, base->getName(), 1, itemValue};

		idItem = addToDetailsVector(row);
	}
	// Prefer alphabetical listing of detailed rows.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return Unicode::naturalCompare(a.description, b.description);
		}
	);

	// Insert Subtotals
	int subTotal, screenTotal = 0;
	if (isSubtotalNeeded(0)) // Geoscape income
	{
		subTotal = calculateSubtotalValue(0);
		screenTotal += subTotal;
		row = {0, 0, true, tr("MCDS_SUBTOTAL_GEO_INCOME"), -1, subTotal};
		_details.insert(_details.begin(), row);
	}
	if (isSubtotalNeeded(1)) // Geoscape payback
	{
		subTotal = -1 * calculateSubtotalValue(1);
		screenTotal += subTotal;
		row = {1, 1, true, tr("MCDS_SUBTOTAL_GEO_PAYBACK"), -1, subTotal};
		_details.insert(_details.begin(), row);
	}
	if (isSubtotalNeeded(2)) // Bases Maintenance
	{
		subTotal = -1 * calculateSubtotalValue(2);
		screenTotal += subTotal;
		row = {2, 2, true, tr("MCDS_SUBTOTAL_BASES_MAINTENANCE"), -1, subTotal};
		_details.insert(_details.begin(), row);
	}
	if (isSubtotalNeeded(3)) // Bases Revenue
	{
		subTotal = calculateSubtotalValue(3);
		screenTotal += subTotal;
		row = {3, 3, true, tr("MCDS_SUBTOTAL_BASES_REVENUE"), -1, subTotal};
		_details.insert(_details.begin(), row);
	}
	// Ensure elements are shown below appropriate subtotal.
	std::stable_sort(_details.begin(), _details.end(),
		[](const BeanCounter a, const BeanCounter b)
		{
			return a.parentId < b.parentId;
		}
	);

	// Allow for double checking: use a different method (w.r.t. previous screen) for total.
	_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(screenTotal, true).c_str());
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
		ssValue << Unicode::formatFunding(_details[i].totalValue, unconditionallyShowSign);

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
