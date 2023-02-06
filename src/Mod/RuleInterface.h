#pragma once
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
#include <string>
#include <map>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

class Mod;

struct Element
{
	/// basic rect info, and 3 colors.
	int x, y, w, h, color, color2, border, custom;
	/// defines inversion behaviour
	bool TFTDMode, customBool;
	std::vector<std::string> customList;
};

// Definition in progress ... expect changes.
struct InterfaceOption
{
	// Control activation of options
	bool isActive;
	int variant; // If defined "isActive" is not needed.
};

class RuleInterface
{
private:
	std::string _type;
	std::string _palette;
	std::string _parent;
	std::string _backgroundImage;
	std::string _altBackgroundImage;
	std::string _music;
	int _sound;

	std::map <std::string, Element> _elements;
	std::map <std::string, InterfaceOption> _options;
public:
	/// Constructor.
	RuleInterface(const std::string & type);
	/// Destructor.
	~RuleInterface();
	/// Load from YAML.
	void load(const YAML::Node& node, Mod *mod);
	/// Get an element.
	Element *getElement(const std::string &id);
	/// Get a screen specific option.
	InterfaceOption *getOption(const std::string &id);
	/// Get palette.
	const std::string &getPalette() const;
	/// Get parent interface rule.
	const std::string &getParent() const;
	/// Get background image.
	const std::string &getBackgroundImage() const;
	/// Get alternative background image (for battlescape theme).
	const std::string &getAltBackgroundImage() const;
	/// Get music.
	const std::string &getMusic() const;
	/// Get sound.
	int getSound() const;
};

}
