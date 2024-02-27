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
#include <yaml-cpp/yaml.h>
#include <climits>

namespace OpenXcom
{

enum class TransferSortDirection : int
{
	BY_LIST_ORDER,
	BY_UNIT_SIZE,
	BY_TOTAL_SIZE,
	BY_UNIT_COST,
	BY_TOTAL_COST
};

enum TransferType { TRANSFER_ITEM, TRANSFER_CRAFT, TRANSFER_SOLDIER, TRANSFER_SCIENTIST, TRANSFER_ENGINEER };

struct TransferRow
{
	TransferType type = TRANSFER_ITEM;
	const void *rule = nullptr;
	std::string name = "";
	int cost = 0;
	int qtySrc = 0; // Amount on 1st base (screen left side).
	int qtyDst = 0; // Amount on 2nd base or world market (screen right side).
	/**
	 * Requested change.
	 *
	 * + Positive values moves an item from `Src` to `Dst` (e.g. Left to Right).
	 * + Negative values moves an item from `Dst` to `Src` (e.g  Right to Left).
	 */
	int amount = 0;
	// Controls position in items list.
	// Default is chosen to push troublesome entries to the top.
	int listOrder = INT_MIN;
	double size = 0, totalSize = 0; // List sorting by (combined) item sizes?
	int64_t totalCost = 0;          // List Sorting by combined item cost?
	// Amount currently on route **to** `Src` / `Dst`.
	// Anything that will eventually end up in `qtySrc` / `qtyDst`.
	int transferSrc = 0, transferDst = 0;
	// Display only: Currently allocated items (e.g. reserved).
	int allocatedSrc = 0, allocatedDst = 0;
	// Display only: Add this amount to display of `qtySrc` / `qtyDst`,
	// Allows display of non-refundable amounts.
	int protectedSrc = 0, protectedDst = 0;

	TransferRow() = default;
	// Compatibility for existing code.
	TransferRow(TransferType _type, const void *_rule, std::string _name,
		int _cost, int _qtySrc, int _qtyDst, int _amount, int _listOrder,
		double _size, double _totalSize, int64_t _totalCost
		) :
		type(_type), rule(_rule), name(_name), cost(_cost), qtySrc(_qtySrc),
		qtyDst(_qtyDst), amount(_amount), listOrder(_listOrder), size(_size),
		totalSize(_totalSize), totalCost(_totalCost)
		{ }
};

class Soldier;
class Craft;
class Language;
class Base;
class Mod;
class RuleItem;
class SavedGame;

/**
 * Represents an item transfer.
 * Items are placed "in transit" whenever they are
 * purchased or transferred between bases.
 */
class Transfer
{
private:
	int _hours;
	Soldier *_soldier;
	Craft *_craft;
	const RuleItem* _itemId;
	int _itemQty, _scientists, _engineers;
	bool _delivered;
public:
	/// Creates a new transfer.
	Transfer(int hours);
	/// Cleans up the transfer.
	~Transfer();
	/// Loads the transfer from YAML.
	bool load(const YAML::Node& node, Base *base, const Mod *mod, SavedGame *save);
	/// Saves the transfer to YAML.
	YAML::Node save(const Base *b, const Mod *mod) const;
	/// Sets the soldier of the transfer.
	void setSoldier(Soldier *soldier);
	/// Sets the craft of the transfer.
	void setCraft(Craft *craft);
	/// Gets the craft of the transfer.
	Craft *getCraft() const;
	/// Gets the items of the transfer.
	const RuleItem* getItems() const;
	/// Sets the items of the transfer.
	void setItems(const RuleItem* rule, int qty = 1);
	/// Sets the scientists of the transfer.
	void setScientists(int scientists);
	/// Sets the engineers of the transfer.
	void setEngineers(int engineers);
	/// Gets the name of the transfer.
	std::string getName(Language *lang) const;
	/// Gets the hours remaining of the transfer.
	int getHours() const;
	/// Gets the quantity of the transfer.
	int getQuantity() const;
	/// Gets the type of the transfer.
	TransferType getType() const;
	/// Advances the transfer.
	void advance(Base *base);
	/// Get a pointer to the soldier being transferred.
	Soldier *getSoldier() const;

};

}
