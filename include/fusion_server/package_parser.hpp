/**
 * @file package_parser.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the PackageParser class.
 *
 * (c) 2019 by Kamil Rusin
 */

#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace fusion_server {

/**
 * This class is a JSON parser used to parse incomming messages from the
 * WebSocket sessions.
 */
class PackageParser {
 public:
  /**
   * This is a type of a parsed package.
   */
  using JSON = nlohmann::json;

  /**
   * This method parses the given package. It returns parsed package.
   * If the package cannot be parsed (e.g. it contains ill-formed JSON),
   * the result has no value.
   *
   * @param[in] package
   *   The package to be parsed.
   *
   * @return
   *  The parsed package or no value, if the package cannot be properly parsed.
   */
  std::optional<JSON> Parse(const std::string& package) const noexcept;
};

}  // namespace fusion_server
