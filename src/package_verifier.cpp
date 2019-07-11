/**
 * @file package_verifier.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the PackageVerifier class.
 *
 * (c) 2019 by Kamil Rusin
 */

#include <fusion_server/package_verifier.hpp>

namespace fusion_server {

std::pair<bool, PackageParser::JSON>
PackageVerifier::Verify(std::string raw_package) const noexcept {
  auto parsed = package_parser_.Parse(std::move(raw_package));
  if (!parsed) {
    return std::make_pair(false, makeNotValidJSON());
  }

  auto json = std::move(parsed.value());

  if (!(json.contains("type") &&
  json["type"].type() == decltype(json)::value_t::string
  )) {
    return std::make_pair(false, makeTypeNotFound());
  }

  if (json["type"] == "join") {

    if (!(json.contains("id") &&
    json.contains("nick") &&
    json.contains("game") &&
    json.size() ==  3 + 1 &&
    json["id"].type() == decltype(json)::value_t::number_unsigned &&
    json["nick"].type() == decltype(json)::value_t::string &&
    json["game"].type() == decltype(json)::value_t::string
    )) {
      return std::make_pair(false, makeNotValidJoin());
    }

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
    json["angle"].type() == decltype(json)::value_t::number_float
    )) {
      return std::make_pair(false, makeNotValidUpdate());
    }
  }

  if (json["type"] == "leave") {
    if (json.size() != 1) {
      return std::make_pair(false, makeNotValidLeave());
    }
  }

  return std::make_pair(true, std::move(json));
}

PackageParser::JSON PackageVerifier::makeNotValidJSON() const noexcept {
  PackageParser::JSON ret = PackageParser::JSON::object();
  ret["closed"] = true;
  ret["type"] = "error";
  ret["message"] = "One of the packages didn't contain a valid JSON.";
  return ret;
}

PackageParser::JSON PackageVerifier::makeTypeNotFound() const noexcept {
  PackageParser::JSON ret = PackageParser::JSON::object();
  ret["closed"] = true;
  ret["type"] = "error";
  ret["message"] = "One of the packages didn't have a \"type\" field.";
  return ret;
}

PackageParser::JSON PackageVerifier::makeNotValidJoin() const noexcept {
  PackageParser::JSON ret = PackageParser::JSON::object();
  ret["closed"] = true;
  ret["type"] = "error";
  ret["message"] = "A \"JOIN\" was ill-formed.";
  return ret;
}

PackageParser::JSON PackageVerifier::makeNotValidUpdate() const noexcept {
  PackageParser::JSON ret = PackageParser::JSON::object();
  ret["closed"] = true;
  ret["type"] = "error";
  ret["message"] = "A \"UPDATE\" was ill-formed.";
  return ret;
}

PackageParser::JSON PackageVerifier::makeNotValidLeave() const noexcept {
  PackageParser::JSON ret = PackageParser::JSON::object();
  ret["closed"] = true;
  ret["type"] = "error";
  ret["message"] = "A \"LEAVE\" was ill-formed.";
  return ret;
}

}  // namespace fusion_server