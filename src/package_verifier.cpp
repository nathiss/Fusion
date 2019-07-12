/**
 * @file package_verifier.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the PackageVerifier class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <fusion_server/package_verifier.hpp>

namespace fusion_server {

std::pair<bool, PackageParser::JSON>
PackageVerifier::Verify(const std::string& raw_package) const noexcept {
  auto parsed = package_parser_.Parse(raw_package);
  if (!parsed) {
    return std::make_pair(false, MakeNotValidJSON());
  }

  auto json = std::move(parsed.value());

  if (!(json.contains("type") &&
  json["type"].type() == decltype(json)::value_t::string)) {
    return std::make_pair(false, MakeTypeNotFound());
  }

  if (json["type"] == "join") {
    if (!(json.contains("id") &&
    json.contains("nick") &&
    json.contains("game") &&
    json.size() ==  3 + 1 &&
    json["id"].type() == decltype(json)::value_t::number_unsigned &&
    json["nick"].type() == decltype(json)::value_t::string &&
    json["game"].type() == decltype(json)::value_t::string)) {
      return std::make_pair(false, MakeNotValidJoin());
    }
    return std::make_pair(true, std::move(json));
  }

  if (json["type"] == "update") {
    if (!(json.contains("team_id") &&
    json.contains("position") &&
    json.contains("angle") &&
    json.size() ==  3 + 1 &&
    json["team_id"].type() == decltype(json)::value_t::number_unsigned &&
    json["position"].type() == decltype(json)::value_t::array &&
    json["position"].size() == 2 &&
    json["position"][0].type() == decltype(json)::value_t::number_float &&
    json["position"][1].type() == decltype(json)::value_t::number_float &&
    json["angle"].type() == decltype(json)::value_t::number_float)) {
      return std::make_pair(false, MakeNotValidUpdate());
    }
    return std::make_pair(true, std::move(json));
  }

  if (json["type"] == "leave") {
    if (json.size() != 1) {
      return std::make_pair(false, MakeNotValidLeave());
    }
    return std::make_pair(true, std::move(json));
  }


  // Received an undentified package.
  return std::make_pair(false, MakeUnidentified());
}

PackageParser::JSON PackageVerifier::MakeNotValidJSON() const noexcept {
  return PackageParser::JSON({
    {"closed", {true}},
    {"type", {"error"}},
    {"message", {"One of the packages didn't contain a valid JSON."}}
  }, false, PackageParser::JSON::value_t::object);
}

PackageParser::JSON PackageVerifier::MakeTypeNotFound() const noexcept {
  return PackageParser::JSON({
    {"closed", {true}},
    {"type", {"error"}},
    {"message", {"One of the packages didn't have a \"type\" field."}}
  }, false, PackageParser::JSON::value_t::object);
}

PackageParser::JSON PackageVerifier::MakeNotValidJoin() const noexcept {
  return PackageParser::JSON({
    {"closed", {true}},
    {"type", {"error"}},
    {"message", {"A \"JOIN\" was ill-formed.."}}
  }, false, PackageParser::JSON::value_t::object);
}

PackageParser::JSON PackageVerifier::MakeNotValidUpdate() const noexcept {
  return PackageParser::JSON({
    {"closed", {true}},
    {"type", {"error"}},
    {"message", {"A \"UPDATE\" was ill-formed."}}
  }, false, PackageParser::JSON::value_t::object);
}

PackageParser::JSON PackageVerifier::MakeNotValidLeave() const noexcept {
  return PackageParser::JSON({
    {"closed", {true}},
    {"type", {"error"}},
    {"message", {"A \"LEAVE\" was ill-formed."}}
  }, false, PackageParser::JSON::value_t::object);
}

PackageParser::JSON PackageVerifier::MakeUnidentified() const noexcept {
  return PackageParser::JSON({
    {"closed", {true}},
    {"type", {"error"}},
    {"message", {"Cannot identify a packge."}}
  }, false, PackageParser::JSON::value_t::object);
}

}  // namespace fusion_server
