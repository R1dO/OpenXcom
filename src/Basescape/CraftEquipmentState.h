#pragma once
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
#include "../Engine/State.h"
#include <vector>
#include <string>
#include "../Mod/RuleItem.h"
#include <map>

namespace OpenXcom
{

/**
 * Structure to keep track of requested changes to craft equipment.
 *
 * Deliberately not using TransferRow since the latter one is geared more toward
 * transferring between bases (can lead to false expectations).
 */
struct EquipmentRow
{
	RuleItem *rule;  ///< Pointer to ruleset of item.
	std::wstring name;  ///< Translated name of item.
	int bQty;  ///< Starting amount of base stores.
	int cQty;  ///< Starting amount of craft.
	int assignedQty;  /**< Reserved amount of item on the craft.
	                   *
	                   * Reservation due to assignment to soldier / HWP clips
	                   */
	int amount;  /**< Requested change.
	              *
	              * For consistency with other base specific transfer actions the
	              * direction of change is defined as:
	              * * Positive values moves an item from craft to base stores.
	              * * Negative values moves an item from base stores to craft.
	              */
};

class TextButton;
class Window;
class Text;
class TextList;
class Timer;
class Base;
class Craft;

/**
 * Equipment screen that lets the player
 * pick the equipment to carry on a craft.
 */
class CraftEquipmentState : public State
{
private:
	TextButton *_btnOk, *_btnClear, *_btnInventory;
	Window *_window;
	Text *_txtTitle, *_txtItem, *_txtStores, *_txtAvailable, *_txtUsed, *_txtCrew;
	TextList *_lstEquipment;
	Timer *_timerLeft, *_timerRight;
	size_t _sel, _craftId;
	Base *_base;
	Craft *_craft;
	std::vector<EquipmentRow> _items;
	std::vector<int> _rows;
	std::map<std::string, size_t> _ammoMap;
	int _totalCraftItems, _totalCraftVehicles, _totalCraftCrewSpace;
	Uint8 _ammoColor;
	/// Gets the row of the current selection.
	EquipmentRow &getRow() { return _items[_rows[_sel]]; }
	/// Resets state.
	void init();
	/// Runs the arrow timers.
	void think();
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Empties the contents of the craft, moving all of the items back to the base.
	void btnClearClick(Action *action);
	/// Handler for clicking the Inventory button.
	void btnInventoryClick(Action *action);
	/// Handler for pressing a Move Left arrow in the list.
	void lstEquipmentLeftArrowPress(Action *action);
	/// Handler for releasing a Move Left arrow in the list.
	void lstEquipmentLeftArrowRelease(Action *action);
	/// Handler for clicking a Move Left arrow in the list.
	void lstEquipmentLeftArrowClick(Action *action);
	/// Handler for pressing a Move Right arrow in the list.
	void lstEquipmentRightArrowPress(Action *action);
	/// Handler for releasing a Move Right arrow in the list.
	void lstEquipmentRightArrowRelease(Action *action);
	/// Handler for clicking a Move Right arrow in the list.
	void lstEquipmentRightArrowClick(Action *action);
	/// Handler for pressing-down a mouse-button in the list.
	void lstEquipmentMousePress(Action *action);
	/// Moves the selected item to the base.
	void moveLeft();
	/// Moves the selected item to the craft.
	void moveRight();
	/// Moves the given number of items to the base.
	void moveToBase(int change);
	/// Moves the given number of items to the craft.
	void moveToCraft(int change);
	/// Performs the transfer between base and craft.
	void performTransfer();
	/// Updates derived values entities.
	void updateDerivedInfo();
	/// Updates quantity strings and row color of the selected item.
	void updateItemRow();
	/// Updates displayed item list.
	void updateList();
public:
	/// Creates the Craft Equipment state.
	CraftEquipmentState(Base *base, size_t craft);
	/// Cleans up the Craft Equipment state.
	~CraftEquipmentState();
};

}
