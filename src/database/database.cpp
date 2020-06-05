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
//      (c) Copyright 2019-2020 by Andrettin
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

#include "database/database.h"

#include "age.h"
#include "animation.h"
#include "campaign.h"
#include "character.h"
#include "civilization.h"
#include "civilization_group.h"
#include "civilization_supergroup.h"
#include "database/data_type_metadata.h"
#include "database/defines.h"
#include "database/module.h"
#include "database/sml_data.h"
#include "database/sml_operator.h"
#include "database/sml_parser.h"
#include "database/sml_property.h"
#include "dialogue.h"
#include "faction.h"
#include "faction_tier.h"
#include "gender.h"
#include "item_class.h"
#include "item_slot.h"
#include "map/map_template.h"
#include "map/region.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "missile/missile_class.h"
#include "missile.h"
#include "plane.h"
#include "player_color.h"
#include "quest.h"
#include "religion/deity.h"
#include "resource.h"
#include "sound/sound.h"
#include "species.h"
#include "time/calendar.h"
#include "time/time_of_day.h"
#include "time/timeline.h"
#include "ui/button_level.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/icon.h"
#include "unit/historical_unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_structs.h"
#include "util/qunique_ptr.h"
#include "util/string_util.h"
#include "world.h"

namespace stratagus {

/**
**	@brief	Process a SML property for an instance of a QObject-derived class
**
**	@param	object		The object
**	@param	property	The property
*/
void database::process_sml_property_for_object(QObject *object, const sml_property &property)
{
	const QMetaObject *meta_object = object->metaObject();
	const std::string class_name = meta_object->className();
	const int property_count = meta_object->propertyCount();
	for (int i = 0; i < property_count; ++i) {
		QMetaProperty meta_property = meta_object->property(i);
		const char *property_name = meta_property.name();

		if (property_name != property.get_key()) {
			continue;
		}

		const QVariant::Type property_type = meta_property.type();

		if (property_type == QVariant::Type::List || property_type == QVariant::Type::StringList) {
			database::modify_list_property_for_object(object, property_name, property.get_operator(), property.get_value());
			return;
		} else if (property_type == QVariant::String) {
			if (property.get_operator() != sml_operator::assignment) {
				throw std::runtime_error("Only the assignment operator is available for string properties.");
			}

			const std::string method_name = "set_" + property.get_key();

			const bool success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(const std::string &, property.get_value()));

			if (!success) {
				throw std::runtime_error("Failed to set value for string property \"" + property.get_key() + "\".");
			}
			return;
		} else {
			QVariant new_property_value = database::process_sml_property_value(property, meta_property, object);
			bool success = object->setProperty(property_name, new_property_value);
			if (!success) {
				throw std::runtime_error("Failed to set value for property \"" + std::string(property_name) + "\".");
			}
			return;
		}
	}

	throw std::runtime_error("Invalid " + std::string(meta_object->className()) + " property: \"" + property.get_key() + "\".");
}

QVariant database::process_sml_property_value(const sml_property &property, const QMetaProperty &meta_property, const QObject *object)
{
	const std::string class_name = meta_property.enclosingMetaObject()->className();
	const char *property_name = meta_property.name();
	const std::string property_class_name = meta_property.typeName();
	const QVariant::Type property_type = meta_property.type();

	QVariant new_property_value;
	if (property_type == QVariant::Bool) {
		if (property.get_operator() != sml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for boolean properties.");
		}

		new_property_value = string::to_bool(property.get_value());
	} else if (property_type == QVariant::Int) {
		int value = std::stoi(property.get_value());

		if (property.get_operator() == sml_operator::addition) {
			value = object->property(property_name).toInt() + value;
		} else if (property.get_operator() == sml_operator::subtraction) {
			value = object->property(property_name).toInt() - value;
		}

		new_property_value = value;
	} else if (property_type == QVariant::Double) {
		double value = std::stod(property.get_value());

		if (property.get_operator() == sml_operator::addition) {
			value = object->property(property_name).toDouble() + value;
		} else if (property.get_operator() == sml_operator::subtraction) {
			value = object->property(property_name).toDouble() - value;
		}

		new_property_value = value;
	} else if (property_type == QVariant::DateTime) {
		if (property.get_operator() != sml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for date-time properties.");
		}

		new_property_value = string::to_date(property.get_value());
	} else if (property_type == QVariant::Type::UserType) {
		if (property.get_operator() != sml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for object reference properties.");
		}

		if (property_class_name == "stratagus::age*") {
			new_property_value = QVariant::fromValue(age::get(property.get_value()));
		} else if (property_class_name == "stratagus::animation_set*") {
			new_property_value = QVariant::fromValue(animation_set::get(property.get_value()));
		} else if (property_class_name == "stratagus::button_level*") {
			new_property_value = QVariant::fromValue(button_level::get(property.get_value()));
		} else if (property_class_name == "stratagus::calendar*") {
			new_property_value = QVariant::fromValue(calendar::get(property.get_value()));
		} else if (property_class_name == "stratagus::campaign*") {
			new_property_value = QVariant::fromValue(campaign::get(property.get_value()));
		} else if (property_class_name == "stratagus::character*") {
			new_property_value = QVariant::fromValue(character::get(property.get_value()));
		} else if (property_class_name == "stratagus::civilization*") {
			new_property_value = QVariant::fromValue(civilization::get(property.get_value()));
		} else if (property_class_name == "stratagus::civilization_group*") {
			new_property_value = QVariant::fromValue(civilization_group::get(property.get_value()));
		} else if (property_class_name == "stratagus::civilization_supergroup*") {
			new_property_value = QVariant::fromValue(civilization_supergroup::get(property.get_value()));
		} else if (property_class_name == "stratagus::cursor*") {
			new_property_value = QVariant::fromValue(cursor::get(property.get_value()));
		} else if (property_class_name == "stratagus::cursor_type") {
			new_property_value = QVariant::fromValue(string_to_cursor_type(property.get_value()));
		} else if (property_class_name == "stratagus::deity*") {
			new_property_value = QVariant::fromValue(deity::get(property.get_value()));
		} else if (property_class_name == "stratagus::dialogue*") {
			new_property_value = QVariant::fromValue(dialogue::get(property.get_value()));
		} else if (property_class_name == "stratagus::faction*") {
			new_property_value = QVariant::fromValue(faction::get(property.get_value()));
		} else if (property_class_name == "stratagus::faction_tier") {
			new_property_value = QVariant::fromValue(string_to_faction_tier(property.get_value()));
		} else if (property_class_name == "stratagus::gender") {
			new_property_value = QVariant::fromValue(string_to_gender(property.get_value()));
		} else if (property_class_name == "stratagus::icon*") {
			new_property_value = QVariant::fromValue(icon::get(property.get_value()));
		} else if (property_class_name == "stratagus::item_class") {
			new_property_value = QVariant::fromValue(string_to_item_class(property.get_value()));
		} else if (property_class_name == "stratagus::item_slot") {
			new_property_value = QVariant::fromValue(string_to_item_slot(property.get_value()));
		} else if (property_class_name == "stratagus::map_template*") {
			new_property_value = QVariant::fromValue(map_template::get(property.get_value()));
		} else if (property_class_name == "stratagus::missile_class") {
			new_property_value = QVariant::fromValue(string_to_missile_class(property.get_value()));
		} else if (property_class_name == "stratagus::missile_type*") {
			new_property_value = QVariant::fromValue(missile_type::get(property.get_value()));
		} else if (property_class_name == "stratagus::module*") {
			new_property_value = QVariant::fromValue(database::get()->get_module(property.get_value()));
		} else if (property_class_name == "stratagus::plane*") {
			new_property_value = QVariant::fromValue(plane::get(property.get_value()));
		} else if (property_class_name == "stratagus::player_color*") {
			new_property_value = QVariant::fromValue(player_color::get(property.get_value()));
		} else if (property_class_name == "stratagus::quest*") {
			new_property_value = QVariant::fromValue(quest::get(property.get_value()));
		} else if (property_class_name == "stratagus::region*") {
			new_property_value = QVariant::fromValue(region::get(property.get_value()));
		} else if (property_class_name == "stratagus::resource*") {
			new_property_value = QVariant::fromValue(resource::get(property.get_value()));
		} else if (property_class_name == "stratagus::site*") {
			new_property_value = QVariant::fromValue(site::get(property.get_value()));
		} else if (property_class_name == "stratagus::sound*") {
			new_property_value = QVariant::fromValue(sound::get(property.get_value()));
		} else if (property_class_name == "stratagus::species*") {
			new_property_value = QVariant::fromValue(species::get(property.get_value()));
		} else if (property_class_name == "stratagus::terrain_type*") {
			new_property_value = QVariant::fromValue(terrain_type::get(property.get_value()));
		} else if (property_class_name == "stratagus::time_of_day*") {
			new_property_value = QVariant::fromValue(time_of_day::get(property.get_value()));
		} else if (property_class_name == "stratagus::unit_class*") {
			new_property_value = QVariant::fromValue(unit_class::get(property.get_value()));
		} else if (property_class_name == "stratagus::unit_type*") {
			new_property_value = QVariant::fromValue(unit_type::get(property.get_value()));
		} else if (property_class_name == "CUpgrade*") {
			new_property_value = QVariant::fromValue(CUpgrade::get(property.get_value()));
		} else if (property_class_name == "stratagus::upgrade_class*") {
			new_property_value = QVariant::fromValue(upgrade_class::get(property.get_value()));
		} else if (property_class_name == "stratagus::world*") {
			new_property_value = QVariant::fromValue(world::get(property.get_value()));
		} else {
			throw std::runtime_error("Unknown type (\"" + property_class_name + "\") for object reference property \"" + std::string(property_name) + "\" (\"" + property_class_name + "\").");
		}
	} else {
		throw std::runtime_error("Invalid type for property \"" + std::string(property_name) + "\": \"" + std::string(meta_property.typeName()) + "\".");
	}

	return new_property_value;
}

void database::process_sml_scope_for_object(QObject *object, const sml_data &scope)
{
	const QMetaObject *meta_object = object->metaObject();
	const std::string class_name = meta_object->className();
	const int property_count = meta_object->propertyCount();
	for (int i = 0; i < property_count; ++i) {
		QMetaProperty meta_property = meta_object->property(i);
		const char *property_name = meta_property.name();

		if (property_name != scope.get_tag()) {
			continue;
		}

		const QVariant::Type property_type = meta_property.type();

		if (scope.get_operator() == sml_operator::assignment) {
			if ((property_type == QVariant::Type::List || property_type == QVariant::Type::StringList) && !scope.get_values().empty()) {
				for (const std::string &value : scope.get_values()) {
					database::modify_list_property_for_object(object, property_name, sml_operator::addition, value);
				}
				return;
			} else if (property_type == QVariant::Type::List && scope.has_children()) {
				scope.for_each_child([&](const sml_data &child_scope) {
					database::modify_list_property_for_object(object, property_name, sml_operator::addition, child_scope);
				});
				return;
			}
		} else {
			if (property_type == QVariant::Type::List) {
				database::modify_list_property_for_object(object, property_name, scope.get_operator(), scope);
				return;
			}
		}

		QVariant new_property_value = database::process_sml_scope_value(scope, meta_property);
		const bool success = object->setProperty(property_name, new_property_value);
		if (!success) {
			throw std::runtime_error("Failed to set value for scope property \"" + std::string(property_name) + "\".");
		}
		return;
	}

	throw std::runtime_error("Invalid " + std::string(meta_object->className()) + " scope property: \"" + scope.get_tag() + "\".");
}

QVariant database::process_sml_scope_value(const sml_data &scope, const QMetaProperty &meta_property)
{
	const std::string class_name = meta_property.enclosingMetaObject()->className();
	const char *property_name = meta_property.name();
	const std::string property_class_name = meta_property.typeName();
	const QVariant::Type property_type = meta_property.type();

	QVariant new_property_value;
	if (property_type == QVariant::Color) {
		if (scope.get_operator() != sml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for color properties.");
		}

		new_property_value = scope.to_color();
	} else if (property_type == QVariant::Point) {
		if (scope.get_operator() != sml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for point properties.");
		}

		new_property_value = scope.to_point();
	} else if (property_type == QVariant::PointF) {
		if (scope.get_operator() != sml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for point properties.");
		}

		new_property_value = scope.to_pointf();
	} else if (property_type == QVariant::Size) {
		if (scope.get_operator() != sml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for size properties.");
		}

		new_property_value = scope.to_size();
	} else if (property_class_name == "QGeoCoordinate") {
		if (scope.get_operator() != sml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for geocoordinate properties.");
		}

		new_property_value = QVariant::fromValue(scope.to_geocoordinate());
	} else {
		throw std::runtime_error("Invalid type for scope property \"" + std::string(property_name) + "\": \"" + std::string(meta_property.typeName()) + "\".");
	}

	return new_property_value;
}

void database::modify_list_property_for_object(QObject *object, const std::string &property_name, const sml_operator sml_operator, const std::string &value)
{
	const QMetaObject *meta_object = object->metaObject();
	const std::string class_name = meta_object->className();
	const int property_index = meta_object->indexOfProperty(property_name.c_str());
	QMetaProperty meta_property = meta_object->property(property_index);
	const QVariant::Type property_type = meta_property.type();

	if (sml_operator == sml_operator::assignment) {
		throw std::runtime_error("The assignment operator is not available for list properties.");
	}

	std::string method_name;
	if (sml_operator == sml_operator::addition) {
		method_name = "add_";
	} else if (sml_operator == sml_operator::subtraction) {
		method_name = "remove_";
	}

	method_name += string::get_singular_form(property_name);

	bool success = false;

	if (property_name == "dependencies") {
		module *module_value = database::get()->get_module(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(module *, module_value));
	} else if (property_name == "factions" || property_name == "cores") {
		faction *faction_value = faction::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(faction *, faction_value));
	} else if (property_name == "files") {
		const std::filesystem::path filepath(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(const std::filesystem::path &, filepath));
	} else if (property_name == "map_templates") {
		map_template *map_template_value = map_template::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(map_template *, map_template_value));
	} else if (property_name == "regions" || property_name == "superregions") {
		region *region_value = region::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(region *, region_value));
	} else if (property_name == "terrain_types" || property_name == "base_terrain_types" || property_name == "outer_border_terrain_types" || property_name == "inner_border_terrain_types") {
		terrain_type *terrain_type_value = terrain_type::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(terrain_type *, terrain_type_value));
	} else if (property_name == "unit_classes" || property_name == "building_classes") {
		unit_class *unit_class_value = unit_class::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(unit_class *, unit_class_value));
	} else if (property_name == "upgrades" || property_name == "acquired_upgrades") {
		CUpgrade *upgrade_value = CUpgrade::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(CUpgrade *, upgrade_value));
	} else if (property_type == QVariant::Type::StringList) {
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(const std::string &, value));
	} else {
		throw std::runtime_error("Unknown type for list property \"" + property_name + "\" (in class \"" + class_name + "\").");
	}

	if (!success) {
		throw std::runtime_error("Failed to add or remove value for list property \"" + property_name + "\".");
	}
}

void database::modify_list_property_for_object(QObject *object, const std::string &property_name, const sml_operator sml_operator, const sml_data &scope)
{
	const QMetaObject *meta_object = object->metaObject();
	const std::string class_name = meta_object->className();
	const int property_index = meta_object->indexOfProperty(property_name.c_str());
	QMetaProperty meta_property = meta_object->property(property_index);
	const QVariant::Type property_type = meta_property.type();

	if (sml_operator == sml_operator::assignment) {
		throw std::runtime_error("The assignment operator is not available for list properties.");
	}

	std::string method_name;
	if (sml_operator == sml_operator::addition) {
		method_name = "add_";
	} else if (sml_operator == sml_operator::subtraction) {
		method_name = "remove_";
	}

	method_name += string::get_singular_form(property_name);

	bool success = false;

	if (property_name == "colors") {
		const QColor color = scope.to_color();
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(QColor, color));
	} else {
		throw std::runtime_error("Unknown type for list property \"" + property_name + "\" (in class \"" + class_name + "\").");
	}

	if (!success) {
		throw std::runtime_error("Failed to add or remove value for list property \"" + property_name + "\".");
	}
}

std::filesystem::path database::get_base_path(const module *module)
{
	if (module != nullptr) {
		return module->get_path();
	}

	return database::get_root_path();
}

void database::parse_folder(const std::filesystem::path &path, std::vector<sml_data> &sml_data_list)
{
	std::filesystem::recursive_directory_iterator dir_iterator(path);

	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_regular_file() || dir_entry.path().extension() != ".txt") {
			continue;
		}

		sml_parser parser(dir_entry.path());
		sml_data_list.push_back(parser.parse());
	}
}

database::database()
{
}

database::~database()
{
}

void database::parse()
{
	for (const auto &kv_pair : database::get()->get_data_paths_with_module()) {
		const std::filesystem::path &path = kv_pair.first;
		const module *module = kv_pair.second;

		//parse the files in each data type's folder
		for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
			metadata->get_parsing_function()(path, module);
		}
	}
}

void database::load(const bool initial_definition)
{
	if (initial_definition) {
		//sort the metadata instances so they are placed after their class' dependencies' metadata
		std::sort(this->metadata.begin(), this->metadata.end(), [](const std::unique_ptr<data_type_metadata> &a, const std::unique_ptr<data_type_metadata> &b) {
			if (a->has_database_dependency_on(b)) {
				return false;
			} else if (b->has_database_dependency_on(a)) {
				return true;
			}

			return a->get_database_dependency_count() < b->get_database_dependency_count();
		});

		this->process_modules();
		this->parse();
	}

	try {
		//create or process data entries for each data type
		for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
			metadata->get_processing_function()(initial_definition);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to process database."));
	}
}

void database::load_defines()
{
	for (const auto &kv_pair : database::get()->get_data_paths_with_module()) {
		const std::filesystem::path &path = kv_pair.first;
		const module *module = kv_pair.second;

		try {
			defines::get()->load(path);
		} catch (...) {
			if (module != nullptr) {
				std::throw_with_nested(std::runtime_error("Failed to load the defines for the \"" + module->get_identifier() + "\" module."));
			} else {
				std::throw_with_nested(std::runtime_error("Failed to load defines."));
			}
		}
	}
}

void database::load_history()
{
	civilization::load_history_database();
	faction::load_history_database();
	site::load_history_database();
	historical_unit::load_history_database();
}

void database::initialize()
{
	//initialize data entries for each data type
	for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
		try {
			metadata->get_initialization_function()();
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Error initializing the instances of the " + metadata->get_class_identifier() + " class."));
		}
	}

	//check if data entries are valid for each data type
	for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
		try {
			metadata->get_checking_function()();
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Error when checking the instances of the " + metadata->get_class_identifier() + " class."));
		}
	}
}

void database::clear()
{
	//clear data entries for each data type
	for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
		metadata->get_clearing_function()();
	}
}

void database::register_metadata(std::unique_ptr<data_type_metadata> &&metadata)
{
	this->metadata.push_back(std::move(metadata));
}

void database::process_modules()
{
	if (std::filesystem::exists(database::get_modules_path())) {
		this->process_modules_at_dir(database::get_modules_path());
	}

	if (defines::get()->is_documents_modules_loading_enabled() && std::filesystem::exists(database::get_documents_modules_path())) {
		this->process_modules_at_dir(database::get_documents_modules_path());
	}

	for (const qunique_ptr<module> &module : this->modules) {
		const std::filesystem::path module_filepath = module->get_path() / "module.txt";

		if (std::filesystem::exists(module_filepath)) {
			sml_parser parser(module_filepath);
			database::process_sml_data(module, parser.parse());
		}
	}

	std::sort(this->modules.begin(), this->modules.end(), [](const qunique_ptr<module> &a, const qunique_ptr<module> &b) {
		if (a->depends_on(b.get())) {
			return false;
		} else if (b->depends_on(a.get())) {
			return true;
		}

		return a->get_dependency_count() < b->get_dependency_count();
	});
}

void database::process_modules_at_dir(const std::filesystem::path &path, module *parent_module)
{
	std::filesystem::directory_iterator dir_iterator(path);

	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_directory()) {
			continue;
		}

		if (dir_entry.path().stem().string().front() == '.') {
			continue; //ignore hidden directories, e.g. ".git" dirs
		}

		const std::string module_identifier = dir_entry.path().stem().string();
		auto module = make_qunique<stratagus::module>(module_identifier, dir_entry.path(), parent_module);

		std::filesystem::path submodules_path = dir_entry.path() / "modules";
		if (std::filesystem::exists(submodules_path)) {
			this->process_modules_at_dir(submodules_path, module.get());
		}

		this->modules_by_identifier[module_identifier] = module.get();
		this->modules.push_back(std::move(module));
	}
}

std::vector<std::filesystem::path> database::get_module_paths() const
{
	std::vector<std::filesystem::path> module_paths;

	for (const qunique_ptr<module> &module : this->modules) {
		module_paths.push_back(module->get_path());
	}

	return module_paths;
}

std::vector<std::pair<std::filesystem::path, const module *>> database::get_module_paths_with_module() const
{
	std::vector<std::pair<std::filesystem::path, const module *>> module_paths;

	for (const qunique_ptr<module> &module : this->modules) {
		module_paths.emplace_back(module->get_path(), module.get());
	}

	return module_paths;
}

}
