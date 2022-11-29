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
#include <vector>
#include "../Engine/State.h"

namespace OpenXcom
{

enum SoldierArmorOrigin
{
	SA_GEOSCAPE,
	SA_BATTLESCAPE
};

class Base;
class TextButton;
class ToggleTextButton;
class Window;
class Text;
class TextEdit;
class TextList;
class ComboBox;
class Armor;
class ArrowButton;

/// Armor sorting modes.
enum ArmorSort
{
	ARMOR_SORT_NONE,
	ARMOR_SORT_NAME_ASC,
	ARMOR_SORT_NAME_DESC,
};

struct ArmorItem
{
	ArmorItem(const std::string &_type, const std::string &_name, const std::string &_quantity, const int &_listOrder)
		: type(_type), name(_name), quantity(_quantity)
	{
	}
	std::string type;
	std::string name, quantity;
	const Armor *armor = nullptr; // Reduces '_game->getMod()->getArmor()' calls
	int qty = 0;            // Quantity in base stores, -1 for infinite.
	int listOrder = 0;      // Screen specific listOrder.
	int id = 0;             // Subtotal if 'id == parentId'.
	int parentId = 0;       // To allow grouping of child rows (for folding), '0' means orphan.
	bool isKnown = false;   // Can we see ufopaedia entry (e.g. is researched).
	bool isVisible = false; // By default children are hidden unless unfolded.

	void resetIdsAndVisibility(bool visible = false)
	{
		id = 0;
		parentId = 0;
		isVisible = visible;
	}
};


/**
 * Select Armor window that allows changing
 * of the armor equipped on a soldier.
 */
class SoldierArmorState : public State
{
private:
	Base *_base;
	size_t _soldier;

	SoldierArmorOrigin _origin;
	TextButton *_btnCancel;
	ToggleTextButton *_btnCompare;
	TextEdit *_btnQuickSearch;
	Window *_window;
	Text *_txtTitle, *_txtType, *_txtQuantity;
	TextList *_lstArmor;
	ArrowButton *_sortName;
	std::vector<ArmorItem> _armors;
	std::vector<size_t> _indices;
	ArmorSort _armorOrder, _previousOrder;
	void updateArrows();

	bool _alternateScreen;
	ComboBox *_cbxCategory;
	std::vector<std::string> _cats;

	/// Handler for changing the category filter.
	void cbxCategoryChange(Action *action);
	void fillArmorList();
	void drawList();
	size_t _sel;
	ArmorItem &getRow() {return _armors[_indices[_sel]];}
public:
	/// Creates the Soldier Armor state.
	SoldierArmorState(Base *base, size_t soldier, SoldierArmorOrigin origin);
	/// Cleans up the Soldier Armor state.
	~SoldierArmorState();
	/// Sorts the armor list.
	void sortList();
	/// Updates the armor list.
	void updateList();
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action* action);
	void btnQuickSearchApply(Action* action);
	/// Handler for clicking the Weapons list.
	void lstArmorClick(Action *action);
	/// Handler for clicking the Weapons list.
	void lstArmorClickMiddle(Action *action);
	void lstArmorClickRight(Action *action);
	/// Handler for clicking the Name arrow.
	void sortNameClick(Action *action);
};

}
