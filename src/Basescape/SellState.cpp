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
#include "ManufactureDependenciesTreeState.h"
#include <algorithm>
#include <locale>
#include <sstream>
#include <cmath>
#include <iomanip>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/ComboBox.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Vehicle.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Engine/Timer.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Mod/RuleInterface.h"
#include "../Battlescape/DebriefingState.h"
#include "TransferBaseState.h"
#include "TechTreeViewerState.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Sell/Sack screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param origin Game section that originated this state.
 */
SellState::SellState(Base *base, DebriefingState *debriefingState, OptionsOrigin origin) : _base(base), _debriefingState(debriefingState), _sel(0), _total(0), _spaceChange(0), _origin(origin),
	_reset(false), _sellAllButOne(false), _delayedInitDone(false), _previousSort(TransferSortDirection::BY_LIST_ORDER), _currentSort(TransferSortDirection::BY_LIST_ORDER)
{
	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)&SellState::increase);
	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)&SellState::decrease);
}

/**
 * Delayed constructor functionality.
 */
void SellState::delayedInit()
{
	if (_delayedInitDone)
	{
		return;
	}
	_delayedInitDone = true;

	bool overfull = _debriefingState == 0 && Options::storageLimitsEnforced && _base->storesOverfull();
	bool overfullCritical = overfull ? _base->storesOverfullCritical() : false;
	_invertFilter = false;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 13);
	//_btnOk = new TextButton(overfull? 288:148, 16, overfull? 16:8, 176);
	_btnOk = new TextButton(148, 16, 8, 176);
	_btnCancel = new TextButton(148, 16, 164, 176);
	_btnTransfer = new TextButton(148, 16, 164, 176);
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
	add(_btnQuickSearch, "button", "sellMenu");
	add(_btnOk, "button", "sellMenu");
	add(_btnCancel, "button", "sellMenu");
	add(_btnTransfer, "button", "sellMenu");
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
	setWindowBackground(_window, "sellMenu");

	_btnOk->setText(tr("STR_SELL_SACK"));
	_btnOk->onMouseClick((ActionHandler)&SellState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&SellState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&SellState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SellState::btnCancelClick, Options::keyCancel);

	_btnTransfer->setText(tr("STR_GO_TO_TRANSFERS"));
	_btnTransfer->onMouseClick((ActionHandler)&SellState::btnTransferClick);

	_btnCancel->setVisible(!overfull);
	_btnOk->setVisible(!overfull);
	_btnTransfer->setVisible(overfull);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELL_ITEMS_SACK_PERSONNEL"));

	_txtSpaceUsed->setVisible(Options::storageLimitsEnforced);

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtSell->setText(tr("STR_SELL_SACK"));

	_txtValue->setText(tr("STR_VALUE"));

	if (Options::r1doStyle_sellState && Options::r1doReservedAmountBehavior > 0)
	{
		_lstItems->setWidth(290);
		_lstItems->setScrolling(true, 1); // default = 4
		_lstItems->setWordWrap(true);
		_lstItems->setArrowColumn(184, ARROW_VERTICAL);

		// Column width limits:
		// * Items on base allow for "9999"
		// * Reserved amount allow for "999" + what is needed for arrow buttons and indicators.
		// * Amount of sold items allow for "9999"
		// * Costs allow for "$99 999 999"
		// Column pixel separation = 3px.
		//
		// Leads to item descriptions having ~4-5 less characters (hence the word wrap).
		int arrowColumnReservation = 23; // 23 = _lstEquipment->getArrowsRightEdge() - _lstEquipment->getArrowsLeftEdge()
		_lstItems->setColumns(5, 129, 26, 26 + arrowColumnReservation + 4, 26, 54);
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

	_cats.push_back("STR_ALL_ITEMS");

	// Original behavior makes sense: No display of named soldiers assigned to craft or in-transfer.
	// Prevents display clutter. Wounded soldiers are fair game though.
	for (auto* soldier : *_base->getSoldiers())
	{
		if (_debriefingState) break;
		if (soldier->getCraft() == 0)
		{
			TransferRow row = { TRANSFER_SOLDIER, soldier, soldier->getName(true), 0, 1, 0, 0, -4, 0, 0, 0 };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	// Original behavior makes sense: No display of named aircraft currently on a mission or in-transfer.
	// Prevents display clutter (and no need for reserved amounts).
	for (auto* craft : *_base->getCrafts())
	{
		if (_debriefingState) break;
		if (craft->getStatus() != "STR_OUT")
		{
			TransferRow row = { TRANSFER_CRAFT, craft, craft->getName(_game->getLanguage()), craft->getRules()->getSellCost(), 1, 0, 0, -3, 0, 0, craft->getRules()->getSellCost() };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	// Sell screen kinda acts like a storestate-lite view.
	// Calculate/show even if no scientists are currently available
	if (_debriefingState == 0)
	{
		TransferRow row = { TRANSFER_SCIENTIST, 0, tr("STR_SCIENTIST"), 0, _base->getAvailableScientists(), 0, 0, -2, 0, 0, 0 };
		if (Options::r1doStyle_sellState)
		{
			row.transferSrc = _base->getTotalScientists() - _base->getTotalScientists(true);
		}
		if (Options::r1doStyle_sellState && Options::r1doReservedAmountBehavior > 1) // Don't allow soldiers to claim scientists, that is a different kind of game.
		{
			row.allocatedSrc = _base->getAllocatedScientists();
		}
		// This screen does not support removing scientists from research projects.
		row.protectedSrc = row.allocatedSrc;

		if (row.qtySrc > 0 || row.transferSrc > 0 || row.allocatedSrc > 0)
		{
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	// Sell screen kinda acts like a storestate-lite view.
	// Calculate/show even if no engineers are currently available.
	if (_debriefingState == 0)
	{
		TransferRow row = { TRANSFER_ENGINEER, 0, tr("STR_ENGINEER"), 0, _base->getAvailableEngineers(), 0, 0, -1, 0, 0, 0 };
		if (Options::r1doStyle_sellState)
		{
			row.transferSrc = _base->getTotalEngineers() - _base->getTotalEngineers(true);
		}
		if (Options::r1doStyle_sellState && Options::r1doReservedAmountBehavior > 1) // Don't allow soldiers to claim engineers.
		{
			row.allocatedSrc = _base->getAllocatedEngineers();
		}
		// This screen does not support removing engineers from projects.
		row.protectedSrc = row.allocatedSrc;

		if (row.qtySrc > 0 || row.transferSrc > 0 || row.allocatedSrc > 0)
		{
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	for (auto& itemType : _game->getMod()->getItemsList())
	{
		const RuleItem *rule = _game->getMod()->getItem(itemType, true);
		if (!rule)
			continue;
		if (rule->isAlien() == true && Options::canSellLiveAliens == false)
			continue;

		int qty = 0, transferSrc = 0;
		// Debriefing state logic is now an override.
		// In order to facilitate new list column draw options.
		qty = _base->getStorageItems()->getItem(rule);
		if (Options::storageLimitsEnforced && (_origin == OPT_BATTLESCAPE || overfullCritical))
		{
			for (auto* transfer : *_base->getTransfers())
			{
				if (transfer->getItems() == rule)
				{
					transferSrc += transfer->getQuantity();
				}
				else if (transfer->getCraft())
				{
					transferSrc += overfullCritical ? transfer->getCraft()->getTotalItemCount(rule) : transfer->getCraft()->getItems()->getItem(rule);
				}
			}
			for (auto* craft : *_base->getCrafts())
			{
				qty +=  overfullCritical ? craft->getTotalItemCount(rule) : craft->getItems()->getItem(rule);
			}
		}
		else if (Options::r1doStyle_sellState)
		{
			transferSrc += _base->getItemCountTransfers(rule);
		}
		// Display only variables: `<allocated,protected>`.
		std::pair<int, int> displayOnlySrc = std::make_pair(0,0);
		if (Options::r1doReservedAmountBehavior > 0)
		{
			displayOnlySrc = getAllocatedAndProtectedCountsSrc(rule, overfullCritical);
		}

		// Loot screen overrides
		if (_debriefingState != 0)
		{
			displayOnlySrc.second += transferSrc;
			transferSrc = 0;

			int loot = _debriefingState->getRecoveredItemCount(rule);
			displayOnlySrc.second += qty - loot;
			qty = loot;
		}

		// Recognize there might still be a claim on an item while stock is depleted.
		if (qty > 0 || transferSrc > 0 || (displayOnlySrc.first > 0 && _debriefingState == 0))
		{
			TransferRow row = { TRANSFER_ITEM, rule, tr(itemType), rule->getSellCost(), qty, 0, 0, rule->getListOrder(), rule->getSize(), 0, 0 };
			row.allocatedSrc = displayOnlySrc.first;
			row.protectedSrc = displayOnlySrc.second;
			row.transferSrc = transferSrc;
			row.totalSize = (qty + transferSrc) * rule->getSize();
			row.totalCost = (qty + transferSrc) * rule->getSellCost();

			if ((_debriefingState != 0) && (_game->getSavedGame()->getAutosell(rule)))
			{
				row.amount = qty + transferSrc;
				_total += (qty + transferSrc) * row.cost;
				_spaceChange -= (qty + transferSrc) * rule->getSize();
			}
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	_vanillaCategories = _cats.size();
	if (_game->getMod()->getDisplayCustomCategories() > 0)
	{
		bool hasUnassigned = false;

		// first find all relevant item categories
		std::vector<std::string> tempCats;
		for (const auto& transferRow : _items)
		{
			if (transferRow.type == TRANSFER_ITEM)
			{
				RuleItem *rule = (RuleItem*)(transferRow.rule);
				if (rule->getCategories().empty())
				{
					hasUnassigned = true;
				}
				for (auto& itemCategoryName : rule->getCategories())
				{
					if (std::find(tempCats.begin(), tempCats.end(), itemCategoryName) == tempCats.end())
					{
						tempCats.push_back(itemCategoryName);
					}
				}
			}
		}
		// then use them nicely in order
		if (_game->getMod()->getDisplayCustomCategories() == 1)
		{
			_cats.clear();
			_cats.push_back("STR_ALL_ITEMS");
			_vanillaCategories = _cats.size();
		}
		for (auto& categoryName : _game->getMod()->getItemCategoriesList())
		{
			if (std::find(tempCats.begin(), tempCats.end(), categoryName) != tempCats.end())
			{
				_cats.push_back(categoryName);
			}
		}
		if (hasUnassigned)
		{
			_cats.push_back("STR_UNASSIGNED");
		}
	}

	_cbxCategory->setOptions(_cats, true);
	_cbxCategory->onChange((ActionHandler)&SellState::cbxCategoryChange);
	_cbxCategory->onKeyboardPress((ActionHandler)&SellState::btnSellAllClick, Options::keySellAll);
	_cbxCategory->onKeyboardPress((ActionHandler)&SellState::btnSellAllButOneClick, Options::keySellAllButOne);

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&SellState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	// OK button is not always visible, so bind it here
	_cbxCategory->onKeyboardRelease((ActionHandler)&SellState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	updateList();
	updateSubtitleLine();
	updateOkButton();
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
* Resets stuff when coming back from other screens.
*/
void SellState::init()
{
	delayedInit();

	State::init();

	if (_reset)
	{
		_game->popState();
		_game->pushState(new SellState(_base, _debriefingState, _origin));
	}
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
			if (rule->getVehicleUnit())
				return "STR_PERSONNEL"; // OXCE: critters fighting for us
			if (rule->isAlien())
				return "STR_PRISONERS"; // OXCE: live aliens
			return "STR_ALIENS";
		}
		if (rule->getBattleType() == BT_NONE)
		{
			if (_game->getMod()->isCraftWeaponStorageItem(rule))
				return "STR_CRAFT_ARMAMENT";
			if (_game->getMod()->isArmorStorageItem(rule))
				return "STR_ARMORS"; // OXCE: armors
			return "STR_COMPONENTS";
		}
		return "STR_EQUIPMENT";
	}
	return "STR_ALL_ITEMS";
}

/**
 * Determines if a row item belongs to a given category.
 * @param sel Selected row.
 * @param cat Category.
 * @returns True if row item belongs to given category, otherwise False.
 */
bool SellState::belongsToCategory(int sel, const std::string &cat) const
{
	switch (_items[sel].type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
	case TRANSFER_CRAFT:
		return false;
	case TRANSFER_ITEM:
		RuleItem *rule = (RuleItem*)_items[sel].rule;
		return rule->belongsToCategory(cat);
	}
	return false;
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void SellState::btnQuickSearchToggle(Action *action)
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
void SellState::btnQuickSearchApply(Action *)
{
	updateList();
}

/**
 * Filters the current list of items.
 */
void SellState::updateList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	_lstItems->clearList();
	_rows.clear();

	int sellPriceCoefficient = _game->getSavedGame()->getSellPriceCoefficient();

	size_t selCategory = _cbxCategory->getSelected();
	const std::string selectedCategory = _cats[selCategory];
	bool categoryFilterEnabled = (selectedCategory != "STR_ALL_ITEMS");
	bool categoryUnassigned = (selectedCategory == "STR_UNASSIGNED");

	if (_previousSort != _currentSort)
	{
		switch (_currentSort)
		{
		case TransferSortDirection::BY_TOTAL_COST: std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.totalCost > b.totalCost; }); break;
		case TransferSortDirection::BY_UNIT_COST:  std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.cost > b.cost; }); break;
		case TransferSortDirection::BY_TOTAL_SIZE: std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.totalSize > b.totalSize; }); break;
		case TransferSortDirection::BY_UNIT_SIZE:  std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.size > b.size; }); break;
		default:                                   std::stable_sort(_items.begin(), _items.end(), [](const TransferRow a, const TransferRow b) { return a.listOrder < b.listOrder; }); break;
		}
	}

	for (size_t i = 0; i < _items.size(); ++i)
	{
		// filter
		bool hideItem = false;
		if (selCategory >= _vanillaCategories)
		{
			if (categoryUnassigned && _items[i].type == TRANSFER_ITEM)
			{
				RuleItem* rule = (RuleItem*)_items[i].rule;
				if (!rule->getCategories().empty())
				{
					hideItem = true;
				}
			}
			else if (categoryFilterEnabled && !belongsToCategory(i, selectedCategory))
			{
				hideItem = true;
			}
		}
		else
		{
			if (categoryFilterEnabled && selectedCategory != getCategory(i))
			{
				hideItem = true;
			}
		}
		hideItem ^= _invertFilter;
		if (hideItem && categoryFilterEnabled) continue;

		// quick search
		if (!searchString.empty())
		{
			std::string projectName = _items[i].name;
			Unicode::upperCase(projectName);
			if (projectName.find(searchString) == std::string::npos)
			{
				continue;
			}
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

		int64_t adjustedCost = _items[i].cost;
		adjustedCost = adjustedCost * sellPriceCoefficient / 100;
		// Temporal override for column alignment
		//adjustedCost = 99'999'999;
		if (Options::r1doStyle_sellState && Options::r1doReservedAmountBehavior > 0)
		{
			_lstItems->addRow(5, name.c_str(), "", "", "", Unicode::formatFunding(adjustedCost).c_str());
		}
		else
		{
			_lstItems->addRow(4, name.c_str(), "", "", Unicode::formatFunding(adjustedCost).c_str());
		}
		_rows.push_back(i);

		// Apply amounts and correct row color.
		_sel = _lstItems->getLastRowIndex();
		updateItemStrings();
	}
	_sel = 0; // During the loop it was reset to end of list, time to undo.

	// Inform player inverse filter is in effect.
	if (categoryFilterEnabled && _invertFilter)
	{
		_cbxCategory->setText(tr("STR_INVERSE_FILTER_INDICATOR").arg(tr(selectedCategory)));
	}
}

/**
 * Sells the selected items.
 * @param action Pointer to an action.
 */
void SellState::btnOkClick(Action *)
{
	int64_t adjustedTotal = _total * _game->getSavedGame()->getSellPriceCoefficient() / 100;
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + adjustedTotal);

	auto cleanUpContainer = [&](ItemContainer* container, const RuleItem* rule, int toRemove) -> int
	{
		int curr = container->getItem(rule);
		if (curr >= toRemove)
		{
			container->removeItem(rule, toRemove);
			return 0;
		}
		else
		{
			container->removeItem(rule, INT_MAX);
			return toRemove - curr;
		}
	};

	auto cleanUpCraft = [&](Craft* craft2, const RuleItem* rule, int toRemove) -> int
	{
		struct S
		{
			int ToRemove, ToSave;
			const RuleItem* rule;
		};

		auto tryRemove = [&toRemove, rule](int curr, const RuleItem* i) -> S
		{
			if (i == rule)
			{
				int r = std::min(toRemove, curr);
				toRemove -= r;
				curr -= r;
				return S{ r, curr, i };
			}
			else
			{
				return S{ 0, curr, i };
			}
		};
		auto tryStore = [&](S s)
		{
			if (s.ToSave > 0)
			{
				_base->getStorageItems()->addItem(s.rule, s.ToSave);
			}
		};

		for (auto*& w :* craft2->getWeapons())
		{
			if (w != nullptr)
			{
				auto* wr = w->getRules();

				auto launcher = tryRemove(1, wr->getLauncherItem());
				auto clip = tryRemove(w->getClipsLoaded(), wr->getClipItem());
				if (launcher.ToRemove || clip.ToRemove)
				{
					tryStore(launcher);
					tryStore(clip);

					delete w;
					w = nullptr;
				}
			}
		}

		Collections::deleteIf(
			*craft2->getVehicles(),
			[&](Vehicle* v)
			{
				auto* clipType = v->getRules()->getVehicleClipAmmo();

				auto launcher = tryRemove(1, v->getRules());
				auto clip = tryRemove(v->getRules()->getVehicleClipsLoaded(), clipType);

				if (launcher.ToRemove || clip.ToRemove)
				{
					tryStore(launcher);
					tryStore(clip);

					return true;
				}
				else
				{
					return false;
				}
			}
		);

		return toRemove;
	};

	auto cleanUpItemTransfers = [&](const RuleItem* rule, int toRemove, bool includeArmament = true) -> int
	{
		for (auto transferIt = _base->getTransfers()->begin(); transferIt != _base->getTransfers()->end() && toRemove;)
		{
			auto* transfer = (*transferIt);
			if (transfer->getItems() == rule)
			{
				if (transfer->getQuantity() <= toRemove)
				{
					toRemove -= transfer->getQuantity();
					delete transfer;
					transferIt = _base->getTransfers()->erase(transferIt);
				}
				else
				{
					transfer->setItems(transfer->getItems(), transfer->getQuantity() - toRemove);
					toRemove = 0;
				}
			}
			else
			{
				if (transfer->getCraft())
				{
					toRemove = cleanUpContainer(transfer->getCraft()->getItems(), rule, toRemove);
					if (toRemove > 0 && includeArmament)
					{
						toRemove = cleanUpCraft(transfer->getCraft(), rule, toRemove);
					}
				}
				++transferIt;
			}
		}
		return toRemove;
	};

	auto cleanUpPersonnelTransfers = [&](TransferType personType, int toRemove) -> int
	{
		if (personType != TRANSFER_SCIENTIST || personType != TRANSFER_ENGINEER)
			return 0;

		for (auto transferIt = _base->getTransfers()->begin(); transferIt != _base->getTransfers()->end() && toRemove;)
		{
			auto* transfer = (*transferIt);
			if (transfer->getType() == personType)
			{
				if (transfer->getQuantity() <= toRemove)
				{
					toRemove -= transfer->getQuantity();
					delete transfer;
					transferIt = _base->getTransfers()->erase(transferIt);
				}
				else
				{
					transfer->setItems(transfer->getItems(), transfer->getQuantity() - toRemove);
					toRemove = 0;
				}
			}
			else
			{
				++transferIt;
			}
		}
		return toRemove;
	};

	Soldier* tmpSoldier;
	Craft* tmpCraft;

	for (const auto& transferRow : _items)
	{
		// Note: Possible to reduce level of indentation by changing order.
		// (transferRow.amount <= 0) {content of else block with a continue statement}.
		if (transferRow.amount > 0)
		{
			int toRemove = transferRow.amount;
			switch (transferRow.type)
			{
			case TRANSFER_SOLDIER:
				tmpSoldier = (Soldier*)transferRow.rule;
				for (auto soldierIt = _base->getSoldiers()->begin(); soldierIt != _base->getSoldiers()->end(); ++soldierIt)
				{
					if (*soldierIt == tmpSoldier)
					{
						if (tmpSoldier->getArmor()->getStoreItem())
						{
							_base->getStorageItems()->addItem(tmpSoldier->getArmor()->getStoreItem());
						}
						_base->getSoldiers()->erase(soldierIt);
						break;
					}
				}
				delete tmpSoldier;
				break;
			case TRANSFER_CRAFT:
				tmpCraft = (Craft*)transferRow.rule;
				_base->removeCraft(tmpCraft, true);
				delete tmpCraft;
				break;
			case TRANSFER_SCIENTIST:
				if (Options::r1doStyle_sellState && transferRow.transferSrc > 0)
				{
					// Well ... if the player is that bend on burning cash ...
					toRemove = cleanUpPersonnelTransfers(transferRow.type, toRemove);
				}
				_base->setScientists(_base->getScientists() - toRemove);
				break;
			case TRANSFER_ENGINEER:
				if (Options::r1doStyle_sellState && transferRow.transferSrc > 0)
				{
					// Well ... if the player is that bend on burning cash ...
					toRemove = cleanUpPersonnelTransfers(transferRow.type, toRemove);
				}
				_base->setEngineers(_base->getEngineers() - toRemove);
				break;
			case TRANSFER_ITEM:
				RuleItem *item = (RuleItem*)transferRow.rule;
				{
					// Our rookies are famous for their unrivalled ability
					// to (accidentally?) kill high ranking officers,
					// especially when shots are deemed impossible.
					//
					// Players can be considered *the* highest rank,
					// perhaps only outranked by devs.
					//
					// This explains why I would go to great length as to not
					// take away the toys of our (battle ready) soldiers.
					//
					// Luckily in transfer crafts cannot accommodate any troops,
					// one only needs to protect craft that are on base.
					//
					// Soldiers also tend to not develop feelings towards
					// HWP's and craft armament, making life easier.

					// This scenario thus allows for the sale of inbound items
					// before the normal routine.
					// + It assumes direct items get a 'return to sender'
					//   upon arrival / are rerouted by the postal office.
					// + It assumes items on board of in-transer craft lose
					//   their storage reservation and will be (re)packaged
					//   for shipping upon arrival.
					if (Options::r1doStyle_sellState && transferRow.transferSrc > 0)
					{
						toRemove = cleanUpItemTransfers(item, toRemove, false);
					}

					// remove all of said items from base
					toRemove = cleanUpContainer(_base->getStorageItems(), item, toRemove);

					// if we still need to remove any, remove them from the crafts first, and keep a running tally
					for (auto* craft : *_base->getCrafts())
					{
						if (toRemove <= 0) break; // loop finished
						toRemove = cleanUpContainer(craft->getItems(), item, toRemove);
						if (toRemove > 0)
						{
							toRemove = cleanUpCraft(craft, item, toRemove);
						}
					}

					// if there are STILL any left to remove, take them from the transfers, and if necessary, delete it.
					toRemove = cleanUpItemTransfers(item, toRemove);

					if (toRemove != 0)
					{
						std::string warning = "Tried to sell: " +
							std::to_string(transferRow.amount) +
							" pieces of " + transferRow.name +
							". Could not find the last: " +
							std::to_string(toRemove) + " items.";
						Log(LOG_WARNING) << warning;
					}
				}

				// Note: this only updates a helper map, it doesn't affect real item recovery (that has already happened and all items are already in the base)
				if (_debriefingState != 0)
				{
					// remember the decreased amount for next sell/transfer
					_debriefingState->decreaseRecoveredItemCount(item, transferRow.amount);

					// set autosell status if we sold all of the item
					_game->getSavedGame()->setAutosell(item, (transferRow.qtySrc == transferRow.amount));
				}
				break;
			}
		}
		else
		{
			if (_debriefingState != 0 && transferRow.type == TRANSFER_ITEM)
			{
				// disable autosell since we haven't sold any of the item.
				_game->getSavedGame()->setAutosell((RuleItem*)transferRow.rule, false);
			}
		}
	}
	if (_debriefingState != 0 && _debriefingState->getTotalRecoveredItemCount() <= 0)
	{
		_debriefingState->hideSellTransferButtons();
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
* Opens the Transfer UI and gives the player an option to transfer stuff instead of selling it.
* Returns back to this screen when finished.
* @param action Pointer to an action.
*/
void SellState::btnTransferClick(Action *)
{
	_reset = true;
	_game->pushState(new TransferBaseState(_base, nullptr));
}

/**
* Increase all items to max, i.e. sell everything.
* @param action Pointer to an action.
*/
void SellState::btnSellAllClick(Action *)
{
	bool allItemsSelected = true;
	for (size_t i = 0; i < _lstItems->getTexts(); ++i)
	{
		if (_items[_rows[i]].qtySrc > _items[_rows[i]].amount)
		{
			allItemsSelected = false;
			break;
		}
	}
	int dir = allItemsSelected ? -1 : 1;

	size_t backup = _sel;
	for (size_t i = 0; i < _lstItems->getTexts(); ++i)
	{
		_sel = i;
		changeByValue(INT_MAX, dir);
	}
	_sel = backup;
}

/**
* Increase all items to max - 1, i.e. sell everything but one.
* @param action Pointer to an action.
*/
void SellState::btnSellAllButOneClick(Action *)
{
	_sellAllButOne = true;
	btnSellAllClick(nullptr);
	_sellAllButOne = false;
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
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			return;
		}
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule != 0)
			{
				_game->pushState(new ManufactureDependenciesTreeState(rule->getType()));
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule != 0)
			{
				std::string articleId = rule->getUfopediaType();
				ArticleDefinition* article = _game->getMod()->getUfopaediaArticle(articleId, false);
				if (_game->isCtrlPressed() || !article || !Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
				{
					const RuleResearch* selectedTopic = _game->getMod()->getResearch(articleId, false);
					if (selectedTopic)
					{
						_game->pushState(new TechTreeViewerState(selectedTopic, 0));
					}
				}
				else
				{
					Ufopaedia::openArticle(_game, articleId);
				}
			}
		}
		else if (getRow().type == TRANSFER_CRAFT)
		{
			Craft *rule = (Craft*)getRow().rule;
			if (rule != 0)
			{
				std::string articleId = rule->getRules()->getType();
				if (_game->isCtrlPressed())
				{
					Ufopaedia::openArticle(_game, articleId);
				}
				else
				{
					_game->pushState(new TechTreeViewerState(0, 0, 0, rule->getRules()));
				}
			}
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
	if (dir > 0)
	{
		if (0 >= change || getRow().qtySrc + getRow().transferSrc <= getRow().amount) return;
		change = std::min(getRow().qtySrc + getRow().transferSrc - getRow().amount, change);
		if (_sellAllButOne && change > 0)
		{
			--change;
		}
	}
	else
	{
		if (0 >= change || 0 >= getRow().amount) return;
		change = std::min(getRow().amount, change);
	}
	getRow().amount += dir * change;
	_total += dir * getRow().cost * change;

	// Calculate the change in storage space.
	Soldier *soldier;
	const RuleItem *item;
	switch (getRow().type)
	{
	case TRANSFER_SOLDIER:
		soldier = (Soldier*)getRow().rule;
		if (soldier->getArmor()->getStoreItem())
		{
			_spaceChange += dir * soldier->getArmor()->getStoreItem()->getSize();
		}
		break;
	case TRANSFER_CRAFT:
		// Note: in OXCE, there is no storage space change, everything on the craft is already included in the base storage space calculations
		break;
	case TRANSFER_ITEM:
		item = (const RuleItem*)getRow().rule;
		_spaceChange -= dir * change * item->getSize();
		break;
	default:
		//TRANSFER_SCIENTIST and TRANSFER_ENGINEER do not own anything that takes storage
		break;
	}

	updateItemStrings();
	updateSubtitleLine();
	updateOkButton();
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
	int qtyOnBase = getRow().qtySrc + getRow().transferSrc - getRow().amount;
	int qtyAmount = getRow().amount;
	// Temporal overrides for column alignment (using regular screen).
	//qtyOnBase = 9'999;
	//qtyAmount = 9'999;

	if (Options::r1doStyle_sellState && Options::r1doReservedAmountBehavior > 0)
	{
		int qtyAllocated = getRow().allocatedSrc;
		int qtyProtected = getRow().protectedSrc;
		// Temporal overrides for column alignment (using regular screen).
		//qtyAllocated = 999;
		//qtyProtected = 0;

		_lstItems->setCellText(_sel, 3, Unicode::formatNumber(qtyAmount).c_str());
		if (_debriefingState != 0)
		{
			// Show only looted items.
			_lstItems->setCellText(_sel, 1, Unicode::formatNumber(qtyOnBase).c_str());
		}
		else
		{
			_lstItems->setCellText(_sel, 1, Unicode::formatNumber(qtyOnBase + qtyProtected).c_str());
		}

		std::string allocatedString;
		if (qtyAllocated == 0)
		{
			allocatedString = "";
		}
		else if ( qtyOnBase + qtyProtected > qtyAllocated)
		{
			allocatedString = tr("STR_IS_BIGGER_THAN_ALLOCATED_NUMBER").arg(Unicode::formatNumber(qtyAllocated));
		}
		else if ( qtyOnBase + qtyProtected < qtyAllocated)
		{
			allocatedString = tr("STR_IS_SMALLER_THAN_ALLOCATED_NUMBER").arg(Unicode::formatNumber(qtyAllocated));
		}
		else
		{
			allocatedString = tr("STR_IS_EQUAL_TO_ALLOCATED_NUMBER").arg(Unicode::formatNumber(qtyAllocated));
		}
		_lstItems->setCellText(_sel, 2, allocatedString);
	}
	else
	{
		std::ostringstream ss, ss2;
		ss << qtyAmount;
		ss2 << qtyOnBase;
		_lstItems->setCellText(_sel, 1, ss2.str());
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
}

/**
* Updates the production list to match the category filter.
*/
void SellState::cbxCategoryChange(Action *action)
{
	_invertFilter = action->getDetails()->button.button == SDL_BUTTON_RIGHT;
	_previousSort = _currentSort;

	if (_game->isCtrlPressed())
	{
		_currentSort = _game->isShiftPressed() ? TransferSortDirection::BY_UNIT_SIZE : TransferSortDirection::BY_TOTAL_SIZE;
	}
	else if (_game->isAltPressed())
	{
		_currentSort = _game->isShiftPressed() ? TransferSortDirection::BY_UNIT_COST : TransferSortDirection::BY_TOTAL_COST;
	}
	else
	{
		_currentSort = TransferSortDirection::BY_LIST_ORDER;
	}

	updateList();
}

/**
 * Updates variable texts between screen title and spreadsheet.
 */
void SellState::updateSubtitleLine()
{
	_txtFunds->setText(tr("STR_FUNDS").arg(Unicode::formatFunding(_game->getSavedGame()->getFunds())));

	int64_t adjustedTotal = _total * _game->getSavedGame()->getSellPriceCoefficient() / 100;
	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Unicode::formatFunding(adjustedTotal)));

	if (_txtSpaceUsed->getVisible())
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
		_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss.str()));
	}
}

/**
 * Enables or disables the OK button based.
 *
 * Based on wether or not base stores are overflowing.
 */
void SellState::updateOkButton()
{
	if (_debriefingState == 0 && Options::storageLimitsEnforced)
	{
		_btnOk->setVisible(!_base->storesOverfull(_spaceChange));
	}
}

/**
 * Calculates the allocated and protected amounts based on user preference.
 *
 * @param itemRule Type of item.
 * @param overfullCritical Do we recognize some items are now sellable, while normally not.
 * @return How much of an item is considered allocated/protected in format: `<allocated,protected>`.
 */
std::pair<int, int> SellState::getAllocatedAndProtectedCountsSrc(const RuleItem* itemRule, bool overfullCritical) const
{
	if (!itemRule || Options::r1doReservedAmountBehavior == 0)
		return std::make_pair(0,0);

	int qtyA = 0, qtyP = 0;
	if (Options::r1doReservedAmountBehavior != 2)
	{
		// Claims by soldiers.
		qtyA += _base->getItemCountSoldierEquipment(itemRule, true);
		qtyA += _base->getItemCountTransfersSoldierEquipment(itemRule, true);

		// Unlike other items, soldiers take armor directly from base stores.
		// To paint a complete picture add armor to display of "on base" count.
		qtyP += qtyA;
		qtyP -= _base->getItemCountSoldierEquipment(itemRule);
		qtyP -= _base->getItemCountTransfersSoldierEquipment(itemRule);
	}
	if (Options::r1doReservedAmountBehavior > 1)
	{
		// Claims by craft/manufacture/research/buildings.

		// Show capacity and future requirements.
		// 'protected' takes care of current allocation.
		qtyA += _base->getItemCountCraftArmament(itemRule, true);
		qtyA += _base->getItemCountCraftCargoBay(itemRule);
		qtyA += _base->getItemCountCraftFuel(itemRule, true);
		// Next line is debatable though.
		// One could also make a case that one wants to know the requirement
		// for a theoretical max base defense instead of just a single pass.
		qtyA += _base->getItemCountDefenses(itemRule);
		qtyA += _base->getItemCountDefensesWithOwnAmmo(itemRule, true);
		// Next line could lead to a display case where allocated < protected.
		// e.g. The lowest on base number will be larger than the allocated one.
		qtyA += _base->getItemCountFacilities(itemRule, true);
		qtyA += _base->getItemCountManufacture(itemRule, true);
		qtyA += _base->getItemCountResearch(itemRule);
		qtyA += _base->getItemCountTransfersCraftArmament(itemRule, true);
		qtyA += _base->getItemCountTransfersCraftFuel(itemRule, true);
		qtyA += _base->getItemCountTransfersCraftCargoBay(itemRule);

		// Show items one cannot get back via this screen's methods,
		// by adding them as 'protected' value to the on base column.
		qtyP += _base->getItemCountDefensesWithOwnAmmo(itemRule);
		qtyP += _base->getItemCountFacilities(itemRule);
		qtyP += _base->getItemCountManufacture(itemRule);
		qtyP += _base->getItemCountResearch(itemRule);
		// Next line can be used to derive if there is still enough elerium left
		// to fuel craft (original driver for this screen's deviation).
		qtyP += _base->getItemCountCraftFuel(itemRule);
		qtyP += _base->getItemCountTransfersCraftFuel(itemRule);

		// By default we are not allowed to take away craft armament.
		// Those only count towards base stores if `overfullCritical == true`.
		if (!Options::storageLimitsEnforced || !overfullCritical)
		{
			qtyP += _base->getItemCountCraftArmament(itemRule);
			qtyP += _base->getItemCountTransfersCraftArmament(itemRule);
		}
		// By default we are not allowed to take items away from craft cargo bay.
		// Those only count towards base stores if
		// `overfullCritical == true` or `_origin == OPT_BATTLESCAPE`.
		if (!Options::storageLimitsEnforced || _origin != OPT_BATTLESCAPE || !overfullCritical)
		{
			qtyP += _base->getItemCountCraftCargoBay(itemRule);
			qtyP += _base->getItemCountTransfersCraftCargoBay(itemRule);
		}
	}
	if (Options::r1doReservedAmountBehavior == 3)
	{
		// Greedy claim (max of 1 and 2)

		// Cargo hold and (on board) soldier items leads to double counting.
		// This must be corrected **per** craft.
		int correction = 0;
		for (auto* craft : *_base->getCrafts())
		{
			if (!craft)
				continue;

			auto soldierItems = craft->getSoldierItems();
			if (soldierItems->empty())
			{
				craft->calculateTotalSoldierEquipment();
				soldierItems = craft->getSoldierItems();
			}
			correction += std::min(craft->getItemCountCargoBay(itemRule), soldierItems->getItem(itemRule));
		}
		for (auto* transfer : *_base->getTransfers())
		{
			if (transfer->getCraft())
			{
				if (!transfer->getCraft())
					continue;

				auto soldierItems = transfer->getCraft()->getSoldierItems();
				if (soldierItems->empty())
				{
					transfer->getCraft()->calculateTotalSoldierEquipment();
					soldierItems = transfer->getCraft()->getSoldierItems();
				}
				correction += std::min(transfer->getCraft()->getItemCountCargoBay(itemRule), soldierItems->getItem(itemRule));
			}
		}
		qtyA -= correction;

		// No correction is needed for protected.
		// Only soldier armor can enter protected,
		// which is not stored in craft cargo bay.
	}
	return std::make_pair(qtyA, qtyP);
}

}
