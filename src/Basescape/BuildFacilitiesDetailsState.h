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
#include "../Mod/RuleCountry.h"
#include "../Mod/RuleRegion.h"

namespace OpenXcom
{

class Base;

class TextButton;
class ToggleTextButton;
class Window;
class Text;
class TextList;
class RuleBaseFacility;

/**
 * Base facilities details subwindow
 *
 * Shows reasons for build errors and facilities requirements.
 */
class BuildFacilitiesDetailsState : public State
{
	friend class BuildFacilitiesState;
protected:
	enum class Tabs { Blockers, Requirements };
private:
	struct BeanCounter
	{
		// Use parent-child relation to enable collapsable details.
		// We have a subtotal (parent) if 'childId == parentId'.
		size_t childId = 0; // Obsolete? Can use vector element to test against?
		size_t parentId = 0;
		std::string description = ""; // First display Column
		int amount = 0;               // How many times a contribution is present on the base.
		int baseValue = 0;            // Basic value of this contribution (e.g. for an amount of 1).

		// Overrides
		bool isRowVisible = false;       // By default children are hidden unless unfolded.
		std::string amountOverride = ""; // Specialized string for 'amount' column.
		std::string valueOverride = "";  // Specialized string for 'result' column.

		// Perhaps use
		bool drawAmount = true; // Does this row draw something in the amount column? Used to be ".amount == -1"

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
	RuleBaseFacility *_facRuleSelected;
	Tabs _activeTab;

	Window *_window;
	Text *_txtTitle, *_txtSource, *_txtResult;
	ToggleTextButton *_tabRequirements, *_tabBlockers;
	TextList *_lstDetails;
	TextButton *_btnOk;

	std::vector<BeanCounter> _details;
	std::vector<int> _rows;
	size_t _sel;
	RuleBaseFacilityFunctions _facilitiesOnlyServices, _countriesOnlyServices, _regionsOnlyServices;
	RuleBaseFacilityFunctions _providedBaseFunc, _futureBaseFunc, _forbiddenBaseFunc;
	RuleBaseFacilityFunctions _requiredFacFunc, _forbiddenFacFunc, _providedFacFunc;
	const RuleCountry *_countryRule = nullptr;
	const RuleRegion *_regionRule = nullptr;
	std::map<const RuleBaseFacility*, int> _baseFacilitiesAndCount;

	void tabClick(Action *action);
	void lstDetailsMousePress(Action *);
	void btnOkClick(Action *);

	void drawBody();
	void setupTabBlockers();
	void subcategoryBlockedByCountry();
	void subcategoryBlockedByRegion();
	void subcategoryBlockedByFacilities();
	void subcategoryBlockedByRequiredItems();
	void subcategoryBlockedByFunds();
	void updateList();

	void add2_detailsVector(std::vector<BeanCounter> &subCategory, bool forceInclude = false);
	BeanCounter &getRow() {return _details[_rows[_sel]];}


public:
	/// Creates the facilities details state.
	BuildFacilitiesDetailsState(Base *base, RuleBaseFacility *currentFacility, Tabs currentTab = Tabs::Requirements);
	/// Cleans up the facilities details state.
	~BuildFacilitiesDetailsState();
};

}
