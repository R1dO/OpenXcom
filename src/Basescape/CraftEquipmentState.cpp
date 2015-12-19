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
#include "CraftEquipmentState.h"
#include <climits>
#include <sstream>
#include <algorithm>
#include "../Engine/Screen.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Timer.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Mod/Armor.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Vehicle.h"
#include "../Savegame/SavedGame.h"
#include "../Menu/ErrorMessageState.h"
#include "../Battlescape/InventoryState.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Equipment screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param craft ID of the selected craft.
 */
CraftEquipmentState::CraftEquipmentState(Base *base, size_t craft) : _sel(0), _craft(craft), _base(base), _totalCraftItems(0), _totalCraftVehicles(0), _totalCraftCrewSpace(0), _ammoColor(0)
{
	Craft *c = _base->getCrafts()->at(_craft);
	bool craftHasACrew = c->getNumSoldiers() > 0;
	bool isNewBattle = _game->isSkirmish();

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton((craftHasACrew || isNewBattle)? 148:288, 16, (craftHasACrew || isNewBattle)? 164:16, 176);
	_btnClear = new TextButton(148, 16, 8, 176);
	_btnInventory = new TextButton(148, 16, 8, 176);
	_txtTitle = new Text(300, 17, 16, 7);
	_txtItem = new Text(144, 9, 16, 32);
	_txtStores = new Text(150, 9, 160, 32);
	_txtAvailable = new Text(110, 9, 16, 24);
	_txtUsed = new Text(110, 9, 130, 24);
	_txtCrew = new Text(71, 9, 244, 24);
	_lstEquipment = new TextList(288, 128, 8, 40);

	// Set palette
	setInterface("craftEquipment");

	_ammoColor = _game->getMod()->getInterface("craftEquipment")->getElement("ammoColor")->color;

	add(_window, "window", "craftEquipment");
	add(_btnOk, "button", "craftEquipment");
	add(_btnClear, "button", "craftEquipment");
	add(_btnInventory, "button", "craftEquipment");
	add(_txtTitle, "text", "craftEquipment");
	add(_txtItem, "text", "craftEquipment");
	add(_txtStores, "text", "craftEquipment");
	add(_txtAvailable, "text", "craftEquipment");
	add(_txtUsed, "text", "craftEquipment");
	add(_txtCrew, "text", "craftEquipment");
	add(_lstEquipment, "list", "craftEquipment");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK04.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftEquipmentState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnOkClick, Options::keyCancel);

	_btnClear->setText(tr("STR_UNLOAD_CRAFT"));
	_btnClear->onMouseClick((ActionHandler)&CraftEquipmentState::btnClearClick);
	_btnClear->setVisible(isNewBattle);

	_btnInventory->setText(tr("STR_INVENTORY"));
	_btnInventory->onMouseClick((ActionHandler)&CraftEquipmentState::btnInventoryClick);
	_btnInventory->setVisible(craftHasACrew && !isNewBattle);

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_EQUIPMENT_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

	_txtItem->setText(tr("STR_ITEM"));

	_txtStores->setText(tr("STR_STORES"));

	std::ostringstream ss3;
	ss3 << tr("STR_SOLDIERS_UC") << ">" << Unicode::TOK_COLOR_FLIP << c->getNumSoldiers();
	_txtCrew->setText(ss3.str());

	_lstEquipment->setArrowColumn(203, ARROW_HORIZONTAL);
	_lstEquipment->setColumns(3, 156, 83, 41);
	_lstEquipment->setSelectable(true);
	_lstEquipment->setBackground(_window);
	_lstEquipment->setMargin(8);
	_lstEquipment->onLeftArrowPress((ActionHandler)&CraftEquipmentState::lstEquipmentLeftArrowPress);
	_lstEquipment->onLeftArrowRelease((ActionHandler)&CraftEquipmentState::lstEquipmentLeftArrowRelease);
	_lstEquipment->onLeftArrowClick((ActionHandler)&CraftEquipmentState::lstEquipmentLeftArrowClick);
	_lstEquipment->onRightArrowPress((ActionHandler)&CraftEquipmentState::lstEquipmentRightArrowPress);
	_lstEquipment->onRightArrowRelease((ActionHandler)&CraftEquipmentState::lstEquipmentRightArrowRelease);
	_lstEquipment->onRightArrowClick((ActionHandler)&CraftEquipmentState::lstEquipmentRightArrowClick);
	_lstEquipment->onMousePress((ActionHandler)&CraftEquipmentState::lstEquipmentMousePress);

	// Since vehicle ammo is stored implicitly while assigned to craft we need something to
	// temporarily store vehicle ammo on craft (we cannot assume it is already in the list when
	// needed while building the struct).
	std::map<std::string, int> craftVehicleAmmo; // <first compatible vehicle ammo, amount in craft>.
	const std::vector<std::string> &items = _game->getMod()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		RuleItem *item = _game->getMod()->getItem(*i);
		if (item->getBattleType() != BT_NONE &&
		    item->getBattleType() != BT_CORPSE &&
		    _game->getSavedGame()->isResearched(item->getRequirements()))
		{
			int bQty = _base->getStorageItems()->getItem(*i);
			int cQty = 0;  // Number of items in craft
			int size = 0;  // Crew space cost of item.
			if (! item->isFixed()) // Normal equipment
			{
				cQty = c->getItems()->getItem(*i);
				_totalCraftItems += cQty;
			}
			// Assume we can only assign vehicles to craft (not fixed weapons like BIODRONE_MELEE_WEAPON).
			else if (item->isFixed() && _game->getMod()->getUnit(item->getType()))
			{
				Unit *vehicle = _game->getMod()->getUnit(item->getType());
				size = vehicle->getBattleSize();
				cQty = c->getVehicleCount(*i);
				_totalCraftVehicles += cQty;
				_totalCraftCrewSpace += cQty * size;

				// Check if vehicle needs ammo.
				std::map<std::string, int> vehicleAmmoClips = vehicle->getCompatibleAmmoClips();
				if (! vehicleAmmoClips.empty())
				{
					// If we want to get fancy iterate instead of just choosing ``begin()``.
					// Current code will choose the ammo that comes first alphabetically.
					int clipsPerVehicle = vehicleAmmoClips.begin()->second;
					std::map<std::string, int>::iterator ammoKey = craftVehicleAmmo.find(vehicleAmmoClips.begin()->first);
					// Update post-processing map, allow multiple vehicles to share ammo
					if (ammoKey == craftVehicleAmmo.end())
					{
						craftVehicleAmmo.insert(std::make_pair(vehicleAmmoClips.begin()->first, clipsPerVehicle * cQty));
					}
					else
					{
						ammoKey->second += clipsPerVehicle * cQty;
					}
				}
			}

			if (bQty > 0 || cQty > 0)
			{
				EquipmentRow row = { item, tr(*i), size, bQty, cQty, 0 };
				_items.push_back(row);
			}
		}
	}
	// Post-process: Update vehicle ammo in struct and store its row #.
	_vehicleAmmoRow.clear();
	for (std::vector<EquipmentRow>::iterator i = _items.begin(); i != _items.end(); ++i)
	{
		RuleItem *rule = static_cast<RuleItem *> (i->rule);
		std::map<std::string, int>::const_iterator search = craftVehicleAmmo.find(rule->getType());
		if (search != craftVehicleAmmo.end())
		{
			_vehicleAmmoRow.insert(std::make_pair(rule->getType(), i - _items.begin()));
			i->cQty += search->second;
		}
	}

	updateDerivedInfo();
	updateList();

	_timerLeft = new Timer(250);
	_timerLeft->onTimer((StateHandler)&CraftEquipmentState::moveLeft);
	_timerRight = new Timer(250);
	_timerRight->onTimer((StateHandler)&CraftEquipmentState::moveRight);
}

/**
 *
 */
CraftEquipmentState::~CraftEquipmentState()
{
	delete _timerLeft;
	delete _timerRight;
}

/**
 * Resets the savegame when coming back from the inventory.
 */
void CraftEquipmentState::init()
{
	State::init();

	_game->getSavedGame()->setBattleGame(0);

	Craft *c = _base->getCrafts()->at(_craft);
	c->setInBattlescape(false);
}

/**
 * Updates displayed item list.
 * Sets the name of each list item, uses updateItemRow() to set values.
 * Will always reset the row pointed to by the mouse.
 */
void CraftEquipmentState::updateList()
{
	_lstEquipment->clearList();
	_rows.clear();
	for (size_t row = 0; row < _items.size(); ++row)
	{
		RuleItem *item = _items[row].rule;
//		if (item->getBigSprite() == -1) // Vehicle ammo
//		{
//			continue;
//		}
		std::wstring ssName = _items[row].name;
		if (item->getBattleType() == BT_AMMO)
		{
			ssName.insert(0, L"  ");
		}
		_lstEquipment->addRow(3, ssName.c_str(), L"", L"");
		_rows.push_back(row);

		_sel = row; // Mimic selection of row by mouse.
		updateItemRow();
	}
	_sel = 0;  // Clean up
}

/**
 * Runs the arrow timers.
 */
void CraftEquipmentState::think()
{
	State::think();

	_timerLeft->think(this, 0);
	_timerRight->think(this, 0);
}


/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Starts moving the item to the base.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentLeftArrowPress(Action *action)
{
	_sel = _lstEquipment->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerLeft->isRunning()) _timerLeft->start();
}

/**
 * Stops moving the item to the base.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentLeftArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerLeft->stop();
	}
}

/**
 * Moves all the items to the base on right-click.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentLeftArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) moveToBase(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveToBase(1);
		_timerRight->setInterval(250);
		_timerLeft->setInterval(250);
	}
}

/**
 * Starts moving the item to the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentRightArrowPress(Action *action)
{
	_sel = _lstEquipment->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerRight->isRunning()) _timerRight->start();
}

/**
 * Stops moving the item to the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentRightArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerRight->stop();
	}
}

/**
 * Moves all the items (as much as possible) to the craft on right-click.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentRightArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) moveToCraft(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveToCraft(1);
		_timerRight->setInterval(250);
		_timerLeft->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::lstEquipmentMousePress(Action *action)
{
	_sel = _lstEquipment->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerRight->stop();
		_timerLeft->stop();
		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveToCraft(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerRight->stop();
		_timerLeft->stop();
		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveToBase(Options::changeValueByMouseWheel);
		}
	}
}

/**
 * Updates quantity strings and row color of the selected item.
 */
void CraftEquipmentState::updateItemRow()
{
	std::wostringstream ssQtyBase, ssQtyCraft;
	if (_game->isCampaign())
	{
		ssQtyBase << getRow().bQty + getRow().amount;
	}
	else
	{
		ssQtyBase << "-";
	}
	ssQtyCraft << getRow().cQty - getRow().amount;
	_lstEquipment->setCellText(_sel, 1, ssQtyBase.str());
	_lstEquipment->setCellText(_sel, 2, ssQtyCraft.str());

	RuleItem *rule = static_cast<RuleItem*> (getRow().rule);
	Uint8 color = _lstEquipment->getColor();
	if (getRow().cQty - getRow().amount != 0)
	{
		color = _lstEquipment->getSecondaryColor();
	}
	else if (rule->getBattleType() == BT_AMMO)
	{
		color = _ammoColor;
	}
	_lstEquipment->setRowColor(_sel, color);
}

/**
 * Moves the selected item to the base.
 */
void CraftEquipmentState::moveLeft()
{
	_timerLeft->setInterval(50);
	_timerRight->setInterval(50);
	moveToBase(1);
}

/**
 * Moves the given number of items to the base.
 * Updates EquipmentRow::amount and derived totals.
 * @param change Amount to move from craft to base.
 */
void CraftEquipmentState::moveToBase(int change)
{
	if (change < 0 || getRow().cQty <= getRow().amount) return; // Invalid or not enough in craft
	change = std::min(getRow().cQty - getRow().amount, change); // Max based on in craft items, taking into account previous changes.

	if (getRow().space > 0) // Its a vehicle
	{
		RuleItem *item = static_cast<RuleItem*> (getRow().rule);
		std::map<std::string, int> vehicleAmmoClips = _game->getMod()->getUnit(item->getType())->getCompatibleAmmoClips();

		if (! vehicleAmmoClips.empty())
		{
			// Refund ammoClips.
			RuleItem *ammo = _game->getMod()->getItem(vehicleAmmoClips.begin()->first); //
			int clipsPerVehicle = vehicleAmmoClips.begin()->second;

			// Locate and update ammo in list.
			std::map<std::string, size_t>::const_iterator search = _vehicleAmmoRow.find(ammo->getType());
			if (search != _vehicleAmmoRow.end())
			{
				// Adjust compatible ammo row.
				_totalCraftItems += change * clipsPerVehicle;	// Vehicle ammo is supposed to be free of charge.
				size_t currentSel = _sel;
				_sel = search->second; // Mimic mouse selection (even when row is hidden).
				moveToBase(change*clipsPerVehicle);
				_sel = currentSel; // Return focus to vehicle row.
			}
		}
		// Positive values move towards base, hence amount in craft decrease.
		_totalCraftVehicles -= change;
		_totalCraftCrewSpace -= change * getRow().space;
	}
	else
	{
		_totalCraftItems -= change;
	}
	getRow().amount += change; // To base is positive

	if (_game->isSkirmish())
	{
		getRow().bQty = -1 * getRow().amount;
	}

	updateItemRow();
	updateDerivedInfo();
}

/**
 * Moves the selected item to the craft.
 */
void CraftEquipmentState::moveRight()
{
	_timerLeft->setInterval(50);
	_timerRight->setInterval(50);
	moveToCraft(1);
}

/**
 * Moves the given number of items to the craft.
 * Updates EquipmentRow::amount and derived totals.
 * @param change Amount to move from base to craft.
 */
void CraftEquipmentState::moveToCraft(int change)
{
	if (_game->isSkirmish())
	{
		if (change == INT_MAX)
		{
			change = 10;
		}
		getRow().bQty += change;
	}
	if (change < 0 || getRow().bQty <= -1 * getRow().amount) return; // Invalid or not enough in craft
	std::wstring errorMessage;
	Craft *c = _base->getCrafts()->at(_craft);

	change = std::min(getRow().bQty + getRow().amount, change);  // Take into account previous changes.
	if (getRow().space > 0) // Its a vehicle
	{
		// Check if there's enough room.
		int room = std::min((c->getRules()->getVehicles() - _totalCraftVehicles),
		                    (c->getSpaceMax() - c->getNumSoldiers() - _totalCraftCrewSpace) / getRow().space);
		if (room < 0) // RuleSet changes since last save.
		{
			moveToBase(abs(room));
			return;
		}
		change = std::min(room, change);
		RuleItem *item = static_cast<RuleItem*> (getRow().rule);
		std::map<std::string, int> vehicleAmmoClips = _game->getMod()->getUnit(item->getType())->getCompatibleAmmoClips();

		if (! vehicleAmmoClips.empty())
		{
			// And now let's see if we can add the total number of vehicles.
			// This will always return false if first compatibleAmmo has too little clips, even when a
			// hypothetical 2nd kind has enough. Not that it matters since we can't choose ingame which
			// kind of compatibleAmmo a vehicle should use.
			RuleItem *ammo = _game->getMod()->getItem(vehicleAmmoClips.begin()->first); //
			int clipsPerVehicle = vehicleAmmoClips.begin()->second;

			// Locate and update ammo in list.
			std::map<std::string, size_t>::const_iterator search = _vehicleAmmoRow.find(ammo->getType());
			if (search == _vehicleAmmoRow.end())
			{
				// Ammo disappeared from _items, or vehicle was teleported into _items.
				errorMessage = tr("STR_NOT_ENOUGH_AMMO_TO_ARM_HWP").arg(tr(ammo->getType()));
				return;
			}
			// Check for discrepancy in clips on craft vs vehicle (when ammo row is visible).
			int extraClips =  (_items[search->second].cQty - _items[search->second].amount) -
			                  (getRow().cQty - getRow().amount) * clipsPerVehicle;
			int maxByClips = change;
			if (_game->isCampaign())
			{
				maxByClips = (_items[search->second].bQty + _items[search->second].amount + extraClips) / clipsPerVehicle;
			}
			if (maxByClips < change)
			{
				// So we haven't managed to increase the count of vehicles because of the ammo
				errorMessage = tr("STR_NOT_ENOUGH_AMMO_TO_ARM_HWP").arg(tr(ammo->getType()));
				change = maxByClips;
			}

			_totalCraftItems -= change * clipsPerVehicle - extraClips;  // Vehicle ammo is supposed to be free of charge.
			size_t currentSel = _sel;
			_sel = search->second; // Mimic mouse selection (even when row is hidden).
			moveToCraft(change * clipsPerVehicle - extraClips);
			_sel = currentSel; // Return focus to vehicle row.

		}
		_totalCraftVehicles += change;
		_totalCraftCrewSpace += change * getRow().space;
	}
	else
	{
		if (c->getRules()->getMaxItems() > 0 && (_totalCraftItems + change > c->getRules()->getMaxItems()))
		{
			errorMessage = tr("STR_NO_MORE_EQUIPMENT_ALLOWED").arg(c->getRules()->getMaxItems());
			change = std::max(-1*getRow().cQty, c->getRules()->getMaxItems() - _totalCraftItems);
		}
		_totalCraftItems += change;
	}

	getRow().amount -= change; // To craft is negative.
	if (_game->isSkirmish())
	{
		getRow().bQty = -1 * getRow().amount;
	}
	updateItemRow();
	updateDerivedInfo();

	if (! errorMessage.empty())
	{
		RuleInterface *menuInterface = _game->getMod()->getInterface("craftEquipment");
		_timerRight->stop();
		_game->pushState(new ErrorMessageState(errorMessage, _palette, menuInterface->getElement("errorMessage")->color, "BACK04.SCR", menuInterface->getElement("errorPalette")->color));
	}
}

/**
 * Updates derived values entities.
 *
 * Values between title and list.
 */
void CraftEquipmentState::updateDerivedInfo()
{
	Craft *c = _base->getCrafts()->at(_craft);
	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceMax() - c->getNumSoldiers() - _totalCraftCrewSpace));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getNumSoldiers() + _totalCraftCrewSpace));
}

/**
 * Empties the contents of the craft, moving all of the items back to the base.
 */
void CraftEquipmentState::btnClearClick(Action *)
{
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		moveToBase(INT_MAX);
	}
}

/**
 * Displays the inventory screen for the soldiers
 * inside the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::btnInventoryClick(Action *)
{
	Craft *craft = _base->getCrafts()->at(_craft);
	if (craft->getNumSoldiers() != 0)
	{
		SavedBattleGame *bgame = new SavedBattleGame();
		_game->getSavedGame()->setBattleGame(bgame);

		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
		bgen.runInventory(craft);

		_game->getScreen()->clear();
		_game->pushState(new InventoryState(false, 0));
	}
}

}
