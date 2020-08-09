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
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the
//      "Software"), to deal in the Software without restriction, including
//      without limitation the rights to use, copy, modify, merge, publish,
//      distribute, sublicense, and/or sell copies of the Software, and to
//      permit persons to whom the Software is furnished to do so, subject to
//      the following conditions:
//
//      The above copyright notice and this permission notice shall be included
//      in all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

namespace stratagus::vector {

static inline const std::vector<std::string> empty_string_vector;

template <typename T, typename U>
bool contains(const std::vector<T> &vector, const U &value)
{
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

template <typename T, typename U>
void merge(std::vector<T> &vector, const U &other_container)
{
	for (const typename U::value_type &element : other_container) {
		vector.push_back(element);
	}
}

template <typename T, typename U>
void merge(std::vector<T> &vector, U &&other_container)
{
	for (typename U::value_type &element : other_container) {
		vector.push_back(std::move(element));
	}
}

template <typename T>
inline void remove(std::vector<T> &vector, const typename std::vector<T>::value_type &element )
{
	vector.erase(std::remove(vector.begin(), vector.end(), element), vector.end());
}

template <typename T>
inline void remove(std::vector<std::unique_ptr<T>> &vector, const T *element)
{
	for (size_t i = 0; i < vector.size();) {
		if (vector[i].get() == element) {
			vector.erase(vector.begin() + i);
			return; //since we are using unique pointers, the element is necessarily unique, so there is nothing further to do
		} else {
			++i;
		}
	}
}

template <typename T>
inline void remove_one(std::vector<T> &vector, const T &element)
{
	vector.erase(std::find(vector.begin(), vector.end(), element));
}

}
