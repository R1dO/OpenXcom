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
#include "../Savegame/Transfer.h"
#include <vector>
#include <string>
#include <set>

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class ComboBox;
class Timer;
class Base;

/**
 * Transfer screen that lets the player pick
 * what items to transfer between bases.
 */
class TransferItemsState : public State
{
private:
	/**
	 * Tailored struct to store variables of importance to the spreadsheet.
	 */
	struct TransferItemRow
	{
		TransferType type;   ///< Item category.
		void *rule;          ///< Pointer to ruleset of item.
		std::string name;    ///< Translated name of item.
		int cost;            ///< Cost of transferring this item.
		int qtySrc, qtyDst;  ///< Starting amount on bases (stores + transfer + craft).
		int amount;          /**< Requested change.
		                      *
		                      * + Positive values moves an item towards target base.
		                      * + Negative values moves an item towards current base.
		                      */
		int transferSrc, transferDst;  ///< Amount currently on route to bases.
		int reservedSrc, reservedDst;  ///< Reserved amount of items(s).
	};

	Base *_baseFrom, *_baseTo;
	TextButton *_btnOk, *_btnCancel;
	Window *_window;
	Text *_txtTitle, *_txtQuantity, *_txtAmountTransfer, *_txtAmountDestination, *_txtFunds;
	ComboBox *_cbxCategory;
	TextList *_lstItems;
	std::vector<TransferItemRow> _items;
	std::vector<int> _rows;
	std::vector<std::string> _cats;
	std::set<std::string> _craftWeapons, _armors;
	size_t _sel;
	int _total, _pQty, _cQty, _aQty;
	double _iQty;
	double _distance;
	Uint8 _ammoColor;
	Timer *_timerInc, *_timerDec;
	/// Gets the category of the current selection.
	std::string getCategory(int sel) const;
	/// Gets the row of the current selection.
	TransferItemRow &getRow() { return _items[_rows[_sel]]; }
	/// Gets distance between bases.
	double getDistance() const;
	/// Do we use the alternate base screen option?
	bool _alternateScreen;
	/// Updates entities below screen title.
	void updateSubtitleLine();
	/// Updates variable cells in the spreadsheet header row.
	void updateSpreadsheetHeader();
public:
	/// Creates the Transfer Items state.
	TransferItemsState(Base *baseFrom, Base *baseTo);
	/// Cleans up the Transfer Items state.
	~TransferItemsState();
	/// Runs the timers.
	void think();
	/// Updates the item list.
	void updateList();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Completes the transfer between bases.
	void completeTransfer();
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handler for pressing an Increase arrow in the list.
	void lstItemsLeftArrowPress(Action *action);
	/// Handler for releasing an Increase arrow in the list.
	void lstItemsLeftArrowRelease(Action *action);
	/// Handler for clicking an Increase arrow in the list.
	void lstItemsLeftArrowClick(Action *action);
	/// Handler for pressing a Decrease arrow in the list.
	void lstItemsRightArrowPress(Action *action);
	/// Handler for releasing a Decrease arrow in the list.
	void lstItemsRightArrowRelease(Action *action);
	/// Handler for clicking a Decrease arrow in the list.
	void lstItemsRightArrowClick(Action *action);
	/// Handler for pressing-down a mouse-button in the list.
	void lstItemsMousePress(Action *action);
	/// Increases the quantity of an item by one.
	void increase();
	/// Increases the quantity of an item by the given value.
	void increaseByValue(int change);
	/// Decreases the quantity of an item by one.
	void decrease();
	/// Decreases the quantity of an item by the given value.
	void decreaseByValue(int change);
	/// Updates the quantity-strings of the selected item.
	void updateItemStrings();
	/// Gets the total of the transfer.
	int getTotal() const;
	/// Handler for changing the category filter.
	void cbxCategoryChange(Action *action);
};

}
