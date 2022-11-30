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
ManufactureDependenciesTreeState::ManufactureDependenciesTreeState(const std::string &selectedItem, ScreenType screen) : _selectedItem(selectedItem), _showAll(false)
{
	_screen = false;
	_currentScreen = screen;

	_window = new Window(this, 222, 144, 49, 32);
	_txtTitle = new Text(182, 9, 53, 42);
	_lstTopics = new TextList(198, 96, 53, 54);
	_btnShowAll = new TextButton(100, 16, 57, 153);
	_btnOk = new TextButton(100, 16, 163, 153);
	_btnToggle = new TextButton(28, 14, 235, 39);

	// Set palette
	setInterface("dependencyTree");

	add(_window, "window", "dependencyTree");
	add(_txtTitle, "text", "dependencyTree");
	add(_btnShowAll, "button", "dependencyTree");
	add(_btnOk, "button", "dependencyTree");
	add(_lstTopics, "list", "dependencyTree");
	add(_btnToggle, "button", "dependencyTree");

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
	_btnToggle->setText(">>");
	_btnToggle->onMouseClick((ActionHandler)&ManufactureDependenciesTreeState::screenToggle);
	_btnToggle->onKeyboardPress((ActionHandler)&ManufactureDependenciesTreeState::screenToggle, Options::keyBattleNextUnit);
	_btnToggle->onKeyboardPress((ActionHandler)&ManufactureDependenciesTreeState::screenToggle, Options::keyBattlePrevUnit);

	_lstTopics->setColumns(1, 182);
	_lstTopics->setBackground(_window);
	_lstTopics->setMargin(0);
	_lstTopics->setAlign(ALIGN_CENTER);

	if (Options::oxceDisableProductionDependencyTree)
	{
		_txtTitle->setHeight(_txtTitle->getHeight() * 11);
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr("STR_THIS_FEATURE_IS_DISABLED_3"));
		_btnShowAll->setVisible(false);
		_lstTopics->setVisible(false);
		_btnToggle->setVisible(false);
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

	fillTopicsList();

	if (!Options::oxceDisableProductionDependencyTree)
	{
		if (_currentScreen == ST_DEPENDENCIES)
		{
			screenDependencies();
		}
		else
		{
			screenProviders();
		}
	}
}

/**
* Build workhorse vector of topics for this item.
*/
void ManufactureDependenciesTreeState::fillTopicsList()
{
	_topics.clear();

	int id = 0;
	int parentId = 0;
	int screenListOrder = 0; // We have multiple topics with (duplicate) item listorders.
	TopicsBackend row = {};

	// Item research potential (independent of base facilities).
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
			row = {id, parentId, screenListOrder, true, true, tr("STR_CAN_RESEARCH").arg(tr("STR_YES"))};
		}
		else
		{
			// No point in showing (by default) if item is no longer available for research.
			row = {id, parentId, screenListOrder, _showAll, true, tr("STR_CAN_RESEARCH").arg(tr("STR_NO"))};
		}

		_topics.push_back(row);
		id++;
		parentId++;
		screenListOrder++;
		break;
	}
	// If item cannot be used in research there is no need to add this element to the list.
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
 * Goes to the next screen on item's manufacture
 * @param action Pointer to an action.
 */
void ManufactureDependenciesTreeState::screenToggle(Action *)
{
	if (_currentScreen == ST_DEPENDENCIES)
	{
		screenProviders();
		_currentScreen = ST_PROVIDERS;
	}
	else
	{
		screenDependencies();
		_currentScreen = ST_DEPENDENCIES;
	}
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

	if (_currentScreen == ST_DEPENDENCIES)
		screenDependencies();
	else
		screenProviders();
}

/**
* Shows the dependencies tree.
*/
void ManufactureDependenciesTreeState::screenDependencies()
{
	_lstTopics->clearList();

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

	std::vector<const RuleBaseFacility*> facilitiesLevel;
	for (auto& facilityId : _game->getMod()->getBaseFacilitiesList())
	{
		RuleBaseFacility* facilityRule = _game->getMod()->getBaseFacility(facilityId);
		for (auto& itemRequired : facilityRule->getBuildCostItems())
		{
			if (itemRequired.first == _selectedItem)
			{
				facilitiesLevel.push_back(facilityRule);
				break;
			}
		}
	}

	if (firstLevel.empty() && facilitiesLevel.empty())
	{
		_lstTopics->addRow(1, tr("STR_NO_DEPENDENCIES").c_str());
		_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		++row;
		return;
	}

	// first level
	_lstTopics->addRow(1, tr("STR_DIRECT_DEPENDENCIES").c_str());
	_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
	++row;

	bool hasHiddenRows = false;
	// first list all the dependent base facilities
	for (auto& i : facilitiesLevel)
	{
		if (_showAll || _game->getSavedGame()->isResearched(i->getRequirements()))
		{
			_lstTopics->addRow(1, tr(i->getType()).c_str());
			++row;
		}
		else
		{
			hasHiddenRows = true;
		}
	}

	for (std::vector<std::string>::const_iterator i = firstLevel.begin(); i != firstLevel.end(); ++i)
	{
		if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture((*i))->getRequirements()))
		{
			_lstTopics->addRow(1, tr((*i)).c_str());
			++row;
		}
		else
		{
			hasHiddenRows = true;
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
	// Expose less info, only tell there exist unlocked opportunities.
	if (hasHiddenRows)
	{
		_lstTopics->addRow(1, "***");
		++row;
	}

	_lstTopics->addRow(1, "");
	++row;
	if (secondLevel.empty())
	{
		_lstTopics->addRow(1, tr("STR_END_OF_SEARCH").c_str());
		_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		++row;
		return;
	}

	// second level
	_lstTopics->addRow(1, tr("STR_LEVEL_2_DEPENDENCIES").c_str());
	_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
	++row;

	hasHiddenRows = false;
	for (std::vector<std::string>::const_iterator i = secondLevel.begin(); i != secondLevel.end(); ++i)
	{
		if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture((*i))->getRequirements()))
		{
			_lstTopics->addRow(1, tr((*i)).c_str());
			++row;
		}
		else
		{
			hasHiddenRows = true;
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
	if (hasHiddenRows)
	{
		_lstTopics->addRow(1, "***");
		++row;
	}

	_lstTopics->addRow(1, "");
	++row;
	if (thirdLevel.empty())
	{
		_lstTopics->addRow(1, tr("STR_END_OF_SEARCH").c_str());
		_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		++row;
		return;
	}

	// third level
	_lstTopics->addRow(1, tr("STR_LEVEL_3_DEPENDENCIES").c_str());
	_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
	++row;

	hasHiddenRows = false;
	for (std::vector<std::string>::const_iterator i = thirdLevel.begin(); i != thirdLevel.end(); ++i)
	{
		if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture((*i))->getRequirements()))
		{
			_lstTopics->addRow(1, tr((*i)).c_str());
			++row;
		}
		else
		{
			hasHiddenRows = true;
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
	if (hasHiddenRows)
	{
		_lstTopics->addRow(1, "***");
		++row;
	}

	_lstTopics->addRow(1, "");
	++row;
	if (fourthLevel.empty())
	{
		_lstTopics->addRow(1, tr("STR_END_OF_SEARCH").c_str());
		_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		++row;
		return;
	}

	// fourth level
	_lstTopics->addRow(1, tr("STR_LEVEL_4_DEPENDENCIES").c_str());
	_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
	++row;

	hasHiddenRows = false;
	for (std::vector<std::string>::const_iterator i = fourthLevel.begin(); i != fourthLevel.end(); ++i)
	{
		if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture((*i))->getRequirements()))
		{
			_lstTopics->addRow(1, tr((*i)).c_str());
			++row;
		}
		else
		{
			hasHiddenRows = true;
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
	if (hasHiddenRows)
	{
		_lstTopics->addRow(1, "***");
		++row;
	}

	_lstTopics->addRow(1, "");
	++row;
	if (fifthLevel.empty())
	{
		_lstTopics->addRow(1, tr("STR_END_OF_SEARCH").c_str());
		_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		++row;
		return;
	}

	_lstTopics->addRow(1, tr("STR_MORE_DEPENDENCIES").c_str());
	_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
	++row;
}

/**
 * Shows the providers tree.
 *
 * Only shows direct ways of acquiring (no levels).
 * Only takes into account research checks not if base has required functionalities.
 *
 * Recognize 4 possible categories
 * (0) Get from Manufacture directly
 * (1) Get from Manufacture as random item
 * (2) Get from facilities : Mechanic does not exist
 * (3) Get from direct buy : Outside scope for this screen
*/
void ManufactureDependenciesTreeState::screenProviders()
{
	_lstTopics->clearList();

	RuleItem *ruleSelected = _game->getMod()->getItem(_selectedItem);
	bool isBuyable = ruleSelected->getBuyCost() != 0;

	// provider: Vector of manufacture projects that (might) provide this item.
	std::vector<std::string> providerDirect, providerRandom;

	const std::vector<std::string> &manufactureProjects = _game->getMod()->getManufactureList();
	for (std::vector<std::string>::const_iterator i = manufactureProjects.begin(); i != manufactureProjects.end(); ++i)
	{
		RuleManufacture *ruleProject = _game->getMod()->getManufacture((*i));
		for (auto& ruleNormal : ruleProject->getProducedItems())
		{
			//std::map<const RuleItem*, int>
			if (ruleNormal.first == ruleSelected)
			{
				providerDirect.push_back((*i));
				break;
			}
		}
		// Include random production, not sure if that does expose too much.
		for (auto& ruleRandom : ruleProject->getRandomProducedItems())
		{
			//std::vector<std::pair<int, std::map<const RuleItem*, int> > >
			for (auto itemRandom : ruleRandom.second)
			{
				if (itemRandom.first == ruleSelected)
				{
					// Prevent duplicates
					if ( std::find(providerRandom.begin(), providerRandom.end(), *i) == providerRandom.end() )
					{
						providerRandom.push_back((*i));
						break;
					}
				}
			}
		}
	}

	int row = 0;
	if (providerDirect.empty() && providerRandom.empty() && !isBuyable)
	{
		_lstTopics->addRow(1, tr("STR_NO_PROVIDERS").c_str());
		_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		++row;
		return;
	}

	// Can we buy item
	std::ostringstream ss;
	if(_showAll || (_game->getSavedGame()->isResearched(ruleSelected->getRequirements()) && _game->getSavedGame()->isResearched(ruleSelected->getBuyRequirements())))
	{
		ss << Unicode::TOK_COLOR_FLIP << tr("STR_CAN_BUY").arg(isBuyable ? tr("STR_YES") : tr("STR_NO"));
	}
	else
	{
		ss << Unicode::TOK_COLOR_FLIP << tr("STR_CAN_BUY").arg(tr("STR_UNKNOWN"));
	}
	_lstTopics->addRow(1, ss.str().c_str());
	++row;
	_lstTopics->addRow(1, "");
	++row;

	// Get from Manufacture directly
	if (!providerDirect.empty())
	{
		_lstTopics->addRow(1, tr("STR_DIRECT_PROVIDERS").c_str());
		_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		++row;

		bool hasHiddenRows = false;
		for (auto directManufacture : providerDirect)
		{
			if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture(directManufacture)->getRequirements()))
			{
				_lstTopics->addRow(1, tr(directManufacture).c_str());
				++row;
			}
			else
			{
				hasHiddenRows = true;
			}
		}
		// Expose less info, only tell there exist unlocked opportunities.
		if (hasHiddenRows)
		{
			_lstTopics->addRow(1, "***");
			++row;
		}

		_lstTopics->addRow(1, "");
		++row;
	}

	// Get from Manufacture as random item
	if (!providerRandom.empty())
	{
		_lstTopics->addRow(1, tr("STR_RANDOM_PROVIDERS").c_str());
		_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
		++row;

		bool hasHiddenRows = false;
		for (auto randomManufacture : providerRandom)
		{
			if (_showAll || _game->getSavedGame()->isResearched(_game->getMod()->getManufacture(randomManufacture)->getRequirements()))
			{
				_lstTopics->addRow(1, tr(randomManufacture).c_str());
				++row;
			}
			else
			{
				hasHiddenRows = true;
			}
		}
		// Expose less info, only tell there exist unlocked opportunities.
		if (hasHiddenRows)
		{
			_lstTopics->addRow(1, "***");
			++row;
		}

		_lstTopics->addRow(1, "");
		++row;
	}

	_lstTopics->addRow(1, tr("STR_END_OF_SEARCH").c_str());
	_lstTopics->setRowColor(row, _lstTopics->getSecondaryColor());
}

}
