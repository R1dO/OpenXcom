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
#include "SoldierArmorState.h"
#include <sstream>
#include <algorithm>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ArrowButton.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/ComboBox.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleEnviroEffects.h"
#include "../Savegame/AlienBase.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Ufo.h"
#include "../Mod/AlienDeployment.h"
#include "../Mod/AlienRace.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleStartingCondition.h"
#include "../Mod/RuleTerrain.h"
#include "../Ufopaedia/Ufopaedia.h"

namespace OpenXcom
{

struct compareArmorName
{
	typedef ArmorItem& first_argument_type;
	typedef ArmorItem& second_argument_type;
	typedef bool result_type;

	bool _reverse;

	compareArmorName(bool reverse) : _reverse(reverse) {}

	bool operator()(const ArmorItem &a, const ArmorItem &b) const
	{
		return Unicode::naturalCompare(a.name, b.name);
	}
};


/**
 * Initializes all the elements in the Soldier Armor window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
SoldierArmorState::SoldierArmorState(Base *base, size_t soldier, SoldierArmorOrigin origin) : _base(base), _soldier(soldier), _origin(origin)
{
	_screen = false;
	_alternateScreen = Options::alternateBaseScreens;

	// Create objects
	_window = new Window(this, 192, 160, 64, 20, POPUP_BOTH);
	_btnQuickSearch = new TextEdit(this, 48, 9, 80, 43);
	_btnCancel = new TextButton(140, 16, 90, 156);
	_txtTitle = new Text(182, 16, 69, 28);
	_txtType = new Text(90, 9, 80, 52);
	_txtQuantity = new Text(70, 9, 190, 52);
	_lstArmor = new TextList(160, 80, 73, 68);
	_sortName = new ArrowButton(ARROW_NONE, 11, 8, 80, 52);
	_cbxCategory = new ComboBox(this, 120, 16, 73, 48);

	// Set palette
	if (_origin == SA_BATTLESCAPE)
	{
		setStandardPalette("PAL_BATTLESCAPE");
	}
	else
	{
		setInterface("soldierArmor");
	}

	add(_window, "window", "soldierArmor");
	add(_btnQuickSearch, "button", "soldierArmor");
	add(_btnCancel, "button", "soldierArmor");
	add(_txtTitle, "text", "soldierArmor");
	add(_txtType, "text", "soldierArmor");
	add(_txtQuantity, "text", "soldierArmor");
	add(_lstArmor, "list", "soldierArmor");
	add(_sortName, "text", "soldierArmor");
	add(_cbxCategory, "text", "soldierArmor");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "soldierArmor");

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SoldierArmorState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SoldierArmorState::btnCancelClick, Options::keyCancel);

	Soldier *s = _base->getSoldiers()->at(_soldier);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELECT_ARMOR_FOR_SOLDIER").arg(s->getName()));

	_txtType->setText(tr("STR_TYPE"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstArmor->setColumns(2, 132, 21);
	_lstArmor->setSelectable(true);
	_lstArmor->setBackground(_window);
	_lstArmor->setMargin(8);

	_sortName->setX(_sortName->getX() + _txtType->getTextWidth() + 4);
	_sortName->onMouseClick((ActionHandler)&SoldierArmorState::sortNameClick);

	// Add deployment to filter categories if one of the following conditions hold.
	// - it has startingConditions on armors.
	// - it has environmental armorTransformations defined.
	auto addToCats = [&](AlienDeployment *deploymentRule)
	{
		if (deploymentRule == 0) return;

		const RuleStartingCondition *startConditions = _game->getMod()->getStartingCondition(deploymentRule->getStartingCondition());
		const RuleEnviroEffects *enviroEffects = _game->getMod()->getEnviroEffects(deploymentRule->getEnviroEffects());

		if (startConditions)
		{
			auto listAllowed = startConditions->getAllowedArmors();
			auto listForbidden = startConditions->getForbiddenArmors();
			if (!listAllowed.empty() || !listForbidden.empty())
			{
				_cats.push_back(deploymentRule->getType());
				return;
			}
		}
		if (enviroEffects && enviroEffects->hasArmorTransformation())
		{
			_cats.push_back(deploymentRule->getType());
			return;
		}

		// Terrain might have environmental effects defined.
		for (auto terrain : deploymentRule->getTerrains())
		{
			RuleTerrain* terrainRule = _game->getMod()->getTerrain(terrain);
			enviroEffects = _game->getMod()->getEnviroEffects(terrainRule->getEnviroEffects());
			if (enviroEffects && enviroEffects->hasArmorTransformation())
			{
				_cats.push_back(deploymentRule->getType());
				return;
			}
		}
	};

	_cats.push_back("STR_DEFAULT");
	// Filter categories of allowed armors for detected alien deployments.
	// Based on: 'ConfirmLandingState::checkStartingCondition()'
	for (auto missionSite : *_game->getSavedGame()->getMissionSites())
	{
		if (!missionSite->getDetected()) continue;

		// We got vip tickets for an exclusive festival.
		addToCats(_game->getMod()->getDeployment(missionSite->getDeployment()->getType()));
	}
	for (auto alienBase : *_game->getSavedGame()->getAlienBases())
	{
		if (!alienBase->isDiscovered()) continue;

		// There might exist alien specific deployments.
		AlienRace *race = _game->getMod()->getAlienRace(alienBase->getAlienRace());
		AlienDeployment *ruleDeploy = _game->getMod()->getDeployment(race->getBaseCustomMission());
		if (!ruleDeploy) ruleDeploy = _game->getMod()->getDeployment(alienBase->getDeployment()->getType());

		// Hello neighbour, can I borrow a cup of sugar?
		addToCats(ruleDeploy);
	}
	for (auto ufo : *_game->getSavedGame()->getUfos())
	{
		if (!ufo->getDetected()) continue;

		// Only ufo's that can be considered 'stationary'.
		if (!(ufo->getStatus() == Ufo::LANDED || ufo->getStatus() == Ufo::CRASHED))
			continue;

		std::string ufoMissionName = ufo->getRules()->getType();

		// Nice day to get some fresh air.
		addToCats(_game->getMod()->getDeployment(ufoMissionName));

		// For fake underwater deployments we need access to globe texture
		// See also 'GeoscapeState::time5Seconds()'.
		// Since that is not readily available from this class it would mean:
		// - Adapting multiple classes for globe (or state) passthrough.
		// - Adds lots of globe related include dependencies
		//
		// A bit of overkill for this functionality. Instead I opted for the
		// 'brute force' approach below and accept the possibility of
		// extra categories without a geoscape ufo site.
		ufoMissionName = ufo->getRules()->getType() + "_UNDERWATER";

		// Anybody in for some skinny-dipping?
		addToCats(_game->getMod()->getDeployment(ufoMissionName));
	}

	_cbxCategory->setOptions(_cats, true);
	_cbxCategory->onChange((ActionHandler)&SoldierArmorState::cbxCategoryChange);
	_cbxCategory->setText(tr("STR_TYPE"));

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&SoldierArmorState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnCancel->onKeyboardRelease((ActionHandler)&SoldierArmorState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	_armorOrder = ARMOR_SORT_NONE;
	_previousOrder = ARMOR_SORT_NONE;
	updateArrows();

	_lstArmor->onMouseClick((ActionHandler)&SoldierArmorState::lstArmorClick);
	_lstArmor->onMouseClick((ActionHandler)&SoldierArmorState::lstArmorClickMiddle, SDL_BUTTON_MIDDLE);
	_lstArmor->onMouseClick((ActionHandler)&SoldierArmorState::lstArmorClickRight, SDL_BUTTON_RIGHT);

	// switch to battlescape theme if called from inventory
	if (_origin == SA_BATTLESCAPE)
	{
		applyBattlescapeTheme("soldierArmor");
	}

	// Show filtering only if there are missions which limit the armors
	if (_alternateScreen && _cats.size() > 1)
	{
		_btnQuickSearch->setX(_btnQuickSearch->getX() - 6);
		_btnQuickSearch->setY(_btnQuickSearch->getY() - 6);
		_txtType->setVisible(false);
		_txtQuantity->setX(_txtQuantity->getX() + 5);
		_sortName->setVisible(false);
	}
	else
	{
		_cbxCategory->setVisible(false);
	}

	fillArmorList();
}

/**
 *
 */
SoldierArmorState::~SoldierArmorState()
{

}

/**
 * Build workhorse vector of armors available for this soldier.
 *
 * Includes all armors subject to the following condition:
 * - In stores or visible in Ufopaedia.
 */
void SoldierArmorState::fillArmorList()
{
	_armors.clear();
	Soldier *s = _base->getSoldiers()->at(_soldier);

	// 'Subtotal' for armors with unknown parents.
	// Since 'type = ""' is not allowed in rulesets there should not be collisions.
	ArmorItem headerRow = {"", tr("STR_UNKNOWN"), "", {}};
	_armors.push_back(headerRow);

	// Don't depend on item listOrder, it does not exist for "STR_NONE" armors (storeItem is nullpointer).
	int screenListOrder = 1;
	const auto &armors = _game->getMod()->getArmorsForSoldiers();
	for (auto* a : armors)
	{
		if (!a->getCanBeUsedBy(s->getRules())) continue;

		if (a->getRequiredResearch() && !_game->getSavedGame()->isResearched(a->getRequiredResearch()))
			continue;

		int qty = -1;
		if (!a->hasInfiniteSupply())
		{
			bool addSoldierArmor = (s->getArmor()->getStoreItem() == a->getStoreItem()); // True for the complete armor family
			qty = _base->getStorageItems()->getItem(a->getStoreItem()) + addSoldierArmor;
		}

		bool isKnown = false;
		ArticleDefinition* article = _game->getMod()->getUfopaediaArticle(a->getType(), false);
		if (article && _game->getSavedGame()->isResearched(article->requires))
		{
			isKnown = true;
		}

		// Armor does not satisfy condition as mentioned in method description.
		if (qty == 0 && !isKnown) continue;

		ArmorItem row = {a->getType(), tr(a->getType()), "", {} };
		row.armor = a;
		row.qty = qty;
		row.listOrder = screenListOrder;
		row.isKnown = isKnown;
		// id, parentId & isVisible need not be set here.
		_armors.push_back(row);

		screenListOrder++;
	}

	updateList();
}

/**
* Updates the sorting arrows based
* on the current setting.
*/
void SoldierArmorState::updateArrows()
{
	_sortName->setShape(ARROW_NONE);
	switch (_armorOrder)
	{
	case ARMOR_SORT_NAME_ASC:
		_sortName->setShape(ARROW_SMALL_UP);
		break;
	case ARMOR_SORT_NAME_DESC:
		_sortName->setShape(ARROW_SMALL_DOWN);
		break;
	default:
		break;
	}
}

/**
* Sorts the armor list.
* @param sort Order to sort the armors in.
*/
void SoldierArmorState::sortList()
{
	switch (_armorOrder)
	{
	case ARMOR_SORT_NAME_ASC:
		std::sort(_armors.begin(), _armors.end(), compareArmorName(false));
		break;
	case ARMOR_SORT_NAME_DESC:
		std::sort(_armors.rbegin(), _armors.rend(), compareArmorName(true));
		break;
	default:
		std::sort(_armors.begin(), _armors.end(), [](const ArmorItem a, const ArmorItem b) { return a.listOrder < b.listOrder; });
		break;
	}
}

/**
* Updates the armor list subject to filter and button.
*/
void SoldierArmorState::updateList()
{
	// There is a trade-off here.
	// To prevent listing of all armors 'fillArmorList()' could only include
	// unknown armors with a 'storeItem' on base. This means transformed armors
	// will only be recognized if they have a pedia entry.

	size_t selCategory = _cbxCategory->getSelected();
	const std::string selectedCategory = _cats[selCategory];
	bool categoryFilterEnabled = (selectedCategory != "STR_DEFAULT");

	// Early sort to ensure both parents and children honor setting.
	sortList();

	if (categoryFilterEnabled)
	{
		const AlienDeployment *filterDeployment = _game->getMod()->getDeployment(selectedCategory);
		const RuleStartingCondition *filterStartCondition = _game->getMod()->getStartingCondition(filterDeployment->getStartingCondition());
		const RuleEnviroEffects *filterEnviroEffects = _game->getMod()->getEnviroEffects(filterDeployment->getEnviroEffects());
		if (!filterEnviroEffects)
		{
			// Try to get one from deployment terrain. First one with transformations wins.
			// Acceptable since actual terrain is determined at map generation (not known here).
			for (auto terrain : filterDeployment->getTerrains())
			{
				RuleTerrain *terrainRule = _game->getMod()->getTerrain(terrain);
				if (terrainRule->getEnviroEffects() != "" && _game->getMod()->getEnviroEffects(terrainRule->getEnviroEffects())->hasArmorTransformation())
				{
					filterEnviroEffects = _game->getMod()->getEnviroEffects(terrainRule->getEnviroEffects());
					break;
				}
			}
		}
		// There also exist terrain and deployment from missionTexture.
		// Those need access to globe though, hence not implemented.
		// Besides that, those are only needed in case '*filter...'
		// variables are still 'nullptrs' at this stage.

		// Soldier is not allowed on mission.
		//'Unkown' should suffice to indicate something is going on.
		// Hopefully that means: player choses to check mission description.
		Soldier *soldier = _base->getSoldiers()->at(_soldier);
		if (filterStartCondition && !filterStartCondition->isSoldierTypePermitted(soldier->getRules()->getType()))
		{
			for (auto& armorItem : _armors)
			{
				armorItem.isVisible = armorItem.type == "";
			}
			drawList();
			return;
		}

		// Get resulting armor as if it was an actual deployment.
		// Based on: `BattlescapeGenerator::deployXCOM()`, `::run()` and `::nextStage()`
		auto getResultingArmor = [&](const Armor* original) -> const Armor*
		{
			Armor* resultingArmor = nullptr;

			// 1. Deployment and Terrain based environmental armor transforms
			if (filterEnviroEffects)
			{
				resultingArmor = filterEnviroEffects->getArmorTransformation(original);
			}

			// 2. Deployment startingConditions (allowed, denied and default armors)
			if (!resultingArmor && !filterStartCondition)
			{
				// No transformation AND no startcondition limitations.
				return original;
			}
			else if (!resultingArmor)
			{
				std::string soldierType = _base->getSoldiers()->at(_soldier)->getRules()->getType();
				std::string replacedArmorType = filterStartCondition->getArmorReplacement(soldierType, original->getType());
				if (replacedArmorType == "")
				{
					return original;
				}

				resultingArmor = _game->getMod()->getArmor(replacedArmorType, true);
				if (resultingArmor && resultingArmor->getSize() > original->getSize())
				{
					// Cannot switch into a bigger armor size!
					resultingArmor = nullptr;
				}
			}

			return resultingArmor;
		};

		// Find parent of selected armor.
		auto findParentId = [&](const Armor *convertedArmor) -> int
		{
			auto bean = std::find_if(_armors.begin(), _armors.end(),
				[&](const ArmorItem row) { return row.armor == convertedArmor; });
			if (bean != _armors.end())
			{
				return (*bean).parentId;
			}
			// No (known) parent found or convertedArmor was nullptr.
			// Categorize as "Unknown" (and mark parent for display).
			auto crumb = std::find_if(_armors.begin(), _armors.end(),
				[&](ArmorItem row) { return row.type == ""; });
			if (crumb != _armors.end())
			{
				(*crumb).isVisible |= true;
				return (*crumb).parentId;
			}

			return -1; // Warning indicator: Armor will be pushed to the top.
		};

		auto hasChildren = [&](const int parent) -> bool
		{
			auto candy = std::find_if(_armors.begin(), _armors.end(),
			[&](const ArmorItem row) {return row.parentId == parent && row.id != parent;});

			return !(candy == _armors.end());
		};

		// 2-pass logic to ensure both 'subtotals' and 'elements' conform to sort order.
		// 1st pass: Handle all parents, reset all others.
		int parentId = 1; // Reserve 0 for blank slates.
		for (auto& armorItem : _armors)
		{
			if (armorItem.type == "" && armorItem.armor == nullptr)
			{
				// "Unknown" category. 2nd pass will set visibility.
				armorItem.id = armorItem.parentId = parentId;
				armorItem.isVisible = false;
				parentId++;
				continue;
			}
			else if (armorItem.armor == nullptr)
			{
				armorItem.resetIdsAndVisibility();
				continue;
			}

			const Armor* transformedArmor = getResultingArmor(armorItem.armor);

			// Armor cannot be used or is not a parent
			if (!transformedArmor || transformedArmor != armorItem.armor)
			{
				armorItem.resetIdsAndVisibility();
			}
			else
			{
				armorItem.id = armorItem.parentId = parentId;
				armorItem.isVisible = true;
				parentId++;
			}
		}
		// 2nd pass: Mark all (child) armors subject to transformations.
		int idArmor = parentId + 1;
		for (auto& armorItem : _armors)
		{
			if (armorItem.parentId != 0) continue;    // Parents were already set.
			if (armorItem.armor == nullptr) continue; // Safety
			if (armorItem.qty == 0) continue;         // Only available armors.

			const Armor* transformedArmor = getResultingArmor(armorItem.armor);

			// For as far as I can tell 'transformedArmor == nullptr' means
			// soldier will keep original armor when placed on the battlescape.
			// In that case "Unknown" category is acceptable.
			armorItem.parentId = findParentId(transformedArmor);
			armorItem.id = idArmor;
			armorItem.isVisible = true;
			idArmor++;
		}

		// Parents should always be listed before children.
		std::stable_sort(_armors.begin(), _armors.end(),
			[](const ArmorItem a, const ArmorItem b)
			{
				return std::tie(a.id, a.parentId) < std::tie(b.id, b.parentId);
			}
		);
		// Group parents and children.
		std::stable_sort(_armors.begin(), _armors.end(),
			[](const ArmorItem a, const ArmorItem b)
			{
				return a.parentId < b.parentId;
			}
		);

		// Only show 'virtual' (e.g. no-storeitem) parents if they have children
		for (auto& armorItem : _armors)
		{
			if (armorItem.id == armorItem.parentId && armorItem.qty == 0)
			{
				armorItem.isVisible = hasChildren(armorItem.parentId);
			}
		}
	}
	else
	{
		for (std::vector<ArmorItem>::iterator j = _armors.begin(); j != _armors.end(); ++j)
		{
			if ((*j).qty == 0)
			{
				(*j).resetIdsAndVisibility();
			}
			else
			{
				(*j).resetIdsAndVisibility(true);
			}
		}
	}

	drawList();
}

/**
 * Draws the armor list
 * @param action Pointer to an action.
 */
void SoldierArmorState::drawList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	bool isSkirmish = _game->getSavedGame()->getMonthsPassed() == -1;
	Armor *soldierArmor = _base->getSoldiers()->at(_soldier)->getArmor();
	_lstArmor->clearList();
	_indices.clear();

	for (size_t i = 0; i < _armors.size(); ++i)
	{
		if (!_armors[i].isVisible) continue;

		// quick search
		if (!searchString.empty())
		{
			std::string armorName = _armors[i].name;
			Unicode::upperCase(armorName);
			if (armorName.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		std::string quantity= ""; // qty == -1 e.g. infinite.
		if (_armors[i].qty == 0)
		{
			quantity = "*"; // Virtual armors or normals ones not present on this base.
		}
		else if (_armors[i].qty > 0)
		{
			quantity = (isSkirmish ? "-" : std::to_string(_armors[i].qty));
		}
		std::string armorName;
		// Use indentation to signal when an armor will be converted to parent armor.
		if(_armors[i].id != _armors[i].parentId)
		{
			armorName = " " + _armors[i].name;
			quantity = " " + quantity;
		}
		else
		{
			armorName = _armors[i].name;
		}
		_lstArmor->addRow(2, armorName.c_str(), quantity.c_str());

		if (_armors[i].id != _armors[i].parentId)
		{
			// Dont want to introduce a color2 yet to the interfaces.rul
			_lstArmor->setRowColor(_lstArmor->getLastRowIndex(), _window->getColor());
		}
		// Mark armor currently worn.
		if (_armors[i].armor == soldierArmor)
		{
			// Use tertiary color since secondary is not defined in default interfaces.rul.
			_lstArmor->setRowColor(_lstArmor->getLastRowIndex(), _lstArmor->getScrollbarColor());
		}
		_indices.push_back(i);
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierArmorState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Quick search toggle.
 * @param action Pointer to an action.
 */
void SoldierArmorState::btnQuickSearchToggle(Action* action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText("");
		_btnQuickSearch->setVisible(false);
		btnQuickSearchApply(action);
	}
	else
	{
		_btnQuickSearch->setVisible(true);
		_btnQuickSearch->setFocus(true);
	}
}

/**
 * Quick search.
 * @param action Pointer to an action.
 */
void SoldierArmorState::btnQuickSearchApply(Action*)
{
	updateList();
}

/**
 * Equips the armor on the soldier and returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierArmorState::lstArmorClick(Action *)
{
	if (_armors[_indices[_lstArmor->getSelectedRow()]].qty == 0)
		return;

	Soldier *soldier = _base->getSoldiers()->at(_soldier);
	Armor *prev = soldier->getArmor();
	Armor *next = _game->getMod()->getArmor(_armors[_indices[_lstArmor->getSelectedRow()]].type);
	Craft *craft = soldier->getCraft();
	if (craft)
	{
		if (!craft->validateArmorChange(prev->getSize(), next->getSize()))
		{
			_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_CRAFT_SPACE"), _palette, _game->getMod()->getInterface("soldierInfo")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("soldierInfo")->getElement("errorPalette")->color));
			return;
		}
	}
	if (_game->getSavedGame()->getMonthsPassed() != -1)
	{
		// We undress before dress: It should be safe to include currently worn armor.
		if (prev->getStoreItem())
		{
			_base->getStorageItems()->addItem(prev->getStoreItem());
		}
		if (next->getStoreItem())
		{
			_base->getStorageItems()->removeItem(next->getStoreItem());
		}
	}
	soldier->setArmor(next, true);
	_game->getSavedGame()->setLastSelectedArmor(next->getType());

	_game->popState();
}

/**
* Shows corresponding Ufopaedia article.
* @param action Pointer to an action.
*/
void SoldierArmorState::lstArmorClickMiddle(Action *action)
{
	if (!_armors[_indices[_lstArmor->getSelectedRow()]].isKnown)
		return;

	auto armor = _armors[_indices[_lstArmor->getSelectedRow()]].armor;
	std::string articleId = armor->getUfopediaType();
	Ufopaedia::openArticle(_game, articleId);
}

/**
* Toggles folding state of a category
* @param action Pointer to an action.
*/
void SoldierArmorState::lstArmorClickRight(Action *action)
{
	_sel = _lstArmor->getSelectedRow();
	size_t scrollPos = _lstArmor->getScroll();
	int listSizeOld = _lstArmor->getLastRowIndex();

	// Blank state (all parents) does not have collapse functionality
	if (getRow().parentId == 0) return;

	// (Un)fold appropriate childs.
	for (size_t i = 0; i < _armors.size(); ++i)
	{
		if (_armors[i].parentId == getRow().parentId && _armors[i].id != _armors[i].parentId)
		{
			_armors[i].isVisible ^= true;
		}
	}
	drawList();

	// Approximate scroll position (size of list might have changed).
	scrollPos = listSizeOld > 0 ? scrollPos * _lstArmor->getLastRowIndex() / listSizeOld : scrollPos;
	_lstArmor->scrollTo(scrollPos);
}

/**
* Sorts the armors by name.
* @param action Pointer to an action.
*/
void SoldierArmorState::sortNameClick(Action *)
{
	if (_armorOrder == ARMOR_SORT_NAME_ASC)
	{
		_armorOrder = ARMOR_SORT_NAME_DESC;
	}
	else
	{
		_armorOrder = ARMOR_SORT_NAME_ASC;
	}
	updateArrows();
	updateList();
}

/**
* Updates the production list to match the category filter.
*/
void SoldierArmorState::cbxCategoryChange(Action *)
{
	_previousOrder = _armorOrder;

	if (_game->isAltPressed())
	{
		_armorOrder = _game->isShiftPressed() ? ArmorSort::ARMOR_SORT_NAME_DESC : ArmorSort::ARMOR_SORT_NAME_ASC;
	}
	else
	{
		_armorOrder = ArmorSort::ARMOR_SORT_NONE;
	}

	updateList();
}

}
