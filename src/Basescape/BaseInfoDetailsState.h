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
#include "../Mod/RuleBaseFacilityFunctions.h"

namespace OpenXcom
{

class Base;
class TextButton;
class Window;
class Text;
class TextList;

enum DetailsCategory {
	DC_SOLDIERS, DC_ENGINEERS, DC_SCIENTISTS,
	DC_QUARTERS, DC_STORES, DC_LABORATORIES, DC_WORKSHOPS, DC_CONTAINMENT, DC_HANGARS,
	DC_DEFENSE, DC_DETECTION};

/**
 * Monthly Costs category breakdown subwindow
 *
 * Shows detailed contributions to the cost category.
 */
class BaseInfoDetailsState : public State
{
private:
	struct BeanCounter
	{
		// Use parent-child relation to enable collapsable details.
		int childId = 0;        // We have a subtotal if 'childId == parentId'.
		int parentId = 0;       // To allow collapsing of child rows.
		bool isVisible = false; // By default children are hidden unless unfolded.
		std::string description = "";
		int amount = 0;         // How many times a contribution is present on the base (-1 means do not draw).
		int value = 0;          // Value for this contribution.
		std::string amountOverride = ""; // Specialized string for 'amount' column.
		std::string valueOverride = "";  // Specialized string for 'result' column.
	};

	Base *_base;
	DetailsCategory _currentCategory;

	TextButton *_btnOk, *_btnPrev, *_btnNext;
	Window *_window;
	Text *_txtTitle, *_txtSource, *_txtQuantity, *_txtResult, *_txtTotal;
	TextList *_lstDetails;
	std::vector<BeanCounter> _details;
	std::vector<int> _rows;
	size_t _sel;
	RuleBaseFacilityFunctions _unlockedServicesBaseType = 0;

	void drawBody();
	void categorySoldiers();
	void addSubCategoryPsionicTraining();
	void addSubCategoryPhysicalTraining();
	void addSubCategoryWoundRecovery();
	void addSubCategoryWoundRecoveryInProgress();
	void addSubCategoryHealthRecovery();
	void addSubCategoryHealthRecoveryInProgress();
	void addSubCategoryManaRecovery();
	void addSubCategoryManaRecoveryInProgress();
	void addSubCategoryTransformations();
	void categoryQuarters();
	void addSubCategoryQuarterProviders();
	void addSubCategoryQuarterUsage();
	void addSubCategoryHiringServices();
	void categoryStorage();
	void addSubCategoryStorageProviders();
	void addSubCategoryStoragesUsage();
	void addSubCategoryPurchaseServices();
	void addSubCategoryPurchaseLimits();
	void addSubCategoryPurchaseCountries();
	void categoryLabs();
	void addSubCategoryLabProviders();
	void addSubCategoryLabUsage();
	void addSubCategoryLabServices();
	void categoryWorkshops();
	void addSubCategoryWorkshopProviders();
	void addSubCategoryWorkshopUsage();
	void addSubCategoryWorkshopServices();
	void categoryAlienContainment();
	void addSubCategoryContainmentType(int type = 0);
	void categoryHangars();
	void categoryDefense();
	void categoryDetection();

	void drawList();
	void lstDetailsMousePress(Action *action);

	BeanCounter &getRow() {return _details[_rows[_sel]];}
	int addToDetailsVector(BeanCounter row, bool updateValueField = true);
	bool isSubtotalNeeded(int parentId);
	int calculateSubtotalValue(int parentId);
	int calculateSubtotalAmount(int parentId);
	int getSubtotalValueMax(int parentId);

	// Helpers
	std::string toStringHp(float value);
	std::string toStringHp(int value);
	std::string toStringMana(int value);
	std::string toStringPercent(float value);
	void sortChildren(size_t startIndex, int skipChilds = 0);
	void addServices(RuleBaseFacilityFunctions services, std::string subTotalDescription);
public:
	/// Creates the cost details state.
	BaseInfoDetailsState(Base *base, DetailsCategory currentCategory);
	/// Cleans up the cost details state.
	~BaseInfoDetailsState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);

	/// Handler for clicking Previous button.
	void btnPrevClick(Action *action);
	/// Handler for clicking Next button.
	void btnNextClick(Action *action);
};

}
