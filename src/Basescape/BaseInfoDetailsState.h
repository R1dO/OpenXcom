#pragma once
/*
 * Copyright 2024 OpenXcom Developers.
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
#include "../Mod/RuleBaseFacilityFunctions.h"

namespace OpenXcom
{

class Base;
class TextButton;
class ToggleTextButton;
class Window;
class Text;
class TextList;

enum class BaseInfoDetailsCategory {
	SOLDIERS, ENGINEERS, SCIENTISTS,
	QUARTERS, STORES, LABORATORIES, WORKSHOPS, CONTAINMENT, HANGARS,
	DEFENSE, DETECTION
	};
constexpr std::string_view enum2string(BaseInfoDetailsCategory category) // C++17 -> string_view
{
	switch (category)
	{
	case BaseInfoDetailsCategory::SOLDIERS:     return "Soldiers";
	case BaseInfoDetailsCategory::ENGINEERS:    return "Engineers";
	case BaseInfoDetailsCategory::SCIENTISTS:   return "Scientists";
	case BaseInfoDetailsCategory::QUARTERS:     return "Quarters";
	case BaseInfoDetailsCategory::STORES:       return "Stores";
	case BaseInfoDetailsCategory::LABORATORIES: return "Laboratories";
	case BaseInfoDetailsCategory::WORKSHOPS:    return "Workshops";
	case BaseInfoDetailsCategory::CONTAINMENT:  return "Containment";
	case BaseInfoDetailsCategory::HANGARS:      return "Hangars";
	case BaseInfoDetailsCategory::DEFENSE:      return "Defense";
	case BaseInfoDetailsCategory::DETECTION:    return "Detections";
	default: return "Internal enum error"; // Should not be reachable.
	}
}

/**
 * Base info category breakdown subwindow
 *
 * Shows which facilities contribute to selected category.
 */
class BaseInfoDetailsState : public State
{
private:
	struct BeanCounter
	{
		// Use parent-child relation to enable collapsable details.
		// We have a subtotal (parent) if 'childId == parentId'.
		size_t childId = 0;
		size_t parentId = 0;
		std::string description = ""; // First display Column
		int amount = 0;               // How many times a contribution is present on the base.
		int baseValue = 0;            // Basic value of this contribution (e.g. for an amount of 1).

		// Overrides
		bool isRowVisible = false;       // By default children are hidden unless unfolded.
		std::string amountOverride = ""; // Specialized string for 'amount' column.
		std::string valueOverride = "";  // Specialized string for 'result' column.

		BeanCounter() = default;
		// Shortcut for minimum amount necessary
		BeanCounter(size_t _childId, size_t _parentId, std::string _description,
			int _amount, int _value, bool _isVisible
			) :
			childId(_childId), parentId(_parentId), description(_description),
			amount(_amount), baseValue(_value), isRowVisible(_isVisible)
			{ }
	};


	Base *_base;
	BaseInfoDetailsCategory _category;

	TextButton *_btnOk, *_btnPrev, *_btnNext;
	ToggleTextButton *_btnQueuedFacilities;
	Window *_window;
	Text *_txtTitle, *_txtSource, *_txtQuantity, *_txtResult, *_txtTotal;
	TextList *_lstDetails;
	std::vector<BeanCounter> _details;
	std::vector<int> _rows;
	size_t _sel;

	void btnOkClick(Action *);
	void btnNextClick(Action *);
	void btnPrevClick(Action *);
	void btnToggleQueuedFacilities(Action *);
	void lstDetailsMousePress(Action *);

	void drawBody();
	void setupPlaceholders();
	void setupCategoryDetection();
	void subcategoryBaseCamouflage();
	void subcategoryUfoDetection();
	void subcategoryAlienBaseDetection();
	void updateList();

	void add2vector(std::vector<BeanCounter> &subCategory, BeanCounter row);
	void add2screenList(std::vector<BeanCounter> &subCategory);
	void sortChildrenByDescription(std::vector<BeanCounter> &subCategory, size_t skipChildren = 0);
	int countContributingFacilities(std::vector<BeanCounter> &subCategory);
	double calcProbabilityAtLeastOne(std::vector<BeanCounter> &subCategory);
	double calcProbabilityAtLeastOne(int baseChance, int tries);
	BeanCounter &getRow() {return _details[_rows[_sel]];}
public:
	/// Creates the info details state.
	BaseInfoDetailsState(Base *base, BaseInfoDetailsCategory category);
	/// Cleans up the info details state.
	~BaseInfoDetailsState();
};

}
