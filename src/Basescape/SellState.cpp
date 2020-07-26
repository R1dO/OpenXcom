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
#include "SellState.h"
#include <algorithm>
#include <sstream>
#include <climits>
#include <cmath>
#include <iomanip>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/ComboBox.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Engine/Timer.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Sell/Sack screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param origin Game section that originated this state.
 */
SellState::SellState(Base *base, OptionsOrigin origin) : _base(base), _sel(0), _total(0), _spaceChange(0), _origin(origin)
{
	bool overfull = Options::storageLimitsEnforced && _base->storesOverfull();
	_alternateScreen = Options::alternateBaseScreens;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(overfull? 288:148, 16, overfull? 16:8, 176);
	_btnCancel = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtSales = new Text(150, 9, 10, 24);
	_txtFunds = new Text(150, 9, 160, 24);
	_txtSpaceUsed = new Text(150, 9, 160, 34);
	_txtQuantity = new Text(54, 9, 136, 44);
	_txtSell = new Text(96, 9, 190, 44);
	_txtValue = new Text(40, 9, 270, 44);
	_cbxCategory = new ComboBox(this, 120, 16, 10, 36);
	_lstItems = new TextList(287, 120, 8, 54);

	// Set palette
	setInterface("sellMenu");

	_ammoColor = _game->getMod()->getInterface("sellMenu")->getElement("ammoColor")->color;

	add(_window, "window", "sellMenu");
	add(_btnOk, "button", "sellMenu");
	add(_btnCancel, "button", "sellMenu");
	add(_txtTitle, "text", "sellMenu");
	add(_txtSales, "text", "sellMenu");
	add(_txtFunds, "text", "sellMenu");
	add(_txtSpaceUsed, "text", "sellMenu");
	add(_txtQuantity, "text", "sellMenu");
	add(_txtSell, "text", "sellMenu");
	add(_txtValue, "text", "sellMenu");
	add(_lstItems, "list", "sellMenu");
	add(_cbxCategory, "text", "sellMenu");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_SELL_SACK"));
	_btnOk->onMouseClick((ActionHandler)&SellState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&SellState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&SellState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SellState::btnCancelClick, Options::keyCancel);

	if (overfull)
	{
		_btnCancel->setVisible(false);
		_btnOk->setVisible(false);
	}

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELL_ITEMS_SACK_PERSONNEL"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtSell->setText(tr("STR_SELL_SACK"));

	_txtValue->setText(tr("STR_VALUE"));

	if (_alternateScreen)
	{
		_lstItems->setArrowColumn(189, ARROW_VERTICAL);
		// Use an empty column to reserve space (28) for the arrows. To allow for arbitrary cell text alignment.
		_lstItems->setColumns(6, 140, 22, 22, 28, 18, 55);
	}
	else
	{
		_lstItems->setArrowColumn(182, ARROW_VERTICAL);
		_lstItems->setColumns(4, 156, 54, 24, 53);
	}
	_lstItems->setSelectable(true);
	_lstItems->setBackground(_window);
	_lstItems->setMargin(2);
	_lstItems->onLeftArrowPress((ActionHandler)&SellState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)&SellState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)&SellState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)&SellState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)&SellState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)&SellState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)&SellState::lstItemsMousePress);
	_lstItems->setWordWrap(true);

	_cats.push_back("STR_ALL_ITEMS");

	const std::vector<std::string> &cw = _game->getMod()->getCraftWeaponsList();
	for (std::vector<std::string>::const_iterator i = cw.begin(); i != cw.end(); ++i)
	{
		RuleCraftWeapon *rule = _game->getMod()->getCraftWeapon(*i);
		_craftWeapons.insert(rule->getLauncherItem());
		_craftWeapons.insert(rule->getClipItem());
	}
	const std::vector<std::string> &ar = _game->getMod()->getArmorsList();
	for (std::vector<std::string>::const_iterator i = ar.begin(); i != ar.end(); ++i)
	{
		Armor *rule = _game->getMod()->getArmor(*i);
		_armors.insert(rule->getStoreItem());
	}

	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if ((*i)->getCraft() == 0)
		{
			SellRow row = { TRANSFER_SOLDIER, (*i), (*i)->getName(true), 0, 1, 0, 0, 0, true };
			// Original behavior makes sense (no display of named soldiers assigned to
			// craft or in-transfer, to protect against sale).
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	for (std::vector<Craft*>::iterator i = _base->getCrafts()->begin(); i != _base->getCrafts()->end(); ++i)
	{
		if ((*i)->getStatus() != "STR_OUT")
		{
			SellRow row = { TRANSFER_CRAFT, (*i), (*i)->getName(_game->getLanguage()), (*i)->getRules()->getSellCost(), 1, 0, 0, 0, true };
			// Original behavior makes sense (no display of named aircraft currently
			// on a mission or in-transfer, to protect against sale).
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	{ //Scientists
		SellRow row = { TRANSFER_SCIENTIST, 0, tr("STR_SCIENTIST"), 0, _base->getAvailableScientists(), 0, 0, 0, true };
		if (_alternateScreen)
		{
			row.qtySrc = _base->getTotalScientists();
			row.reserved = _base->getAllocatedScientists();
			row.inTransfer = row.qtySrc - _base->getTotalScientists(false);
		}
		if (row.qtySrc > 0)
		{
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	{ //Engineers
		SellRow row = { TRANSFER_ENGINEER, 0, tr("STR_ENGINEER"), 0, _base->getAvailableEngineers(), 0, 0, 0, true };
		if (_alternateScreen)
		{
			row.qtySrc = _base->getTotalEngineers();
			row.reserved = _base->getAllocatedEngineers();
			row.inTransfer = row.qtySrc - _base->getAvailableEngineers() - row.reserved;
		}
		if (row.qtySrc > 0)
		{
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	const std::vector<std::string> &items = _game->getMod()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		RuleItem *rule = _game->getMod()->getItem(*i, true);
		SellRow row = { TRANSFER_ITEM, rule, tr(*i), rule->getSellCost(), _base->getStorageItems()->getItem(*i), 0, 0, 0, true };

		// If returning from a mission allow sale of reserved items.
		if (_origin == OPT_BATTLESCAPE)
		{
			row.protectReserved = false;

			// Unless it is one of the special categories below
			for (std::vector<Craft*>::const_iterator j = _base->getCrafts()->begin(); j != _base->getCrafts()->end(); ++j)
			{
				// Fuel (elerium / zrbite)
				if ((*j)->getRules()->getRefuelItem() == (*i))
				{
					row.protectReserved = true;
				}
				// Armament
				else if ((*j)->getNumWeapons() > 0 && (*j)->getArmamentCount(*i, _game->getMod()) > 0)
				{
					row.protectReserved = true;
				}
				// HWP (vehicle and ammo).
				else if ((*j)->getNumVehicles() > 0 && ( (*j)->getVehicleCount(*i) > 0 || (*j)->getVehicleAmmoCount(*i, _game->getMod()) > 0))
				{
					row.protectReserved = true;
				}
			}
			// Any armor
			if (_armors.find(*i) != _armors.end())
			{
				row.protectReserved = true;
			}
		}

		if (_alternateScreen)
		{
			row.reserved = _base->getCraftItemCount(*i) + _base->getSoldierArmorCount(*i);
			row.inTransfer = _base->getTransferItemCount(*i);
			row.qtySrc += row.reserved + row.inTransfer;
		}
		else if (Options::storageLimitsEnforced && _origin == OPT_BATTLESCAPE)
		{
			row.qtySrc += _base->getTransferItemCount(*i);
			if (row.protectReserved == false)
			{
				row.qtySrc += _base->getCraftItemCount(*i);
			}
		}

		if (row.qtySrc > 0 && (Options::canSellLiveAliens || !rule->isAlien()))
		{
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	_cbxCategory->setOptions(_cats, true);
	_cbxCategory->onChange((ActionHandler)&SellState::cbxCategoryChange);

	updateList();
	updateSubtitleLine();

	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)&SellState::increase);
	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)&SellState::decrease);
}

/**
 *
 */
SellState::~SellState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void SellState::think()
{
	State::think();

	_timerInc->think(this, 0);
	_timerDec->think(this, 0);
}

/**
 * Determines the category a row item belongs in.
 * @param sel Selected row.
 * @returns Item category.
 */
std::string SellState::getCategory(int sel) const
{
	RuleItem *rule = 0;
	switch (_items[sel].type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
		return "STR_PERSONNEL";
	case TRANSFER_CRAFT:
		return "STR_CRAFT_ARMAMENT";
	case TRANSFER_ITEM:
		rule = (RuleItem*)_items[sel].rule;
		if (rule->getBattleType() == BT_CORPSE || rule->isAlien())
		{
			return "STR_ALIENS";
		}
		if (rule->getBattleType() == BT_NONE)
		{
			if (_craftWeapons.find(rule->getType()) != _craftWeapons.end())
			{
				return "STR_CRAFT_ARMAMENT";
			}
			if (_armors.find(rule->getType()) != _armors.end())
			{
				return "STR_EQUIPMENT";
			}
			return "STR_COMPONENTS";
		}
		return "STR_EQUIPMENT";
	}
	return "STR_ALL_ITEMS";
}

/**
 * Filters the current list of items.
 */
void SellState::updateList()
{
	_lstItems->clearList();
	_rows.clear();
	for (size_t i = 0; i < _items.size(); ++i)
	{
		std::string cat = _cats[_cbxCategory->getSelected()];
		if (cat != "STR_ALL_ITEMS" && cat != getCategory(i))
		{
			continue;
		}
		std::string name = _items[i].name;
		bool ammo = false;
		if (_items[i].type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)_items[i].rule;
			ammo = (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0));
			if (ammo)
			{
				name.insert(0, "  ");
			}
		}
		std::ostringstream ssQty, ssAmount;
		ssQty << _items[i].qtySrc - _items[i].amount;
		if (_alternateScreen)
		{
			std::ostringstream ssReserved;
			if (_items[i].reserved != 0)
			{
				ssReserved << "(" << _items[i].reserved << ")";
			}
			if (_items[i].amount != 0)
			{
				ssAmount << _items[i].amount;
			}
			_lstItems->addRow(6, name.c_str(), ssQty.str().c_str(), ssReserved.str().c_str(), "", ssAmount.str().c_str(), Unicode::formatFunding(_items[i].cost).c_str());
		}
		else
		{
			ssAmount << _items[i].amount;
			_lstItems->addRow(4, name.c_str(), ssQty.str().c_str(), ssAmount.str().c_str(), Unicode::formatFunding(_items[i].cost).c_str());
		}
		_rows.push_back(i);

		if (_items[i].amount > 0)
		{
			_lstItems->setRowColor(_rows.size() - 1, _lstItems->getSecondaryColor());
		}
		else if (ammo)
		{
			_lstItems->setRowColor(_rows.size() - 1, _ammoColor);
		}
	}
}

/**
 * Sells the selected items.
 * @param action Pointer to an action.
 */
void SellState::btnOkClick(Action *)
{
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + _total);
	Soldier *soldier;
	Craft *craft;
	for (std::vector<SellRow>::const_iterator i = _items.begin(); i != _items.end(); ++i)
	{
		if (i->amount > 0)
		{
			switch (i->type)
			{
			case TRANSFER_SOLDIER:
				soldier = (Soldier*)i->rule;
				for (std::vector<Soldier*>::iterator s = _base->getSoldiers()->begin(); s != _base->getSoldiers()->end(); ++s)
				{
					if (*s == soldier)
					{
						if ((*s)->getArmor()->getStoreItem() != Armor::NONE)
						{
							_base->getStorageItems()->addItem((*s)->getArmor()->getStoreItem());
						}
						_base->getSoldiers()->erase(s);
						break;
					}
				}
				delete soldier;
				break;
			case TRANSFER_CRAFT:
				craft = (Craft*)i->rule;
				_base->removeCraft(craft, true);
				delete craft;
				break;
			case TRANSFER_SCIENTIST:
				_base->setScientists(_base->getScientists() - i->amount);
				break;
			case TRANSFER_ENGINEER:
				_base->setEngineers(_base->getEngineers() - i->amount);
				break;
			case TRANSFER_ITEM:
				int toRemove = i->amount;
				RuleItem *item = (RuleItem*)i->rule;
				// Start by canceling outstanding orders (rerouted or returned to sender by post-office)
				for (std::vector<Transfer*>::iterator j = _base->getTransfers()->begin(); j != _base->getTransfers()->end() && toRemove;)
				{
					if ((*j)->getItems() == item->getType())
					{
						if ((*j)->getQuantity() <= toRemove)
						{
							toRemove -= (*j)->getQuantity();
							delete *j;
							j = _base->getTransfers()->erase(j);
						}
						else
						{
							(*j)->setItems((*j)->getItems(), (*j)->getQuantity() - toRemove);
							toRemove = 0;
						}
					}
					else
					{
						++j;
					}
				}

				// If we still need to remove any, take them from the base storage facilities.
				if (_base->getStorageItems()->getItem(item->getType()) < toRemove)
				{
					// Remove all of said items from base
					toRemove -= _base->getStorageItems()->getItem(item->getType());
					_base->getStorageItems()->removeItem(item->getType(), INT_MAX);
				}
				else
				{
					_base->getStorageItems()->removeItem(item->getType(), toRemove);
					toRemove = 0;
				}

				// As a last resort remove them from the crafts. We postponed it as long
				// as possible but now it is time for our ready-for-battle veterans to
				// hand over their shiny stuff .
				// Better stay away from the troopers for a while commander ... UPROAR!!
				if (toRemove > 0)
				{
					for (std::vector<Craft*>::iterator j = _base->getCrafts()->begin(); j != _base->getCrafts()->end() && toRemove; ++j)
					{
						if ((*j)->getItems()->getItem(item->getType()) < toRemove)
						{
							toRemove -= (*j)->getItems()->getItem(item->getType());
							(*j)->getItems()->removeItem(item->getType(), INT_MAX);
						}
						else
						{
							(*j)->getItems()->removeItem(item->getType(), toRemove);
						}
					}
				}
				break;
			}
		}
	}
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SellState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Starts increasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerInc->isRunning()) _timerInc->start();
}

/**
 * Stops increasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerInc->stop();
	}
}

/**
 * Increases the selected item;
 * by one on left-click, to max on right-click.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) changeByValue(INT_MAX, 1);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		changeByValue(1,1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Starts decreasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerDec->isRunning()) _timerDec->start();
}

/**
 * Stops decreasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDec->stop();
	}
}

/**
 * Decreases the selected item;
 * by one on left-click, to 0 on right-click.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) changeByValue(INT_MAX, -1);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		changeByValue(1,-1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void SellState::lstItemsMousePress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, 1);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, -1);
		}
	}
}

/**
 * Increases the quantity of the selected item to sell by one.
 */
void SellState::increase()
{
	_timerDec->setInterval(50);
	_timerInc->setInterval(50);
	changeByValue(1,1);
}

/**
 * Increases or decreases the quantity of the selected item to sell.
 * @param change How much we want to add or remove.
 * @param dir Direction to change, +1 to increase or -1 to decrease.
 */
void SellState::changeByValue(int change, int dir)
{
	if (dir > 0 && getRow().protectReserved == true)
	{
		if (0 >= change || (getRow().qtySrc - getRow().reserved) <= getRow().amount) return;
		change = std::min((getRow().qtySrc - getRow().reserved) - getRow().amount, change);
	}
	else if (dir > 0)
	{
		if (0 >= change || getRow().qtySrc <= getRow().amount) return;
		change = std::min(getRow().qtySrc - getRow().amount, change);
	}
	else
	{
		if (0 >= change || 0 >= getRow().amount) return;
		change = std::min(getRow().amount, change);
	}
	getRow().amount += dir * change;
	_total += dir * getRow().cost * change;

	// Calculate the change in storage space.
	Craft *craft;
	Soldier *soldier;
	RuleItem *armor, *item, *weapon, *ammo;
	double total = 0.0;
	switch (getRow().type)
	{
	case TRANSFER_SOLDIER:
		soldier = (Soldier*)getRow().rule;
		if (soldier->getArmor()->getStoreItem() != Armor::NONE)
		{
			armor = _game->getMod()->getItem(soldier->getArmor()->getStoreItem(), true);
			_spaceChange += dir * armor->getSize();
		}
		break;
	case TRANSFER_CRAFT:
		craft = (Craft*)getRow().rule;
		for (std::vector<CraftWeapon*>::iterator w = craft->getWeapons()->begin(); w != craft->getWeapons()->end(); ++w)
		{
			if (*w)
			{
				weapon = _game->getMod()->getItem((*w)->getRules()->getLauncherItem(), true);
				total += weapon->getSize();
				ammo = _game->getMod()->getItem((*w)->getRules()->getClipItem());
				if (ammo)
					total += ammo->getSize() * (*w)->getClipsLoaded(_game->getMod());
			}
		}
		_spaceChange += dir * total;
		break;
	case TRANSFER_ITEM:
		item = (RuleItem*)getRow().rule;
		_spaceChange -= dir * change * item->getSize();
		break;
	default:
		//TRANSFER_SCIENTIST and TRANSFER_ENGINEER do not own anything that takes storage
		break;
	}

	updateItemStrings();
}

/**
 * Decreases the quantity of the selected item to sell by one.
 */
void SellState::decrease()
{
	_timerInc->setInterval(50);
	_timerDec->setInterval(50);
	changeByValue(1,-1);
}

/**
 * Updates the quantity-strings of the selected item.
 */
void SellState::updateItemStrings()
{
	std::ostringstream ss, ss2, ss3;
	ss2 << getRow().qtySrc - getRow().amount;
	_lstItems->setCellText(_sel, 1, ss2.str());
	if (_alternateScreen)
	{
		if (getRow().amount != 0)
		{
			ss << getRow().amount;
		}
		_lstItems->setCellText(_sel, 4, ss.str());
	}
	else
	{
		ss << getRow().amount;
		_lstItems->setCellText(_sel, 2, ss.str());
	}

	if (getRow().amount > 0)
	{
		_lstItems->setRowColor(_sel, _lstItems->getSecondaryColor());
	}
	else
	{
		_lstItems->setRowColor(_sel, _lstItems->getColor());
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0))
			{
				_lstItems->setRowColor(_sel, _ammoColor);
			}
		}
	}

	if (Options::storageLimitsEnforced)
	{
		_btnOk->setVisible(!_base->storesOverfull(_spaceChange));
	}

	updateSubtitleLine();
}

/**
* Updates the production list to match the category filter.
*/
void SellState::cbxCategoryChange(Action *)
{
	updateList();
}

/**
* Updates entities below screen title.
*
* The (derived) values between title and list.
*/
void SellState::updateSubtitleLine()
{
	std::ostringstream ss;
	ss << _base->getUsedStores();
	if (std::abs(_spaceChange) > 0.05)
	{
		ss << "(";
		if (_spaceChange > 0.05)
			ss << "+";
		ss << std::fixed << std::setprecision(1) << _spaceChange << ")";
	}
	ss << ":" << _base->getAvailableStores();

	if (_alternateScreen)
	{
		std::ostringstream ssFunds;
		ssFunds << Unicode::formatFunding(_game->getSavedGame()->getFunds());
		if (_total > 0)
		{
			ssFunds << " (+" << Unicode::formatFunding(_total) << ")";
		}
		// Let "_txtSales" show the current funds & the change.
		_txtSales->setText(tr("STR_FUNDS").arg(ssFunds.str()));
		// Abuse default "_txtFunds" to display used space (since "_txtSales" already shows purchase cost)
		_txtFunds->setText(tr("STR_SPACE_USED").arg(ss.str()));
	}
	else
	{
		_txtSpaceUsed->setVisible(Options::storageLimitsEnforced);

		_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Unicode::formatFunding(_total)));
		_txtFunds->setText(tr("STR_FUNDS").arg(Unicode::formatFunding(_game->getSavedGame()->getFunds())));
		_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss.str()));
	}
}

}
