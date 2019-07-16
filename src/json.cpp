#include <fusion_server/json.hpp>

namespace fusion_server::json {

namespace {

/**
 * This method returns a JSON object contains an error message for the client
 * informing about not valid JSON syntax.
 *
 * @return
 *   A JSON object contains an error message for the client informing about
 *   not valid JSON syntax is returned.
 */
JSON MakeNotValidJSON() noexcept {
  return JSON({
    {"closed", true},
    {"type", "error"},
    {"message", "One of the packages didn't contain a valid JSON."}
    }, false, JSON::value_t::object);
}

/**
 * This method returns a JSON object contains an error message for the client
 * informing that "type" field is not present is the received package.
 *
 * @return
 *   A JSON object contains an error message for the client informing that
 *   "type" field is not present is the received package is returned.
 */
JSON MakeTypeNotFound() noexcept {
  return JSON({
    {"closed", true},
    {"type", "error"},
    {"message", "One of the packages didn't have a \"type\" field."}
    }, false, JSON::value_t::object);
}

/**
 * This method returns a JSON object contains an error message for the client
 * informing that a "JOIN" package was ill-formed.
 *
 * @return
 *   A JSON object contains an error message for the client informing that
 *   a "JOIN" package was ill-formed is returned.
 */
JSON MakeNotValidJoin() noexcept {
  return JSON({
    {"closed", true},
    {"type", "error"},
    {"message", "A \"JOIN\" was ill-formed.."}
    }, false, JSON::value_t::object);
}

/**
 * This method returns a JSON object contains an error message for the client
 * informing that a "UPDATE" package was ill-formed.
 *
 * @return
 *   A JSON object contains an error message for the client informing that
 *   a "UPDATE" package was ill-formed is returned.
 */
JSON MakeNotValidUpdate() noexcept {
  return JSON({
    {"closed", true},
    {"type", "error"},
    {"message", "A \"UPDATE\" was ill-formed."}
    }, false, JSON::value_t::object);
}

/**
 * This method returns a JSON object contains an error message for the client
 * informing that a "LEAVE" package was ill-formed.
 *
 * @return
 *   A JSON object contains an error message for the client informing that
 *   a "LEAVE" package was ill-formed is returned.
 */
JSON MakeNotValidLeave() noexcept {
  return JSON({
    {"closed", true},
    {"type", "error"},
    {"message", "A \"LEAVE\" was ill-formed."}
    }, false, JSON::value_t::object);
}

/**
 * This method returns a JSON object contains an error message for the client
 * informing that a package was unidentified.
 *
 * @return
 *   A JSON object contains an error message for the client informing that
 *   package was unidentified is returned.
 */
JSON MakeUnidentified() noexcept {
  return JSON({
    {"closed", true},
    {"type", "error"},
    {"message", "Cannot identify a package."}
    }, false, JSON::value_t::object);
}

}  // namespace

std::pair<bool, JSON>  Verify(const std::string& raw_package) noexcept {
  auto parsed = Parse(raw_package.begin(), raw_package.end());
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


  // Received an undefined package.
  return std::make_pair(false, MakeUnidentified());
}

}  // namespace fusion_server::json