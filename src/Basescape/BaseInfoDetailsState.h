#pragma once
/*
 * Copyright 2024 OpenXcom Developers.
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

class Base;
class TextButton;
class ToggleTextButton;
class Window;
class Text;
class TextList;

enum class BaseInfoDetailsCategory {
	SOLDIERS, ENGINEERS, SCIENTISTS,
	QUARTERS, STORES, LABORATORIES, WORKSHOPS, CONTAINMENT, HANGARS,
	DEFENSE, DETECTION
	};
constexpr std::string_view enum2string(BaseInfoDetailsCategory category) //C++17 -> string_view
{
	switch (category)
	{
	case BaseInfoDetailsCategory::SOLDIERS:     return "Soldiers";
	case BaseInfoDetailsCategory::ENGINEERS:    return "Engineers";
	case BaseInfoDetailsCategory::SCIENTISTS:   return "Scientists";
	case BaseInfoDetailsCategory::QUARTERS:     return "Quarters";
	case BaseInfoDetailsCategory::STORES:       return "Stores";
	case BaseInfoDetailsCategory::LABORATORIES: return "Laboratories";
	case BaseInfoDetailsCategory::WORKSHOPS:    return "Workshops";
	case BaseInfoDetailsCategory::CONTAINMENT:  return "Containment";
	case BaseInfoDetailsCategory::HANGARS:      return "Hangars";
	case BaseInfoDetailsCategory::DEFENSE:      return "Defense";
	case BaseInfoDetailsCategory::DETECTION:    return "Detections";
	default: return "Internal enum error"; // Should not be reachable.
	}
}

/**
 * Base info category breakdown subwindow
 *
 * Shows which facilities contribute to selected category.
 */
class BaseInfoDetailsState : public State
{
private:
	Base *_base;
	BaseInfoDetailsCategory _category;

	TextButton *_btnOk, *_btnPrev, *_btnNext;
	ToggleTextButton *_btnAllBases;
	Window *_window;
	Text *_txtTitle, *_txtSource, *_txtQuantity, *_txtResult;
	TextList *_lstDetails, *_lstTotal;

	void drawBody();

	/// Handler for clicking the Grand Total button.
	void btnAllBasesClick(Action *action);
public:
	/// Creates the info details state.
	BaseInfoDetailsState(Base *base, BaseInfoDetailsCategory category);
	/// Cleans up the info details state.
	~BaseInfoDetailsState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);

	/// Handler for clicking Previous button.
	void btnPrevClick(Action *action);
	/// Handler for clicking Next button.
	void btnNextClick(Action *action);
};

}
