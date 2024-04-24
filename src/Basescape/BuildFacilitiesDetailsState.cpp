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
	_txtSource = new Text(214, 9, 25, 35+16+2+13);
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
		_lstDetails->setColumns(2, 155+15+45, 70-15);

		_txtSource->setText("Construction/Usage details");
		_txtResult->setText("On base");
		_txtResult->setVisible(true);
		//_btnQueuedFacilities->setVisible(true);
		setupTabRequirements();
	}
	else
	{
		_lstDetails->setColumns(1, 270);

		_txtSource->setText("Blockers");
		//_txtQuantity->setVisible(false);
		_txtResult->setVisible(false);
		//_btnQueuedFacilities->setVisible(false);
		setupTabBlockers();
	}

	updateList();
}

/**
* Setup list with requirements for this facility.
*
* @note
* This includes facilities which cannot coexist due to service blockers.
* Perhaps even countries/regions that are not compatible.
*/
void BuildFacilitiesDetailsState::setupTabRequirements()
{

	subcategoryReqsFunds();
	subcategoryReqsItems();
	subcategoryReqsServices();
	if (true) // Show potential blockers button?
	{
		addSeparatorLine();
		subcategoryLimitedByAmount();
		subcategoryProvidesServicesBlockedByOthers();
		subcategoryBlocksServicesProvidedByOthers();
	}
	// Diverse categories
	addSeparatorLine();
	subcategoryReqsRecurring();
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
void BuildFacilitiesDetailsState::subcategoryReqsFunds()
{
	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	int requiredFunds = _facRuleSelected->getBuildCost();
	// No need to check on sign and corresponding cases, or zero.
	// Not useful and player can easily deduce this from value in description
	description = "Funds Required: " + Unicode::formatFunding(requiredFunds); // TODO: unhardcode
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
void BuildFacilitiesDetailsState::subcategoryReqsItems()
{
	if (_facRuleSelected->getBuildCostItems().empty()) return;

	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	description = "Item(s) required for construction";
	bean = {0, parentId, description , 0, 0, true};
	subCategory.push_back(bean);

	bool allItemRequirementsMet = true;
	for (auto& item : _facRuleSelected->getBuildCostItems())
	{
		int needed = item.second.first;
		int available = _base->getItemCountStorage(item.first);
		allItemRequirementsMet &= available >= needed;

		description = tr(item.first).c_str();
		description.append(": " + std::to_string(needed));
		//description = tr("STR_BFDS_BLOCKER_DETAIL_MISSING_ITEMS_AMOUNT")
			//.arg(needed)
			//.arg(tr(item.first));
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
void BuildFacilitiesDetailsState::subcategoryReqsServices()
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
		auto serviceName = _game->getMod()->getBaseFunctionNames(currentService).front();

		description = "Service required: " + serviceName;
		bean = {0, parentId, description , 0, 0, true};
		childValueOverride((currentService & _providedBaseFunc).any()); // Current limits build, not future
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
			description = "Provided by country: ";
			description.append(tr(ruleCountry->getType().c_str()));
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
			description = "Provided by region: ";
			description.append(tr(ruleRegion->getType().c_str()));
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
			description = "Provided by facility: ";
			description.append(tr(ruleFacility->getType().c_str()));
			bean = {0, parentId, description , existsOnBase, 0, false};
			childValueOverride(existsOnBase);
			subCategory.push_back(bean);
		}
		sortChildrenByDescription(subCategory, skipChildren);
		sortChildrenByExistOnBase(subCategory);
		add2_detailsVector(subCategory);
		subCategory.clear();
	}
}

/**
 * Setup and add "reached maximum amount" to `_details` vector.
 */
void BuildFacilitiesDetailsState::subcategoryLimitedByAmount()
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
	description = "Only "; // TODO: unhardcode
	description.append(std::to_string(_facRuleSelected->getMaxAllowedPerBase()));
	description.append(" allowed on base");
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
void BuildFacilitiesDetailsState::subcategoryProvidesServicesBlockedByOthers()
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
		auto serviceName = _game->getMod()->getBaseFunctionNames(currentService).front();

		description = "Facility service blocked: " + serviceName;
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
			description = "Forbidden by country: ";
			description.append(tr(ruleCountry->getType().c_str()));
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
			description = "Forbidden by region: ";
			description.append(tr(ruleRegion->getType().c_str()));
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
			description = "Forbidden by facility: ";
			description.append(tr(ruleFacility->getType().c_str()));
			bean = {0, parentId, description , existsOnBase, 0, false};
			childValueOverride(existsOnBase);
			subCategory.push_back(bean);
		}
		sortChildrenByDescription(subCategory, skipChildren);
		sortChildrenByExistOnBase(subCategory);
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
void BuildFacilitiesDetailsState::subcategoryBlocksServicesProvidedByOthers()
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
		auto serviceName = _game->getMod()->getBaseFunctionNames(currentService).front();

		description = "Facility blocks service: " + serviceName;
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
			description = "Provided by country: ";
			description.append(tr(ruleCountry->getType().c_str()));
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
			description = "Provided by region: ";
			description.append(tr(ruleRegion->getType().c_str()));
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
			description = "Provided by facility: ";
			description.append(tr(ruleFacility->getType().c_str()));
			bean = {0, parentId, description , existsOnBase, 0, false};
			childValueOverride(existsOnBase);
			subCategory.push_back(bean);
		}
		sortChildrenByDescription(subCategory, skipChildren);
		sortChildrenByExistOnBase(subCategory);
		add2_detailsVector(subCategory);
		subCategory.clear();
	}
}

/**
 * Setup and add "recurring costs" to `_details` vector.
 * @note:
 * Not needed it is better to show this in base details (cost) view.
 */
void BuildFacilitiesDetailsState::subcategoryReqsRecurring()
{
	int recurringCosts = _facRuleSelected->getMonthlyCost();
	if (recurringCosts == 0) return;

	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	description = "Monthly funds Required: " + Unicode::formatFunding(recurringCosts); // TODO: unhardcode
	bean = {0, parentId, description , 0, 0, true};
	bean.valueOverride = Unicode::formatFunding(_game->getSavedGame()->getFunds());

	subCategory.push_back(bean);
	add2_detailsVector(subCategory, true);

	// Items per base defense .. grav shields seperate?
}

/**
 * Setup and add "facilities that can be renovated by this one" to `_details` vector.
 *
 * @note
 * Shows any facility that allows renovation.
 * For consistency reasons this means it is not desired to check if a renovation
 * is allowed (by spatial limitations) for those that exist on base.
 */
void BuildFacilitiesDetailsState::subCategoryCanRenovate()
{
	size_t parentId = _details.size();
	std::string description;
	BeanCounter bean;
	std::vector<BeanCounter> subCategory;

	description = "Can renovate existing facilities"; // TODO: unhardcode
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
	sortChildrenByExistOnBase(subCategory);
	add2_detailsVector(subCategory);
}

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
	if (missingFunds <= 0) return;
	
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
			ssValue << " ";
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
		// ssAmount << _details[i].amount;

		//if (_tabBlockers->getPressed())
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
		// if (_details[i].amount > 0)
		// {
		// 	_lstDetails->addRow(2, description.c_str(), ssValue.str().c_str());
		// 	//_lstDetails->addRow(3, description.c_str(), ssAmount.str().c_str(), ssValue.str().c_str());
		// }
		// else
		// {
		// 	_lstDetails->addRow(2, description.c_str(), ssValue.str().c_str());
		// 	//_lstDetails->addRow(3, description.c_str(), "", ssValue.str().c_str());
		// }
		_rows.push_back(i);

		if(_details[i].parentId == i)
		{
			_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), colorSecondary);
			//_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), _game->getMod()->getInterface("baseInfoDetails")->getElement("list")->border);
		}
		// else
		// {
		// 	_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), colorPrimary);
		// }
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
