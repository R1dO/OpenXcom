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

namespace OpenXcom
{

class Base;
class TextButton;
class Window;
class Text;
class TextList;

enum CostCategory {
	CC_CRAFTS_ALL, CC_CRAFTS_FIGHTERS, CC_CRAFTS_TRANSPORTS, CC_CRAFTS_MIXED,
	CC_SALARIES_ALL, CC_SOLDIERS, CC_SCIENTISTS, CC_ENGINEERS, CC_ITEMS,
	CC_FACILITIES, CC_GLOBAL_RESULT};

struct BeanCounter
{
	// Use parent-child relation to enable collapsable details.
	int id;         // We have a subtotal if 'id == parentId'.
	int parentId;   // To allow collapsing of child rows.
	bool isVisible; // Collapsed children should not be drawn.
	std::string description;
	int amount;     // Zero is used to indicate: Don't draw this column.
	int64_t value;  // Cost or Income per element.
};

/**
 * Monthly Costs category breakdown subwindow
 *
 * Shows detailed contributions to the cost category.
 */
class MonthlyCostsDetailsState : public State
{
private:
	Base *_base;
	CostCategory _currentCategory;

	TextButton *_btnOk, *_btnPrev, *_btnNext;
	Window *_window;
	Text *_txtTitle, *_txtSource, *_txtQuantity, *_txtResult;
	TextList *_lstDetails, *_lstTotal;
	std::vector<BeanCounter> _details;
	std::vector<int> _rows;
	size_t _sel;

	void drawBody();
	void categoryFacilityMaintenance();
	void categoryGlobalResult();

	BeanCounter &getRow() {return _details[_rows[_sel]];}
	/// Handler for pressing-down a mouse-button in the list.
	void lstDetailsMousePress(Action *action);

/// Updates the item list.
	void updateList();
public:
	/// Creates the cost details state.
	MonthlyCostsDetailsState(Base *base, CostCategory currentCategory);
	/// Cleans up the cost details state.
	~MonthlyCostsDetailsState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);

	/// Handler for clicking Previous button.
	void btnPrevClick(Action *action);
	/// Handler for clicking Next button.
	void btnNextClick(Action *action);
};

}
