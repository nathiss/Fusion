/**
 * @file package_parser.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the PackageParser class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <fusion_server/package_parser.hpp>

namespace fusion_server {

std::optional<PackageParser::JSON>
PackageParser::Parse(const std::string& package) const noexcept {
  try {
    return JSON::parse(package, nullptr, true);
  }
  catch(...) {
    return {};
  }
}

}  // namespace fusion_server
