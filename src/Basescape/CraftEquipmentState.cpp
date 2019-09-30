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
#include "../Mod/RuleItem.h"
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
CraftEquipmentState::CraftEquipmentState(Base *base, size_t craft) : _sel(0), _craft(craft), _base(base), _totalItems(0), _ammoColor(0), _alternateScreen(false)
{
	_currentCraft = _base->getCrafts()->at(_craft);
	bool craftHasACrew = _currentCraft->getNumSoldiers() > 0;
	bool isNewBattle = _game->getSavedGame()->getMonthsPassed() == -1;
	_alternateScreen = Options::alternateBaseScreens;
	_reservedItems = _currentCraft->getItemsClaimedBySoldiers()->getContents();

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton((craftHasACrew || isNewBattle)? 148:288, 16, (craftHasACrew || isNewBattle)? 164:16, 176);
	_btnClear = new TextButton(148, 16, 8, 176);
	_btnInventory = new TextButton(148, 16, 8, 176);
	_txtTitle = new Text(300, 17, 16, 7);
	_txtItem = new Text(144, 9, 16, 34);
	_txtStores = new Text(150, 9, 160, 34);
	_txtAvailable = new Text(110, 9, 16, 24);
	_txtUsed = new Text(110, 9, 130, 24);
	_txtCrew = new Text(71, 9, 244, 24);
	_lstEquipment = new TextList(288, 128, 8, 42);
	_txtVehicleUsage = new Text(110, 9, 130, 24);
	_txtCraftEquipment = new Text(85, 9, 235, 34);

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
	add(_txtVehicleUsage, "text", "craftEquipment");
	add(_txtCraftEquipment, "text", "craftEquipment");

	if (_alternateScreen)
	{
		_txtUsed->setVisible(false);
	}
	else
	{
		_txtVehicleUsage->setVisible(false);
		_txtCraftEquipment->setVisible(false);
	}

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
	_txtTitle->setText(tr("STR_EQUIPMENT_FOR_CRAFT").arg(_currentCraft->getName(_game->getLanguage())));

	_txtItem->setText(tr("STR_ITEM"));

	_txtStores->setText(tr("STR_STORES"));

	_lstEquipment->setArrowColumn(203, ARROW_HORIZONTAL);
	if (_alternateScreen)
	{
		_lstEquipment->setColumns(4, 156, 25+48, 25, 26);
	}
	else
	{
		_lstEquipment->setColumns(3, 156, 83, 41);
	}
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

	int row = 0;
	const std::vector<std::string> &items = _game->getMod()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		RuleItem *rule = _game->getMod()->getItem(*i);
		int cQty = 0;
		if (rule->isFixed())
		{
			cQty = _currentCraft->getVehicleCount(*i);
		}
		else
		{
			cQty = _currentCraft->getItems()->getItem(*i);
			_totalItems += cQty;
		}
		if (rule->getBigSprite() > -1 && rule->getBattleType() != BT_NONE && rule->getBattleType() != BT_CORPSE &&
			_game->getSavedGame()->isResearched(rule->getRequirements()) &&
			(_base->getStorageItems()->getItem(*i) > 0 || cQty > 0))
		{
			_items.push_back(*i);
			std::ostringstream ss, ss2;
			if (_game->getSavedGame()->getMonthsPassed() > -1)
			{
				ss << _base->getStorageItems()->getItem(*i);
			}
			else
			{
				ss << "-";
			}
			ss2 << cQty;

			std::string s = tr(*i);
			if (rule->getBattleType() == BT_AMMO)
			{
				s.insert(0, "  ");
			}

			if (_alternateScreen)
			{
				std::string ssClaimed;
				std::map<std::string, int>::const_iterator search = _reservedItems->find(*i);
				if (search != _reservedItems->end())
				{
					int rQty = search->second;
					ssClaimed = createAssignedToSoldiersString(cQty, rQty);
				}
				_lstEquipment->addRow(4, s.c_str(), ss.str().c_str(), ss2.str().c_str(), ssClaimed.c_str());
			}
			else
			{
				_lstEquipment->addRow(3, s.c_str(), ss.str().c_str(), ss2.str().c_str());
			}

			Uint8 color;
			if (cQty == 0)
			{
				if (rule->getBattleType() == BT_AMMO)
				{
					color = _ammoColor;
				}
				else
				{
					color = _lstEquipment->getColor();
				}
			}
			else
			{
					color = _lstEquipment->getSecondaryColor();
			}
			_lstEquipment->setRowColor(row, color);

			++row;
		}
	}
	updateSubtitleLine();
	updateSpreadsheetHeader();

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

	_currentCraft->setInBattlescape(false);

	// Inventory visit might have changed item claims.
	_reservedItems = _currentCraft->getItemsClaimedBySoldiers()->getContents();
	updateEquipmentList();
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
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) moveLeftByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveLeftByValue(1);
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
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) moveRightByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveRightByValue(1);
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
			moveRightByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerRight->stop();
		_timerLeft->stop();
		if (action->getAbsoluteXMouse() >= _lstEquipment->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstEquipment->getArrowsRightEdge())
		{
			moveLeftByValue(Options::changeValueByMouseWheel);
		}
	}
}

/**
 * Updates the displayed quantities of the
 * selected item on the list.
 */
void CraftEquipmentState::updateQuantity()
{
	RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int cQty = 0;
	if (item->isFixed())
	{
		cQty = _currentCraft->getVehicleCount(_items[_sel]);
		updateSubtitleLine();
	}
	else
	{
		cQty = _currentCraft->getItems()->getItem(_items[_sel]);
		updateSpreadsheetHeader();
	}
	std::ostringstream ss, ss2;
	if (_game->getSavedGame()->getMonthsPassed() > -1)
	{
		ss << _base->getStorageItems()->getItem(_items[_sel]);
	}
	else
	{
		ss << "-";
	}
	ss2 << cQty;

	Uint8 color;
	if (cQty == 0)
	{
		RuleItem *rule = _game->getMod()->getItem(_items[_sel], true);
		if (rule->getBattleType() == BT_AMMO)
		{
			color = _ammoColor;
		}
		else
		{
			color = _lstEquipment->getColor();
		}
	}
	else
	{
		color = _lstEquipment->getSecondaryColor();
	}
	_lstEquipment->setRowColor(_sel, color);
	_lstEquipment->setCellText(_sel, 1, ss.str());
	_lstEquipment->setCellText(_sel, 2, ss2.str());

	if (_alternateScreen)
	{
		std::string ssClaimed;
		std::map<std::string, int>::const_iterator search = _reservedItems->find(_items[_sel]);
		if (search != _reservedItems->end())
		{
			int rQty = search->second;
			ssClaimed = createAssignedToSoldiersString(cQty, rQty);
		}
		_lstEquipment->setCellText(_sel, 3, ssClaimed);
	}
}

/**
 * Moves the selected item to the base.
 */
void CraftEquipmentState::moveLeft()
{
	_timerLeft->setInterval(50);
	_timerRight->setInterval(50);
	moveLeftByValue(1);
}

/**
 * Moves the given number of items (selected) to the base.
 * @param change Item difference.
 */
void CraftEquipmentState::moveLeftByValue(int change)
{
	RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int cQty = 0;
	if (item->isFixed()) cQty = _currentCraft->getVehicleCount(_items[_sel]);
	else cQty = _currentCraft->getItems()->getItem(_items[_sel]);
	if (change <= 0 || cQty <= 0) return;
	change = std::min(cQty, change);
	// Convert vehicle to item
	if (item->isFixed())
	{
		if (!item->getCompatibleAmmo()->empty())
		{
			// Calculate how much ammo needs to be added to the base.
			RuleItem *ammo = _game->getMod()->getItem(item->getCompatibleAmmo()->front(), true);
			int ammoPerVehicle;
			if (ammo->getClipSize() > 0 && item->getClipSize() > 0)
			{
				ammoPerVehicle = item->getClipSize() / ammo->getClipSize();
			}
			else
			{
				ammoPerVehicle = ammo->getClipSize();
			}
			// Put the vehicles and their ammo back as separate items.
			if (_game->getSavedGame()->getMonthsPassed() != -1)
			{
				_base->getStorageItems()->addItem(_items[_sel], change);
				_base->getStorageItems()->addItem(ammo->getType(), ammoPerVehicle * change);
			}
			// now delete the vehicles from the craft.
			for (std::vector<Vehicle*>::iterator i = _currentCraft->getVehicles()->begin(); i != _currentCraft->getVehicles()->end() && change > 0; )
			{
				if ((*i)->getRules() == item)
				{
					delete (*i);
					i = _currentCraft->getVehicles()->erase(i);
					--change;
				}
				else ++i;
			}
		}
		else
		{
			if (_game->getSavedGame()->getMonthsPassed() != -1)
			{
				_base->getStorageItems()->addItem(_items[_sel], change);
			}
			for (std::vector<Vehicle*>::iterator i = _currentCraft->getVehicles()->begin(); i != _currentCraft->getVehicles()->end() && change > 0; )
			{
				if ((*i)->getRules() == item)
				{
					delete (*i);
					i = _currentCraft->getVehicles()->erase(i);
					--change;
				}
				else ++i;
			}
		}
	}
	else
	{
		_currentCraft->getItems()->removeItem(_items[_sel], change);
		_totalItems -= change;
		if (_game->getSavedGame()->getMonthsPassed() > -1)
		{
			_base->getStorageItems()->addItem(_items[_sel], change);
		}
	}
	updateQuantity();
}

/**
 * Moves the selected item to the craft.
 */
void CraftEquipmentState::moveRight()
{
	_timerLeft->setInterval(50);
	_timerRight->setInterval(50);
	moveRightByValue(1);
}

/**
 * Moves the given number of items (selected) to the craft.
 * @param change Item difference.
 */
void CraftEquipmentState::moveRightByValue(int change)
{
	RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int bqty = _base->getStorageItems()->getItem(_items[_sel]);
	if (_game->getSavedGame()->getMonthsPassed() == -1)
	{
		if (change == INT_MAX)
		{
			change = 10;
		}
		bqty = change;
	}
	if (0 >= change || 0 >= bqty) return;
	change = std::min(bqty, change);
	// Do we need to convert item to vehicle?
	if (item->isFixed())
	{
		int size = 4;
		if (_game->getMod()->getUnit(item->getType()))
		{
			size = _game->getMod()->getArmor(_game->getMod()->getUnit(item->getType())->getArmor(), true)->getSize();
			size *= size;
		}
		// Check if there's enough room
		int room = std::min(_currentCraft->getRules()->getVehicles() - _currentCraft->getNumVehicles(), _currentCraft->getSpaceAvailable() / size);
		if (room > 0)
		{
			change = std::min(room, change);
			if (!item->getCompatibleAmmo()->empty())
			{
				// And now let's see if we can add the total number of vehicles.
				RuleItem *ammo = _game->getMod()->getItem(item->getCompatibleAmmo()->front(), true);
				int ammoPerVehicle, clipSize;
				if (ammo->getClipSize() > 0 && item->getClipSize() > 0)
				{
					clipSize = item->getClipSize();
					ammoPerVehicle = clipSize / ammo->getClipSize();
				}
				else
				{
					clipSize = ammo->getClipSize();
					ammoPerVehicle = clipSize;
				}

				int baseQty = _base->getStorageItems()->getItem(ammo->getType()) / ammoPerVehicle;
				if (_game->getSavedGame()->getMonthsPassed() == -1)
					baseQty = change;
				int canBeAdded = std::min(change, baseQty);
				if (canBeAdded > 0)
				{
					for (int i = 0; i < canBeAdded; ++i)
					{
						if (_game->getSavedGame()->getMonthsPassed() != -1)
						{
							_base->getStorageItems()->removeItem(ammo->getType(), ammoPerVehicle);
							_base->getStorageItems()->removeItem(_items[_sel]);
						}
						_currentCraft->getVehicles()->push_back(new Vehicle(item, clipSize, size));
					}
				}
				else
				{
					// So we haven't managed to increase the count of vehicles because of the ammo
					_timerRight->stop();
					LocalizedText msg(tr("STR_NOT_ENOUGH_AMMO_TO_ARM_HWP").arg(ammoPerVehicle).arg(tr(ammo->getType())));
					_game->pushState(new ErrorMessageState(msg, _palette, _game->getMod()->getInterface("craftEquipment")->getElement("errorMessage")->color, "BACK04.SCR", _game->getMod()->getInterface("craftEquipment")->getElement("errorPalette")->color));
				}
			}
			else
				for (int i = 0; i < change; ++i)
				{
					_currentCraft->getVehicles()->push_back(new Vehicle(item, item->getClipSize(), size));
					if (_game->getSavedGame()->getMonthsPassed() != -1)
					{
						_base->getStorageItems()->removeItem(_items[_sel]);
					}
				}
		}
	}
	else
	{
		if (_currentCraft->getRules()->getMaxItems() > 0 && _totalItems + change > _currentCraft->getRules()->getMaxItems())
		{
			_timerRight->stop();
			LocalizedText msg(tr("STR_NO_MORE_EQUIPMENT_ALLOWED", _currentCraft->getRules()->getMaxItems()));
			_game->pushState(new ErrorMessageState(msg, _palette, _game->getMod()->getInterface("craftEquipment")->getElement("errorMessage")->color, "BACK04.SCR", _game->getMod()->getInterface("craftEquipment")->getElement("errorPalette")->color));
			change = _currentCraft->getRules()->getMaxItems() - _totalItems;
		}
		_currentCraft->getItems()->addItem(_items[_sel],change);
		_totalItems += change;
		if (_game->getSavedGame()->getMonthsPassed() > -1)
		{
			_base->getStorageItems()->removeItem(_items[_sel],change);
		}
	}
	updateQuantity();
}

/**
 * Updates entities below screen title.
 *
 * The (derived) values between title and list.
 */
void CraftEquipmentState::updateSubtitleLine()
{
	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(_currentCraft->getSpaceAvailable()));

	if (_alternateScreen)
	{
		std::ostringstream ss1;
		ss1 << tr("STR_HWPS") << ">" << Unicode::TOK_COLOR_FLIP << _currentCraft->getNumVehicles() << ":" << _currentCraft->getRules()->getVehicles();
		_txtVehicleUsage->setText(ss1.str());
	}
	else
	{
		_txtUsed->setText(tr("STR_SPACE_USED").arg(_currentCraft->getSpaceUsed()));
	}

	std::ostringstream ss;
	ss << tr("STR_SOLDIERS_UC") << ">" << Unicode::TOK_COLOR_FLIP << _currentCraft->getNumSoldiers();
	_txtCrew->setText(ss.str());
}

/**
 * Updates variable cells in the spreadsheet header row.
 */
void CraftEquipmentState::updateSpreadsheetHeader()
{
	if (_alternateScreen)
	{
		std::ostringstream ss;
		ss << tr("STR_CRAFT") << ">" << Unicode::TOK_COLOR_FLIP << _totalItems;
		if (_currentCraft->getRules()->getMaxItems() > 0)
		{
			ss << ":" << _currentCraft->getRules()->getMaxItems();
		}
		_txtCraftEquipment->setText(ss.str());
	}
}

/**
 * Updates all rows in the equipment list.
 */
void CraftEquipmentState::updateEquipmentList()
{
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		updateQuantity();
	}
}

/**
 * Create the "claimed items" string.
 *
 * The amount of this item assigned to all soldiers on the craft.
 * @param cQty Amount of the item assigned to the craft.
 * @param rQty Amount of the item claimed by soldiers on the craft.
 * @return Text to insert in the cell.
 */

std::string CraftEquipmentState::createAssignedToSoldiersString(const int cQty, const int rQty) const
{
	std::ostringstream  itemsClaimedBySoldiers;

	if (cQty > rQty)
	{
		itemsClaimedBySoldiers << "> " << rQty; // or STR_LARGER
	}
	else if (cQty < rQty)
	{
		itemsClaimedBySoldiers << "< " << rQty; // or STR_SMALLER
	}
	else
	{
		itemsClaimedBySoldiers << "  " << rQty; // or STR_EQUAL
	}

	return itemsClaimedBySoldiers.str();
}

/**
 * Empties the contents of the craft, moving all of the items back to the base.
 */
void CraftEquipmentState::btnClearClick(Action *)
{
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		moveLeftByValue(INT_MAX);
	}
}

/**
 * Displays the inventory screen for the soldiers
 * inside the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::btnInventoryClick(Action *)
{
	if (_currentCraft->getNumSoldiers() != 0)
	{
		SavedBattleGame *bgame = new SavedBattleGame();
		_game->getSavedGame()->setBattleGame(bgame);

		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
		bgen.runInventory(_currentCraft);

		_game->getScreen()->clear();
		_game->pushState(new InventoryState(false, 0));
	}
}

}
