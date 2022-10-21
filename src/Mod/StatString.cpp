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
#include "StatString.h"
#include "Unit.h"
#include <vector>
#include <algorithm>
#include "../Engine/Language.h"
#include "../Engine/Unicode.h"

namespace OpenXcom
{

/**
 * Creates a blank StatString.
 */
StatString::StatString()
{
}

/**
 * Cleans up the extra StatString.
 */
StatString::~StatString()
{
}

/**
 * Loads the StatString from a YAML file.
 * @param node YAML node.
 */
void StatString::load(const YAML::Node &node)
{
	std::string conditionNames[] = {"psiStrength", "psiSkill", "bravery", "strength", "firing", "reactions", "stamina", "tu", "health", "throwing", "melee", "psiTraining", "manaPool"};
	_stringToBeAddedIfAllConditionsAreMet = node["string"].as<std::string>(_stringToBeAddedIfAllConditionsAreMet);
	for (size_t i = 0; i < std::size(conditionNames); i++)
	{
		if (node[conditionNames[i]])
		{
			_conditions.push_back(getCondition(conditionNames[i], node));
		}
		// Recognize percentage based definitions.
		if (node[conditionNames[i] + "Percent"])
		{
			_conditions.push_back(getCondition(conditionNames[i] + "Percent", node));
		}
	}
}

/**
 * Generates a condition from YAML.
 * @param conditionName Stat name of the condition.
 * @param node YAML node.
 * @return New StatStringCondition.
 */
StatStringCondition *StatString::getCondition(const std::string &conditionName, const YAML::Node &node)
{
	// These are the defaults from xcomutil
	int minValue = 0, maxValue = 255;
	if (node[conditionName][0])
	{
		minValue = node[conditionName][0].as<int>(minValue);
	}
	if (node[conditionName][1])
	{
		maxValue = node[conditionName][1].as<int>(maxValue);
	}
	StatStringCondition *thisCondition = new StatStringCondition(conditionName, minValue, maxValue);
	return thisCondition;
}

/**
 * Returns the conditions associated with this StatString.
 * @return List of StatStringConditions.
 */
const std::vector<StatStringCondition*> &StatString::getConditions() const
{
	return _conditions;
}

/**
 * Returns the string to add to a name for this StatString.
 * @return StatString... string.
 */
std::string StatString::getString() const
{
	return _stringToBeAddedIfAllConditionsAreMet;
}

/**
 * Calculates the list of StatStrings that apply to certain unit stats.
 * @param currentStats Unit stats.
 * @param statCaps  Unit type stat caps
 * @param statStrings List of statString rules.
 * @param psiStrengthEval Are psi stats available?
 * @return Resulting string of all valid StatStrings.
 */
std::string StatString::calcStatString(UnitStats &currentStats, UnitStats &statCaps, const std::vector<StatString *> &statStrings, bool psiStrengthEval, bool inTraining)
{
	std::string statString;
	std::map<std::string, int> currentStatsMap = getCurrentStats(currentStats);
	std::map<std::string, int> currentStatPercentageMap = getCurrentStatPercent(currentStats, statCaps);
	if (inTraining)
	{
		currentStatsMap["psiTraining"] = 1;
	}
	for (std::vector<StatString *>::const_iterator i = statStrings.begin(); i != statStrings.end(); ++i)
	{
		bool conditionsMet = true;
		for (std::vector<StatStringCondition*>::const_iterator j = (*i)->getConditions().begin(); j != (*i)->getConditions().end() && conditionsMet; ++j)
		{
			// Start with 'currentStatPercentageMap' so that reaching the end does not matter.
			std::map<std::string, int>::iterator percent = currentStatPercentageMap.find((*j)->getConditionName());
			if (percent != currentStatPercentageMap.end())
			{
				conditionsMet = conditionsMet && (*j)->isMet(percent->second, currentStats.psiSkill > 0 || psiStrengthEval);
				continue; // Condition evaluated, go to next one.
			}
			// Not a percentage based condition, can safely continue to normal conditions (the original implementation).
			std::map<std::string, int>::iterator name = currentStatsMap.find((*j)->getConditionName());
			if (name != currentStatsMap.end())
			{
				conditionsMet = conditionsMet && (*j)->isMet(name->second, currentStats.psiSkill > 0 || psiStrengthEval);
			}
			else
			{
				// if name == currentStatsMap.end() we've searched for a stat that doesn't exist.
				// this means psi training. if there's no "psiTraining" stat in the statsMap,
				// this soldier isn't in training, so we won't append his name with the psiTraining tag.
				// presumably conditionsMet was originally initialized as false, but for whatever reason that was changed, hence this.
				conditionsMet = false;
			}
		}
		if (conditionsMet)
		{
			std::string wstring = (*i)->getString();
			statString += wstring;
			if (Unicode::codePointLengthUTF8(wstring) > 1)
			{
				break;
			}
		}
	}
	return statString;
}

/**
 * Get a map associating stat names to unit stats.
 * @param currentStats Unit stats to use.
 * @return Map of unit stats.
 */
std::map<std::string, int> StatString::getCurrentStats(UnitStats &currentStats)
{
	std::map<std::string, int> currentStatsMap;
	currentStatsMap["psiStrength"] = currentStats.psiStrength;
	currentStatsMap["psiSkill"] = currentStats.psiSkill;
	currentStatsMap["bravery"] = currentStats.bravery;
	currentStatsMap["strength"] = currentStats.strength;
	currentStatsMap["firing"] = currentStats.firing;
	currentStatsMap["reactions"] = currentStats.reactions;
	currentStatsMap["stamina"] = currentStats.stamina;
	currentStatsMap["tu"] = currentStats.tu;
	currentStatsMap["health"] = currentStats.health;
	currentStatsMap["throwing"] = currentStats.throwing;
	currentStatsMap["melee"] = currentStats.melee;
	currentStatsMap["manaPool"] = currentStats.mana;
	return currentStatsMap;
}

/**
 * Get a map associating stat names to unit's percentage of it's statCaps.
 *
 * @param currentStats Unit stats to use.
 * @param currentCaps  Unit type stat caps
 * @return Map of unit's stats percentage w.r.t. statCaps.
 */
std::map<std::string, int> StatString::getCurrentStatPercent(UnitStats &currentStats, UnitStats &currentCaps)
{
	auto normalizedPercentage = [&](UnitStats::Type current, UnitStats::Type cap)
	{
		if ((int)cap == 0)
		{
			// Prevent non-existing stat from being used. Ensure return value is
			// below default limit (0) as defined by 'getCondition()'.
			return -1;
		}
		else
		{
			// Even though 'current' could be slightly larger than 'cap' due to circumstances.
			// The resulting percentage (which will not be far from 100) wil not go beyond the
			// default upper limit (255) as defined by 'getCondition()'.
			return 100 * current / cap;
		}
	};

	std::map<std::string, int> currentStatCapsMap;
	currentStatCapsMap["psiStrengthPercent"] = normalizedPercentage(currentStats.psiStrength, currentCaps.psiStrength);
	currentStatCapsMap["psiSkillPercent"] =  normalizedPercentage(currentStats.psiSkill, currentCaps.psiSkill);
	currentStatCapsMap["braveryPercent"] =  normalizedPercentage(currentStats.bravery, currentCaps.bravery);
	currentStatCapsMap["strengthPercent"] =  normalizedPercentage(currentStats.strength, currentCaps.strength);
	currentStatCapsMap["firingPercent"] =  normalizedPercentage(currentStats.firing, currentCaps.firing);
	currentStatCapsMap["reactionsPercent"] =  normalizedPercentage(currentStats.reactions, currentCaps.reactions);
	currentStatCapsMap["staminaPercent"] =  normalizedPercentage(currentStats.stamina, currentCaps.stamina);
	currentStatCapsMap["tuPercent"] =  normalizedPercentage(currentStats.tu, currentCaps.tu);
	currentStatCapsMap["healthPercent"] =  normalizedPercentage(currentStats.health, currentCaps.health);
	currentStatCapsMap["throwingPercent"] =  normalizedPercentage(currentStats.throwing, currentCaps.throwing);
	currentStatCapsMap["meleePercent"] =  normalizedPercentage(currentStats.melee, currentCaps.melee);
	currentStatCapsMap["manaPoolPercent"] =  normalizedPercentage(currentStats.mana, currentCaps.mana);
	return currentStatCapsMap;
}

}
