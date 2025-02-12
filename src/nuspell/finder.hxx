/* Copyright 2016-2019 Dimitrij Mijoski
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Finding dictionaries, PUBLIC HEADER.
 */

#ifndef NUSPELL_FINDER_HXX
#define NUSPELL_FINDER_HXX
#include <string>
#include <utility>
#include <vector>

namespace nuspell {
inline namespace v3 {
class Finder {
	using Dict_List = std::vector<std::pair<std::string, std::string>>;

	std::vector<std::string> paths;
	Dict_List dictionaries;

      public:
	using const_iterator = Dict_List::const_iterator;

	auto add_default_dir_paths() -> void;
	[[deprecated]] auto add_mozilla_dir_paths() -> void;
	auto add_libreoffice_dir_paths() -> void;
	[[deprecated]] auto add_openoffice_dir_paths() -> void;
	auto search_for_dictionaries() -> void;

	auto static search_all_dirs_for_dicts() -> Finder;

	auto& get_dir_paths() const { return paths; }
	auto& get_dictionaries() const { return dictionaries; }
	auto begin() const { return dictionaries.begin(); }
	auto end() const { return dictionaries.end(); }
	auto find(const std::string& dict) const -> const_iterator;
	auto equal_range(const std::string& dict) const
	    -> std::pair<const_iterator, const_iterator>;
	auto get_dictionary_path(const std::string& dict) const -> std::string;
};
} // namespace v3
} // namespace nuspell

#endif // NUSPELL_FINDER_HXX
