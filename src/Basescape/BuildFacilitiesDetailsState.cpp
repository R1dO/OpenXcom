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
#include "../Mod/RuleCountry.h"
#include "../Mod/RuleRegion.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"

#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Initializes all elements in the base info category breakdown subwindow.
 *
 * @param base Pointer to the base to get info from.
 * @param currentTab Category to open
 */
BuildFacilitiesDetailsState::BuildFacilitiesDetailsState(Base *base, SelectedTab currentTab) : _base(base), _category(currentTab)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 320, 184, 0, 8, POPUP_BOTH); // TransferState Style
	_btnPrev = new TextButton(28, 14, 8, 18);
	_btnNext = new TextButton(28, 14, 284, 18);
	_txtTitle = new Text(278, 17, 21, 18);
	_tabCapabilities = new ToggleTextButton(136, 16, 23, 35);
	_tabBuildLimitations = new ToggleTextButton(136, 16, 159, 35);
	_txtSource = new Text(114, 9, 25, 35+16+2);
	_txtQuantity = new Text(54, 9, 180+15, 35+16+2);
	_txtResult = new Text(54, 9, 225+15, 35+16+2);
	_lstDetails = new TextList(272, 104-16, 23, 46+16+2); // Height = 13*8 (8 due to rowheight overlap using default rules).
	_txtTotal = new Text(133, 9, 171, 154+2);
	_btnOk = new TextButton(148, 16, 164, 169);
	_btnQueuedFacilities = new ToggleTextButton(148, 16, 9, 169);

	// Set palette
	setInterface("baseInfoDetails");

	add(_window, "window", "baseInfoDetails");
	add(_btnPrev, "button", "baseInfoDetails");
	add(_btnNext, "button", "baseInfoDetails");
	add(_txtTitle, "text", "baseInfoDetails");
	add(_tabCapabilities, "button", "baseInfoDetails");
	add(_tabBuildLimitations, "button", "baseInfoDetails");
	add(_txtSource, "text", "baseInfoDetails");
	add(_txtQuantity, "text", "baseInfoDetails");
	add(_txtResult, "text", "baseInfoDetails");
	add(_lstDetails, "list", "baseInfoDetails");
	add(_txtTotal, "text", "baseInfoDetails");
	add(_btnOk, "button", "baseInfoDetails");
	add(_btnQueuedFacilities, "button", "baseInfoDetails");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "baseInfoDetails");

	_btnNext->setText(">>");
	_btnNext->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnNextClick, Options::keyGeoRight);
	_btnPrev->setText("<<");
	_btnPrev->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnPrevClick, Options::keyGeoLeft);
	_tabCapabilities->setText(tr("STR_BIDS_TAB_CAPABILITIES"));
	_tabCapabilities->setPressed(true); // Default screen
	_tabCapabilities->onMouseClick((ActionHandler)&BaseInfoDetailsState::tabClick);
	_tabBuildLimitations->setText(tr("STR_BIDS_TAB_BUILD_LIMITATIONS"));
	_tabBuildLimitations->setPressed(false);
	_tabBuildLimitations->onMouseClick((ActionHandler)&BaseInfoDetailsState::tabClick);
	_btnQueuedFacilities->setText(tr("STR_INCLUDE_QUEUED_FACILITIES"));
	//_btnQueuedFacilities->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnToggleQueuedFacilities, 0); // Any mouse button.
	_btnQueuedFacilities->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnToggleQueuedFacilities); // LMB only
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&BaseInfoDetailsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&BaseInfoDetailsState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtSource->setText(tr("STR_SOURCE"));
	_txtQuantity->setText(tr("STR_FACILITIES"));
	_txtResult->setText(tr("STR_VALUE"));

	//_lstDetails->setColumns(3, 155+15, 45, 70-15);
	_lstDetails->setSelectable(true); // Required for collapse/fold functionality.
	_lstDetails->setBackground(_window);
	_lstDetails->setScrolling(true);
	_lstDetails->setMargin(2);        // Shifts **all** columns 2px to the right.
	_lstDetails->onMousePress((ActionHandler)&BaseInfoDetailsState::lstDetailsMousePress, SDL_BUTTON_LEFT);
	_lstDetails->onMousePress((ActionHandler)&BaseInfoDetailsState::lstDetailsMousePress, SDL_BUTTON_RIGHT);
	_lstDetails->setDot(true);
	_lstDetails->setWordWrap(true);
	//_lstDetails->setAlign(ALIGN_RIGHT, 2);

	// Current country/region is needed for list of build limitations
	// Based on `Base::calculateServices()`
	_baseCountry = nullptr;
	_baseRegion = nullptr;
	for (const auto* country : *_game->getSavedGame()->getCountries())
	{
		if (country->getRules()->insideCountry(base->getLongitude(), base->getLatitude()))
		{
			_baseCountry = country;
			break;
		}
	}
	for (const auto* region : *_game->getSavedGame()->getRegions())
	{
		if (region->getRules()->insideRegion(base->getLongitude(), base->getLatitude()))
		{
			_baseRegion = region;
			break;
		}
	}

	// Services that don't depend on current category.
	_providedBaseFunc = base->getProvidedBaseFunc({});
	_futureBaseFunc = base->getFutureBaseFunc({});
	_forbiddenBaseFunc = base->getForbiddenBaseFunc({});

	drawBody();
}

/**
 *
 */
BaseInfoDetailsState::~BaseInfoDetailsState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void BaseInfoDetailsState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Show details from all bases.
 */
void BaseInfoDetailsState::btnToggleQueuedFacilities(Action *)
{
	drawBody();
}

/**
 * Goes to the next details category.
 * @param action Pointer to an action.
 */
void BaseInfoDetailsState::btnNextClick(Action *)
{
	switch (_category)
	{
	case BaseInfoDetailsCategory::DETECTION:
		_category = BaseInfoDetailsCategory::SOLDIERS;
		break;
	case BaseInfoDetailsCategory::SOLDIERS:
	case BaseInfoDetailsCategory::ENGINEERS:
	case BaseInfoDetailsCategory::SCIENTISTS:
		// Skip categories for which I could not devise meaningful screen content.
		_category = BaseInfoDetailsCategory::QUARTERS;
		break;
	default:
		_category = static_cast<BaseInfoDetailsCategory>(static_cast<int>(_category) + 1);
		break;
	}

	drawBody();
}

/**
 * Goes to the previous details category.
 * @param action Pointer to an action.
 */
void BaseInfoDetailsState::btnPrevClick(Action *)
{
	switch (_category)
	{
	case BaseInfoDetailsCategory::SOLDIERS:
		_category = BaseInfoDetailsCategory::DETECTION;
		break;
	case BaseInfoDetailsCategory::QUARTERS:
	case BaseInfoDetailsCategory::SCIENTISTS:
	case BaseInfoDetailsCategory::ENGINEERS:
		// Skip categories for which I could not devise meaningful screen content.
		_category = BaseInfoDetailsCategory::SOLDIERS;
		break;
	default:
		_category = static_cast<BaseInfoDetailsCategory>(static_cast<int>(_category) - 1);
		break;
	}

	drawBody();
}

/**
 * Toggle screen tab
 */
void BaseInfoDetailsState::tabClick(Action *action)
{
	if (action->getSender() == _tabCapabilities)
	{
		_tabBuildLimitations->setPressed(!_tabBuildLimitations->getPressed());
	}
	else
	{
		_tabCapabilities->setPressed(!_tabCapabilities->getPressed());
	}

	drawBody();
}

/**
* Handles mouse-clicks on the list rows.
*/
void BaseInfoDetailsState::lstDetailsMousePress(Action *)
{
	_sel = _lstDetails->getSelectedRow();

	// Flip visibility of child elements.
	for (size_t i = 0; i < _details.size(); ++i)
	{
		if (_details[i].parentId == _details[i].childId) continue;
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
 *  * list details
 *  * (optional) Grand total
 */
void BaseInfoDetailsState::drawBody()
{
	_details.clear();
	_lstDetails->clearList();

	if (_tabCapabilities->getPressed())
	{
	 	_lstDetails->setColumns(3, 155+15, 45, 70-15);
		_txtQuantity->setVisible(true);
		_txtResult->setVisible(true);
		_btnQueuedFacilities->setVisible(true);
	}
	else
	{
		_lstDetails->setColumns(2, 155+15+45, 70-15);
		_txtQuantity->setVisible(false);
		_txtResult->setVisible(false);
		_btnQueuedFacilities->setVisible(false);
	}
	updateBlockedFacilitiesSets();

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
		setupCategoryDetection();
		break;
	default:
		setupPlaceholders();
		break;
	}
}

/**
 * Creates all placeholder elements.
*/
void BaseInfoDetailsState::setupPlaceholders()
{
	std::ostringstream ssTitle, screenTotal;
	// Do not end sentence with '.'
	// Automatic font scaling "setText()" does not like that (Cause: b1b6f9ae).
	ssTitle << "Category " << enum2string(_category) << " not implemented yet";
	screenTotal << tr("STR_TOTAL") << ">\t" << Unicode::formatFunding(999'999'999'999);
	_txtTitle->setText(ssTitle.str());
	_txtTotal->setText(screenTotal.str());

	BeanCounter row;
	size_t parent, childId = 0;
	for (auto i = 0; i < 5; i++)
	{
		parent = childId;
		row = {childId, parent, "Long text explaining the source", 999, 999'999'999, true};
		_details.push_back(row);
		childId++;
		for (auto j = 0; j < 5; j++)
		{
			row = {childId, parent, "Normally collapsed (moaar details)", 99, 999'999'999, false};
			_details.push_back(row);
			childId++;
		}
	}
	updateList();
}

/**
 * Setup base detection abilities and camouflage screen.
 *
 * Facilities contributing to the following subcategories:
 *  - Base Camouflage, e.g. chance of staying undetected.
 *  - UFO detection per range, including hyperwave abilities.
 *  - Alien Base detection per range.
 * And show the effect of those facilities on base services (if any).
 */
void BaseInfoDetailsState::setupCategoryDetection()
{
	_txtTitle->setText(tr("STR_BIDS_CATEGORY_DETECTION"));
	_txtTotal->setText("");

	if (_tabCapabilities->getPressed())
	{
		subcategoryBaseCamouflage();
		subcategoryUfoDetection();
		subcategoryAlienBaseDetection();
	}
	else
	{
		subcategoryForbiddenFacilities();
	}
	// subcat detection related facilities not able to be build due to missing requirements.

	updateList(); //2024
}

/**
 * Setup and add base camouflage to `_details` vector.
 *
 * Camouflage is the chance of staying undetected (per 10 minute time interval).
 * A value that feels a bit less technical/cheaty, even though it is not.
 *
 * @remark
 * Ruleset variables: `mind`, `mindPower` and `size` (or `sizeX`and`sizeY`).
 *
 * @note
 * Uses +/- as hint for player that adding/subtracting percentages
 * is viable in this situation.
 *
 * @remark
 * One could argue it should be part of defense category,
 * you can't target what you can't detect.
 * It is listed here since this screen is expected to have less subcategories.
 *
 * @remark
 * See: `Base::getDetectionChance()` for reference formula.
 * + Lowest possible camouflage value is 79% (pretty high).
 * + Could lead to a false sense of security if one is not aware that
 *   check runs in 10 minute intervals.
 *   Hence one might argue it is better not to show this sub-category at all.
 */
void BaseInfoDetailsState::subcategoryBaseCamouflage()
{
	std::vector<BeanCounter> subCategory;
	size_t childId, parentId = _details.size(); // `childId` is set later.
	BeanCounter row;

	// Sub category header (a.k.a. subtotal).
	row = {parentId, parentId, tr("STR_BIDS_TITLE_CAMOUFLAGE"), 0, 0, true};
	subCategory.push_back(row);

	// A base must contain at least 1 facility, hence next child always exist.
	childId = subCategory.size() + parentId;
	row = {childId, parentId, tr("STR_BIDS_DETAIL_CAMOUFLAGE_BASE_SIZE"), 0, 0, false};
	subCategory.push_back(row);

	int totalFacilityAmount = 0;
	int totalFacilitySize = 0;
	int totalMindPower = 0;
	for (auto facility : *_base->getFacilities())
	{
		// Skip buildings under construction (unless we demand their inclusion).
		if (facility->getBuildTime() > 0 && !_btnQueuedFacilities->getPressed())
			continue;

		// Contribution due to base size in grid units.
		totalFacilityAmount++;
		totalFacilitySize += facility->getRules()->getSizeX() * facility->getRules()->getSizeY();

		if (facility->getRules()->isMindShield())
		{
			int currentMindPower = 0;
			std::string description;
			std::string valueOverride = "";
			if (facility->getDisabled())
			{
				description = tr("STR_BIDS_DETAIL_DISABLED_MINDSHIELD").arg(tr(facility->getRules()->getType()));
				valueOverride = "---";
			}
			else
			{
				description = tr(facility->getRules()->getType());
				currentMindPower = facility->getRules()->getMindShieldPower();
				totalMindPower += currentMindPower;
			}

			childId = subCategory.size() + parentId;
			row = {childId, parentId, description ,1 , currentMindPower, false};
			row.valueOverride = valueOverride;
			add2vector(subCategory, row);
		}
	}

	// Multiple unique mind shields (with unique `mindPower`) might exist.
	std::vector<std::pair<size_t, int>> shieldTypes;
	for (auto element : subCategory)
	{
		// At this point only active mindShields can have `.baseValue > 0`.
		if (element.baseValue == 0) continue;

		shieldTypes.push_back(std::make_pair(element.childId, element.amount * element.baseValue));
	}

	// Apply field overrides where necessary.
	// Some behind the scenes calculations depend on fractional values.
	float detectionP = (totalFacilitySize/6.0 + 15)/(totalMindPower + 1.0);
	float baseSizeEffectP = 15.0 + (totalFacilitySize / 6.0); // Effect on detectionP as if no mindshields are present.
	for (auto &element : subCategory)
	{
		if (element.childId == element.parentId)
		{
			// Internal game functionality uses integer math for Pdetection.
			// We want to display Pcamouflage (= 100 - Pdetection).
			element.valueOverride = Unicode::formatPercentage(100 - std::trunc(detectionP));
			element.amount = totalFacilityAmount;
		}
		else if (element.childId == element.parentId + 1)
		{
			element.amount = totalFacilityAmount;
			element.valueOverride = Unicode::formatPercentage(100 - std::trunc(baseSizeEffectP));
		}
		else if (element.baseValue == 0) // or (element.valueOverride == "---")
		{
			// Do nothing
		}
		else
		{
			// Effect of mindshield depends on base size.
			// For this screen it is chosen to display contribution
			// to camouflage/detection counteracting the effect of base size.
			// Pdetection = Pbase_size - Ptotal_mindshield
			//
			// For each unique shield type we want to show it's contribution.
			// Ptotal_mindshield = Pmindshield1 + Pmindshield2 + ... + PmindshieldN
			//
			// We know that Ptotal_mindshield = F(totalMindPower)
			// For this screen we now assume it is valid to say:
			// Pmindshield1 = Ptotal_mindshield * mindPower1/totalMindPower
			float mindShieldP = detectionP - baseSizeEffectP;
			int currentPower = 0;
			int totalPower = 0;
			for (auto shield : shieldTypes)
			{
				if (shield.first == element.childId)
				{
					currentPower = shield.second;
				}
				totalPower += shield.second;
			}
			// Effect on camouflage is -1 * effect on detection.
			float valueOverride = -1 * mindShieldP * currentPower / std::max(1, totalPower);
			element.valueOverride = Unicode::formatPercentage(std::round(valueOverride), true);
		}
	}

	// Prefer alphabetical listing of named facilities.
	sortChildrenByDescription(subCategory);
	add2screenList(subCategory);

	// TRIVIA:
	// If total 'mindpower' >= 21 base cannot be found by UFO's, due to integer math.
}

/**
 * Setup and add UFO detection capabilities to `_details` vector.
 *
 * + The (per range limit) probability of detection.
 * + The (per range limit) HyperWave/Transmission Resolver functionality.
 *
 * @remark
 * Ruleset variables: `radarRange`, `radarChance` and `hyper`.
 *
 * @note
 * Each range based subtotal shows the combined probability
 * of all facilities contributing to that range.
 *
 * @remark
 * Detection probability = (1- chance_of_not_detecting)^no_of_facilities_participating
 */
void BaseInfoDetailsState::subcategoryUfoDetection()
{
	// Intention is to show detection chance per unique range.
	int hyperMaxRange = 0;
	std::set<int> radarRanges;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0 && !_btnQueuedFacilities->getPressed())
			continue;

		if (facility->getRules()->getRadarRange() == 0)
			continue;

		int currentRange = facility->getRules()->getRadarRange();
		if (facility->getRules()->isHyperwave() && currentRange > hyperMaxRange)
		{
			hyperMaxRange = currentRange;
		}
		radarRanges.insert(currentRange);
	}

	// Ufo detection per range limit.
	for (auto detectionRange : radarRanges)
	{
		std::vector<BeanCounter> subCategory;
		size_t childId, parentId = _details.size(); // `childId` is set later.
		BeanCounter row;
		std::string description;

		// Sub category header (a.k.a. subtotal).
		if (detectionRange <= hyperMaxRange)
		{
			description = tr("STR_BIDS_TITLE_UFO_DETECTION_HYPERWAVE").arg(detectionRange);
		}
		else
		{
			description = tr("STR_BIDS_TITLE_UFO_DETECTION_RADAR").arg(detectionRange);
		}
		row = {parentId, parentId, description, 0, 0, true};
		subCategory.push_back(row);

		for (auto *facility : *_base->getFacilities())
		{
			if (facility->getBuildTime() > 0 && !_btnQueuedFacilities->getPressed())
				continue;

			if (facility->getRules()->getRadarRange() < detectionRange)
				continue;

			// Facility detection chance
			int detectionChance = facility->getRules()->getRadarChance();
			if (facility->getRules()->isHyperwave())
			{
				description = tr("STR_BIDS_DETAIL_UFO_DETECTION_HYPERWAVE").arg(tr(facility->getRules()->getType()));
			}
			else
			{
				description = tr(facility->getRules()->getType());
			}
			childId = subCategory.size() + parentId;
			row = {childId, parentId, description , 1, detectionChance, false};
			add2vector(subCategory, row);
		}

		// Apply field overrides where necessary.
		for (auto &element : subCategory)
		{
			float detectionChance = 0.0;
			if (element.childId == element.parentId)
			{
				detectionChance = calcProbabilityAtLeastOne(subCategory);
				element.amount = countContributingFacilities(subCategory);
			}
			else if (element.baseValue == 0)
			{
				// "0" is a valid detection chance for a hyperwave that only tracks.
			}
			else
			{
				detectionChance = calcProbabilityAtLeastOne(element.baseValue, element.amount);
			}
			element.valueOverride = Unicode::formatPercentage(std::round(100*detectionChance));
		}

		// Prefer alphabetical listing of named facilities.
		sortChildrenByDescription(subCategory);
		add2screenList(subCategory);
	}
}

/**
 * Setup and add alien base detection capabilities to `_details` vector.
 *
 * The (per range limit) probability of detection.
 *
 * @remark
 * Ruleset variables: `sightRange`, `sightChance`.
 *
 * @note
 * Each range based subtotal shows the combined probability
 * of all facilities contributing to that range.
 *
 * @remark
 * Detection probability = (1- chance_of_not_detecting)^no_of_facilities_participating
 * If dynamic detection use 2 ranges @100%ofRange (chance is 0%) and @50%ofRange (chance is 50%)
 *
 * @remark
 * Not sure about this one: one could argue it should be a hidden stat.
 */
void BaseInfoDetailsState::subcategoryAlienBaseDetection()
{
	// Intention is to show chance per unique sight range.
	std::set<int> sightRanges;
	for (auto *facility : *_base->getFacilities())
	{
		if (facility->getBuildTime() > 0 && !_btnQueuedFacilities->getPressed())
			continue;

		if (facility->getRules()->getSightRange() == 0)
			continue;

		int sightRange = facility->getRules()->getSightRange();
		sightRanges.insert(sightRange);
		// For dynamic ranges the chance at max range chance = 0%
		// No need for adding extra ranges to help player deduce.
		// A facility showing "0%" at max range should suffice.
	}

	// Alien base detection per range limit.
	for (auto detectionRange : sightRanges)
	{
		std::vector<BeanCounter> subCategory;
		size_t childId, parentId = _details.size(); // `childId` is set later.
		BeanCounter row;
		std::string description;

		// Sub category header (a.k.a. subtotal).
		description = tr("STR_BIDS_TITLE_ALIEN_BASE_DETECTION").arg(detectionRange);
		row = {parentId, parentId, description, 0, 0, true};
		subCategory.push_back(row);

		for (auto *facility : *_base->getFacilities())
		{
			if (facility->getBuildTime() > 0 && !_btnQueuedFacilities->getPressed())
				continue;

			if (facility->getRules()->getSightRange() < detectionRange)
				continue;

			// Facility detection chance
			int detectionChance = facility->getRules()->getSightChance();
			if (detectionChance == 0)
			{
				// Dynamic, e.g. 0%-50% based on distance.
				// Formula from `GeoscapeState::time1Day()`:
				// `chanceToDetect = 50 - (distance * 50 / facility->getRules()->getSightRange())`
				// For this subroutine we can use: distance = `detectionRange`.
				detectionChance = 50 - (detectionRange * 50) / facility->getRules()->getSightRange();
			}
			childId = subCategory.size() + parentId;
			description = tr(facility->getRules()->getType());
			row = {childId, parentId, description , 1, detectionChance, false};
			add2vector(subCategory, row);
		}

		// Apply field overrides where necessary.
		for (auto &element : subCategory)
		{
			float detectionChance = 0.0;
			if (element.childId == element.parentId)
			{
				detectionChance = calcProbabilityAtLeastOne(subCategory);
				element.amount = countContributingFacilities(subCategory);
			}
			else
			{
				detectionChance = calcProbabilityAtLeastOne(element.baseValue, element.amount);
			}
			element.valueOverride = Unicode::formatPercentage(std::round(100*detectionChance));
		}

		// Prefer alphabetical listing of named facilities.
		sortChildrenByDescription(subCategory);
		add2screenList(subCategory);
	}
}

/**
 * Setup and add list of current category facilities forbidden from being build.
 */
void BaseInfoDetailsState::subcategoryForbiddenFacilities()
{
	for (auto forbiddenFacilityRule : _forbiddenFacilities)
	{
		std::vector<BeanCounter> subCategory;
		size_t childId, parentId = _details.size(); // `childId` is set later.
		BeanCounter row;
		std::string description;
		RuleBaseFacilityFunctions req = forbiddenFacilityRule->getRequireBaseFunc();
		RuleBaseFacilityFunctions forb = forbiddenFacilityRule->getForbiddenBaseFunc();
		RuleBaseFacilityFunctions prov = forbiddenFacilityRule->getProvidedBaseFunc();

		// Sub category header (a.k.a. subtotal).
		description = tr("STR_BIDS_TITLE_FORBIDDEN_FACILITY").arg(tr(forbiddenFacilityRule->getType()));
		row = {parentId, parentId, description, 0, 0, true};
		subCategory.push_back(row);

		if (_baseCountry && (_baseCountry->getRules()->getForbiddenBaseFunc() & prov).any())
		{
			description = tr("STR_BIDS_DETAIL_FORBIDDEN_BY_COUNTRY").arg(tr(_baseCountry->getRules()->getType()));
			childId = subCategory.size() + parentId;
			row = {childId, parentId, description , 0, 0, false};
			add2vector(subCategory, row);
		}
		if (_baseRegion && (_baseRegion->getRules()->getForbiddenBaseFunc() & prov).any())
		{
			description = tr("STR_BIDS_DETAIL_FORBIDDEN_BY_REGION").arg(tr(_baseRegion->getRules()->getType()));
			childId = subCategory.size() + parentId;
			row = {childId, parentId, description , 0, 0, false};
			add2vector(subCategory, row);
		}
		if (_base->isMaxAllowedLimitReached(forbiddenFacilityRule))
		{
			int amountPresent = 0;
			for (auto facility : *_base->getFacilities())
			{
				// Internal calculation does not exclude buildings under construction.
				amountPresent += facility->getRules() == forbiddenFacilityRule; // Implicit conversion bool->int
			}

			description = tr("STR_BIDS_DETAIL_FORBIDDEN_BY_AMOUNT")
				.arg(forbiddenFacilityRule->getMaxAllowedPerBase())
				.arg(amountPresent);
			childId = subCategory.size() + parentId;
			row = {childId, parentId, description , 0, 0, false};
			add2vector(subCategory, row);
		}
		// Ensure anything added up to now is listed above facility services based blockers.
		size_t skipSort = subCategory.size() - 1;

		for (auto facility : *_base->getFacilities())
		{
			if ((facility->getRules()->getForbiddenBaseFunc() & prov).any())
			{
				description = tr("STR_BIDS_DETAIL_FORBIDDEN_SERVICE_BY_EXISTING")
					.arg(tr(facility->getRules()->getType()));
				childId = subCategory.size() + parentId;
				row = {childId, parentId, description , 0, 0, false};

				// Can problem be solved by building over respective facility?
				// Based on: `BaseView::getPlacementError()`.
				if (forbiddenFacilityRule->getCanBuildOverOtherFacility(facility->getRules()) == BPE_None)
				{
					// We are only interested on 'service' blockers
					// Any size/intersection mismatch is part of placement algorithm.
					row.valueOverride = tr("STR_BIDS_DETAIL_FORBIDDEN_RENOVATION_ALLOWED");
				}
				add2vector(subCategory, row);
			}
			if ((facility->getRules()->getProvidedBaseFunc() & forb).any())
			{
				description = tr("STR_BIDS_DETAIL_FORBIDDEN_SERVICE_BY_NEW")
					.arg(tr(facility->getRules()->getType()));
				childId = subCategory.size() + parentId;
				row = {childId, parentId, description , 0, 0, false};

				// Can problem be solved by building over respective facility?
				// Based on: `BaseView::getPlacementError()`.
				if (forbiddenFacilityRule->getCanBuildOverOtherFacility(facility->getRules()) == BPE_None)
				{
					// We are only interested on 'service' blockers
					// Any size/intersection mismatch is part of placement algorithm.
					row.valueOverride = tr("STR_BIDS_DETAIL_FORBIDDEN_RENOVATION_ALLOWED");
				}
				add2vector(subCategory, row);
			}
		}


		// Prefer alphabetical listing of named facilities.
		sortChildrenByDescription(subCategory, skipSort);
		// Always show forbidden facilities even if we cannot determine reason.
		add2screenList(subCategory, true);
	}

	// @note
	// Does not take `buildOverFacilities` into account.
		// At list create time:
		// If a facility is blocked but can build over: do not list it?
		// Or add a "can build over row" (replace only)

		// headers
		// Cannot build $FAC: missing service
		//	$FAC missing	#	Service
		//  || None discovered yet.
		// Cannot build $FAC: blocked service
		//	$FAC blocking	#	Service
		//	$FAC buildover	#	Renovation allowed

}

/**
* Draw (en filter) the current details list.
*/
void BaseInfoDetailsState::updateList()
{
	_lstDetails->clearList();
	_rows.clear();

	if (_details.size() == 0)
	{
		BeanCounter row = {0, 0, tr("STR_BIDS_EMPTY_LIST"), 0, 0, true};
		_details.push_back(row);
	}

	for (size_t i = 0; i < _details.size(); ++i)
	{
		// Filter
		if (!_details[i].isRowVisible) continue;

		std::string description = _details[i].description;
		std::ostringstream ssAmount, ssValue;
		//bool unconditionallyShowSign = true;
		if (_details[i].parentId != _details[i].childId) // A child row.
		{
			description.insert(0, " "); // Do not use dots for description indentation.
			ssAmount << tr("STR_DOTTED_INDENTATION");
			ssValue << tr("STR_DOTTED_INDENTATION");
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
		ssAmount << _details[i].amount;

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

		if (_tabBuildLimitations->getPressed())
		{
			auto originalSecondarycolor = _lstDetails->getSecondaryColor();
			_lstDetails->setSecondaryColor(_game->getMod()->getInterface("baseInfoDetails")->getElement("list")->border);
			_lstDetails->addRow(2, description.c_str(), ssValue.str().c_str());
			_lstDetails->setSecondaryColor(originalSecondarycolor);
		}
		else if (_details[i].amount > 0)
		{
			_lstDetails->addRow(3, description.c_str(), ssAmount.str().c_str(), ssValue.str().c_str());
		}
		else
		{
			_lstDetails->addRow(3, description.c_str(), "", ssValue.str().c_str());
		}
		_rows.push_back(i);

		if(_details[i].parentId == _details[i].childId)
		{
			_lstDetails->setRowColor(_lstDetails->getLastRowIndex(), _lstDetails->getSecondaryColor());
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
* @note
* Only adds if parent has child(ren) otherwise subcategory does not add value.
*
* @param subCategory Vector to add to `_details` vector
* @param forceInclude Include even if parent has no children.
*/
void BaseInfoDetailsState::add2screenList(std::vector<BeanCounter> &subCategory, bool forceInclude)
{
	if (subCategory.size() < 2 - (size_t)forceInclude) // `forceInclude` still requires a subtotal.
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
