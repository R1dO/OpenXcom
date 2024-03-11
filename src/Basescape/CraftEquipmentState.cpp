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
#include "CraftEquipmentLoadState.h"
#include "CraftEquipmentSaveState.h"
#include <climits>
#include <sstream>
#include <algorithm>
#include <locale>
#include <iomanip>
#include "../Engine/CrossPlatform.h"
#include "../Engine/Screen.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Timer.h"
#include "../Engine/Collections.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Mod/Armor.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/ItemContainer.h"
#include "../Mod/RuleItemCategory.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/Vehicle.h"
#include "../Savegame/SavedGame.h"
#include "../Menu/ErrorMessageState.h"
#include "../Battlescape/CannotReequipState.h"
#include "../Battlescape/DebriefingState.h"
#include "../Battlescape/InventoryState.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Mod/RuleInterface.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Interface/ArrowButton.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Equipment screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param craft ID of the selected craft.
 */
CraftEquipmentState::CraftEquipmentState(Base *base, size_t craft) :
	_lstScroll(0), _sel(0), _craft(craft), _base(base), _totalItems(0), _totalItemStorageSize(0.0), _ammoColor(0),
	_reload(true), _returningFromGlobalTemplates(false), _returningFromInventory(false), _firstInit(true), _isNewBattle(false),
	_showSpaceLimit(false), _showItemLimit(false), _showItemSizeLimit(false)
{
	Craft *c = _base->getCrafts()->at(_craft);
	bool craftHasACrew = c->getNumTotalSoldiers() > 0;
	_isNewBattle = _game->getSavedGame()->getMonthsPassed() == -1;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 264, 12);
	_btnOk = new TextButton((craftHasACrew || _isNewBattle)?30:140, 16, (craftHasACrew || _isNewBattle)?274:164, 176);
	_btnClear = new TextButton(102, 16, 164, 176);
	_btnInventory = new TextButton(102, 16, 164, 176);
	_txtTitle = new Text(300, 17, 16, 7);
	_txtItem = new Text(144, 9, 16, 32);
	_txtStores = new Text(150, 9, 160, 32);
	_txtAvailable = new Text(110, 9, 16, 24);
	_txtUsed = new Text(110, 9, 130, 24);
	_txtCrew = new Text(71, 9, 244, 24);
	_lstEquipment = new TextList(288, 128, 8, 40);
	_cbxFilterBy = new ComboBox(this, 140, 16, 16, 176, true);
	if (Options::r1doStyle_craftEquipmentState)
	{
		_txtCraftSpaceUSage = new Text(76, 9, 84, 24);
		_txtItemLimitAmount = new Text(76, 9, 160, 24);
		_txtItemLimitSize = new Text(76, 9, 236, 24);
		// {11,9} is maximum size for left/right buttons before they become ugly.
		_arrowEachItemLeft = new ArrowButton(ARROW_SMALL_LEFT, 11, 9, 205, 33);
		_arrowEachItemRight = new ArrowButton(ARROW_SMALL_RIGHT, 11, 9, 217, 33);
	}

	// Set palette
	setInterface("craftEquipment");

	_ammoColor = _game->getMod()->getInterface("craftEquipment")->getElement("ammoColor")->color;

	add(_window, "window", "craftEquipment");
	add(_btnQuickSearch, "button", "craftEquipment");
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
	add(_cbxFilterBy, "button", "craftEquipment");
	if (Options::r1doStyle_craftEquipmentState)
	{
		add(_txtCraftSpaceUSage, "text", "craftEquipment");
		add(_txtItemLimitSize, "text", "craftEquipment");
		add(_txtItemLimitAmount, "text", "craftEquipment");
		add(_arrowEachItemLeft, "button", "craftEquipment");
		add(_arrowEachItemRight, "button", "craftEquipment");

		// Increase visual spacing between 'subtitle' area and spreadsheet header.
		_txtItem->setY(_txtItem->getY() + 2);
		_txtStores->setY(_txtStores->getY() + 2);
		// Prefer a 1 pixel spacing between spreadsheet header and list (visual pleasing).
		_lstEquipment->setY(_lstEquipment->getY() + 3);

		// Only recognize (and show) item limits if bigger than 0 and non-default.
		// Defaults from: `RuleCraft::RuleCraft()`
		_showSpaceLimit = c->getMaxUnitsClamped() > 0;
		_showItemLimit = c->getMaxItemsClamped() > 0 && c->getMaxItemsClamped() < 999999;
		_showItemSizeLimit = c->getMaxStorageSpaceClamped() > 0.0 && c->getMaxStorageSpaceClamped() < 99999.0;

		// Move Soldiers to left side (static part) in order to create space.
		_txtCrew->setX(_txtAvailable->getX() - 8);
		_txtCrew->setWidth(76);

		_txtAvailable->setVisible(false);
		_txtUsed->setVisible(false);
	}

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "craftEquipment");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftEquipmentState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnClearClick, Options::keyRemoveEquipmentFromCraft);
	_btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnLoadClick, Options::keyCraftLoadoutLoad);
	_btnOk->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnSaveClick, Options::keyCraftLoadoutSave);

	_btnClear->setText(tr("STR_UNLOAD_CRAFT"));
	_btnClear->onMouseClick((ActionHandler)&CraftEquipmentState::btnClearClick);
	_btnClear->setVisible(_isNewBattle);

	_btnInventory->setText(tr("STR_INVENTORY"));
	_btnInventory->onMouseClick((ActionHandler)&CraftEquipmentState::btnInventoryClick);
	_btnInventory->setVisible(craftHasACrew && !_isNewBattle);
	_btnInventory->onKeyboardPress((ActionHandler)&CraftEquipmentState::btnInventoryClick, Options::keyBattleInventory);

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_EQUIPMENT_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

	_txtItem->setText(tr("STR_ITEM"));

	_txtStores->setText(tr("STR_STORES"));

	std::ostringstream ss3;
	ss3 << tr("STR_SOLDIERS_UC") << ">" << Unicode::TOK_COLOR_FLIP << c->getNumTotalSoldiers();
	_txtCrew->setText(ss3.str());

	// populate sort options
	_categoryStrings.push_back("STR_ALL");
	_categoryStrings.push_back("STR_EQUIPPED");
	bool hasUnassigned = false;
	for (auto& itemType : _game->getMod()->getItemsList())
	{
		RuleItem *rule = _game->getMod()->getItem(itemType);
		Unit* isVehicle = rule->getVehicleUnit();
		int cQty = isVehicle ? c->getVehicleCount(itemType) : c->getItems()->getItem(rule);

		if ((isVehicle || rule->isInventoryItem()) && rule->canBeEquippedToCraftInventory() &&
			_game->getSavedGame()->isResearched(rule->getRequirements()) &&
			(_base->getStorageItems()->getItem(rule) > 0 || cQty > 0))
		{
			if (rule->getCategories().empty())
			{
				hasUnassigned = true;
			}
			else
			{
				for (auto& itemCategoryName : rule->getCategories())
				{
					_usedCategoryStrings[itemCategoryName] = true;
				}
			}
		}
	}
	auto& itemCategories = _game->getMod()->getItemCategoriesList();
	for (auto& categoryName : itemCategories)
	{
		if (_usedCategoryStrings[categoryName])
		{
			if (!_game->getMod()->getItemCategory(categoryName)->isHidden())
			{
				_categoryStrings.push_back(categoryName);
			}
		}
	}
	if (hasUnassigned && !itemCategories.empty())
	{
		_categoryStrings.push_back("STR_UNASSIGNED");
	}
	_categoryStrings.push_back("STR_NOT_EQUIPPED");

	_cbxFilterBy->setOptions(_categoryStrings, true);
	_cbxFilterBy->setSelected(0);
	_cbxFilterBy->onChange((ActionHandler)&CraftEquipmentState::cbxFilterByChange);

	if (Options::r1doStyle_craftEquipmentState)
	{
		// Spacing based on placeholders, using following rules.
		// * Items on base allow for "9 999" + what is needed for arrow buttons.
		// * Items on craft allow for "9 999"
		// * Items reserved allow for "999" + what is needed for the (`= `/`< `/`> `) indicators.
		// * Have a 3px 'spacing' between columns (based on numbers above).
		// This still leaves plenty of pixels left for 'adjustment'.
		// Rules above however keeps it reasonably close to original locations.
		_lstEquipment->setArrowColumn(203-6, ARROW_HORIZONTAL);
		int arrowColumnReservation = 23; // 23 = _lstEquipment->getArrowsRightEdge() - _lstEquipment->getArrowsLeftEdge()
		_lstEquipment->setColumns(4, 156, 26 + arrowColumnReservation + 15, 26, 26);
	}
	else
	{
		_lstEquipment->setArrowColumn(203, ARROW_HORIZONTAL);
		_lstEquipment->setColumns(3, 156, 83, 41);

		// Possible improvement for `oxceAlternateCraftEquipmentManagement`.
		// In order to allow for same limits as block above.
		//_lstEquipment->setArrowColumn(203-3, ARROW_HORIZONTAL);
		//_lstEquipment->setColumns(3, 156, 26 + 23 + 25, 50);
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

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&CraftEquipmentState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOk->onKeyboardRelease((ActionHandler)&CraftEquipmentState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	_timerLeft = new Timer(250);
	_timerLeft->onTimer((StateHandler)&CraftEquipmentState::moveLeft);
	_timerRight = new Timer(250);
	_timerRight->onTimer((StateHandler)&CraftEquipmentState::moveRight);
	if (Options::r1doStyle_craftEquipmentState)
	{
		_arrowEachItemLeft->onMousePress((ActionHandler)&CraftEquipmentState::arrowEachItemLeftPress);
		_arrowEachItemLeft->onMouseRelease((ActionHandler)&CraftEquipmentState::arrowEachItemLeftRelease);
		_arrowEachItemLeft->onMouseClick((ActionHandler)&CraftEquipmentState::arrowEachItemLeftClick, 0);
		_arrowEachItemRight->onMousePress((ActionHandler)&CraftEquipmentState::arrowEachItemRightPress);
		_arrowEachItemRight->onMouseRelease((ActionHandler)&CraftEquipmentState::arrowEachItemRightRelease);
		_arrowEachItemRight->onMouseClick((ActionHandler)&CraftEquipmentState::arrowEachItemRightClick, 0);

		_timerEachItemLeft = new Timer(250);
		_timerEachItemLeft->onTimer((StateHandler)&CraftEquipmentState::moveLeftEachItem);
		_timerEachItemRight = new Timer(250);
		_timerEachItemRight->onTimer((StateHandler)&CraftEquipmentState::moveRightEachItem);
	}
}

/**
 *
 */
CraftEquipmentState::~CraftEquipmentState()
{
	delete _timerLeft;
	delete _timerRight;

	if (Options::r1doStyle_craftEquipmentState)
	{
		delete _timerEachItemLeft;
		delete _timerEachItemRight;
	}
}

/**
 * Filters the equipment list by the selected criterion
 * @param action Pointer to an action.
 */
void CraftEquipmentState::cbxFilterByChange(Action *action)
{
	initList();
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

	// don't reload after closing error popups
	if (_reload)
	{
		if ((Options::r1doStyle_craftEquipmentState || Options::oxceAlternateCraftEquipmentManagement) && !_isNewBattle)
		{
			// skip when returning from craft equipment template load/save
			if (!_returningFromGlobalTemplates)
			{
				c->calculateTotalSoldierEquipment();
			}
		}
		if (_returningFromInventory && Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
		{
			// now that we're back from the inventory screen, we need to remove all the excess base gear
			for (_sel = 0; _sel != _items.size(); ++_sel)
			{
				int excessQty = c->getItems()->getItem(_items[_sel]) - (c->getExtraItems()->getItem(_items[_sel]) + c->getSoldierItems()->getItem(_items[_sel]));
				moveLeftByValue(excessQty);
			}
		}
		initList();
	}
	_reload = true;
	_returningFromGlobalTemplates = false;
	_returningFromInventory = false;
	_firstInit = false;
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnQuickSearchToggle(Action *action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText("");
		_btnQuickSearch->setVisible(false);
		btnQuickSearchApply(action);
	}
	else
	{
		_btnQuickSearch->setVisible(true);
		_btnQuickSearch->setFocus(true);
	}
}

/**
* Quick search.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnQuickSearchApply(Action *)
{
	initList();
}

/**
 * Shows the equipment in a list filtered by selected criterion.
 */
void CraftEquipmentState::initList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	size_t selIdx = _cbxFilterBy->getSelected();
	if (selIdx == (size_t)-1)
	{
		return;
	}
	const std::string selectedCategory = _categoryStrings[selIdx];
	bool categoryFilterEnabled = (selectedCategory != "STR_ALL");
	bool categoryUnassigned = (selectedCategory == "STR_UNASSIGNED");
	bool categoryEquipped = (selectedCategory == "STR_EQUIPPED");
	bool categoryNotEquipped = (selectedCategory == "STR_NOT_EQUIPPED");
	bool shareAmmoCategories = _game->getMod()->getShareAmmoCategories();

	Craft *c = _base->getCrafts()->at(_craft);

	// reset
	_totalItems = 0;
	_totalItemStorageSize = 0.0;
	_items.clear();
	_lstEquipment->clearList();

	for (auto& itemType : _game->getMod()->getItemsList())
	{
		const RuleItem *rule = _game->getMod()->getItem(itemType);

		Unit* isVehicle = rule->getVehicleUnit();
		int cQty = 0;
		if (isVehicle)
		{
			cQty = c->getVehicleCount(itemType);
		}
		else
		{
			cQty = c->getItems()->getItem(rule);
			_totalItems += cQty;
			_totalItemStorageSize += cQty * rule->getSize();
		}

		int bQty = _base->getStorageItems()->getItem(rule);
		int reserved = 0;
		if ((Options::r1doStyle_craftEquipmentState || Options::oxceAlternateCraftEquipmentManagement) && !_isNewBattle)
		{
			reserved = c->getSoldierItems()->getItem(rule);
		}
		if ((isVehicle || rule->isInventoryItem()) && rule->canBeEquippedToCraftInventory() &&
			(bQty > 0 || cQty > 0 || reserved > 0))
		{
			// check research requirements
			if (!_game->getSavedGame()->isResearched(rule->getRequirements()))
			{
				continue;
			}

			// filter by category
			if (categoryFilterEnabled)
			{
				if (categoryUnassigned)
				{
					if (!rule->getCategories().empty())
					{
						continue;
					}
				}
				else if (categoryEquipped)
				{
					if (!(cQty > 0))
					{
						continue;
					}
				}
				else if (categoryNotEquipped)
				{
					if (cQty > 0)
					{
						continue;
					}
				}
				else
				{
					bool isOK = rule->belongsToCategory(selectedCategory);
					if (shareAmmoCategories && !isOK && rule->getBattleType() == BT_FIREARM)
					{
						for (auto* ammoRule : *rule->getPrimaryCompatibleAmmo())
						{
							if (_base->getStorageItems()->getItem(ammoRule) > 0 || c->getItems()->getItem(ammoRule) > 0)
							{
								if (ammoRule->isInventoryItem() && ammoRule->canBeEquippedToCraftInventory() && _game->getSavedGame()->isResearched(ammoRule->getRequirements()))
								{
									isOK = ammoRule->belongsToCategory(selectedCategory);
									if (isOK) break;
								}
							}
						}
					}
					if (!isOK) continue;
				}
			}

			// quick search
			if (!searchString.empty())
			{
				std::string projectName = tr(itemType);
				Unicode::upperCase(projectName);
				if (projectName.find(searchString) == std::string::npos)
				{
					continue;
				}
			}

			_items.push_back(itemType);
			if (Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
			{
				// doing this once (on opening the screen) is enough
				// and just to make sure, we must skip this when returning from craft equipment template load/save
				if (_firstInit && !isVehicle && cQty < reserved && !_returningFromGlobalTemplates)
				{
					// try to automatically add more items (if possible)
					int itemsToAdd = std::min(bQty, reserved - cQty);
					if (itemsToAdd > 0)
					{
						_base->getStorageItems()->removeItem(rule, itemsToAdd);
						bQty -= itemsToAdd;
						c->getItems()->addItem(rule, itemsToAdd);
						cQty += itemsToAdd;
						_totalItems += itemsToAdd;
						_totalItemStorageSize += itemsToAdd * rule->getSize();
					}
				}
			}

			std::string s = tr(itemType);
			if (rule->getBattleType() == BT_AMMO)
			{
				s.insert(0, "  ");
			}

			if (Options::r1doStyle_craftEquipmentState)
			{
				_lstEquipment->addRow(4, s.c_str(), "", "", "");
			}
			else
			{
				_lstEquipment->addRow(3, s.c_str(), "", "");
			}

			// Apply amounts and correct row color.
			_sel = _lstEquipment->getLastRowIndex();
			updateQuantity();
		}
	}
	_sel = 0; // During the loop it was reset to end of list, time to undo.
	updateSubtitleArea();

	if (Options::r1doStyle_craftEquipmentState)
	{
		if (_totalItems > c->getMaxItemsClamped())
		{
			std::string msg = tr("STR_NO_MORE_EQUIPMENT_ALLOWED", c->getMaxItemsClamped());
			_errorQueue.insert(msg);
		}
		if (_totalItemStorageSize > c->getMaxStorageSpaceClamped())
		{
			std::string msg = tr("STR_NO_MORE_EQUIPMENT_ALLOWED_BY_SIZE").arg(c->getMaxStorageSpaceClamped());
			_errorQueue.insert(msg);
		}
	}
	updateOkButtonText();

	_lstEquipment->draw();
	if (_lstScroll > 0)
	{
		_lstEquipment->scrollTo(_lstScroll);
		_lstScroll = 0;
	}
}

/**
 * Runs the arrow timers.
 */
void CraftEquipmentState::think()
{
	State::think();

	_timerLeft->think(this, 0);
	_timerRight->think(this, 0);

	if (Options::r1doStyle_craftEquipmentState)
	{
		_timerEachItemLeft->think(this, 0);
		_timerEachItemRight->think(this, 0);
	}
}


/**
 * Handler for clicking the OK button.
 *
 * Returns to the previous screen or shows error messages.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::btnOkClick(Action *)
{
	if (isScreenExitAllowed())
	{
		_game->popState();
		return;
	}

	auto* errorInterface = _game->getMod()->getInterface("craftEquipment");
	int colorUI = errorInterface->getElement("errorMessage")->color;
	int colorBackground = errorInterface->getElement("errorPalette")->color;
	for (const auto& message : _errorQueue)
	{
		_game->pushState(new ErrorMessageState(message, _palette, colorUI, "BACK04.SCR", colorBackground));
		_reload = false;
	}
}

/**
 * Change appearance of OK button based on error queue.
 */
void CraftEquipmentState::updateOkButtonText()
{
	if (_errorQueue.empty())
	{
		_btnOk->setText(tr("STR_OK"));
	}
	else
	{
		_btnOk->setText(tr("STR_OK_BUTTON_WARNING"));
	}
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
	else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		_lstScroll = _lstEquipment->getScroll();
		RuleItem *rule = _game->getMod()->getItem(_items[_sel]);
		std::string articleId = rule->getUfopediaType();
		Ufopaedia::openArticle(_game, articleId);
	}
}

/**
 * Handler for pressing the Move Left arrow button (includes mouse wheel).
 *
 * Starts moving each visible by filter item to the base.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::arrowEachItemLeftPress(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerEachItemLeft->isRunning())
	{
		_timerEachItemLeft->start();
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerEachItemRight->stop();
		_timerEachItemLeft->stop();
		moveRightByValueEachItem(Options::changeValueByMouseWheel);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerEachItemRight->stop();
		_timerEachItemLeft->stop();
		moveLeftByValueEachItem(Options::changeValueByMouseWheel);
	}
}

/**
 * Handler for releasing the Move Left arrow button.
 *
 * Stops moving each visible by filter item to the base.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::arrowEachItemLeftRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerEachItemLeft->stop();
	}
}

/**
 * Handler for clicking the Move Left arrow button.
 *
 * Moves each visible by filter item to the base on right-click.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::arrowEachItemLeftClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		moveLeftByValueEachItem(INT_MAX);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveLeftByValueEachItem(1);
		_timerEachItemRight->setInterval(250);
		_timerEachItemLeft->setInterval(250);
	}
}

/**
 * Handler for pressing the Move Right arrow button (includes mouse wheel).
 *
 * Starts moving each visible by filter item to the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::arrowEachItemRightPress(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerEachItemRight->isRunning())
	{
		_timerEachItemRight->start();
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerEachItemRight->stop();
		_timerEachItemLeft->stop();
		moveRightByValueEachItem(Options::changeValueByMouseWheel);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerEachItemRight->stop();
		_timerEachItemLeft->stop();
		moveLeftByValueEachItem(Options::changeValueByMouseWheel);
	}
}

/**
 * Handler for releasing the Move Right arrow button.
 *
 * Stops moving each visible by filter item to the craft.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::arrowEachItemRightRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerEachItemRight->stop();
	}
}

/**
 * Handler for clicking the Move Right arrow button.
 *
 * Moves each visible by filter item to the craft on right-click.
 * @param action Pointer to an action.
 */
void CraftEquipmentState::arrowEachItemRightClick(Action *action)
{

	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		moveRightByValueEachItem(INT_MAX);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		moveRightByValueEachItem(1);
		_timerEachItemRight->setInterval(250);
		_timerEachItemLeft->setInterval(250);
	}
}

/**
 * Updates the displayed quantities of the
 * selected item on the list.
 */
void CraftEquipmentState::updateQuantity()
{
	Craft *c = _base->getCrafts()->at(_craft);
	RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int cQty = 0;
	if (item->getVehicleUnit())
	{
		cQty = c->getVehicleCount(_items[_sel]);
	}
	else
	{
		cQty = c->getItems()->getItem(item);
	}

	int reserved = 0;
	if ((Options::r1doStyle_craftEquipmentState || Options::oxceAlternateCraftEquipmentManagement) && !_isNewBattle)
	{
		reserved = c->getSoldierItems()->getItem(item);
	}

	std::string onBaseString, onCraftString, reservedString;
	if (_isNewBattle)
	{
		onBaseString = "-";
	}
	else if (Options::r1doStyle_craftEquipmentState)
	{
		onBaseString = Unicode::formatNumber(_base->getStorageItems()->getItem(item));
	}
	else
	{
		onBaseString = std::to_string(_base->getStorageItems()->getItem(item));
	}

	// Temporal overrides for column alignment
	//onBaseString = Unicode::formatNumber(9'999);
	//cQty = 9'999;
	//reserved = 999;
	if (Options::r1doStyle_craftEquipmentState)
	{
		if (reserved == 0)
		{
			reservedString = "";
		}
		else if (cQty > reserved)
		{
			reservedString = tr("STR_IS_BIGGER_THAN_CLAIMED_NUMBER").arg(Unicode::formatNumber(reserved));
		}
		else if (cQty < reserved)
		{
			reservedString = tr("STR_IS_SMALLER_THAN_CLAIMED_NUMBER").arg(Unicode::formatNumber(reserved));
		}
		else
		{
			reservedString = tr("STR_IS_EQUAL_TO_CLAIMED_NUMBER").arg(Unicode::formatNumber(reserved));
		}

		onCraftString = Unicode::formatNumber(cQty);
	}
	else if (Options::oxceAlternateCraftEquipmentManagement)
	{
		std::ostringstream ss2;
		if (cQty > reserved)
		{
			ss2 << reserved << "/+" << cQty - reserved;
		}
		else if (cQty < reserved)
		{
			ss2 << cQty << "/" << cQty - reserved;
		}
		else
		{
			ss2 << cQty;
		}
		onCraftString = ss2.str();
	}
	else
	{
		onCraftString = std::to_string(cQty);
	}

	Uint8 color;
	if (cQty == 0)
	{
		if (item->getBattleType() == BT_AMMO)
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
	_lstEquipment->setCellText(_sel, 1, onBaseString);
	_lstEquipment->setCellText(_sel, 2, onCraftString);
	if (Options::r1doStyle_craftEquipmentState)
	{
		_lstEquipment->setCellText(_sel, 3, reservedString);
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
 * Moves each visible by filter item to the base.
 */
void CraftEquipmentState::moveLeftEachItem()
{
	_timerEachItemLeft->setInterval(50);
	_timerEachItemRight->setInterval(50);
	moveLeftByValueEachItem(1);
}

/**
 * Moves the given number of each visible by filter item to the base.
 * @param change Amount of each item to move.
 */
void CraftEquipmentState::moveLeftByValueEachItem(int change)
{
	// `moveLeftByValue()` depends on `_sel` to identify items.
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		if (_game->getMod()->getItem(_items[_sel])->getVehicleUnit())
			continue;

		moveLeftByValue(change);
	}
	_sel = 0;
}

/**
 * Moves the given number of items (selected) to the base.
 * @param change Item difference.
 */
void CraftEquipmentState::moveLeftByValue(int change)
{
	Craft *c = _base->getCrafts()->at(_craft);
	const RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int cQty = 0;
	if (item->getVehicleUnit()) cQty = c->getVehicleCount(_items[_sel]);
	else cQty = c->getItems()->getItem(item);
	if (change <= 0 || cQty <= 0) return;
	if (Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
	{
		int reserved = c->getSoldierItems()->getItem(item);
		if (cQty - reserved > 0)
		{
			change = std::min(cQty - reserved, change);
		}
		else
		{
			//change = 0;
			return;
		}
	}
	else
	{
		change = std::min(cQty, change);
	}
	// Convert vehicle to item
	if (item->getVehicleUnit())
	{
		if (item->getVehicleClipAmmo())
		{
			// Calculate how much ammo needs to be added to the base.
			const RuleItem *ammo = item->getVehicleClipAmmo();
			int ammoPerVehicle = item->getVehicleClipsLoaded();

			// Put the vehicles and their ammo back as separate items.
			if (!_isNewBattle)
			{
				_base->getStorageItems()->addItem(item, change);
				_base->getStorageItems()->addItem(ammo, ammoPerVehicle * change);
			}
			// now delete the vehicles from the craft.
			Collections::deleteIf(*c->getVehicles(), change,
				[&](Vehicle* v)
				{
					return v->getRules() == item;
				}
			);
		}
		else
		{
			if (!_isNewBattle)
			{
				_base->getStorageItems()->addItem(item, change);
			}
			Collections::deleteIf(*c->getVehicles(), change,
				[&](Vehicle* v)
				{
					return v->getRules() == item;
				}
			);
		}
	}
	else
	{
		c->getItems()->removeItem(item, change);
		_totalItems -= change;
		_totalItemStorageSize -= change * item->getSize();
		if (!_isNewBattle)
		{
			_base->getStorageItems()->addItem(item, change);
		}

		if(Options::r1doStyle_craftEquipmentState && !_errorQueue.empty())
		{
			if (_totalItems <= c->getMaxItemsClamped())
			{
				std::string msg = tr("STR_NO_MORE_EQUIPMENT_ALLOWED", c->getMaxItemsClamped());
				_errorQueue.erase(msg);
			}
			if (_totalItemStorageSize <= c->getMaxStorageSpaceClamped())
			{
				std::string msg = tr("STR_NO_MORE_EQUIPMENT_ALLOWED_BY_SIZE").arg(c->getMaxStorageSpaceClamped());
				_errorQueue.erase(msg);
			}
		}
	}
	updateQuantity();
	updateSubtitleArea();
	updateOkButtonText();
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
 * Moves each visible by filter item to the craft.
 */
void CraftEquipmentState::moveRightEachItem()
{
	_timerEachItemLeft->setInterval(50);
	_timerEachItemRight->setInterval(50);
	moveRightByValueEachItem(1);
}

/**
 * Moves the given number of each visible by filter item to the craft.
 * @param change Amount of each item to move.
 */
void CraftEquipmentState::moveRightByValueEachItem(int change)
{
	// `moveRightByValue()` depends on `_sel` to identify items.
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		if (_game->getMod()->getItem(_items[_sel])->getVehicleUnit())
			continue;

		moveRightByValue(change, true);
	}
	_sel = 0;
}

/**
 * Moves the given number of items (selected) to the craft.
 * @param change Item difference.
 * @param suppressErrors Suppress error messages?
 */
void CraftEquipmentState::moveRightByValue(int change, bool suppressErrors)
{
	Craft *c = _base->getCrafts()->at(_craft);
	const RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
	int bqty = _base->getStorageItems()->getItem(item);
	if (_isNewBattle)
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
	if (item->getVehicleUnit())
	{
		int size = item->getVehicleUnit()->getArmor()->getTotalSize();
		// Check if there's enough room
		int room = c->validateAddingVehicles(size);
		if (room > 0)
		{
			change = std::min(room, change);
			if (item->getVehicleClipAmmo())
			{
				// And now let's see if we can add the total number of vehicles.
				const RuleItem *ammo = item->getVehicleClipAmmo();
				int ammoPerVehicle = item->getVehicleClipsLoaded();

				int baseQty = _base->getStorageItems()->getItem(ammo) / ammoPerVehicle;
				if (_isNewBattle)
					baseQty = change;
				int canBeAdded = std::min(change, baseQty);
				if (canBeAdded > 0)
				{
					for (int i = 0; i < canBeAdded; ++i)
					{
						if (!_isNewBattle)
						{
							_base->getStorageItems()->removeItem(ammo, ammoPerVehicle);
							_base->getStorageItems()->removeItem(item);
						}
						c->getVehicles()->push_back(new Vehicle(item, item->getVehicleClipSize(), size));
						c->resetCustomDeployment(); // adding a vehicle into a craft invalidates a custom craft deployment
					}
				}
				else
				{
					if (!suppressErrors)
					{
						// So we haven't managed to increase the count of vehicles because of the ammo
						_timerRight->stop();
						LocalizedText msg(tr("STR_NOT_ENOUGH_AMMO_TO_ARM_HWP").arg(ammoPerVehicle).arg(tr(ammo->getType())));
						_game->pushState(new ErrorMessageState(msg, _palette, _game->getMod()->getInterface("craftEquipment")->getElement("errorMessage")->color, "BACK04.SCR", _game->getMod()->getInterface("craftEquipment")->getElement("errorPalette")->color));
						_reload = false;
					}
					// Not using 'Options::r1doStyle_craftEquipmentState' error queue for this message.
					// Vehicles need special care and are not allowed to overflow (unlike normal items).
				}
			}
			else
				for (int i = 0; i < change; ++i)
				{
					c->getVehicles()->push_back(new Vehicle(item, item->getVehicleClipSize(), size));
					c->resetCustomDeployment(); // adding a vehicle into a craft invalidates a custom craft deployment
					if (!_isNewBattle)
					{
						_base->getStorageItems()->removeItem(item);
					}
				}
		}
	}
	else
	{
		if (_totalItems + change > c->getMaxItemsClamped())
		{
			std::string msg(tr("STR_NO_MORE_EQUIPMENT_ALLOWED", c->getMaxItemsClamped()));
			if (!suppressErrors)
			{
				_timerRight->stop();
				_game->pushState(new ErrorMessageState(msg, _palette, _game->getMod()->getInterface("craftEquipment")->getElement("errorMessage")->color, "BACK04.SCR", _game->getMod()->getInterface("craftEquipment")->getElement("errorPalette")->color));
				_reload = false;
			}

			if (Options::r1doStyle_craftEquipmentState)
			{
				_errorQueue.insert(msg);
			}
			else
			{
				change = c->getMaxItemsClamped() - _totalItems;
			}
		}
		if (_totalItemStorageSize + (change * item->getSize()) > c->getMaxStorageSpaceClamped() + 0.05)
		{
			std::string msg(tr("STR_NO_MORE_EQUIPMENT_ALLOWED_BY_SIZE").arg(c->getMaxStorageSpaceClamped()));
			if (!suppressErrors)
			{
				_timerRight->stop();
				_game->pushState(new ErrorMessageState(msg, _palette, _game->getMod()->getInterface("craftEquipment")->getElement("errorMessage")->color, "BACK04.SCR", _game->getMod()->getInterface("craftEquipment")->getElement("errorPalette")->color));
				_reload = false;
			}
			if (Options::r1doStyle_craftEquipmentState)
			{
				_errorQueue.insert(msg);
			}
			else if (item->getSize() > 0.0)
			{
				change = (int)floor((c->getMaxStorageSpaceClamped() + 0.05 - _totalItemStorageSize) / item->getSize());
				// if the player is already over the maximum (e.g. after a mod update), don't go into some ridiculous minus values
				change = std::max(0, change);
			}
		}
		c->getItems()->addItem(item, change);
		_totalItems += change;
		_totalItemStorageSize += change * item->getSize();
		if (!_isNewBattle)
		{
			_base->getStorageItems()->removeItem(item, change);
		}
	}
	updateQuantity();
	updateSubtitleArea();
	updateOkButtonText();
}

/**
 * Updates dynamic texts between screen title and spreadsheet.
 *
 * When craft has limits, and `r1doStyle_craftEquipmentState` is active, it will:
 * + Merge "Space Available" and "Space Used" into a single text.
 * + Add 2 texts for running numbers of item size and item amount.
 */
void CraftEquipmentState::updateSubtitleArea()
{
	Craft *c = _base->getCrafts()->at(_craft);

	if (Options::r1doStyle_craftEquipmentState)
	{
		std::ostringstream ssSpaceUsage, ssItemAmount, ssItemSize;
		ssSpaceUsage << tr("STR_SPACE_UC") << ">" << Unicode::TOK_COLOR_FLIP << c->getSpaceUsed();
		if (_showSpaceLimit)
		{
			ssSpaceUsage << ":" << c->getMaxUnitsClamped();
		}
		ssItemAmount << tr("STR_ITEMS_UC") << ">" << Unicode::TOK_COLOR_FLIP << _totalItems;
		if (_showItemLimit)
		{
			ssItemAmount << ":" << c->getMaxItemsClamped();
		}
		// Dealing with floating point
		// - std::setprecision(1) : Save space, 1 digit is enough to inform player.
		// - std::fixed           : Prevent scientific notation (1.4524e-16, 2e+2).
		// - std::max()           : Prevent negative numbers due to round-off errors.
		ssItemSize << std::fixed << std::setprecision(1);
		ssItemSize << tr("STR_SIZE_UC") << ">" << Unicode::TOK_COLOR_FLIP;
		ssItemSize << std::max(_totalItemStorageSize, 0.0);
		if (_showItemSizeLimit)
		{
			ssItemSize << ":" << c->getMaxStorageSpaceClamped();
		}

		_txtCraftSpaceUSage->setText(ssSpaceUsage.str().c_str());
		_txtItemLimitAmount->setText(ssItemAmount.str().c_str());
		_txtItemLimitSize->setText(ssItemSize.str().c_str());
	}
	else
	{
		_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
		_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
	}
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

	// in New Battle, clear also stuff that is not displayed on the GUI (for whatever reason)
	if (_isNewBattle)
	{
		Craft* c = _base->getCrafts()->at(_craft);
		c->getItems()->clear();
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
	if (craft->getNumTotalSoldiers() > 0)
	{
		_lstScroll = _lstEquipment->getScroll();

		if (Options::oxceAlternateCraftEquipmentManagement && !_isNewBattle)
		{
			// This is a bit tricky... here's what we're doing:
			// * Remember the extra craft items (i.e. items that are on the craft, but not equipped by soldiers)
			// * Move all equipment from the base into the craft.
			// * Run the inventory screen.
			// * Remove excess items from the craft when CraftEquipmentState::init() is called after leaving the inventory screen.
			// (After this, the craft should have all the updated soldier equipment, and the same extra items as before.)

			// Note: the current implementation assumes no limit to the number or size of items a craft can hold.
			//       If the craft has limited space, then we just won't have all the base items available on the inventory screen.

			auto& extras = *craft->getExtraItems();
			extras.clear();
			for (_sel = 0; _sel != _items.size(); ++_sel)
			{
				RuleItem* rule = _game->getMod()->getItem(_items[_sel], true);
				if (craft->getItems()->getItem(rule) > 0)
				{
					extras.addItem(rule, craft->getItems()->getItem(rule) - craft->getSoldierItems()->getItem(rule));
				}
				if (!rule->getVehicleUnit() && rule->canBeEquippedBeforeBaseDefense())
				{
					moveRightByValue(INT_MAX, true);
				}
			}
		}

		SavedBattleGame *bgame = new SavedBattleGame(_game->getMod(), _game->getLanguage());
		_game->getSavedGame()->setBattleGame(bgame);

		if (_game->isCtrlPressed() && _game->isAltPressed())
		{
			_game->getSavedGame()->setDisableSoldierEquipment(true);
		}
		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
		bgen.runInventory(craft);

		_game->getScreen()->clear();
		_game->pushState(new InventoryState(false, 0, _base));
		_returningFromInventory = true;
	}
}

void CraftEquipmentState::saveGlobalLoadout(int index)
{
	// clear the template
	ItemContainer *tmpl = _game->getSavedGame()->getGlobalCraftLoadout(index);
	tmpl->clear();

	Craft *c = _base->getCrafts()->at(_craft);
	// save only what is visible on the screen (can be DIFFERENT than what's really in the craft for various reasons)
	for (const auto& itemType : _items)
	{
		const RuleItem *item = _game->getMod()->getItem(itemType, true);
		int cQty = 0;
		if (item->getVehicleUnit())
		{
			cQty = c->getVehicleCount(itemType);
		}
		else
		{
			cQty = c->getItems()->getItem(item);
		}
		if (cQty > 0)
		{
			tmpl->addItem(item, cQty);
		}
	}
}

void CraftEquipmentState::loadGlobalLoadout(int index, bool onlyAddItems)
{
	// temporarily turn off alternate craft equipment management to allow removing all items from the craft
	bool backup = Options::oxceAlternateCraftEquipmentManagement;
	Options::oxceAlternateCraftEquipmentManagement = false;

	// reset filters and reload the full equipment list
	_btnQuickSearch->setText("");
	_cbxFilterBy->setSelected(0);
	initList();

	Craft* c = _base->getCrafts()->at(_craft);

	ItemContainer craftItemsBackup;
	std::vector<Vehicle*> craftVehiclesBackup;
	if (onlyAddItems)
	{
		// remember for later, make copies
		craftItemsBackup = *c->getItems();
		craftVehiclesBackup = *c->getVehicles();
	}
	else
	{
		// first move everything visible back to base
		for (_sel = 0; _sel != _items.size(); ++_sel)
		{
			moveLeftByValue(INT_MAX);
		}
	}

	// now start applying the template (consider ONLY items visible on the GUI)
	ItemContainer *tmpl = _game->getSavedGame()->getGlobalCraftLoadout(index);
	for (_sel = 0; _sel != _items.size(); ++_sel)
	{
		RuleItem *item = _game->getMod()->getItem(_items[_sel], true);
		int tQty = tmpl->getItem(item);
		moveRightByValue(tQty, true);
	}

	// lastly check and report what's missing
	std::string craftName = c->getName(_game->getLanguage());
	std::vector<ReequipStat> _missingItems;
	for (const auto& templateItem : *tmpl->getContents())
	{
		const RuleItem *item = templateItem.first;
		if (item)
		{
			int tQty = templateItem.second;
			int cQty = 0;
			if (item->getVehicleUnit())
			{
				// Note: we will also report HWPs as missing:
				// - if there is not enough ammo to arm them
				// - if there is not enough cargo space in the craft
				cQty = c->getVehicleCount(item->getType());
				if (onlyAddItems)
				{
					int total = 0;
					for (const auto* vehicle : craftVehiclesBackup)
					{
						if (vehicle->getRules() == item)
						{
							total++;
						}
					}
					cQty -= total; // i.e. only count newly added vehicles
				}
			}
			else
			{
				cQty = c->getItems()->getItem(item);
				if (onlyAddItems)
				{
					cQty -= craftItemsBackup.getItem(item); // i.e. only count newly added items
				}
			}
			int missing = tQty - cQty;
			if (missing > 0)
			{
				ReequipStat stat = { item->getType(), missing, craftName, item->getListOrder() };
				_missingItems.push_back(stat);
			}
		}
	}

	if (!_missingItems.empty())
	{
		std::sort(_missingItems.begin(), _missingItems.end(), [](const ReequipStat &a, const ReequipStat &b)
			{
				return a.listOrder < b.listOrder;
			}
		);
		_game->pushState(new CannotReequipState(_missingItems, _base));
	}

	// turn back the original setting
	Options::oxceAlternateCraftEquipmentManagement = backup;
}

/**
* Opens the CraftEquipmentLoadState screen.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnLoadClick(Action *)
{
	if (!_isNewBattle)
	{
		_game->pushState(new CraftEquipmentLoadState(this));
		_returningFromGlobalTemplates = true;
	}
}

/**
* Opens the CraftEquipmentSaveState screen.
* @param action Pointer to an action.
*/
void CraftEquipmentState::btnSaveClick(Action *)
{
	if (!_isNewBattle)
	{
		_game->pushState(new CraftEquipmentSaveState(this));
		_returningFromGlobalTemplates = true;
	}
}

/**
 * Determine if we are allowed to exit this screen.
 *
 * To protect against mod changes exit is allowed for those cases where
 * a player cannot reasonably meet the requirements:
 * + No unclaimed items on craft left but still something in error queue.
 *
 * @note Includes a manual override (`CTRL + ALT`) as last resort to
 *       prevent player getting stuck on this screen, this will be logged.
 * @return Whether we are allowed to exit screen.
 */
bool CraftEquipmentState::isScreenExitAllowed()
{
	if (_errorQueue.empty())
		return true;

	if (_game->isCtrlPressed() && _game->isAltPressed())
	{
		// Craft state might be broken due to this exit.
		Log(LOG_WARNING) << "Player forcefully exited craft equipment screen.";
		return true;
	}

	// We have too much items either by size or by number.
	Craft *c = _base->getCrafts()->at(_craft);
	for (const auto& craftItem : *c->getItems()->getContents())
	{
		if (craftItem.second <= 0)
			continue;

		// There are more of this item in cargo bay than strictly necessary.
		if (craftItem.second > c->getSoldierItems()->getItem(craftItem.first))
			return false;
	}
	Log(LOG_WARNING) << "Allowed exit of craft equipment screen, due to broken item limits. Most likely case: Mod recently changed those limits.";
	return true;
}

}
