/**
 * @file json.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the JSON type and free functions used to parse from range types.
 *
 * Copyright 2019 Kamil Rusin
 */

#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace fusion_server::json {

/**
 * This is the JSON type used in the program.
 */
using JSON = nlohmann::json;

/**
 * @brief Parses the JSON object from given range.
 *
 * @tparam It1
 *   An iterator type.
 *
 * @tparam It2
 *   An iterator type.
 *
 * @param [in] it1
 *   An iterator of the beginning of the range.
 *
 * @param [in] it2
 *   An iterator of the end of the range.
 *
 * @return
 *   If the operation was successful it returns a JSON object, otherwise it
 *   returns an "invalid-state" object.
 */
template <typename It1, typename It2>
std::optional<JSON> Parse(It1&& it1, It2&& it2) noexcept {
  try {
    return JSON::parse(std::forward<It1>(it1), std::forward<It2>(it2));
  }
  catch (...) {
    return {};
  }
}

/**
 * This function returns a pair of an indication whether or not the verification
 * was successful and a JSON object which is either a parsed package or
 * an error message.
 *
 * @param[in] raw_package
 *   The raw package read from a client.
 *
 * @return
 *   A pair of an indication whether or not the verification was successful
 *   and a JSON object which is either a parsed package or an error message is
 *   returned.
 */
std::pair<bool, JSON>
Verify(const std::string& raw_package) noexcept;

}  // namespace fusion_server::json
