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
 */
BuildFacilitiesDetailsState::BuildFacilitiesDetailsState(Base *base, RuleBaseFacility *currentFacility) :
	_base(base), _facRuleSelected(currentFacility)
{
	_screen = false;

	// Window size and position chosen to keep base funds just out of view
	// and have window centered vertically to base grid.
	_window = new Window(this, 320, 200-24-16, 0, 24, POPUP_BOTH);
	_txtTitle = new Text(280, 16, 20, 24+8);
	_txtDescription = new Text(220-5, 9, 19, 32+16+3);
	_txtOnBase = new Text(54+5, 9, 240-5, 32+16+3);
	_lstDetails = new TextList(272, 96, 23, 51+9+2); // Height: 12*8 = 96 (8 due to rowheight overlap using default rules).
	_btnOk = new TextButton(288, 16, 16, 62+96+2);   // Good alignment w.r.t. bottom of window

	// Set palette
	setInterface("buildFacilitiesDetails");

	add(_window, "window", "buildFacilitiesDetails");
	add(_txtTitle, "text", "buildFacilitiesDetails");
	add(_txtDescription, "text", "buildFacilitiesDetails");
	add(_txtOnBase, "text", "buildFacilitiesDetails");
	add(_lstDetails, "list", "buildFacilitiesDetails");
	add(_btnOk, "button", "buildFacilitiesDetails");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "buildFacilitiesDetails");

	_txtTitle->setText(tr("STR_BFDS_TITLE").arg(tr(_facRuleSelected->getType())));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtDescription->setText(tr("STR_BFDS_COLUMN_HEADER_DESCRIPTION"));
	_txtOnBase->setText(tr("STR_BFDS_COLUMN_HEADER_ONBASE"));
	_lstDetails->setColumns(2, 210, 60); // Last column allows for "$999 999 999", after indentation.
	_lstDetails->setSelectable(true);    // Required for collapse/fold functionality.
	_lstDetails->setBackground(_window);
	_lstDetails->setScrolling(true);
	_lstDetails->setMargin(2);           // Shifts **all** columns 2px to the right.
	_lstDetails->onMousePress((ActionHandler)&BuildFacilitiesDetailsState::lstDetailsMousePress, SDL_BUTTON_LEFT);
	//_lstDetails->onMousePress((ActionHandler)&BuildFacilitiesDetailsState::lstDetailsMousePress, SDL_BUTTON_RIGHT);
	//_lstDetails->setWordWrap(true);
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
	// _validFacilties = ?? // List of valid facilities (rules pointer) for this base (to have a shorter list to operate on).

	setupListContents();
	updateList();
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
* Handles mouse-clicks on the list rows.
*/
void BuildFacilitiesDetailsState::lstDetailsMousePress(Action *)
{
	_sel = _lstDetails->getSelectedRow(); // Needed for `getRow()`.
	auto curScroll = _lstDetails->getScroll();

	// Flip visibility of child elements.
	for (size_t i = 0; i < _details.size(); ++i)
	{
		if (_details[i].parentId == i) continue;
		if (_details[i].parentId != getRow().parentId) continue;

		_details[i].isRowVisible ^= true;
	}
	updateList();

	// Good enough taking into account list can have arbitrary sizes due to (un)folding.
	_lstDetails->scrollTo(curScroll);
}

/**
* Setup `_details` vector with this facilities requirements/blockers.
*
* @remark
* This includes facilities which cannot coexist due to service blockers.
* Perhaps even countries/regions that are not compatible.
*/
void BuildFacilitiesDetailsState::setupListContents()
{
	categoryReqsFunds();
	categoryReqsItems();
	categoryReqsServices();
	addSeparatorLine();

	// All potential build blockers
	auto oldListSize = _details.size();
	categoryLimitedByAmount();
	categoryProvidesServicesBlockedByOthers();
	categoryBlocksServicesProvidedByOthers();
	if (oldListSize < _details.size()) // Prevent double blanc lines
	{
		addSeparatorLine();
	}

	// Diverse
	oldListSize = _details.size();
	categoryCanRenovate();
	// Ensure children of last category become visible when unfolding a big list.
	// Otherwise players has to derive their existence from the scrollbar.
	if (oldListSize < _details.size())
	{
		addSeparatorLine();
	}
}

/**
 * Insert an empty line
 */
void BuildFacilitiesDetailsState::addSeparatorLine()
{
	size_t parentId = _details.size();
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	bean = {0, parentId, "" , 0, 0, true};
	bean.valueOverride = " "; // Space is needed for `updateList()` to recognize this field must be empty.

	subCategory.push_back(bean);
	add2_detailsVector(subCategory, true);
}

/**
 * Setup and add "Required funds" to `_details` vector.
 *
 * @remark
 * Not taking into account refund values for buildover of existing facilities.
 * It is outside the scope of this list to show affected facilities.
 * @remark
 * If the desire arises though this category is the most suitable place.
 * In that case introduce renovation cost *per allowed facility*.
 */
void BuildFacilitiesDetailsState::categoryReqsFunds()
{
	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	int requiredFunds = _facRuleSelected->getBuildCost();
	// No need to check on sign and corresponding cases, or zero.
	// Not useful and player can easily deduce this from value in description
	description = tr("STR_REQUIRED_FUNDS").arg(Unicode::formatFunding(requiredFunds));
	bean = {0, parentId, description , 0, 0, true};
	bean.valueOverride = Unicode::formatFunding(_game->getSavedGame()->getFunds());

	subCategory.push_back(bean);
	add2_detailsVector(subCategory, true);
}

/**
 * Setup and add "Required items" to `_details` vector.
 *
 * @remark
 * Not taking into account item refund values for buildover of existing facilities.
 * It is outside the scope of this list to show affected facilities.
 * @remark
 * If the desire arises though this category is the most suitable place.
 * In that case introduce renovation cost *per allowed facility*.
 */
void BuildFacilitiesDetailsState::categoryReqsItems()
{
	if (_facRuleSelected->getBuildCostItems().empty()) return;

	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	description = tr("STR_BFDS_REQUIRED_ITEMS_CAT");
	bean = {0, parentId, description , 0, 0, true};
	subCategory.push_back(bean);

	bool allItemRequirementsMet = true;
	for (auto& item : _facRuleSelected->getBuildCostItems())
	{
		int needed = item.second.first;
		int available = _base->getItemCountStorage(item.first);
		allItemRequirementsMet &= available >= needed;

		description = tr("STR_BFDS_REQUIRED_ITEMS_DETAIL")
			.arg(tr(item.first))
			.arg(needed);
		bean = {0, parentId, description , 0, available, false};
		subCategory.push_back(bean);
	}

	if (allItemRequirementsMet)
	{
		subCategory.front().valueOverride = "Yes";
	}
	else
	{
		subCategory.front().valueOverride = "No";
	}

	sortChildrenByDescription(subCategory);
	add2_detailsVector(subCategory);
}

/**
 * Setup and add "Required services" to `_details` vector.
 *
 * @note
 * Lists all known ones regardless if possible for *this* base.
 */
void BuildFacilitiesDetailsState::categoryReqsServices()
{
	if (_requiredFacFunc.none()) return;

	BeanCounter bean;
	auto childValueOverride = [&bean](bool testResult) -> void
	{
		bean.valueOverride = testResult ? "Yes" : "No";
	};

	// Show each required service as separate subcategory.
	// Let children show their providers (regardless if allowed on this base).
	for (size_t i = 0; i < _requiredFacFunc.size(); ++i)
	{
		if (!_requiredFacFunc.test(i)) continue;

		size_t parentId = _details.size();
		std::string description;
		std::vector<BeanCounter> subCategory;
		int skipChildren = 0;

		RuleBaseFacilityFunctions currentService = 0;
		currentService.flip(i);

		description = tr("STR_BFDS_REQUIRED_SERVICE_CAT")
			.arg(_game->getMod()->getBaseFunctionNames(currentService).front());
		bean = {0, parentId, description , 0, 0, true};
		childValueOverride((currentService & _providedBaseFunc).any());
		subCategory.push_back(bean);

		// Just loop over all countries/regions/facilities.
		// Lists are considered small enough and it prevents
		// adding another level of indentation (or `continue` halfway this loop).
		for (const auto* country : *_game->getSavedGame()->getCountries())
		{
			const RuleCountry *ruleCountry = country->getRules();
			if (!ruleCountry) continue;
			if ((currentService & ruleCountry->getProvidedBaseFunc()).none())
				continue;

			bool appliesToBase = ruleCountry->insideCountry(_base->getLongitude(), _base->getLatitude());
			description = tr("STR_PROVIDED_BY_COUNTRY")
				.arg(tr(ruleCountry->getType()));
			bean = {0, parentId, description , appliesToBase, 0, false};
			childValueOverride(appliesToBase);
			subCategory.push_back(bean);
			skipChildren += 1;
		}
		// Just loop over all regions, no need to test beforehand (small enough list)
		for (const auto* region : *_game->getSavedGame()->getRegions())
		{
			RuleRegion *ruleRegion = region->getRules();
			if (!ruleRegion) continue;
			if ((currentService & ruleRegion->getProvidedBaseFunc()).none())
				continue;

			bool appliesToBase = ruleRegion->insideRegion(_base->getLongitude(), _base->getLatitude());
			description = tr("STR_PROVIDED_BY_REGION")
				.arg(tr(ruleRegion->getType()));
			bean = {0, parentId, description , appliesToBase, 0, false};
			childValueOverride(appliesToBase);
			subCategory.push_back(bean);
			skipChildren += 1;
		}
		for (auto& facilityType : _game->getMod()->getBaseFacilitiesList())
		{
			RuleBaseFacility *ruleFacility = _game->getMod()->getBaseFacility(facilityType);
			if (!ruleFacility) continue;
			if (!ruleFacility->isAllowedForBaseType(_base->isFakeUnderwater()))
				continue; // No need to include facilities that are never allowed here.
			if ((currentService & ruleFacility->getProvidedBaseFunc()).none())
				continue;
			if (!_game->getSavedGame()->isResearched(ruleFacility->getRequirements()))
				continue;

			bool existsOnBase = false;
			for (auto& facility : *_base->getFacilities())
			{
				const RuleBaseFacility *existingFac = facility->getRules();
				if (existingFac != ruleFacility) continue;

				existsOnBase = true;
				break; // We are only interested if at least one exists
			}
			description = tr("STR_PROVIDED_BY_FACILITY")
				.arg(tr(ruleFacility->getType()));
			bean = {0, parentId, description , existsOnBase, 0, false};
			childValueOverride(existsOnBase);
			subCategory.push_back(bean);
		}
		sortChildrenByDescription(subCategory, skipChildren);
		// sortChildrenByExistOnBase(subCategory);
		add2_detailsVector(subCategory);
		subCategory.clear();
	}
}

/**
 * Setup and add "reached maximum amount" to `_details` vector.
 */
void BuildFacilitiesDetailsState::categoryLimitedByAmount()
{
	if (_facRuleSelected->getMaxAllowedPerBase() == 0) return;

	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	int countFac = 0;
	for (auto& facility : *_base->getFacilities())
	{
		if (facility->getRules() != _facRuleSelected) continue;

		countFac++;
	}
	description = tr("STR_BFDS_LIMITED_BY_AMOUNT_CAT")
		.arg(_facRuleSelected->getMaxAllowedPerBase());
	bean = {0, parentId, description , 0, countFac, true};
	subCategory.push_back(bean);

	add2_detailsVector(subCategory, true);
}

/**
 * Setup and add "non-compatible services" to `_details` vector.
 *
 * @note
 * Lists all known ones regardless if possible for *this* base.
 */
void BuildFacilitiesDetailsState::categoryProvidesServicesBlockedByOthers()
{
	if (_providedFacFunc.none()) return;

	BeanCounter bean;
	auto childValueOverride = [&bean](bool testResult) -> void
	{
		bean.valueOverride = testResult ? "Yes" : "No";
	};

	// Show each potential blocked service as separate subcategory.
	// Let children show their providers (regardless if allowed on this base).
	for (size_t i = 0; i < _providedFacFunc.size(); ++i)
	{
		if (!_providedFacFunc.test(i)) continue;

		size_t parentId = _details.size();
		std::string description;
		std::vector<BeanCounter> subCategory;
		int skipChildren = 0;

		RuleBaseFacilityFunctions currentService = 0;
		currentService.flip(i);

		description = tr("STR_BFDS_BLOCKED_BY_OTHERS_CAT")
			.arg(_game->getMod()->getBaseFunctionNames(currentService).front());
		bean = {0, parentId, description , 0, 0, true};
		childValueOverride((currentService & _forbiddenBaseFunc).any());
		subCategory.push_back(bean);

		// Just loop over all countries/regions/facilities.
		// Lists are considered small enough and it prevents
		// adding another level of indentation (or `continue` halfway this loop).
		for (const auto* country : *_game->getSavedGame()->getCountries())
		{
			const RuleCountry *ruleCountry = country->getRules();
			if (!ruleCountry) continue;
			if ((currentService & ruleCountry->getForbiddenBaseFunc()).none())
				continue;

			bool appliesToBase = ruleCountry->insideCountry(_base->getLongitude(), _base->getLatitude());
			description = tr("STR_FORBIDDEN_BY_COUNTRY")
				.arg(tr(ruleCountry->getType()));
			bean = {0, parentId, description , appliesToBase, 0, false};
			childValueOverride(appliesToBase);
			subCategory.push_back(bean);
			skipChildren += 1;
		}
		// Just loop over all regions, no need to test beforehand (small enough list)
		for (const auto* region : *_game->getSavedGame()->getRegions())
		{
			RuleRegion *ruleRegion = region->getRules();
			if (!ruleRegion) continue;
			if ((currentService & ruleRegion->getForbiddenBaseFunc()).none())
				continue;

			bool appliesToBase = ruleRegion->insideRegion(_base->getLongitude(), _base->getLatitude());
			description = tr("STR_FORBIDDEN_BY_REGION")
				.arg(tr(ruleRegion->getType()));
			bean = {0, parentId, description , appliesToBase, 0, false};
			childValueOverride(appliesToBase);
			subCategory.push_back(bean);
			skipChildren += 1;
		}
		for (auto& facilityType : _game->getMod()->getBaseFacilitiesList())
		{
			const RuleBaseFacility *ruleFacility = _game->getMod()->getBaseFacility(facilityType);
			if (!ruleFacility) continue;
			if (!ruleFacility->isAllowedForBaseType(_base->isFakeUnderwater()))
				continue; // No need to include facilities that are never allowed here.
			if ((currentService & ruleFacility->getForbiddenBaseFunc()).none())
				continue;
			if (!_game->getSavedGame()->isResearched(ruleFacility->getRequirements()))
				continue;

			bool existsOnBase = false;
			for (auto& facility : *_base->getFacilities())
			{
				const RuleBaseFacility *existingFac = facility->getRules();
				if (existingFac != ruleFacility) continue;

				existsOnBase = true;
				break;
			}
			description = tr("STR_FORBIDDEN_BY_FACILITY")
				.arg(tr(ruleFacility->getType()));
			bean = {0, parentId, description , existsOnBase, 0, false};
			childValueOverride(existsOnBase);
			subCategory.push_back(bean);
		}
		sortChildrenByDescription(subCategory, skipChildren);
		// sortChildrenByExistOnBase(subCategory);
		add2_detailsVector(subCategory);
		subCategory.clear();
	}
}

/**
 * Setup and add "non-compatible services" to `_details` vector.
 *
 * @note
 * Lists all known ones regardless if possible for *this* base.
 */
void BuildFacilitiesDetailsState::categoryBlocksServicesProvidedByOthers()
{
	if (_forbiddenFacFunc.none()) return;

	BeanCounter bean;
	auto childValueOverride = [&bean](bool testResult) -> void
	{
		bean.valueOverride = testResult ? "Yes" : "No";
	};

	// Show each potential blocked service as separate subcategory.
	// Let children show their providers (regardless if allowed on this base).
	for (size_t i = 0; i < _forbiddenFacFunc.size(); ++i)
	{
		if (!_forbiddenFacFunc.test(i)) continue;

		size_t parentId = _details.size();
		std::string description;
		std::vector<BeanCounter> subCategory;
		int skipChildren = 0;

		RuleBaseFacilityFunctions currentService = 0;
		currentService.flip(i);

		description = tr("STR_BFDS_BLOCKS_OTHERS_CAT")
			.arg(_game->getMod()->getBaseFunctionNames(currentService).front());
		bean = {0, parentId, description , 0, 0, true};
		childValueOverride((currentService & _futureBaseFunc).any()); // Future, since planned facilities might be blocked
		subCategory.push_back(bean);

		// Just loop over all countries/regions/facilities.
		// Lists are considered small enough and it prevents
		// adding another level of indentation (or `continue` halfway this loop).
		for (const auto* country : *_game->getSavedGame()->getCountries())
		{
			const RuleCountry *ruleCountry = country->getRules();
			if (!ruleCountry) continue;
			if ((currentService & ruleCountry->getProvidedBaseFunc()).none())
				continue;

			bool appliesToBase = ruleCountry->insideCountry(_base->getLongitude(), _base->getLatitude());
			description = tr("STR_PROVIDED_BY_COUNTRY")
				.arg(tr(ruleCountry->getType()));
			bean = {0, parentId, description , appliesToBase, 0, false};
			childValueOverride(appliesToBase);
			subCategory.push_back(bean);
			skipChildren += 1;
		}
		// Just loop over all regions, no need to test beforehand (small enough list)
		for (const auto* region : *_game->getSavedGame()->getRegions())
		{
			RuleRegion *ruleRegion = region->getRules();
			if (!ruleRegion) continue;
			if ((currentService & ruleRegion->getProvidedBaseFunc()).none())
				continue;

			bool appliesToBase = ruleRegion->insideRegion(_base->getLongitude(), _base->getLatitude());
			description = tr("STR_PROVIDED_BY_REGION")
				.arg(tr(ruleRegion->getType()));
			bean = {0, parentId, description , appliesToBase, 0, false};
			childValueOverride(appliesToBase);
			subCategory.push_back(bean);
			skipChildren += 1;
		}
		for (auto& facilityType : _game->getMod()->getBaseFacilitiesList())
		{
			RuleBaseFacility *ruleFacility = _game->getMod()->getBaseFacility(facilityType);
			if (!ruleFacility) continue;
			if (!ruleFacility->isAllowedForBaseType(_base->isFakeUnderwater()))
				continue; // No need to include facilities that are never allowed here.
			if ((currentService & ruleFacility->getProvidedBaseFunc()).none())
				continue;
			if (!_game->getSavedGame()->isResearched(ruleFacility->getRequirements()))
				continue;

			bool existsOnBase = false;
			for (auto& facility : *_base->getFacilities())
			{
				const RuleBaseFacility *existingFac = facility->getRules();
				if (existingFac != ruleFacility) continue;

				existsOnBase = true;
				break;
			}
			description = tr("STR_PROVIDED_BY_FACILITY")
				.arg(tr(ruleFacility->getType()));
			bean = {0, parentId, description , existsOnBase, 0, false};
			childValueOverride(existsOnBase);
			subCategory.push_back(bean);
		}
		sortChildrenByDescription(subCategory, skipChildren);
		// sortChildrenByExistOnBase(subCategory);
		add2_detailsVector(subCategory);
		subCategory.clear();
	}
}

/**
 * Setup and add "facilities that can be renovated by this one" to `_details` vector.
 *
 * @note
 * Shows any facility that allows renovation.
 * For consistency reasons this means it is not desired to check if a renovation
 * is allowed (by spatial limitations) for those that exist on base.
 */
void BuildFacilitiesDetailsState::categoryCanRenovate()
{
	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	description = tr("STR_BFDS_CAN_RENOVATE_CAT");
	bean = {0, parentId, description , 0, 0, true};
	subCategory.push_back(bean);

	bool canRenovateAtLeastOne = false;
	for (auto& facility : _game->getMod()->getBaseFacilitiesList())
	{
		const RuleBaseFacility *ruleFacility = _game->getMod()->getBaseFacility(facility);
		if (!ruleFacility) continue;
		if (!ruleFacility->isAllowedForBaseType(_base->isFakeUnderwater()))
			continue; // No need to include facilities that are never allowed here.
		if (!_game->getSavedGame()->isResearched(ruleFacility->getRequirements()))
			continue;
		if (_facRuleSelected->getCanBuildOverOtherFacility(ruleFacility) != BPE_None)
			continue;

		int onBase = 0;
		if (auto search = _baseFacilitiesAndCount.find(ruleFacility);
			search != _baseFacilitiesAndCount.end())
		{
			onBase = search->second;
		}
		canRenovateAtLeastOne |= onBase;

		description = tr(ruleFacility->getType());
		bean = {0, parentId, description , 0, onBase, false};
		subCategory.push_back(bean);
	}

	if (canRenovateAtLeastOne)
	{
		subCategory.front().valueOverride = "Yes";
	}
	else
	{
		subCategory.front().valueOverride = "No";
	}

	sortChildrenByDescription(subCategory);
	// sortChildrenByExistOnBase(subCategory);
	add2_detailsVector(subCategory);
}

/**
* Draw (en filter) the current details list.
*/
void BuildFacilitiesDetailsState::updateList()
{
	_lstDetails->clearList();
	_rows.clear();

	auto colorPrimary = _lstDetails->getColor();
	auto colorSecondary = _lstDetails->getSecondaryColor();
	auto colorTertiary = _game->getMod()->getInterface("buildFacilitiesDetails")->getElement("list-highlight")->color;
	if (_details.size() == 0)
	{
		_lstDetails->addRow(2, tr("STR_EMPTY_LIST").c_str(), "");
		_rows.push_back(0);
		_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), colorSecondary);
		return;
	}

	for (size_t i = 0; i < _details.size(); ++i)
	{
		if (!_details[i].isRowVisible) continue;

		std::string description = _details[i].description;
		std::ostringstream ssAmount, ssValue;
		if (_details[i].parentId != i) // A child row.
		{
			description.insert(0, " "); // Do not use dots for description indentation.
			ssValue << " ";
		}

		if (_details[i].valueOverride != "")
		{
			//ssValue << Unicode::formatFunding(999'999'999);
			ssValue << _details[i].valueOverride;
		}
		else
		{
			ssValue << _details[i].baseValue;
		}

		_lstDetails->addRow(2, description.c_str(), ssValue.str().c_str());
		if (_details[i].parentId != i)
		{
			_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), colorPrimary, colorTertiary);
		}
		else
		{
			_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), colorSecondary, colorTertiary);
		}
		_rows.push_back(i);
	}
}

/**
* Adds given subcategory vector to screen's global `_details` vector
* (if it has any children).
*
* @param subCategory Vector to add to `_details` vector
* @param forceInclude Include even if parent has no children.
*/
void BuildFacilitiesDetailsState::add2_detailsVector(std::vector<BeanCounter> &subCategory, bool forceInclude)
{
	if (subCategory.size() < 2 && !forceInclude) return;

	_details.insert(_details.end(), subCategory.begin(), subCategory.end());
}

/**
* Alphabetical sort of children (by description)
*
* @param subCategory The method's local `_details` slice copy to work on.
* @param skipChildren Number of starting child rows to be left untouched.
*/
void BuildFacilitiesDetailsState::sortChildrenByDescription(std::vector<BeanCounter> &subCategory, size_t skipChildren)
{
	if (subCategory.size() < 2 + skipChildren) return; // No valid children

	std::stable_sort(std::next(subCategory.begin(), 1 + skipChildren), subCategory.end(),
		[](const BeanCounter a, const BeanCounter b)
		{ return Unicode::naturalCompare(a.description, b.description); }
	);
}

/**
* Sort children by their existence on this base
*
* @param subCategory The method's local `_details` slice copy to work on.
* @param skipChildren Number of starting child rows to be left untouched.
*/
void BuildFacilitiesDetailsState::sortChildrenByExistOnBase(std::vector<BeanCounter> &subCategory, size_t skipChildren)
{
	if (subCategory.size() < 2 + skipChildren) return; // No valid children

	std::stable_sort(std::next(subCategory.begin(), 1 + skipChildren), subCategory.end(),
		[](const BeanCounter a, const BeanCounter b)
		{ return a.amount > b.amount; }
	);
}

}
