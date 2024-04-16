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

#include "BuildFacilitiesDetailsState.h"
//#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the base info category breakdown subwindow.
 *
 * @param base            Pointer to the base to get info from.
 * @param currentFacility Pointer to ruleset of the facility to display.
 * @param currentTab Category to open
 */
BuildFacilitiesDetailsState::BuildFacilitiesDetailsState(Base *base, RuleBaseFacility *currentFacility, Tabs currentTab) :
	_base(base), _facRuleSelected(currentFacility), _activeTab(currentTab)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 320, 192-16, 0, 8+8, POPUP_BOTH); // Vertical center w.r.t. base grid.
	_txtTitle = new Text(278, 16, 21, 25);
	_tabRequirements = new ToggleTextButton(136, 16, 23, 35+8+3);
	_tabBlockers = new ToggleTextButton(136, 16, 159, 35+8+3);
	_txtSource = new Text(114, 9, 25, 35+16+2+13);
	_txtResult = new Text(54, 9, 225+15, 35+16+2+13);
	_lstDetails = new TextList(272, 104-16, 23, 46+16+2+8+5); // Height = 11*8 = 88 (8 due to rowheight overlap using default rules).
	_btnOk = new TextButton(288, 16, 16, 169);

	// Set palette
	setInterface("buildFacilitiesDetails");

	add(_window, "window", "buildFacilitiesDetails");
	add(_txtTitle, "text", "buildFacilitiesDetails");
	add(_tabRequirements, "button", "buildFacilitiesDetails");
	add(_tabBlockers, "button", "buildFacilitiesDetails");
	add(_txtSource, "text", "buildFacilitiesDetails");
	add(_txtResult, "text", "buildFacilitiesDetails");
	add(_lstDetails, "list", "buildFacilitiesDetails");
	add(_btnOk, "button", "buildFacilitiesDetails");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "buildFacilitiesDetails");

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_BFDS_TITLE").arg(tr(_facRuleSelected->getType())));
	_tabRequirements->setText(tr("STR_BFDS_TAB_REQUIREMENTS"));
	_tabRequirements->setPressed(_activeTab == Tabs::Requirements);
	_tabRequirements->onMousePress((ActionHandler)&BuildFacilitiesDetailsState::tabClick, SDL_BUTTON_LEFT);
	_tabBlockers->setText(tr("STR_BFDS_TAB_BLOCKERS"));
	_tabBlockers->setPressed(_activeTab == Tabs::Blockers);
	_tabBlockers->onMousePress((ActionHandler)&BuildFacilitiesDetailsState::tabClick, SDL_BUTTON_LEFT);
	_txtSource->setText(tr("STR_SOURCE"));
	_txtResult->setText(tr("STR_VALUE"));
	_lstDetails->setColumns(2, 155+15+45, 70-15);
	_lstDetails->setSelectable(true); // Required for collapse/fold functionality.
	_lstDetails->setBackground(_window);
	_lstDetails->setScrolling(true);
	_lstDetails->setMargin(2);        // Shifts **all** columns 2px to the right.
	_lstDetails->onMousePress((ActionHandler)&BuildFacilitiesDetailsState::lstDetailsMousePress, SDL_BUTTON_LEFT);
	_lstDetails->onMousePress((ActionHandler)&BuildFacilitiesDetailsState::lstDetailsMousePress, SDL_BUTTON_RIGHT);
	_lstDetails->setWordWrap(true);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&BuildFacilitiesDetailsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&BuildFacilitiesDetailsState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&BuildFacilitiesDetailsState::btnOkClick, Options::keyCancel);

	RuleBaseFacilityFunctions allCountriesServices;
	for (const auto* country : *_game->getSavedGame()->getCountries())
	{
		const RuleCountry *ruleCountry = country->getRules();
		if (!ruleCountry) continue;

		allCountriesServices |= ruleCountry->getProvidedBaseFunc();
		// Blockers/Requirements might depend on services from current country.
		// Based on `Base::calculateServices()`
		if (ruleCountry->insideCountry(_base->getLongitude(), _base->getLatitude()))
		{
			_countryRule = ruleCountry;
		}
	}
	RuleBaseFacilityFunctions allRegionsServices;
	for (const auto* region : *_game->getSavedGame()->getRegions())
	{
		RuleRegion *ruleRegion = region->getRules();
		if (!ruleRegion) continue;

		allRegionsServices |= ruleRegion->getProvidedBaseFunc();
		// Blockers/Requirements might depend on services from current region.
		if (ruleRegion->insideRegion(_base->getLongitude(), _base->getLatitude()))
		{
			_regionRule = ruleRegion;
		}
	}
	RuleBaseFacilityFunctions allFacilitiesServices;
	for (auto& facilityType : _game->getMod()->getBaseFacilitiesList())
	{
		RuleBaseFacility *ruleFacility = _game->getMod()->getBaseFacility(facilityType);
		if (!ruleFacility) continue;

		allFacilitiesServices |= ruleFacility->getProvidedBaseFunc();
	}
	// For completeness:
	// No check here if the `_*OnlyServices` are possible for this base.
	// If it is somehow blocked due to a `forbidden*func()` this should show up
	// in those subcategories other than the one related to the`_*OnlyService`.
	// * As blocked by country
	// * As blocked by region
	// * As blocked by existing facility
	// * As blocked by missing facility/service
	// Player might have to navigate to other facilities to connect the dots.
	_countriesOnlyServices = allCountriesServices & ~allRegionsServices & ~allFacilitiesServices;
	_regionsOnlyServices = allRegionsServices & ~allFacilitiesServices & ~allCountriesServices;
	_facilitiesOnlyServices = allFacilitiesServices & ~allCountriesServices & ~allRegionsServices;

	// Base services based blockers/requirements.
	_providedBaseFunc = _base->getProvidedBaseFunc({});
	_forbiddenBaseFunc = _base->getForbiddenBaseFunc({});
	_futureBaseFunc = _base->getFutureBaseFunc({});
	// Facility specific service based blockers/requirements.
	_requiredFacFunc = _facRuleSelected->getRequireBaseFunc();
	_providedFacFunc = _facRuleSelected->getProvidedBaseFunc();
	_forbiddenFacFunc = _facRuleSelected->getForbiddenBaseFunc();

	// Unique facilities on base, including under construction/queued.
	// No need to track those separately:
	// + Creates too much list clutter due to multiple identical facilities
	// + Under construction / queue exists for a limited amount of time
	// + That level of technical detail is outside the scope of this screen
	for (auto& facility : *_base->getFacilities())
	{
		const RuleBaseFacility *ruleFac = facility->getRules();

		int countFac = 1;
		if (auto search = _baseFacilitiesAndCount.find(ruleFac);
			search != _baseFacilitiesAndCount.end())
		{
			countFac += search->second;
		}
		_baseFacilitiesAndCount.insert_or_assign(ruleFac, countFac);
	}

	drawBody();
}

/**
 *
 */
BuildFacilitiesDetailsState::~BuildFacilitiesDetailsState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void BuildFacilitiesDetailsState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Toggle screen tab
 */
void BuildFacilitiesDetailsState::tabClick(Action *action)
{
	if (action->getSender() == _tabRequirements)
	{
		_tabBlockers->setPressed(!_tabBlockers->getPressed());
	}
	else if (action->getSender() == _tabBlockers)
	{
		_tabRequirements->setPressed(!_tabRequirements->getPressed());
	}

	drawBody();
}

/**
* Handles mouse-clicks on the list rows.
*/
void BuildFacilitiesDetailsState::lstDetailsMousePress(Action *)
{
	_sel = _lstDetails->getSelectedRow(); // Needed for `getRow()`.

	// Flip visibility of child elements.
	for (size_t i = 0; i < _details.size(); ++i)
	{
		if (_details[i].parentId == i) continue;
		if (_details[i].parentId != getRow().parentId) continue;

		_details[i].isRowVisible ^= true;
	}
	updateList();
}

/**
* Check if specified parent has any children.
*
* @param parentId Id of parent to check.
*/
bool BaseInfoDetailsState::parentHasChildren(int parentId)
{
	auto bean = std::find_if(_details.begin(), _details.end(),
		[&](const BeanCounter row)
		{return row.parentId == parentId && row.childId != row.parentId;}
		);
	if (bean == _details.end())
	{
		return false;
	}
	return true;
}

/**
* Calculate sum of children's `amount` field.
*
* @param parentId Id of parent.
* @return The sum of all children's `amounts` fields.
*/
int BaseInfoDetailsState::calculateSumOfChildrenAmountField(int parentId)
{
	int amount = 0;
	for (auto element : _details)
	{
		if (element.parentId == parentId && element.childId != element.parentId)
		{
			amount += element.amount;
		}
	}
	return amount;
};

/**
* Calculate sum of children's `baseValue` field.
*
* @param parentId Id of parent.
* @return The sum of all children's `baseValue` fields.
*/
int BaseInfoDetailsState::calculateSumOfChildrenValueField(int parentId)
{
	int total = 0;
	for (auto element : _details)
	{
		if (element.parentId == parentId && element.childId != element.parentId)
		{
			total += element.baseValue * element.amount; // works for percentage based?
		}
	}
	return total;
};

/**
 * Returns the max baseValue of children's `baseValue` field.
 *
 * @param parentId Id of parent.
 * @return The max baseValue of all children's `baseValue` fields.
 */

int BaseInfoDetailsState::calculateMaxOfChildrenValueField(int parentId)
{
	int total = 0;
	for (auto element : _details)
	{
		if (element.parentId == parentId && element.childId != element.parentId)
		{
			total = std::max(total, element.baseValue);
		}
	}
	return total;
};

/**
 * Setup and draw the screen's body.
 *  * Screen title
 *  * List contents (and 'headers')
 */
void BuildFacilitiesDetailsState::drawBody()
{
	_details.clear();
	_lstDetails->clearList();

	if (_tabRequirements->getPressed())
	{
	 	//_lstDetails->setColumns(3, 155+15, 45, 70-15);
		//_txtQuantity->setVisible(true);
		_txtResult->setText(tr("STR_VALUE"));
		//_btnQueuedFacilities->setVisible(true);
		//setupTabRequirements();
	}
	else
	{
		//_lstDetails->setColumns(2, 155+15+45, 70-15);
		//_txtQuantity->setVisible(false);
		_txtResult->setText(tr("STR_BFDS_SOLVABLE_BY"));
		//_btnQueuedFacilities->setVisible(false);
		setupTabBlockers();
	}

	updateList();
}

// /**
//  * Creates all placeholder elements.
// */
// void BaseInfoDetailsState::setupTabRequirements()
// {
// 	std::ostringstream ssTitle, screenTotal;
// 	// Do not end sentence with '.'
// 	// Automatic font scaling "setText()" does not like that (Cause: b1b6f9ae).
// 	ssTitle << "Category " << enum2string(_activeTab) << " not implemented yet";
// 	screenTotal << tr("STR_TOTAL") << ">\t" << Unicode::formatFunding(999'999'999'999);
// 	_txtTitle->setText(ssTitle.str());
// 	_txtTotal->setText(screenTotal.str());

// 	BeanCounter row;
// 	size_t parent, childId = 0;
// 	for (auto i = 0; i < 5; i++)
// 	{
// 		parent = childId;
// 		row = {childId, parent, "Long text explaining the source", 999, 999'999'999, true};
// 		_details.push_back(row);
// 		childId++;
// 		for (auto j = 0; j < 5; j++)
// 		{
// 			row = {childId, parent, "Normally collapsed (moaar details)", 99, 999'999'999, false};
// 			_details.push_back(row);
// 			childId++;
// 		}
// 	}
// 	updateList();
// }

/**
 * Setup list showing the reasons for blocking the build of this facility.
 *
 * @remark
 * Mostly subcategories are based on:
 * + PlaceFacilityState::viewClick()
 * + BaseView::getPlacementError()
 */
void BuildFacilitiesDetailsState::setupTabBlockers()
{
	// _txtTitle->setText(tr("STR_BIDS_CATEGORY_DETECTION"));
	// _txtTotal->setText("");

	// if (_tabRequirements->getPressed())
	// {
	// }
	// else
	// {
	// }

	subcategoryBlockedByCountry();
	subcategoryBlockedByRegion();
	subcategoryBlockedByFacilities();
	subcategoryBlockedByRequiredItems();
	subcategoryBlockedByFunds();
}

/**
 * Setup and add "blocked by country" to `_details` vector.
 */
void BuildFacilitiesDetailsState::subcategoryBlockedByCountry()
{
	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	if (_countryRule)
	{
		description = tr("STR_BFDS_BLOCKER_BY_COUNTRY")
			.arg(tr(_countryRule->getType()));
		bean = {0, parentId, description , 0, 0, true};
		subCategory.push_back(bean);

		if ((_countryRule->getForbiddenBaseFunc() & _providedFacFunc).any())
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_FORBIDDEN_BY_COUNTRY")
				.arg(tr(_countryRule->getType()));
			bean = {0, parentId, description , 0, 0, false};
			subCategory.push_back(bean);
		}
		if ((_countryRule->getProvidedBaseFunc() & _forbiddenFacFunc).any())
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_NOT_COMPATIBLE_WITH_COUNTRY")
				.arg(tr(_countryRule->getType()));
			bean = {0, parentId, description , 0, 0, false};
			subCategory.push_back(bean);
		}
		// Service only provided by some another country.
		if ((_countriesOnlyServices & _requiredFacFunc & ~_countryRule->getProvidedBaseFunc()).any())
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_MISSING_SERVICE_COUNTRY")
				.arg(tr(_countryRule->getType()));
			bean = {0, parentId, description , 0, 0, false};
			subCategory.push_back(bean);
		}
		add2_detailsVector(subCategory);
	}
	else if ((_countriesOnlyServices & _requiredFacFunc).any())
	{
		// Base is not build inside a country.
		description = tr("STR_BFDS_BLOCKER_NO_COUNTRY_MISSING_SERVICE");
		bean = {0, parentId, description , 0, 0, true};
		subCategory.push_back(bean);
		add2_detailsVector(subCategory, true);
	}
}

/**
 * Setup and add "blocked by region" to `_details` vector.
 */
void BuildFacilitiesDetailsState::subcategoryBlockedByRegion()
{
	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	if (_regionRule)
	{
		description = tr("STR_BFDS_BLOCKER_BY_REGION")
			.arg(tr(_regionRule->getType()));
		bean = {0, parentId, description , 0, 0, true};
		subCategory.push_back(bean);

		if ((_regionRule->getForbiddenBaseFunc() & _providedFacFunc).any())
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_FORBIDDEN_BY_REGION")
				.arg(tr(_regionRule->getType()));
			bean = {0, parentId, description , 0, 0, false};
			subCategory.push_back(bean);
		}
		if ((_regionRule->getProvidedBaseFunc() & _forbiddenFacFunc).any())
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_NOT_COMPATIBLE_WITH_REGION")
				.arg(tr(_regionRule->getType()));
			bean = {0, parentId, description , 0, 0, false};
			subCategory.push_back(bean);
		}
		// Service only provided by some another country.
		if ((_regionsOnlyServices & _requiredFacFunc & ~_regionRule->getProvidedBaseFunc()).any())
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_MISSING_SERVICE_REGION")
				.arg(tr(_regionRule->getType()));
			bean = {0, parentId, description , 0, 0, false};
			subCategory.push_back(bean);
		}
		add2_detailsVector(subCategory);
	}
	else if ((_regionsOnlyServices & _requiredFacFunc).any())
	{
		// Base is not build inside a region.
		description = tr("STR_BFDS_BLOCKER_NO_REGION_MISSING_SERVICE");
		bean = {0, parentId, description , 0, 0, true};
		subCategory.push_back(bean);
		add2_detailsVector(subCategory, true);
	}
}

/**
 * Setup and add "blocked by existing facilities" to `_details` vector.
 */
void BuildFacilitiesDetailsState::subcategoryBlockedByFacilities()
{
	size_t parentId = 0; // Will be set later.
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	for (auto& baseFac : _baseFacilitiesAndCount)
	{
		parentId = _details.size();
		description = tr("STR_BFDS_BLOCKER_BY_FACILITY")
			.arg(tr(baseFac.first->getType()));
		bean = {0, parentId, description , 0, 0, true};
		subCategory.push_back(bean);

		bool canRenovate = _facRuleSelected->getCanBuildOverOtherFacility(baseFac.first) == BPE_None;
		if (baseFac.first == _facRuleSelected && _facRuleSelected->getMaxAllowedPerBase() > 0 &&
			baseFac.second >= _facRuleSelected->getMaxAllowedPerBase())
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_FORBIDDEN_BY_AMOUNT")
				.arg(_facRuleSelected->getMaxAllowedPerBase())
				.arg(baseFac.second);
			bean = {0, parentId, description , baseFac.second, 1, false};
			// Can always be solved.
			// No need to tell player, since it involves:
			// * Removal of existing facility and rebuild.
			// * Renovation of existing facility with same one.
			// A Great way to burn money, nothing more.
			subCategory.push_back(bean);
		}
		if ((baseFac.first->getForbiddenBaseFunc() & _providedFacFunc).any())
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_FORBIDDEN_SERVICE_BY_EXISTING")
				.arg(tr(_facRuleSelected->getType()));
			bean = {0, parentId, description , 0, 1, false};
			subCategory.push_back(bean);
		}
		if ((baseFac.first->getProvidedBaseFunc() & _forbiddenFacFunc).any())
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_FORBIDDEN_SERVICE_BY_NEW")
				.arg(tr(_facRuleSelected->getType()));
			bean = {0, parentId, description , 0, 1, false};
			subCategory.push_back(bean);
		}
		// Only include possible workaround if there is an actual error.
		// No need to list itself, renovation has no added value.
		if (canRenovate && subCategory.size() > 1 && baseFac.first != _facRuleSelected)
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_FORBIDDEN_RENOVATION_ALLOWED");
			bean = {0, parentId, description , 0, 1, false};
			subCategory.push_back(bean);
		}
		add2_detailsVector(subCategory);
		subCategory.clear();
	}
}

/**
 * Setup and add "blocked by existing facilities" to `_details` vector.
 */
void BuildFacilitiesDetailsState::subcategoryBlockedByRequiredItems()
{
	if (_facRuleSelected->getBuildCostItems().empty()) return;

	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	description = tr("STR_BFDS_BLOCKER_BY_ITEMS");
	bean = {0, parentId, description , 0, 0, true};
	subCategory.push_back(bean);

	// Start by listing all missing items
	std::map<const std::string, int> missingItems;
	for (auto& item : _facRuleSelected->getBuildCostItems())
	{
		int needed = item.second.first - _base->getItemCountStorage(item.first);
		if (needed <= 0) continue;

		description = tr("STR_BFDS_BLOCKER_DETAIL_MISSING_ITEMS_AMOUNT")
			.arg(needed)
			.arg(tr(item.first));
		bean = {0, parentId, description , 0, 1, false};
		subCategory.push_back(bean);

		missingItems.insert(std::make_pair(item.first, needed));
	}
	// List all facilities where renovation solves missing items.
	// Not considering if a facility might be queued:
	// + Would easily lead to false expectations if workaround
	//   is only valid for queued facilities.
	for (auto& baseFac : _baseFacilitiesAndCount)
	{
		if (missingItems.empty()) break; // Loop has no value, this trick reduces indentation.
		// No need to list itself, renovation has no added value.
		if (baseFac.first == _facRuleSelected) continue;

		bool canFix = _facRuleSelected->getCanBuildOverOtherFacility(baseFac.first) == BPE_None;
		auto& buildCosts = baseFac.first->getBuildCostItems();
		for (auto& item : missingItems)
		{
			if (canFix == false) break;
			
			if (auto search = buildCosts.find(item.first); search != buildCosts.end())
			{
				canFix &= search->second.second >= item.second; // Refund value bigger than missing
			}
			else
			{
				canFix = false;
			}
		}
		if (canFix)
		{
			description = tr("STR_BFDS_BLOCKER_DETAIL_MISSING_ITEMS_RENOVATION_ALLOWED")
				.arg(tr(baseFac.first->getType()));
			bean = {0, parentId, description , 0, 1, false};
			subCategory.push_back(bean);
		}
	}
	add2_detailsVector(subCategory);
}


/**
 * Setup and add "blocked by existing facilities" to `_details` vector.
 */
void BuildFacilitiesDetailsState::subcategoryBlockedByFunds()
{
	int missingFunds = _facRuleSelected->getBuildCost() - _game->getSavedGame()->getFunds();
	if (missingFunds <= 0)
		return;
	
	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	description = tr("STR_BFDS_BLOCKER_BY_FUNDS");
	bean = {0, parentId, description , 0, 0, true};
	subCategory.push_back(bean);

	description = tr("STR_BFDS_BLOCKER_DETAIL_MISSING_FUNDS")
		.arg(Unicode::formatFunding(missingFunds));
	bean = {0, parentId, description , 0, 1, false};
	subCategory.push_back(bean);

	add2_detailsVector(subCategory);
}

// SOME old snippet
// {
// 	// Prefer alphabetical listing of named facilities.
// 	sortChildrenByDescription(subCategory);
// 	add2_detailsVector(subCategory);

// 	// TRIVIA:
// 	// If total 'mindpower' >= 21 base cannot be found by UFO's, due to integer math.
// }

// /**
//  * Setup and add UFO detection capabilities to `_details` vector.
//  *
//  * + The (per range limit) probability of detection.
//  * + The (per range limit) HyperWave/Transmission Resolver functionality.
//  *
//  * @remark
//  * Ruleset variables: `radarRange`, `radarChance` and `hyper`.
//  *
//  * @note
//  * Each range based subtotal shows the combined probability
//  * of all facilities contributing to that range.
//  *
//  * @remark
//  * Detection probability = (1- chance_of_not_detecting)^no_of_facilities_participating
//  */
// void BaseInfoDetailsState::subcategoryUfoDetection()
// {
// 	// Intention is to show detection chance per unique range.
// 	int hyperMaxRange = 0;
// 	std::set<int> radarRanges;
// 	for (auto *facility : *_base->getFacilities())
// 	{
// 		if (facility->getBuildTime() > 0 && !_btnQueuedFacilities->getPressed())
// 			continue;

// 		if (facility->getRules()->getRadarRange() == 0)
// 			continue;

// 		int currentRange = facility->getRules()->getRadarRange();
// 		if (facility->getRules()->isHyperwave() && currentRange > hyperMaxRange)
// 		{
// 			hyperMaxRange = currentRange;
// 		}
// 		radarRanges.insert(currentRange);
// 	}

// 	// Ufo detection per range limit.
// 	for (auto detectionRange : radarRanges)
// 	{
// 		std::vector<BeanCounter> subCategory;
// 		size_t childId, parentId = _details.size(); // `childId` is set later.
// 		BeanCounter row;
// 		std::string description;

// 		// Sub category header (a.k.a. subtotal).
// 		if (detectionRange <= hyperMaxRange)
// 		{
// 			description = tr("STR_BIDS_TITLE_UFO_DETECTION_HYPERWAVE").arg(detectionRange);
// 		}
// 		else
// 		{
// 			description = tr("STR_BIDS_TITLE_UFO_DETECTION_RADAR").arg(detectionRange);
// 		}
// 		row = {parentId, parentId, description, 0, 0, true};
// 		subCategory.push_back(row);

// 		for (auto *facility : *_base->getFacilities())
// 		{
// 			if (facility->getBuildTime() > 0 && !_btnQueuedFacilities->getPressed())
// 				continue;

// 			if (facility->getRules()->getRadarRange() < detectionRange)
// 				continue;

// 			// Facility detection chance
// 			int detectionChance = facility->getRules()->getRadarChance();
// 			if (facility->getRules()->isHyperwave())
// 			{
// 				description = tr("STR_BIDS_DETAIL_UFO_DETECTION_HYPERWAVE").arg(tr(facility->getRules()->getType()));
// 			}
// 			else
// 			{
// 				description = tr(facility->getRules()->getType());
// 			}
// 			childId = subCategory.size() + parentId;
// 			row = {childId, parentId, description , 1, detectionChance, false};
// 			add2vector(subCategory, row);
// 		}

// 		// Apply field overrides where necessary.
// 		for (auto &element : subCategory)
// 		{
// 			float detectionChance = 0.0;
// 			if (element.childId == element.parentId)
// 			{
// 				detectionChance = calcProbabilityAtLeastOne(subCategory);
// 				element.amount = countContributingFacilities(subCategory);
// 			}
// 			else if (element.baseValue == 0)
// 			{
// 				// "0" is a valid detection chance for a hyperwave that only tracks.
// 			}
// 			else
// 			{
// 				detectionChance = calcProbabilityAtLeastOne(element.baseValue, element.amount);
// 			}
// 			element.valueOverride = Unicode::formatPercentage(std::round(100*detectionChance));
// 		}

// 		// Prefer alphabetical listing of named facilities.
// 		sortChildrenByDescription(subCategory);
// 		add2_detailsVector(subCategory);
// 	}
// }

// /**
//  * Setup and add alien base detection capabilities to `_details` vector.
//  *
//  * The (per range limit) probability of detection.
//  *
//  * @remark
//  * Ruleset variables: `sightRange`, `sightChance`.
//  *
//  * @note
//  * Each range based subtotal shows the combined probability
//  * of all facilities contributing to that range.
//  *
//  * @remark
//  * Detection probability = (1- chance_of_not_detecting)^no_of_facilities_participating
//  * If dynamic detection use 2 ranges @100%ofRange (chance is 0%) and @50%ofRange (chance is 50%)
//  *
//  * @remark
//  * Not sure about this one: one could argue it should be a hidden stat.
//  */
// void BaseInfoDetailsState::subcategoryAlienBaseDetection()
// {
// 	// Intention is to show chance per unique sight range.
// 	std::set<int> sightRanges;
// 	for (auto *facility : *_base->getFacilities())
// 	{
// 		if (facility->getBuildTime() > 0 && !_btnQueuedFacilities->getPressed())
// 			continue;

// 		if (facility->getRules()->getSightRange() == 0)
// 			continue;

// 		int sightRange = facility->getRules()->getSightRange();
// 		sightRanges.insert(sightRange);
// 		// For dynamic ranges the chance at max range chance = 0%
// 		// No need for adding extra ranges to help player deduce.
// 		// A facility showing "0%" at max range should suffice.
// 	}

// 	// Alien base detection per range limit.
// 	for (auto detectionRange : sightRanges)
// 	{
// 		std::vector<BeanCounter> subCategory;
// 		size_t childId, parentId = _details.size(); // `childId` is set later.
// 		BeanCounter row;
// 		std::string description;

// 		// Sub category header (a.k.a. subtotal).
// 		description = tr("STR_BIDS_TITLE_ALIEN_BASE_DETECTION").arg(detectionRange);
// 		row = {parentId, parentId, description, 0, 0, true};
// 		subCategory.push_back(row);

// 		for (auto *facility : *_base->getFacilities())
// 		{
// 			if (facility->getBuildTime() > 0 && !_btnQueuedFacilities->getPressed())
// 				continue;

// 			if (facility->getRules()->getSightRange() < detectionRange)
// 				continue;

// 			// Facility detection chance
// 			int detectionChance = facility->getRules()->getSightChance();
// 			if (detectionChance == 0)
// 			{
// 				// Dynamic, e.g. 0%-50% based on distance.
// 				// Formula from `GeoscapeState::time1Day()`:
// 				// `chanceToDetect = 50 - (distance * 50 / facility->getRules()->getSightRange())`
// 				// For this subroutine we can use: distance = `detectionRange`.
// 				detectionChance = 50 - (detectionRange * 50) / facility->getRules()->getSightRange();
// 			}
// 			childId = subCategory.size() + parentId;
// 			description = tr(facility->getRules()->getType());
// 			row = {childId, parentId, description , 1, detectionChance, false};
// 			add2vector(subCategory, row);
// 		}

// 		// Apply field overrides where necessary.
// 		for (auto &element : subCategory)
// 		{
// 			float detectionChance = 0.0;
// 			if (element.childId == element.parentId)
// 			{
// 				detectionChance = calcProbabilityAtLeastOne(subCategory);
// 				element.amount = countContributingFacilities(subCategory);
// 			}
// 			else
// 			{
// 				detectionChance = calcProbabilityAtLeastOne(element.baseValue, element.amount);
// 			}
// 			element.valueOverride = Unicode::formatPercentage(std::round(100*detectionChance));
// 		}

// 		// Prefer alphabetical listing of named facilities.
// 		sortChildrenByDescription(subCategory);
// 		add2_detailsVector(subCategory);
// 	}
// }



/**
* Draw (en filter) the current details list.
*/
void BuildFacilitiesDetailsState::updateList()
{
	_lstDetails->clearList();
	_rows.clear();

	if (_details.size() == 0)
	{
		_lstDetails->addRow(2, tr("STR_EMPTY_LIST").c_str(), "");
		_rows.push_back(0);
		_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), _lstDetails->getSecondaryColor());
		return;
	}

	auto colorPrimary = _lstDetails->getColor();
	auto colorSecondary = _lstDetails->getSecondaryColor();
	auto colorTertiary = _lstDetails->getScrollbarColor();
	//auto colorTertiary = _game->getMod()->getInterface("buildFacilitiesDetails")->getElement("list")->border;
	for (size_t i = 0; i < _details.size(); ++i)
	{
		if (!_details[i].isRowVisible) continue;

		std::string description = _details[i].description;
		std::ostringstream ssAmount, ssValue;
		//bool unconditionallyShowSign = true;
		if (_details[i].parentId != i) // A child row.
		{
			description.insert(0, " "); // Do not use dots for description indentation.
			// ssAmount << tr("STR_DOTTED_INDENTATION");
			// ssValue << tr("STR_DOTTED_INDENTATION");
			//ssAmount << " ";
			//ssValue << " ";
			//unconditionallyShowSign = false;
		}

		if (_details[i].valueOverride != "")
		{
			ssValue << _details[i].valueOverride;
		}
		else
		{
			ssValue << _details[i].baseValue;
		}

		// // Test for Last column(s) align right
		// if (_details[i].parentId != _details[i].childId) // A child row.
		// {
		// 	description.insert(0, " ");
		// }
		// else
		// {
		// 	ssAmount << " ";
		// 	ssValue << " ";
		// }
		// // End Test

		if (_details[i].parentId != i)
		{
			_lstDetails->setSecondaryColor(colorTertiary);
			_lstDetails->addRow(2, description.c_str(), ssValue.str().c_str());
			_lstDetails->setSecondaryColor(colorSecondary);
		}
		else
		{
			_lstDetails->addRow(2, description.c_str(), ssValue.str().c_str());
		}
		_rows.push_back(i);

		if(_details[i].parentId == i)
		{
			_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), colorSecondary);
			//_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), _game->getMod()->getInterface("baseInfoDetails")->getElement("list")->border);
		}
	}
}

/**
* Adds another contribution to the `_details` vector.
*
* Creates a new entry if needed, updates if an entry already exist.
* @note
* An entry is defined by the unique combination of fields:
*`parentId`, `description` and `valueOverride`.
*
* @param row                      The contents of the row we want to insert.
* @param updateExistingValueField Are we allowed to update the `baseValue` field on existing entries.
* @return Unique identifier for the next element in the list (!not the vector's rowid!).
*/
int BaseInfoDetailsState::addToDetailsVector(BeanCounter row, bool updateExistingValueField)
{
	for (auto &bean : _details)
	{
		if (bean.parentId == row.parentId && bean.description == row.description &&
			bean.valueOverride == row.valueOverride)
		{
			bean.baseValue += row.baseValue * updateExistingValueField; // Branchless programming trick.
			// No checking if bean.amount > -1.
			// It is callers responsibility to supply correct values.
			// Ensures any implementation faults become a bit more visible (weird numbers on screen).
			bean.amount += row.amount;

			return row.childId;
		}
	}
	_details.push_back(row);
	return ++row.childId;
}

/**
* Adds a unique row to the given (temporal) subcategory vector.
*
* Creates a new entry if needed, updates existing one where possible.
*
* @note
* Row uniqueness is defined by the combination of the fields:
*`parentId`, `description` and `baseValue`.
*
* @param subCategory Vector to operate on (`_details` is allowed but not recommend).
* @param row         The contents of the row we want to insert (or update).
*/
void BaseInfoDetailsState::add2vector(std::vector<BeanCounter> &subCategory, BeanCounter row)
{
	for (auto &bean : subCategory)
	{
		// Could probably get away with check on description only.
		// Others are for the (unlikely) case method is called on `_details`.
		if (bean.parentId == row.parentId &&
			bean.baseValue == row.baseValue &&
			bean.description == row.description)
		{
			bean.amount += row.amount;
			return;
		}
	}
	subCategory.push_back(row);
	return;
}

/**
* Adds given subcategory vector to screen's global `_details` vector.
*
* @param subCategory Vector to add to `_details` vector
* @param forceInclude Include even if parent has no children.
*/
void BuildFacilitiesDetailsState::add2_detailsVector(std::vector<BeanCounter> &subCategory, bool forceInclude)
{
	if (subCategory.size() < 2 && !forceInclude)
		return;

	_details.insert(_details.end(), subCategory.begin(), subCategory.end());
}

/**
* Alphabetical sort of children (by description)
*
* @param subCategory The method's local `_details` slice copy to work on.
* @param skipChildren Number of starting child rows to be left untouched.
*/
void BaseInfoDetailsState::sortChildrenByDescription(std::vector<BeanCounter> &subCategory, size_t skipChildren)
{
	// No children
	if (subCategory.size() < 2 + skipChildren) return;

	std::stable_sort(std::next(subCategory.begin(), 1 + skipChildren), subCategory.end(),
		[](const BeanCounter a, const BeanCounter b)
		{ return Unicode::naturalCompare(a.description, b.description); }
	);
}

/**
* Calculate the amount of facilities contributing to the given subCategory.
*
* @param subCategory The method's local `_details` slice copy to work on.
* @return The amount of facilities
*/
int BaseInfoDetailsState::countContributingFacilities(std::vector<BeanCounter> &subCategory)
{
	int count = 0;
	for (auto bean : subCategory)
	{
		// By default the first element is meant to display the result of this method.
		// Hence skip this first one by default.
		if (bean.parentId == bean.childId) continue; // not a child

		count += bean.amount;
	}
	return count;
}

/**
* Calculate the probability of getting a success for the given subCategory.
*
* @remark
* For detection we only need one success.
* Assuming independent checks (which seem to be true):
* - P_success = 1 - P_all_failed
* - P_all_failed = P_check1failed * P_check2failed * ... * P_checkNfailed.
* - P_check#failed = 1 - P_check_success
* - P_check_success = chance_value/100
*
* @param subCategory The method's local `_details` slice copy to work on.
* @return The probability of success (0 < Psucces < 1).
*/
double BaseInfoDetailsState::calcProbabilityAtLeastOne(std::vector<BeanCounter> &subCategory)
{

	double detectionFail = 1.0;
	for (auto bean : subCategory)
	{
		// By default the first element is meant to display the result of this method.
		// Hence skip this first one by default.
		if (bean.parentId == bean.childId) continue; // not a child

		detectionFail *= (1.0 - calcProbabilityAtLeastOne(bean.baseValue, bean.amount));
	}
	// Always round down (even if that results in non probability "0").
	return 1.0 - detectionFail;
}

/**
* Calculate the probability of getting a success.
*
* @remark
* P_success = 1 - P_all_failed
*           = 1 - (P_facility1fails * P_facility1fails + ... + PfacilityNfails)
* Since all tries have the same baseChange the formula becomes:
* P_success = 1 - (1 - P_baseChance)^tries
*
* @param baseChance The success chance for each try (e.g. detection chance).
* @param tries      Number of facilities contributing to this calculation.
* @return The probability of success (0 =< Psuccess < 1, yup ... 0 is allowed in this context).
*/
double BaseInfoDetailsState::calcProbabilityAtLeastOne(int baseChance, int tries)
{
	if (baseChance <= 0) return 0.0;

	return 1.0 - std::pow(1.0 - baseChance/100.0, tries);
}

/**
 * Update sets with category related facilities unable to build at this base.
 *
 * @remark
 * A building is blocked from building under the following circumstances.
 * + It is forbidden (`_forbiddenFacilities`)
 *   - Provides a "service" forbidden on this base.
 *   - Forbids a "service" already present on base.
 *   - Base has reached it's limit for this building.
 * + It misses some requirement (`_missingReqsFacilities`)
 *   - Depends on a service not present on this base.
 *   - Required items are not present on base.
 *
 * @note
 * Don't forget provided and missing services can depend on country / region (`Base::calculateServices()`)
 * This is already included when asking base to calculate those (`base->get*BaseFunc({})`)!
 *
 * @note
 * This method does not take `buildOver` into account.
 * We want to list all buildings that cannot be build directly.
 * Let list fill method (`subcategoryBlockedFacility()`) handle `buildOver`.
 */
void BaseInfoDetailsState::updateBlockedFacilitiesSets()
{
	_forbiddenFacilities.clear();
	_missingReqsFacilities.clear();

	auto provBaseFunc = _providedBaseFunc;
	if (_btnQueuedFacilities->getPressed())
	{
		provBaseFunc = _futureBaseFunc;
	}

	// Get all (known) facilities which cannot be build (anymore) on this base.
	// Based on: BuildFacilitiesState::populateBuildList()
	for (auto &facilityType : _game->getMod()->getBaseFacilitiesList())
	{
		RuleBaseFacility *rule = _game->getMod()->getBaseFacility(facilityType);

		if (!isFacilityPartOfScreenCategory(rule))
			continue;
		if (!rule->isAllowedForBaseType(_base->isFakeUnderwater()))
			continue;
		if (!_game->getSavedGame()->isResearched(rule->getRequirements()))
			continue;

		RuleBaseFacilityFunctions req = rule->getRequireBaseFunc();
		RuleBaseFacilityFunctions forb = rule->getForbiddenBaseFunc();
		RuleBaseFacilityFunctions prov = rule->getProvidedBaseFunc();
		if ((~provBaseFunc & req).any())
		{
			_missingReqsFacilities.insert(rule);
		}
		// Might as well recognize item requirements (`PlaceFacilityState::viewClick()`)
		for (const auto& item: rule->getBuildCostItems())
		{
			int needed = item.second.first - _base->getItemCountStorage(item.first);
			if (needed > 0)
			{
				_missingReqsFacilities.insert(rule);
				break;
			}
		}

		// Following blockers always take queued facilities into account.
		if (_base->isMaxAllowedLimitReached(rule))
		{
			_forbiddenFacilities.insert(rule);
		}
		else if ((_forbiddenBaseFunc & prov).any())
		{
			_forbiddenFacilities.insert(rule);
		}
		else if ((provBaseFunc & forb).any())
		{
			_forbiddenFacilities.insert(rule);
		}
	}
}

/**
* Does this facility contribute to current screen category.
*
* @param rule Pointer to facility ruleset
* @return Whether this facility contributes to current screen category.
*/
bool BaseInfoDetailsState::isFacilityPartOfScreenCategory(RuleBaseFacility *rule)
{
	if (!rule) return false;

	bool result = true;
	switch (_category)
	{
	// case BaseInfoDetailsCategory::SOLDIERS:
	//	break;
	// case BaseInfoDetailsCategory::ENGINEERS:
	// 	break;
	// case BaseInfoDetailsCategory::SCIENTISTS:
	// 	break;
	// case BaseInfoDetailsCategory::QUARTERS:
	// 	break;
	// case BaseInfoDetailsCategory::STORES:
	// 	break;
	// case BaseInfoDetailsCategory::LABORATORIES:
	// 	break;
	// case BaseInfoDetailsCategory::WORKSHOPS:
	// 	break;
	// case BaseInfoDetailsCategory::CONTAINMENT:
	// 	break;
	// case BaseInfoDetailsCategory::HANGARS:
	// 	break;
	// case BaseInfoDetailsCategory::DEFENSE:
	// 	break;
	case BaseInfoDetailsCategory::DETECTION:
		result &= (rule->isMindShield() || rule->getRadarRange() > 0 || rule->getSightRange() > 0);
		break;
	default:
		result = false;
	}
	return result;
}

}
