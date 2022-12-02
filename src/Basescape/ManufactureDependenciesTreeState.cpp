/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "ManufactureDependenciesTreeState.h"
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleSoldierTransformation.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace OpenXcom
{

/**
 * Initializes all the elements on the UI.
 */
ManufactureDependenciesTreeState::ManufactureDependenciesTreeState(const std::string &selectedItem) : _selectedItem(selectedItem), _showAll(false)
{
	_screen = false;

	_window = new Window(this, 222, 144, 49, 32);
	_txtTitle = new Text(182, 9, 53, 42);
	_lstTopics = new TextList(198, 96, 53, 54);
	_btnShowAll = new TextButton(100, 16, 57, 153);
	_btnOk = new TextButton(100, 16, 163, 153);

	// Set palette
	setInterface("dependencyTree");

	add(_window, "window", "dependencyTree");
	add(_txtTitle, "text", "dependencyTree");
	add(_btnShowAll, "button", "dependencyTree");
	add(_btnOk, "button", "dependencyTree");
	add(_lstTopics, "list", "dependencyTree");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "dependencyTree");

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TOPIC").arg(tr(_selectedItem)));

	_btnShowAll->setText(tr("STR_SHOW_ALL"));
	_btnShowAll->onMouseClick((ActionHandler)&ManufactureDependenciesTreeState::btnShowAllClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ManufactureDependenciesTreeState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ManufactureDependenciesTreeState::btnOkClick, Options::keyCancel);

	_lstTopics->setColumns(1, 182);
	_lstTopics->setBackground(_window);
	_lstTopics->setMargin(0);
	_lstTopics->setAlign(ALIGN_CENTER);
	_lstTopics->setSelectable(true);
	_lstTopics->onMouseClick((ActionHandler)&ManufactureDependenciesTreeState::lstTopicsClickRight, SDL_BUTTON_RIGHT);

	if (Options::oxceDisableProductionDependencyTree)
	{
		_txtTitle->setHeight(_txtTitle->getHeight() * 11);
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr("STR_THIS_FEATURE_IS_DISABLED_3"));
		_btnShowAll->setVisible(false);
		_lstTopics->setVisible(false);
		return;
	}
}

ManufactureDependenciesTreeState::~ManufactureDependenciesTreeState()
{
}

/**
* Initializes the screen (fills the list).
*/
void ManufactureDependenciesTreeState::init()
{
	State::init();

	if (!Options::oxceDisableProductionDependencyTree)
	{
		fillTopicsList();
		drawList();
	}
}

/**
 * Build workhorse vector of topics for this item.
 *
 * All topics are based on global acces (no base limitations).
*/
void ManufactureDependenciesTreeState::fillTopicsList()
{
	_topics.clear();

	int parentId = 0;
	addResearchSection(parentId);
	addHowToAcquireItemSections(parentId);
	addNeededForSpecialsSections(parentId);
	addNeededForManufactureSections(parentId);
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ManufactureDependenciesTreeState::btnOkClick(Action *)
{
	_game->popState();
}

/**
* Shows spoilers.
* @param action Pointer to an action.
*/
void ManufactureDependenciesTreeState::btnShowAllClick(Action *)
{
	_showAll = true;
	_btnOk->setWidth(_btnOk->getX() - _btnShowAll->getX() + _btnOk->getWidth());
	_btnOk->setX(_btnShowAll->getX());
	_btnShowAll->setVisible(false);

	fillTopicsList(); // Have to rebuild since `_showAll` impact category headers.
	drawList();
}

/**
* Toggles folding state of a category.
* @param action Pointer to an action.
*/
void ManufactureDependenciesTreeState::lstTopicsClickRight(Action *)
{
	_sel = _lstTopics->getSelectedRow();
	//size_t scrollPos = _lstTopics->getScroll();

	// Check if children of parent are visible.
	auto isExpanded = [&](int parentId) -> bool
	{
		auto bread = std::find_if(_topics.begin(), _topics.end(),
			[&](const TopicsBackend crumb)
			{ return crumb.parentId == parentId && crumb.childId != parentId && crumb.isVisible; }
		);
		return bread != _topics.end();
	};

	// Children collapsed
	if (getTopic().childId == getTopic().parentId && !isExpanded(getTopic().parentId))
	{
		// Show all elements contributing to parent.
		for (auto& topic : _topics)
		{
			if (topic.parentId == getTopic().childId)
			{
				topic.isVisible = true;
			}
		}
	}
	else
	{
		// Collapse all elements contributing to parent.
		for (auto& topic : _topics)
		{
			if (topic.parentId == getTopic().parentId && (topic.childId != topic.parentId))
			{
				topic.isVisible = false;
			}
		}
	}

	drawList();
	//_lstTopics->scrollTo(scrollPos);
}


/**
* Shows the dependencies tree.
*/
void ManufactureDependenciesTreeState::drawList()
{
	_lstTopics->clearList();
	_indices.clear();

	for (size_t i = 0; i < _topics.size(); ++i)
	{
		if (!_topics[i].isVisible) continue;

		_lstTopics->addRow(1, _topics[i].description.c_str());
		_indices.push_back(i);
	}
}

/**
 * Add item has research benefits section to '_topics` backend vector.
 *
 * @param parenId Id to use for the first parent that enters the list (will be updated).
*/
void ManufactureDependenciesTreeState::addResearchSection(int& parentId)
{
	int startingParent = parentId;
	TopicsBackend row = {};

	// Item research potential (only if theoretically possibility exist).
	for (auto& researchProject : _game->getMod()->getResearchList())
	{
		if (researchProject != _selectedItem) continue;

		RuleResearch *researchRule =  _game->getMod()->getResearch(researchProject);
		if (!researchRule || !researchRule->needItem()) continue;

		// Does there exist a possibility this project is/becomes researchable?
		// Adapted snippet from SavedGame::getAvailableResearchProjects()
		if (!_game->getSavedGame()->isResearched(researchRule, false) ||
			_game->getSavedGame()->isResearchRuleStatusDisabled(researchRule->getName()) ||
			_game->getSavedGame()->hasUndiscoveredGetOneFree(researchRule, false) ||
			_game->getSavedGame()->hasUndiscoveredProtectedUnlock(researchRule, _game->getMod()))
		{
			row = {parentId, parentId, true, tr("STR_CAN_RESEARCH").arg(tr("STR_YES"))};
			_topics.push_back(row);
			parentId++;
		}
		// Relation between items and research projects is 1-to-1.
		break;
	}

	// Section divider (no divider if research is hidden).
	if (startingParent < parentId)
	{
		row = {parentId, parentId, true, ""};
		_topics.push_back(row);
		parentId++;
	}
}

/**
 * Add ways to acquire item sections to '_topics` backend vector.
 *
 * Recognize 3 possibilities:
 * + Ability to buy this item.
 * + Manufacture projects that give this item.
 * + Manufacture projects with a random possibility to get this item.
 *
 * @param parenId Id to use for the first parent that enters the list (will be updated).
*/
void ManufactureDependenciesTreeState::addHowToAcquireItemSections(int& parentId)
{
	RuleItem *ruleSelected = _game->getMod()->getItem(_selectedItem);
	if (!ruleSelected) return;

	// Helper data
	std::vector<std::string> providersDirect, providersRandom;
	const std::vector<std::string> &manufactureProjects = _game->getMod()->getManufactureList();
	for (std::vector<std::string>::const_iterator i = manufactureProjects.begin(); i != manufactureProjects.end(); ++i)
	{
		RuleManufacture *ruleProject = _game->getMod()->getManufacture((*i));
		for (auto& ruleNormal : ruleProject->getProducedItems())
		{
			//std::map<const RuleItem*, int>
			if (ruleNormal.first == ruleSelected)
			{
				providersDirect.push_back((*i));
				break;
			}
		}
		// Include random production, although not sure if that does expose too much.
		for (auto& ruleRandom : ruleProject->getRandomProducedItems())
		{
			//std::vector<std::pair<int, std::map<const RuleItem*, int> > >
			for (auto itemRandom : ruleRandom.second)
			{
				if (itemRandom.first == ruleSelected)
				{
					// Prevent duplicates
					if ( std::find(providersRandom.begin(), providersRandom.end(), *i) == providersRandom.end() )
					{
						providersRandom.push_back((*i));
						break;
					}
				}
			}
		}
	}

	int startingParent = parentId;
	TopicsBackend row = {};

	// 1) Ability to buy item (only if theoretically possibility exist).
	// Does not take into account any limits from:
	// - Available global stock
	// - Country favor factor
	// - Base placement.
	if (ruleSelected->getBuyCost() != 0)
	{
		if (_game->getSavedGame()->isResearched(ruleSelected->getRequirements()) &&
			_game->getSavedGame()->isResearched(ruleSelected->getBuyRequirements()))
		{
			row = {parentId, parentId, true, tr("STR_CAN_BUY").arg(tr("STR_YES"))};
			_topics.push_back(row);
			parentId++;
		}
		else if (_showAll)
		{
			row = {parentId, parentId, true, tr("STR_CAN_BUY").arg(tr("STR_NO"))};
			_topics.push_back(row);
			parentId++;
		}
	}

	// 2) Direct manufacture
	if (!providersDirect.empty())
	{
		size_t parentIndex = _topics.size();
		row = {parentId, parentId, true, tr("STR_DIRECT_PROVIDERS").arg(providersDirect.size())};
		_topics.push_back(row);

		size_t countKnown = 0;
		int childId = parentId + 1; // 'childId' must be > 'parentId'.
		for (auto directManufacture : providersDirect)
		{
			if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture(directManufacture)->getRequirements()))
			{
				row = {childId, parentId, false, tr(directManufacture)};
				_topics.push_back(row);
				childId++;
				countKnown++;
			}
		}

		if (countKnown < providersDirect.size())
		{
			_topics[parentIndex].description = tr("STR_DIRECT_PROVIDERS").arg(std::to_string(countKnown) + "+");
			row = {childId, parentId, false, "***"};
			_topics.push_back(row);
		}
		parentId++;
	}

	// 3) Random from manufacture
	if (!providersRandom.empty())
	{
		size_t parentIndex = _topics.size();
		row = {parentId, parentId, true, tr("STR_RANDOM_PROVIDERS").arg(providersRandom.size())};
		_topics.push_back(row);

		size_t countKnown = 0;
		int childId = parentId + 1; // 'childId' must be > 'parentId'.
		for (auto randomManufacture : providersRandom)
		{
			if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture(randomManufacture)->getRequirements()))
			{
				row = {childId, parentId, false, tr(randomManufacture)};
				_topics.push_back(row);
				childId++;
				countKnown++;
			}
		}
		if (countKnown < providersRandom.size())
		{
			_topics[parentIndex].description = tr("STR_RANDOM_PROVIDERS").arg(std::to_string(countKnown) + "+");
			row = {childId, parentId, false, "***"};
			_topics.push_back(row);
		}
		parentId++;
	}

	// Section divider.
	if (startingParent < parentId)
	{
		row = {parentId, parentId, true, ""};
		_topics.push_back(row);
		parentId++;
	}
}

/**
 * Add item required for specials to '_topics` backend vector.
 *
 * Recognize 3 possibilities:
 * + Soldier transformations.
 * + Building facilities.
 * + Ammo for base defenses.
 *
 * @param parenId Id to use for the first parent that enters the list (will be updated).
*/
void ManufactureDependenciesTreeState::addNeededForSpecialsSections(int& parentId)
{
	RuleItem *ruleSelected = _game->getMod()->getItem(_selectedItem);
	if (!ruleSelected) return;

	int startingParent = parentId;
	TopicsBackend row = {};

	// Helper data
	std::vector<std::string> inputTransformations;
	// Transformations, based on `SavedGame::getAvailableTransformations()`.
	const std::vector<std::string> &soldierTransformations = _game->getMod()->getSoldierTransformationList();
	for (auto transformation : soldierTransformations)
	{
		RuleSoldierTransformation *ruleTransformation = _game->getMod()->getSoldierTransformation(transformation);
		for (auto item : ruleTransformation->getRequiredItems())
		{
			if (ruleSelected->getType() == item.first)
			{
				inputTransformations.push_back(transformation);
				break;
			}
		}
	}
	std::vector<const RuleBaseFacility*> inputFacilities, ammoFacilities;
	for (auto& facilityId : _game->getMod()->getBaseFacilitiesList())
	{
		RuleBaseFacility* facilityRule = _game->getMod()->getBaseFacility(facilityId);
		for (auto& itemRequired : facilityRule->getBuildCostItems())
		{
			if (itemRequired.first == _selectedItem)
			{
				inputFacilities.push_back(facilityRule);
				break;
			}
		}
		if (facilityRule->getAmmoItem() == ruleSelected)
		{
			ammoFacilities.push_back(facilityRule);
		}
	}

	// 1) Item required for soldier transformations.
	if (!inputTransformations.empty())
	{
		size_t parentIndex = _topics.size();
		row = {parentId, parentId, true, tr("STR_INPUT_TRANSFORMATIONS").arg(inputTransformations.size())};
		_topics.push_back(row);

		size_t countKnown = 0;
		int childId = parentId + 1;
		for (auto transform : inputTransformations)
		{
			if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getSoldierTransformation(transform)->getRequiredResearch()))
			{
				row = {childId, parentId, false, tr(transform)};
				_topics.push_back(row);
				childId++;
				countKnown++;
			}
		}

		if (countKnown < inputTransformations.size())
		{
			_topics[parentIndex].description = tr("STR_INPUT_TRANSFORMATIONS").arg(std::to_string(countKnown) + "+");
			row = {childId, parentId, false, "***"};
			_topics.push_back(row);
		}
		parentId++;
	}

	// 2) Item required to build facilities.
	if (!inputFacilities.empty())
	{
		size_t parentIndex = _topics.size();
		row = {parentId, parentId, true, tr("STR_INPUT_FACILITIES").arg(inputFacilities.size())};
		_topics.push_back(row);

		size_t countKnown = 0;
		int childId = parentId + 1;
		for (auto facilityRule : inputFacilities)
		{
			if (_showAll || _game->getSavedGame()->isResearched(facilityRule->getRequirements()))
			{
				row = {childId, parentId, false, tr(facilityRule->getType())};
				_topics.push_back(row);
				childId++;
				countKnown++;
			}
		}

		if (countKnown < inputFacilities.size())
		{
			_topics[parentIndex].description = tr("STR_INPUT_FACILITIES").arg(std::to_string(countKnown) + "+");
			row = {childId, parentId, false, "***"};
			_topics.push_back(row);
		}
		parentId++;
	}
	// 3) Item used as ammo for defense facilities.
	if (!ammoFacilities.empty())
	{
		size_t parentIndex = _topics.size();
		row = {parentId, parentId, true, tr("STR_INPUT_DEFENSE").arg(ammoFacilities.size())};
		_topics.push_back(row);

		size_t countKnown = 0;
		int childId = parentId + 1;
		for (auto facilityRule : ammoFacilities)
		{
			if (_showAll || _game->getSavedGame()->isResearched(facilityRule->getRequirements()))
			{
				row = {childId, parentId, false, tr(facilityRule->getType())};
				_topics.push_back(row);
				childId++;
				countKnown++;
			}
		}

		if (countKnown < ammoFacilities.size())
		{
			_topics[parentIndex].description = tr("STR_INPUT_DEFENSE").arg(std::to_string(countKnown) + "+");
			row = {childId, parentId, false, "***"};
			_topics.push_back(row);
		}
		parentId++;
	}

	// Section divider.
	if (startingParent < parentId)
	{
		row = {parentId, parentId, true, ""};
		_topics.push_back(row);
		parentId++;
	}
}


/**
 * Add item required for manufacture to '_topics` backend vector.
 *
 * Original Content of this screen (minus facilities).
 *
 * @param parenId Id to use for the first parent that enters the list (will be updated).
*/
void ManufactureDependenciesTreeState::addNeededForManufactureSections(int& parentId)
{
	int startingParent = parentId;
	TopicsBackend row = {};

	// dependency map (item -> vector of items that needs this item)
	std::unordered_map< std::string, std::vector<std::string> > deps;

	const std::vector<std::string> &manufactureItems = _game->getMod()->getManufactureList();
	for (std::vector<std::string>::const_iterator i = manufactureItems.begin(); i != manufactureItems.end(); ++i)
	{
		RuleManufacture *rule = _game->getMod()->getManufacture((*i));
		for (auto& j : rule->getRequiredItems())
		{
			deps[j.first->getType()].push_back((*i));
		}
	}
	// breadth-first tree search
	const std::vector<std::string> firstLevel = deps[_selectedItem];
	std::vector<std::string> secondLevel;
	std::vector<std::string> thirdLevel;
	std::vector<std::string> fourthLevel;
	std::vector<std::string> fifthLevel;

	std::unordered_set<std::string> alreadyVisited;
	alreadyVisited.insert(_selectedItem);
	for (std::vector<std::string>::const_iterator i = firstLevel.begin(); i != firstLevel.end(); ++i)
	{
		alreadyVisited.insert((*i));
	}

	std::ostringstream ss1;
	if (firstLevel.empty())
	{
		ss1 << Unicode::TOK_COLOR_FLIP << tr("STR_NO_DEPENDENCIES");
		row = {parentId, parentId, true, ss1.str()};
		_topics.push_back(row);
	}
	else
	{
		size_t parentIndex = _topics.size();
		ss1 << Unicode::TOK_COLOR_FLIP << tr("STR_DIRECT_DEPENDENCIES") << " " << Unicode::TOK_COLOR_FLIP;
		row = {parentId, parentId, true, ss1.str() + std::to_string(firstLevel.size())};
		_topics.push_back(row);

		size_t countKnown = 0;
		int childId = parentId + 1;
		for (std::vector<std::string>::const_iterator i = firstLevel.begin(); i != firstLevel.end(); ++i)
		{
			if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture((*i))->getRequirements()))
			{
				row = {childId, parentId, false, tr((*i))};
				_topics.push_back(row);
				childId++;
				countKnown++;
			}

			const std::vector<std::string> goDeeper = deps[(*i)];
			for (std::vector<std::string>::const_iterator j = goDeeper.begin(); j != goDeeper.end(); ++j)
			{
				if (alreadyVisited.find((*j)) == alreadyVisited.end())
				{
					secondLevel.push_back((*j));
					alreadyVisited.insert((*j));
				}
			}
		}

		if (countKnown < firstLevel.size())
		{
			ss1 << countKnown << "+";
			_topics[parentIndex].description = ss1.str();

			row = {childId, parentId, false, "***"};
			_topics.push_back(row);
		}
		parentId++;
	}

	if (!secondLevel.empty())
	{
		size_t parentIndex = _topics.size();
		std::ostringstream ss2;
		ss2 << Unicode::TOK_COLOR_FLIP << tr("STR_LEVEL_2_DEPENDENCIES") << " " << Unicode::TOK_COLOR_FLIP;
		row = {parentId, parentId, true, ss2.str() + std::to_string(secondLevel.size())};
		_topics.push_back(row);

		size_t countKnown = 0;
		int childId = parentId + 1;
		for (std::vector<std::string>::const_iterator i = secondLevel.begin(); i != secondLevel.end(); ++i)
		{
			if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture((*i))->getRequirements()))
			{
				row = {childId, parentId, false, tr((*i))};
				_topics.push_back(row);
				childId++;
				countKnown++;
			}

			const std::vector<std::string> goDeeper = deps[(*i)];
			for (std::vector<std::string>::const_iterator j = goDeeper.begin(); j != goDeeper.end(); ++j)
			{
				if (alreadyVisited.find((*j)) == alreadyVisited.end())
				{
					thirdLevel.push_back((*j));
					alreadyVisited.insert((*j));
				}
			}
		}

		// Expose less info, only tell there exist unlocked opportunities.
		if (countKnown < secondLevel.size())
		{
			// Fix subtopic description,
			ss2 << countKnown << "+";
			_topics[parentIndex].description = ss2.str();

			row = {childId, parentId, false, "***"};
			_topics.push_back(row);
		}
		parentId++;
	}

	if (!thirdLevel.empty())
	{
		size_t parentIndex = _topics.size();
		std::ostringstream ss3;
		ss3 << Unicode::TOK_COLOR_FLIP << tr("STR_LEVEL_3_DEPENDENCIES") << " " << Unicode::TOK_COLOR_FLIP;
		row = {parentId, parentId, true, ss3.str() + std::to_string(thirdLevel.size())};
		_topics.push_back(row);

		size_t countKnown = 0;
		int childId = parentId + 1;
		for (std::vector<std::string>::const_iterator i = thirdLevel.begin(); i != thirdLevel.end(); ++i)
		{
			if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture((*i))->getRequirements()))
			{
				row = {childId, parentId, false, tr((*i))};
				_topics.push_back(row);
				childId++;
				countKnown++;
			}

			const std::vector<std::string> goDeeper = deps[(*i)];
			for (std::vector<std::string>::const_iterator j = goDeeper.begin(); j != goDeeper.end(); ++j)
			{
				if (alreadyVisited.find((*j)) == alreadyVisited.end())
				{
					fourthLevel.push_back((*j));
					alreadyVisited.insert((*j));
				}
			}
		}

		// Expose less info, only tell there exist unlocked opportunities.
		if (countKnown < thirdLevel.size())
		{
			// Fix subtopic description,
			ss3 << countKnown << "+";
			_topics[parentIndex].description = ss3.str();

			row = {childId, parentId, false, "***"};
			_topics.push_back(row);
		}
		parentId++;
	}

	if (!fourthLevel.empty())
	{
		size_t parentIndex = _topics.size();
		std::ostringstream ss4;
		ss4 << Unicode::TOK_COLOR_FLIP << tr("STR_LEVEL_4_DEPENDENCIES") << " " << Unicode::TOK_COLOR_FLIP;
		row = {parentId, parentId, true, ss4.str() + std::to_string(fourthLevel.size())};
		_topics.push_back(row);

		size_t countKnown = 0;
		int childId = parentId + 1;
		for (std::vector<std::string>::const_iterator i = fourthLevel.begin(); i != fourthLevel.end(); ++i)
		{
			if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture((*i))->getRequirements()))
			{
				row = {childId, parentId, false, tr((*i))};
				_topics.push_back(row);
				childId++;
				countKnown++;
			}

			const std::vector<std::string> goDeeper = deps[(*i)];
			for (std::vector<std::string>::const_iterator j = goDeeper.begin(); j != goDeeper.end(); ++j)
			{
				if (alreadyVisited.find((*j)) == alreadyVisited.end())
				{
					fifthLevel.push_back((*j));
					alreadyVisited.insert((*j));
				}
			}
		}

		// Expose less info, only tell there exist unlocked opportunities.
		if (countKnown < fourthLevel.size())
		{
			// Fix subtopic description,
			ss4 << countKnown << "+";
			_topics[parentIndex].description = ss4.str();

			row = {childId, parentId, false, "***"};
			_topics.push_back(row);
		}
		parentId++;
	}

	// Section divider.
	if (startingParent < parentId)
	{
		row = {parentId, parentId, true, ""};
		_topics.push_back(row);
		parentId++;
	}

	std::ostringstream ss5;
	if (!fifthLevel.empty())
	{
		ss5 << Unicode::TOK_COLOR_FLIP << tr("STR_MORE_DEPENDENCIES");
		row = {parentId, parentId, true, ss5.str()};
		_topics.push_back(row);
	}
	else
	{
		ss5 << Unicode::TOK_COLOR_FLIP << tr("STR_END_OF_SEARCH");
		row = {parentId, parentId, true, ss5.str()};
		_topics.push_back(row);
	}
}

}
