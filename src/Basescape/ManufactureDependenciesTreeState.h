#pragma once
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
#include "../Engine/State.h"

namespace OpenXcom
{

class Window;
class Text;
class TextButton;
class TextList;
class TextEdit;

/**
 * Window which displays manufacture dependencies tree.
 */
class ManufactureDependenciesTreeState : public State
{
private:
	struct TopicsBackend
	{
		int childId = 0; // Duplicate numbers are allowed (it only needs to be >= parentId)
		int parentId = 0;
		bool isVisible = false;
		std::string description;
	};

	Window *_window;
	Text *_txtTitle;
	TextList *_lstTopics;
	TextEdit *_btnQuickSearch;
	TextButton *_btnOk, *_btnShowAll, *_btnToggle;
	std::string _selectedItem;
	bool _showAll;
	void drawList();

	std::vector<TopicsBackend> _topics;
	std::vector<size_t> _indices;
	size_t _sel;
	TopicsBackend &getTopic() { return _topics[_indices[_sel]]; }

	void addResearchSection(int& startParentId);
	void addHowToAcquireItemSections(int& startParentId);
	void addNeededForSpecialsSections(int& startParentId);
	void addNeededForManufactureSections(int& startParentId);
	void fillTopicsList();

public:
	/// Creates the ManufactureDependenciesTree state.
	ManufactureDependenciesTreeState(const std::string &selectedItem);
	/// Cleans up the ManufactureDependenciesTree state
	~ManufactureDependenciesTreeState();
	/// Initializes the state.
	void init() override;
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the [Show All] button.
	void btnShowAllClick(Action *action);
	/// Handler for RMB click on list.
	void lstTopicsClickRight(Action *Action);
	/// Handlers for Quick Search.
	void btnQuickSearchToggle(Action *action);
	void btnQuickSearchApply(Action *action);
};
}
