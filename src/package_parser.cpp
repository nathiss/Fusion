/**
 * @file package_parser.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the PackageParser class.
 *
 * (c) 2019 by Kamil Rusin
 */

#include <utility>

#include <fusion_server/package_parser.hpp>

namespace fusion_server {

auto PackageParser::Parse(std::string package) const noexcept
    -> std::optional<JSON> {
  try {
    return JSON::parse(std::move(package));
  }
  catch(...) {
    return {};
  }
}

}  // namespace fusion_server
