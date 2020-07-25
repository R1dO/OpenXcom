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
#include "InventoryState.h"
#include <algorithm>
#include "Inventory.h"
#include "../Engine/Game.h"
#include "../Engine/FileMap.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Interface/Text.h"
#include "../Interface/BattlescapeButton.h"
#include "../Engine/Action.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Soldier.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/Armor.h"
#include "../Engine/Options.h"
#include "UnitInfoState.h"
#include "BattlescapeState.h"
#include "BattlescapeGenerator.h"
#include "TileEngine.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

static const int _templateBtnX = 288;
static const int _createTemplateBtnY = 90;
static const int _applyTemplateBtnY  = 113;

/**
 * Initializes all the elements in the Inventory screen.
 * @param game Pointer to the core game.
 * @param tu Does Inventory use up Time Units?
 * @param parent Pointer to parent Battlescape.
 */
InventoryState::InventoryState(bool tu, BattlescapeState *parent) : _tu(tu), _parent(parent)
{
	_battleGame = _game->getSavedGame()->getSavedBattle();

	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	}
	else if (!_battleGame->getTileEngine())
	{
		Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
		_game->getScreen()->resetDisplay(false);
	}

	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	_soldier = new Surface(320, 200, 0, 0);
	_txtName = new Text(210, 17, 28, 6);
	_txtTus = new Text(40, 9, 245, 24);
	_txtWeight = new Text(70, 9, 245, 24);
	_txtFAcc = new Text(50, 9, 245, 32);
	_txtReact = new Text(50, 9, 245, 40);
	_txtPSkill = new Text(50, 9, 245, 48);
	_txtPStr = new Text(50, 9, 245, 56);
	_txtItem = new Text(160, 9, 128, 140);
	_txtAmmo = new Text(66, 24, 254, 64);
	_btnOk = new BattlescapeButton(35, 22, 237, 1);
	_btnPrev = new BattlescapeButton(23, 22, 273, 1);
	_btnNext = new BattlescapeButton(23, 22, 297, 1);
	_btnUnload = new BattlescapeButton(32, 25, 288, 32);
	_btnGround = new BattlescapeButton(32, 15, 289, 137);
	_btnRank = new BattlescapeButton(26, 23, 0, 0);
	_btnCreateTemplate = new BattlescapeButton(32, 22, _templateBtnX, _createTemplateBtnY);
	_btnApplyTemplate = new BattlescapeButton(32, 22, _templateBtnX, _applyTemplateBtnY);
	_selAmmo = new Surface(RuleInventory::HAND_W * RuleInventory::SLOT_W, RuleInventory::HAND_H * RuleInventory::SLOT_H, 272, 88);
	_inv = new Inventory(_game, 320, 200, 0, 0, _parent == 0);

	// Set palette
	setPalette("PAL_BATTLESCAPE");

	add(_bg);

	// Set up objects
	_game->getMod()->getSurface("TAC01.SCR")->blit(_bg);

	add(_soldier);
	add(_txtName, "textName", "inventory", _bg);
	add(_txtTus, "textTUs", "inventory", _bg);
	add(_txtWeight, "textWeight", "inventory", _bg);
	add(_txtFAcc, "textFiring", "inventory", _bg);
	add(_txtReact, "textReaction", "inventory", _bg);
	add(_txtPSkill, "textPsiSkill", "inventory", _bg);
	add(_txtPStr, "textPsiStrength", "inventory", _bg);
	add(_txtItem, "textItem", "inventory", _bg);
	add(_txtAmmo, "textAmmo", "inventory", _bg);
	add(_btnOk, "buttonOK", "inventory", _bg);
	add(_btnPrev, "buttonPrev", "inventory", _bg);
	add(_btnNext, "buttonNext", "inventory", _bg);
	add(_btnUnload, "buttonUnload", "inventory", _bg);
	add(_btnGround, "buttonGround", "inventory", _bg);
	add(_btnRank, "rank", "inventory", _bg);
	add(_btnCreateTemplate, "buttonCreate", "inventory", _bg);
	add(_btnApplyTemplate, "buttonApply", "inventory", _bg);
	add(_selAmmo);
	add(_inv);

	// move the TU display down to make room for the weight (and Accuracy) display
	if (Options::showMoreStatsInInventoryView)
	{
		_txtTus->setY(_txtTus->getY() + 2*8);
	}

	centerAllSurfaces();



	_txtName->setBig();
	_txtName->setHighContrast(true);

	_txtTus->setHighContrast(true);

	_txtWeight->setHighContrast(true);

	_txtFAcc->setHighContrast(true);

	_txtReact->setHighContrast(true);

	_txtPSkill->setHighContrast(true);

	_txtPStr->setHighContrast(true);

	_txtItem->setHighContrast(true);

	_txtAmmo->setAlign(ALIGN_CENTER);
	_txtAmmo->setHighContrast(true);

	_btnOk->onMouseClick((ActionHandler)&InventoryState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnOkClick, Options::keyBattleInventory);
	_btnOk->setTooltip("STR_OK");
	_btnOk->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnOk->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnPrev->onMouseClick((ActionHandler)&InventoryState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&InventoryState::btnPrevClick, Options::keyBattlePrevUnit);
	_btnPrev->setTooltip("STR_PREVIOUS_UNIT");
	_btnPrev->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnPrev->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnNext->onMouseClick((ActionHandler)&InventoryState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&InventoryState::btnNextClick, Options::keyBattleNextUnit);
	_btnNext->setTooltip("STR_NEXT_UNIT");
	_btnNext->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnNext->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnUnload->onMouseClick((ActionHandler)&InventoryState::btnUnloadClick);
	_btnUnload->setTooltip("STR_UNLOAD_WEAPON");
	_btnUnload->onMouseOver((ActionHandler)&InventoryState::btnUnloadMouseOver);
	_btnUnload->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnUnload->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnGround->onMouseClick((ActionHandler)&InventoryState::btnGroundClick);
	_btnGround->setTooltip("STR_SCROLL_RIGHT");
	_btnGround->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnGround->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnRank->onMouseClick((ActionHandler)&InventoryState::btnRankClick);
	_btnRank->setTooltip("STR_UNIT_STATS");
	_btnRank->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnRank->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnCreateTemplate->onMouseClick((ActionHandler)&InventoryState::btnCreateTemplateClick);
	_btnCreateTemplate->onKeyboardPress((ActionHandler)&InventoryState::btnCreateTemplateClick, Options::keyInvCreateTemplate);
	_btnCreateTemplate->setTooltip("STR_CREATE_INVENTORY_TEMPLATE");
	_btnCreateTemplate->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnCreateTemplate->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnApplyTemplate->onMouseClick((ActionHandler)&InventoryState::btnApplyTemplateClick);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::btnApplyTemplateClick, Options::keyInvApplyTemplate);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::onClearInventory, Options::keyInvClear);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::onAutoequip, Options::keyInvAutoEquip);
	_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
	_btnApplyTemplate->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnApplyTemplate->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);


	// only use copy/paste buttons in setup (i.e. non-tu) mode
	if (_tu)
	{
		_btnCreateTemplate->setVisible(false);
		_btnApplyTemplate->setVisible(false);
	}
	else
	{
		_updateTemplateButtons(true);
	}

	_inv->draw();
	_inv->setTuMode(_tu);
	_inv->setSelectedUnit(_game->getSavedGame()->getSavedBattle()->getSelectedUnit());
	_inv->onMouseClick((ActionHandler)&InventoryState::invClick, 0);
	_inv->onMouseOver((ActionHandler)&InventoryState::invMouseOver);
	_inv->onMouseOut((ActionHandler)&InventoryState::invMouseOut);
	_inv->onMouseIn((ActionHandler)&InventoryState::invMouseIn);

	_txtTus->setVisible(_tu);
	_txtWeight->setVisible(Options::showMoreStatsInInventoryView);
	_txtFAcc->setVisible(Options::showMoreStatsInInventoryView);
	_txtReact->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtPSkill->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtPStr->setVisible(Options::showMoreStatsInInventoryView && !_tu);
}

static void _clearInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate)
{
	for (std::vector<EquipmentLayoutItem*>::iterator eraseIt = inventoryTemplate.begin();
		 eraseIt != inventoryTemplate.end();
		 eraseIt = inventoryTemplate.erase(eraseIt))
	{
		delete *eraseIt;
	}
}

/**
 *
 */
InventoryState::~InventoryState()
{
	_clearInventoryTemplate(_curInventoryTemplate);

	if (_battleGame->getTileEngine())
	{
		if (Options::maximizeInfoScreens)
		{
			Screen::updateScale(Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
			_game->getScreen()->resetDisplay(false);
		}
		Tile *inventoryTile = _battleGame->getSelectedUnit()->getTile();
		_battleGame->getTileEngine()->applyGravity(inventoryTile);
		_battleGame->getTileEngine()->calculateTerrainLighting(); // dropping/picking up flares
		_battleGame->getTileEngine()->recalculateFOV();
	}
	else
	{
		Screen::updateScale(Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, true);
		_game->getScreen()->resetDisplay(false);
	}
}

/**
 * Updates all soldier stats when the soldier changes.
 */
void InventoryState::init()
{
	State::init();
	BattleUnit *unit = _battleGame->getSelectedUnit();

	// no selected unit, close inventory
	if (unit == 0)
	{
		btnOkClick(0);
		return;
	}
	// skip to the first unit with inventory
	if (!unit->hasInventory())
	{
		if (_parent)
		{
			_parent->selectNextPlayerUnit(false, false, true, _tu);
		}
		else
		{
			_battleGame->selectNextPlayerUnit(false, false, true);
		}
		// no available unit, close inventory
		if (_battleGame->getSelectedUnit() == 0 || !_battleGame->getSelectedUnit()->hasInventory())
		{
			// starting a mission with just vehicles
			btnOkClick(0);
			return;
		}
		else
		{
			unit = _battleGame->getSelectedUnit();
		}
	}

	unit->setCache(0);
	_soldier->clear();
	_btnRank->clear();

	_txtName->setBig();
	_txtName->setText(unit->getName(_game->getLanguage()));
	_inv->setSelectedUnit(unit);
	Soldier *s = unit->getGeoscapeSoldier();
	if (s)
	{
		SurfaceSet *texture = _game->getMod()->getSurfaceSet("SMOKE.PCK");
		texture->getFrame(20 + s->getRank())->setX(0);
		texture->getFrame(20 + s->getRank())->setY(0);
		texture->getFrame(20 + s->getRank())->blit(_btnRank);

		std::string look = s->getArmor()->getSpriteInventory();
		if (s->getGender() == GENDER_MALE)
			look += "M";
		else
			look += "F";
		if (s->getLook() == LOOK_BLONDE)
			look += "0";
		if (s->getLook() == LOOK_BROWNHAIR)
			look += "1";
		if (s->getLook() == LOOK_ORIENTAL)
			look += "2";
		if (s->getLook() == LOOK_AFRICAN)
			look += "3";
		look += ".SPK";
		const std::set<std::string> &ufographContents = FileMap::getVFolderContents("UFOGRAPH");
		std::string lcaseLook = look;
		std::transform(lcaseLook.begin(), lcaseLook.end(), lcaseLook.begin(), tolower);
		if (ufographContents.find("lcaseLook") == ufographContents.end() && !_game->getMod()->getSurface(look, false))
		{
			look = s->getArmor()->getSpriteInventory() + ".SPK";
		}
		_game->getMod()->getSurface(look)->blit(_soldier);
	}
	else
	{
		Surface *armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory(), false);
		if (armorSurface)
		{
			armorSurface->blit(_soldier);
		}
	}

	updateStats();
	_refreshMouse();
}

/**
 * Updates the soldier stats (Weight, TU).
 */
void InventoryState::updateStats()
{
	BattleUnit *unit = _battleGame->getSelectedUnit();

	_txtReact->setText(tr("STR_REACTIONS_SHORT").arg(unit->getBaseStats()->reactions));

	if (unit->getBaseStats()->psiSkill > 0)
	{
		_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(unit->getBaseStats()->psiSkill));
	}
	else
	{
		_txtPSkill->setText("");
	}

	if (unit->getBaseStats()->psiSkill > 0 || (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements())))
	{
		_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(unit->getBaseStats()->psiStrength));
	}
	else
	{
		_txtPStr->setText("");
	}

	if (_inv->getSelectedItem() != 0)
	{
		_setSoldierStatAccuracy(_inv->getSelectedItem());
	}
	else
	{
		_setSoldierStatAccuracy(_inv->getMouseOverItem());
	}
	_setSoldierStatTu();
	_setSoldierStatWeight();
}

/**
 * Gets weapon type accuracy for selected unit.
 *
 * @param item Pointer to battle item.
 * @param useModifiers Do we consider all modifiers (wounds, two-handed, melee skill applied, etc)?
 * @return The unit's accuracy for this item.
 */
int InventoryState::_getItemAccuracy(BattleItem *item, bool useModifiers) const
{
	BattleUnit *unit = _battleGame->getSelectedUnit();
	double accuracy = unit->getBaseStats()->firing; // Default value even if no item is selected.

	if (item != 0)
	{
		switch (item->getRules()->getBattleType())
		{
		case BT_MELEE:
			accuracy = unit->getBaseStats()->melee;
			break;
		//case BT_AMMO: // Not sure about this. You can only throw a clip but it is kinda confusing.
		case BT_FLARE:
		case BT_GRENADE:
		case BT_PROXIMITYGRENADE:
			accuracy = unit->getBaseStats()->throwing;
			break;
		default:
			// Do nothing, firing accuracy is already the default.
			break;
		}
	}

	if (item != 0 && useModifiers)
	{
		if (item->getRules()->getBattleType() == BT_MELEE && item->getRules()->isSkillApplied())
		{
			accuracy *=  item->getRules()->getAccuracyMelee() / 100.0;
		}

		if (item->getRules()->isTwoHanded() && (unit->getItem("STR_RIGHT_HAND") != 0 || unit->getItem("STR_LEFT_HAND") != 0))
		{
			bool penalize = true;
			// If item comes from a handslot discard that slot for 2-handiness checks.
			// If we hover over a handslot only take 2-handiness into account if the other hand is not empty.
			if (item->getSlot()->getId() == "STR_LEFT_HAND" && unit->getItem("STR_RIGHT_HAND") == 0)
			{
				penalize = false;
			}
			if (item->getSlot()->getId() == "STR_RIGHT_HAND" && unit->getItem("STR_LEFT_HAND") == 0)
			{
				penalize = false;
			}

			if (_inv->getMouseOverSlot() != 0 && _inv->getMouseOverSlot()->getType() == INV_HAND)
			{
				if ( _inv->getMouseOverSlot()->getId() == "STR_RIGHT_HAND" && (item->getSlot()->getId() == "STR_LEFT_HAND" || unit->getItem("STR_LEFT_HAND") == 0))
				{
					penalize = false;
				}
				if ( _inv->getMouseOverSlot()->getId() == "STR_LEFT_HAND" && (item->getSlot()->getId() == "STR_RIGHT_HAND" || unit->getItem("STR_RIGHT_HAND") == 0))
				{
					penalize = false;
				}
			}

			if (penalize)
			{
				accuracy *= 80.0 / 100.0;
			}
		}
	}

	// Health modifier.
	if (useModifiers  && Options::showMoreStatsInInventoryView)
	{
		// Shooting formula version (takes into account wounds).
		accuracy *= unit->getAccuracyModifier(item, _inv->getMouseOverSlot()) / 100.0;
	}
	else if (Options::showMoreStatsInInventoryView)
	{
		// Make sure unit stats does not accidentally take arm wounds into account.
		accuracy *= unit->getAccuracyModifier() / 100.0;
	}
	else
	{
		// Value as visible on the soldier stat screen.
		accuracy *= (double)unit->getHealth() / unit->getBaseStats()->health;
	}

	return accuracy;
	// Note: it is probably better to move logic to `Savegame::BattleItem` as preview argument.
}

/**
 * Gets weapon (ammo) power.
 *
 * @param item Pointer to battle item.
 * @return The power of the weapon (ammo).
 */
int InventoryState::_getItemPower(BattleItem *item) const
{
	int power = 0;
	// Is we have a clip it defines the item power.
	if (item->getAmmoItem() != 0 && item->needsAmmo())
	{
		power = item->getAmmoItem()->getRules()->getPower();
	}
	else
	{
		power = item->getRules()->getPower();
	}

	// Melee weapon
	if (item->getRules()->isStrengthApplied())
	{
		BattleUnit *unit = _battleGame->getSelectedUnit();
		power += unit->getBaseStats()->strength;
	}

	return power;
	// Note: it is probably better to move logic to `Savegame::BattleItem` as preview argument.
}

/**
 * Gets weapon (ammo) rounds left.
 *
 * @param item Pointer to battle item.
 * @return The rounds left in the weapon (ammo).
 */
int InventoryState::_getItemRounds(BattleItem *item) const
{
	int rounds = 0;

	if (item->getAmmoItem() != 0 && item->needsAmmo())
	{
		rounds = item->getAmmoItem()->getAmmoQuantity();
	}
	else
	{
		// Will return 255 for laser weapons. Take care when using it.
		rounds = item->getAmmoQuantity();
	}

	return rounds;
}

/**
 * Check if item is researched
 *
 * Determine if we are allowed to see 'advanced' item values.
 * Takes into account if an item depends on clips.
 *
 * @param item Pointer to battle item.
 * @param ufopaedia Do we take ufopaedia visibility into account?
 * @return The rounds left in the weapon (ammo).
 */
bool InventoryState::_isItemResearched(BattleItem *item, bool ufopaedia) const
{
	return item->isStatsKnown(_game->getSavedGame(), _game->getMod(), ufopaedia);
}

/**
 * Updates the soldier accuracy info text.
 *
 * Based on the BattleType type of the item under the mouse (defaults to firing accuracy).
 *
 * @param item Pointer to battle item.
 */
void InventoryState::_setSoldierStatAccuracy(BattleItem *item)
{
	_txtFAcc->setText(tr("STR_ACCURACY_SHORT").arg(_getItemAccuracy(item)));
}

/**
 * Updates the soldier TU info text.
 *
 * Recognizes if we move an item between slots (for preview purposes).
 *
 * Taking special care for the following (edge) cases:
 * * Unloading a weapon (using the appropriate button)
 * * Loading a clip into a weapon
 *
 * @param item Pointer to battle item.
 * @param unloadWeapon Are we hovering over the unload button?
 */
void InventoryState::_setSoldierStatTu(BattleItem *item, bool unloadWeapon)
{
	BattleUnit *unit = _battleGame->getSelectedUnit();
	int tu = unit->getTimeUnits();

	if (Options::showMoreStatsInInventoryView && item != 0)
	{
		RuleInventory *slotFrom = item->getSlot();
		RuleInventory *slotTo = _inv->getMouseOverSlot();
		if (unloadWeapon == true && item->needsAmmo() && item->getAmmoItem() != 0)
		{
			tu -= 8;
		}
		else if (slotFrom != 0 && slotTo != 0 && slotFrom != slotTo)
		{
			BattleItem *itemTo = _inv->getMouseOverItem();
			if (itemTo != 0 && itemTo->needsAmmo() && itemTo->getAmmoItem() == 0)
			{
				tu -= 15;
			}
			else
			{
				tu -= slotFrom->getCost(slotTo);
			}
		}
	}

	// Misuse the color info from "weight" to show red negatives.
	if (tu < 0)
	{
		_txtTus->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color2);
	}
	else
	{
		_txtTus->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color);
	}
	_txtTus->setText(tr("STR_TIME_UNITS_SHORT").arg(tu));
}

/**
 * Updates the soldier weight info text.
 *
 * Recognizes if we move a picked up item between a soldier and the ground (for preview purposes).
 *
 * Using the following decission tree:
 *  * If item originates from a soldier slot (general or hand) do nothing unless we mouse-over the ground
 *    + We cannot predict if player wants to put it in another soldier slot or the ground
 *  * If item originates from the ground we can safely assume it will move to a soldier slot
 */
void InventoryState::_setSoldierStatWeight(BattleItem *item)
{
	BattleUnit *unit = _battleGame->getSelectedUnit();
	int weight = unit->getCarriedWeight(); //Deliberately *not* discarding the item grabbed by the mouse.

	if (item != 0)
	{
		int itemWeight = item->getRules()->getWeight();
		if (item->getAmmoItem() != 0 && item->needsAmmo())
		{
			itemWeight += item->getAmmoItem()->getRules()->getWeight();
		}

		RuleInventory *slotTo = 0;
		switch (item->getSlot()->getType())
		{
		case INV_GROUND:
			weight += itemWeight;
			break;
		case INV_SLOT:
		case INV_HAND:
			slotTo = _inv->getMouseOverSlot();
			if (slotTo != 0 && slotTo->getType() == INV_GROUND)
			{
				weight -= itemWeight;
			}
			break;
		default:
			// Do nothing, we are not dragging an item.
			break;
		}
	}

	_txtWeight->setText(tr("STR_WEIGHT").arg(weight).arg(unit->getBaseStats()->strength));
	if (weight > unit->getBaseStats()->strength)
	{
		_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color2);
	}
	else
	{
		_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color);
	}
}

/**
 * Updates the item info text.
 *
 * Will also handle the button tooltips.
 *
 * @param item Pointer to battle item.
 */
void InventoryState::_setTxtItem(BattleItem *item)
{
	std::ostringstream ssTxtItem;
	if (item != 0)
	{
		if (item->getUnit() && item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
		{
			ssTxtItem << item->getUnit()->getName(_game->getLanguage());
		}
		else if (_game->getSavedGame()->isResearched(item->getRules()->getRequirements()))
		{
			ssTxtItem << tr(item->getRules()->getName());
		}
		else
		{
			ssTxtItem << tr("STR_ALIEN_ARTIFACT");
		}
	}
	else if (_currentTooltip.empty())
	{
		// Do nothing string is empty by default.
	}
	else
	{
		ssTxtItem << tr(_currentTooltip);
	}
	_txtItem->setText(ssTxtItem.str());
}

/**
 * Display item stats.
 *
 * Shows item stats (ammo, medikit) and handobj when appropriate.
 * Hides template buttons when needed.
 *
 * When 'showMoreStatsInInventoryView' is in effect the following can be expected:
 * + Display the following stats:
 *   - Unit accuracy for the item type
 *   - Weapon power
 *   - Rounds left in clip/weapon
 * + Those stats are *only* shown if a player can reasonably see them in the ufopaedia
 * + Accuracy is based on the shooting formula with the following adjustments:
 *   - Do not take shot-type into account (we cannot predict it from this screen)
 * note:
 *   The research requirement is stricter than vanilla. Using the rationale that counting the
 *   number of shots (or relate power level left to shots) is research as well.

 * @param item Pointer to battle item.
 */
void InventoryState::_showItemStats(BattleItem *item)
{
	_selAmmo->clear();
	_updateTemplateButtons(!_tu);
	std::ostringstream ssItemStats;
	if (item != 0)
	{
		int power = 0;
		int accuracy = 0;
		int rounds = 0;
		switch (item->getRules()->getBattleType())
		{
		case BT_MEDIKIT:
			ssItemStats << tr("STR_MEDI_KIT_QUANTITIES_LEFT").arg(item->getPainKillerQuantity()).arg(item->getStimulantQuantity()).arg(item->getHealQuantity());
			break;
		case BT_MELEE:
		case BT_FLARE:
		case BT_GRENADE:
		case BT_PROXIMITYGRENADE:
			accuracy = _getItemAccuracy(item, true);
		case BT_AMMO:
			power = _getItemPower(item);
			rounds = _getItemRounds(item);
			break;
		case BT_FIREARM:
			power = _getItemPower(item);
			accuracy = _getItemAccuracy(item, true);
			rounds = _getItemRounds(item);
			// Draw ammo object.
			if (item->getAmmoItem() != 0 && item->needsAmmo())
			{
				SDL_Rect r;
				r.x = 0;
				r.y = 0;
				r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
				r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
				_selAmmo->drawRect(&r, _game->getMod()->getInterface("inventory")->getElement("grid")->color);
				r.x++;
				r.y++;
				r.w -= 2;
				r.h -= 2;
				_selAmmo->drawRect(&r, Palette::blockOffset(0)+15);
				item->getAmmoItem()->getRules()->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), _selAmmo);
				_updateTemplateButtons(false);
			}
			break;
		default:
			ssItemStats.str("");
			break;
		}

		if (Options::showMoreStatsInInventoryView && _isItemResearched(item, true) )
		{
			if (accuracy != 0)
			{
				ssItemStats << tr("STR_ACCURACY_SHORT").arg(accuracy) << Unicode::TOK_COLOR_FLIP;
			}
			ssItemStats << std::endl;

			if (power != 0)
			{
				ssItemStats << tr("STR_POWER_SHORT").arg(power) << Unicode::TOK_COLOR_FLIP;
			}
			ssItemStats << std::endl;

			if (rounds == 255)
			{
				ssItemStats << tr("STR_ROUNDS_").arg("∞");
			}
			else if (rounds != 0)
			{
				ssItemStats << tr("STR_ROUNDS_").arg(rounds);
			}
		}
		else if (!Options::showMoreStatsInInventoryView && rounds != 0 && rounds != 255)
		{
			ssItemStats << tr("STR_AMMO_ROUNDS_LEFT").arg(rounds);
		}
	}
	_txtAmmo->setText(ssItemStats.str());
}

/**
 * Saves the soldiers' equipment-layout.
 */
void InventoryState::saveEquipmentLayout()
{
	for (std::vector<BattleUnit*>::iterator i = _battleGame->getUnits()->begin(); i != _battleGame->getUnits()->end(); ++i)
	{
		// we need X-Com soldiers only
		if ((*i)->getGeoscapeSoldier() == 0) continue;

		std::vector<EquipmentLayoutItem*> *layoutItems = (*i)->getGeoscapeSoldier()->getEquipmentLayout();

		// clear the previous save
		if (!layoutItems->empty())
		{
			for (std::vector<EquipmentLayoutItem*>::iterator j = layoutItems->begin(); j != layoutItems->end(); ++j)
				delete *j;
			layoutItems->clear();
		}

		// save the soldier's items
		// note: with using getInventory() we are skipping the ammos loaded, (they're not owned) because we handle the loaded-ammos separately (inside)
		for (std::vector<BattleItem*>::iterator j = (*i)->getInventory()->begin(); j != (*i)->getInventory()->end(); ++j)
		{
			std::string ammo;
			if ((*j)->needsAmmo() && 0 != (*j)->getAmmoItem()) ammo = (*j)->getAmmoItem()->getRules()->getType();
			else ammo = "NONE";
			layoutItems->push_back(new EquipmentLayoutItem(
				(*j)->getRules()->getType(),
				(*j)->getSlot()->getId(),
				(*j)->getSlotX(),
				(*j)->getSlotY(),
				ammo,
				(*j)->getFuseTimer()
			));
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnOkClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
		return;
	_game->popState();
	Tile *inventoryTile = _battleGame->getSelectedUnit()->getTile();
	if (!_tu)
	{
		saveEquipmentLayout();
		_battleGame->resetUnitTiles();
		if (_battleGame->getTurn() == 1)
		{
			_battleGame->randomizeItemLocations(inventoryTile);
			if (inventoryTile->getUnit())
			{
				// make sure we select the unit closest to the ramp.
				_battleGame->setSelectedUnit(inventoryTile->getUnit());
			}
		}

		// initialize xcom units for battle
		for (std::vector<BattleUnit*>::iterator j = _battleGame->getUnits()->begin(); j != _battleGame->getUnits()->end(); ++j)
		{
			if ((*j)->getOriginalFaction() != FACTION_PLAYER || (*j)->isOut())
			{
				continue;
			}

			(*j)->prepareNewTurn(false);
		}
	}
}

/**
 * Selects the previous soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnPrevClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	if (_parent)
	{
		_parent->selectPreviousPlayerUnit(false, false, true);
	}
	else
	{
		_battleGame->selectPreviousPlayerUnit(false, false, true);
	}
	init();
}

/**
 * Selects the next soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnNextClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}
	if (_parent)
	{
		_parent->selectNextPlayerUnit(false, false, true);
	}
	else
	{
		_battleGame->selectNextPlayerUnit(false, false, true);
	}
	init();
}

/**
 * Unloads the selected weapon.
 * @param action Pointer to an action.
 */
void InventoryState::btnUnloadClick(Action *)
{
	if (_inv->unload())
	{
		_setTxtItem();
		_showItemStats();
		updateStats();
		_game->getMod()->getSoundByDepth(0, Mod::ITEM_DROP)->play();
	}
}

/**
 * Preview TU cost Unload button.
 * @param action Pointer to an action.
 */
void InventoryState::btnUnloadMouseOver(Action *action)
{
	if (_inv->getSelectedItem() != 0)
	{
		_setSoldierStatTu(_inv->getSelectedItem(), true);
	}
}

/**
 * Shows more ground items / rearranges them.
 * @param action Pointer to an action.
 */
void InventoryState::btnGroundClick(Action *)
{
	_inv->arrangeGround();
}

/**
 * Shows the unit info screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnRankClick(Action *)
{
	_game->pushState(new UnitInfoState(_battleGame->getSelectedUnit(), _parent, true, false));
}

void InventoryState::btnCreateTemplateClick(Action *)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	// clear current template
	_clearInventoryTemplate(_curInventoryTemplate);

	// copy inventory instead of just keeping a pointer to it.  that way
	// create/apply can be used as an undo button for a single unit and will
	// also work as expected if inventory is modified after 'create' is clicked
	std::vector<BattleItem*> *unitInv = _battleGame->getSelectedUnit()->getInventory();
	for (std::vector<BattleItem*>::iterator j = unitInv->begin(); j != unitInv->end(); ++j)
	{
		if ((*j)->getRules()->isFixed()) {
			// don't copy fixed weapons into the template
			continue;
		}

		std::string ammo;
		if ((*j)->needsAmmo() && (*j)->getAmmoItem())
		{
			ammo = (*j)->getAmmoItem()->getRules()->getType();
		}
		else
		{
			ammo = "NONE";
		}

		_curInventoryTemplate.push_back(new EquipmentLayoutItem(
				(*j)->getRules()->getType(),
				(*j)->getSlot()->getId(),
				(*j)->getSlotX(),
				(*j)->getSlotY(),
				ammo,
				(*j)->getFuseTimer()));
	}

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
	_refreshMouse();
}

static void _clearInventory(Game *game, std::vector<BattleItem*> *unitInv, Tile *groundTile)
{
	RuleInventory *groundRuleInv = game->getMod()->getInventory("STR_GROUND", true);

	// clear unit's inventory (i.e. move everything to the ground)
	for (std::vector<BattleItem*>::iterator i = unitInv->begin(); i != unitInv->end(); )
	{
		if ((*i)->getRules()->isFixed()) {
			// don't drop fixed weapons
			++i;
			continue;
		}

		(*i)->setOwner(NULL);
		groundTile->addItem(*i, groundRuleInv);
		i = unitInv->erase(i);
	}
}

void InventoryState::btnApplyTemplateClick(Action *)
{
	// don't accept clicks when moving items
	// it's ok if the template is empty -- it will just result in clearing the
	// unit's inventory
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit          = _battleGame->getSelectedUnit();
	std::vector<BattleItem*> *unitInv       = unit->getInventory();
	Tile                     *groundTile    = unit->getTile();
	std::vector<BattleItem*> *groundInv     = groundTile->getInventory();
	RuleInventory            *groundRuleInv = _game->getMod()->getInventory("STR_GROUND", true);

	_clearInventory(_game, unitInv, groundTile);

	// attempt to replicate inventory template by grabbing corresponding items
	// from the ground.  if any item is not found on the ground, display warning
	// message, but continue attempting to fulfill the template as best we can
	bool itemMissing = false;
	std::vector<EquipmentLayoutItem*>::iterator templateIt;
	for (templateIt = _curInventoryTemplate.begin(); templateIt != _curInventoryTemplate.end(); ++templateIt)
	{
		// search for template item in ground inventory
		std::vector<BattleItem*>::iterator groundItem;
		const bool needsAmmo = !_game->getMod()->getItem((*templateIt)->getItemType(), true)->getCompatibleAmmo()->empty();
		bool found = false;
		bool rescan = true;
		while (rescan)
		{
			rescan = false;

			const std::string targetAmmo = (*templateIt)->getAmmoItem();
			BattleItem *matchedWeapon = NULL;
			BattleItem *matchedAmmo   = NULL;
			for (groundItem = groundInv->begin(); groundItem != groundInv->end(); ++groundItem)
			{
				// if we find the appropriate ammo, remember it for later for if we find
				// the right weapon but with the wrong ammo
				const std::string groundItemName = (*groundItem)->getRules()->getType();
				if (needsAmmo && targetAmmo == groundItemName)
				{
					matchedAmmo = *groundItem;
				}

				if ((*templateIt)->getItemType() == groundItemName)
				{
					// if the template item would overlap with an existing item (i.e. a fixed
					// weapon that didn't get cleared in _clearInventory() above), skip it
					if (Inventory::overlapItems(unit, *groundItem,
								    _game->getMod()->getInventory((*templateIt)->getSlot(), true),
								    (*templateIt)->getSlotX(), (*templateIt)->getSlotY())) {
						// don't display 'item not found' warning message
						found = true;
						break;
					}

					// if the loaded ammo doesn't match the template item's,
					// remember the weapon for later and continue scanning
					BattleItem *loadedAmmo = (*groundItem)->getAmmoItem();
					if ((needsAmmo && loadedAmmo && targetAmmo != loadedAmmo->getRules()->getType())
					 || (needsAmmo && !loadedAmmo))
					{
						// remember the last matched weapon for simplicity (but prefer empty weapons if any are found)
						if (!matchedWeapon || matchedWeapon->getAmmoItem())
						{
							matchedWeapon = *groundItem;
						}
						continue;
					}

					// move matched item from ground to the appropriate inv slot
					(*groundItem)->setOwner(unit);
					(*groundItem)->setTile(0);
					(*groundItem)->setSlot(_game->getMod()->getInventory((*templateIt)->getSlot(), true));
					(*groundItem)->setSlotX((*templateIt)->getSlotX());
					(*groundItem)->setSlotY((*templateIt)->getSlotY());
					(*groundItem)->setFuseTimer((*templateIt)->getFuseTimer());
					unitInv->push_back(*groundItem);
					groundInv->erase(groundItem);
					found = true;
					break;
				}
			}

			// if we failed to find an exact match, but found unloaded ammo and
			// the right weapon, unload the target weapon, load the right ammo, and use it
			if (!found && matchedWeapon && (!needsAmmo || matchedAmmo))
			{
				// unload the existing ammo (if any) from the weapon
				BattleItem *loadedAmmo = matchedWeapon->getAmmoItem();
				if (loadedAmmo)
				{
					groundTile->addItem(loadedAmmo, groundRuleInv);
					matchedWeapon->setAmmoItem(NULL);
				}

				// load the correct ammo into the weapon
				if (matchedAmmo)
				{
					matchedWeapon->setAmmoItem(matchedAmmo);
					groundTile->removeItem(matchedAmmo);
				}

				// rescan and pick up the newly-loaded/unloaded weapon
				rescan = true;
			}
		}

		if (!found)
		{
			itemMissing = true;
		}
	}

	if (itemMissing)
	{
		_inv->showWarning(tr("STR_NOT_ENOUGH_ITEMS_FOR_TEMPLATE"));
	}

	// refresh ui
	_inv->arrangeGround(false);
	updateStats();
	_refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

void InventoryState::_refreshMouse()
{
	// send a mouse motion event to refresh any hover actions
	int x, y;
	SDL_GetMouseState(&x, &y);
	SDL_WarpMouse(x+1, y);

	// move the mouse back to avoid cursor creep
	SDL_WarpMouse(x, y);
}

void InventoryState::onClearInventory(Action *)
{
	// don't act when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit       = _battleGame->getSelectedUnit();
	std::vector<BattleItem*> *unitInv    = unit->getInventory();
	Tile                     *groundTile = unit->getTile();

	_clearInventory(_game, unitInv, groundTile);

	// refresh ui
	_inv->arrangeGround(false);
	updateStats();
	_refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

void InventoryState::onAutoequip(Action *)
{
	// don't act when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit          = _battleGame->getSelectedUnit();
	Tile                     *groundTile    = unit->getTile();
	std::vector<BattleItem*> *groundInv     = groundTile->getInventory();
	Mod                      *mod           = _game->getMod();
	RuleInventory            *groundRuleInv = mod->getInventory("STR_GROUND", true);
	int                       worldShade    = _battleGame->getGlobalShade();

	std::vector<BattleUnit*> units;
	units.push_back(unit);
	BattlescapeGenerator::autoEquip(units, mod, NULL, groundInv, groundRuleInv, worldShade, true, true);

	// refresh ui
	_inv->arrangeGround(false);
	updateStats();
	_refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

/**
 * Updates item info.
 * @param action Pointer to an action.
 */
void InventoryState::invClick(Action *)
{
	updateStats();
	_refreshMouse();
}

/**
 * Shows item info.
 * @param action Pointer to an action.
 */
void InventoryState::invMouseOver(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		_showItemStats(_inv->getSelectedItem());
		_setSoldierStatWeight(_inv->getSelectedItem());
		_setSoldierStatTu(_inv->getSelectedItem());
		return;
	}

	_showItemStats(_inv->getMouseOverItem());
	_setSoldierStatAccuracy(_inv->getMouseOverItem());
	_setTxtItem(_inv->getMouseOverItem());
}

/**
 * Hides item info.
 * @param action Pointer to an action.
 */
void InventoryState::invMouseOut(Action *)
{
	_setTxtItem();
	_showItemStats();
}

/**
 * Un-Hide item info.
 * @param action Pointer to an action.
 */
void InventoryState::invMouseIn(Action *)
{
	_setTxtItem(_inv->getSelectedItem());
	_showItemStats(_inv->getSelectedItem());
}

/**
 * Takes care of any events from the core game engine.
 * @param action Pointer to an action.
 */
void InventoryState::handle(Action *action)
{
	State::handle(action);


#ifndef __MORPHOS__
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_X1)
		{
			btnNextClick(action);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_X2)
		{
			btnPrevClick(action);
		}
	}
#endif
}

/**
 * Shows a tooltip for the appropriate button.
 * @param action Pointer to an action.
 */
void InventoryState::txtTooltipIn(Action *action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_setTxtItem();
	}
}

/**
 * Clears the tooltip text.
 * @param action Pointer to an action.
 */
void InventoryState::txtTooltipOut(Action *action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_currentTooltip = "";
			_setTxtItem();
		}
	}
}

void InventoryState::_updateTemplateButtons(bool isVisible)
{
	if (isVisible)
	{
		if (_curInventoryTemplate.empty())
		{
			// use "empty template" icons
			_game->getMod()->getSurface("InvCopy")->blit(_btnCreateTemplate);
			_game->getMod()->getSurface("InvPasteEmpty")->blit(_btnApplyTemplate);
			_btnApplyTemplate->setTooltip("STR_CLEAR_INVENTORY");
		}
		else
		{
			// use "active template" icons
			_game->getMod()->getSurface("InvCopyActive")->blit(_btnCreateTemplate);
			_game->getMod()->getSurface("InvPaste")->blit(_btnApplyTemplate);
			_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
		}
		_btnCreateTemplate->initSurfaces();
		_btnApplyTemplate->initSurfaces();
	}
	else
	{
		_btnCreateTemplate->clear();
		_btnApplyTemplate->clear();
	}
}

}
