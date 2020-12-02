//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
//      (c) Copyright 2020 by Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

#include "stratagus.h"

#include "text_processor.h"

#include "faction.h"
#include "language/word.h"
#include "unit/unit_type.h"
#include "util/queue_util.h"
#include "util/string_util.h"

namespace wyrmgus {

std::string text_processor::process_text(std::string &&text) const
{
	size_t find_pos = 0;
	while ((find_pos = text.find('[', find_pos)) != std::string::npos) {
		const size_t end_pos = text.find(']', find_pos);
		if (end_pos == std::string::npos) {
			return text;
		}

		const std::string substring = text.substr(find_pos + 1, end_pos - (find_pos + 1));
		std::queue<std::string> tokens = string::split_to_queue(substring, '.');
		const std::string replacement_str = this->process_tokens(std::move(tokens));
		text.replace(find_pos, end_pos + 1 - find_pos, replacement_str);

		++find_pos;
	}

	return text;
}

std::string text_processor::process_text(const std::string &text) const
{
	std::string result = text;
	return this->process_text(std::move(result));
}

std::string text_processor::process_tokens(std::queue<std::string> &&tokens) const
{
	if (tokens.empty()) {
		return std::string();
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	std::string str;

	if (front_subtoken == "faction") {
		const wyrmgus::faction *faction = this->faction;
		if (!subtokens.empty()) {
			faction = faction::get(queue::take(subtokens));
		}

		str = this->process_faction_tokens(faction, tokens);
	} else if (front_subtoken == "unit_type") {
		const wyrmgus::unit_type *unit_type = nullptr;
		if (!subtokens.empty()) {
			unit_type = unit_type::get(queue::take(subtokens));
		}

		str = this->process_unit_type_tokens(unit_type, tokens);
	} else if (front_subtoken == "word") {
		const wyrmgus::word *word = nullptr;
		if (!subtokens.empty()) {
			word = word::get(queue::take(subtokens));
		}

		str = this->process_word_tokens(word, tokens);
	} else {
		throw std::runtime_error("Failed to process token \"" + token + "\".");
	}

	if (!tokens.empty()) {
		return this->process_string_tokens(std::move(str), std::move(tokens));
	}

	return str;
}

std::string text_processor::process_string_tokens(std::string &&str, std::queue<std::string> &&tokens) const
{
	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing string tokens.");
	}

	const std::string token = queue::take(tokens);

	if (token == "lowered") {
		string::to_lower(str);
	} else if (token == "normalized") {
		string::normalize(str);
	} else {
		throw std::runtime_error("Failed to process string token \"" + token + "\".");
	}

	if (!tokens.empty()) {
		return this->process_string_tokens(std::move(str), std::move(tokens));
	}

	return str;
}

std::string text_processor::process_faction_tokens(const wyrmgus::faction *faction, std::queue<std::string> &tokens) const
{
	if (faction == nullptr) {
		throw std::runtime_error("No faction provided when processing faction tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing faction tokens.");
	}

	const std::string token = queue::take(tokens);

	if (token == "name") {
		return faction->get_name();
	} else if (token == "titled_name") {
		return faction->get_default_titled_name();
	}

	throw std::runtime_error("Failed to process faction token \"" + token + "\".");
}

std::string text_processor::process_unit_type_tokens(const wyrmgus::unit_type *unit_type, std::queue<std::string> &tokens) const
{
	if (unit_type == nullptr) {
		throw std::runtime_error("No unit type provided when processing unit type tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing unit type tokens.");
	}

	const std::string token = queue::take(tokens);

	if (token == "name") {
		return unit_type->get_name();
	}

	throw std::runtime_error("Failed to process unit type token \"" + token + "\".");
}

std::string text_processor::process_word_tokens(const wyrmgus::word *word, std::queue<std::string> &tokens) const
{
	if (word == nullptr) {
		throw std::runtime_error("No word provided when processing word tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing word tokens.");
	}

	const std::string token = queue::take(tokens);

	if (token == "name") {
		return word->get_name();
	} else if (token == "meanings") {
		return this->process_word_meaning_tokens(word, tokens);
	}

	throw std::runtime_error("Failed to process word token \"" + token + "\".");
}

std::string text_processor::process_word_meaning_tokens(const wyrmgus::word *word, std::queue<std::string> &tokens) const
{
	if (word == nullptr) {
		throw std::runtime_error("No word provided when processing word meaning tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing word meaning tokens.");
	}

	const std::string token = queue::take(tokens);

	return word->get_meanings().at(std::stoi(token) - 1);
}

}
