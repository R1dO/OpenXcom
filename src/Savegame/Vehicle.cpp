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
#include "Vehicle.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"

namespace OpenXcom
{

/**
 * Initializes a vehicle of the specified type.
 * @param rules Pointer to ruleset.
 * @param ammo Initial ammo.
 * @param size Size in tiles.
 */
Vehicle::Vehicle(RuleItem *rules, int ammo, int size) : _rules(rules), _ammo(ammo), _size(size)
{
}

/**
 *
 */
Vehicle::~Vehicle()
{
}

/**
 * Loads the vehicle from a YAML file.
 * @param node YAML node.
 */
void Vehicle::load(const YAML::Node &node)
{
	_ammo = node["ammo"].as<int>(_ammo);
	_size = node["size"].as<int>(_size);
}

/**
 * Saves the base to a YAML file.
 * @return YAML node.
 */
YAML::Node Vehicle::save() const
{
	YAML::Node node;
	node["type"] = _rules->getType();
	node["ammo"] = _ammo;
	node["size"] = _size;
	return node;
}

/**
 * Returns the ruleset for the vehicle's type.
 * @return Pointer to ruleset.
 */
RuleItem *Vehicle::getRules() const
{
	return _rules;
}

/**
 * Returns the ammo (projectiles) contained in this vehicle.
 * @return Weapon ammo (projectiles).
 */
int Vehicle::getAmmo() const
{
	if (_ammo == -1)
	{
		return 255;
	}
	return _ammo;
}

/**
 * Returns the ammo (clips) contained in this vehicle.
 * @return Weapon ammo (clips).
 */
int Vehicle::getClips() const
{
	if (_ammo == -1 || _game->getMod()->getUnit(_rules->getType())->getCompatibleAmmoClips().empty())
	{
		return 0;
	}
	//Shortcut since only 1 ammo kind is stored in the save and game will always use first from list.
	int craftMaxClips = _game->getMod()->getUnit(_rules->getType())->getCompatibleAmmoClips().begin()->second;
	int craftMaxProjectiles = _game->getMod()->getUnit(_rules->getType())->getCompatibleProjectiles().begin()->second;
	return _ammo * craftMaxClips / craftMaxProjectiles;
}

/**
 * Changes the ammo contained in this vehicle.
 * @param ammo Weapon ammo.
 */
void Vehicle::setAmmo(int ammo)
{
	if (_ammo != -1)
	{
		_ammo = ammo;
	}
}

/**
 * Returns the size occupied by this vehicle
 * in a transport craft.
 * @return Size in tiles.
 */
int Vehicle::getSize() const
{
	return _size;
}

}
