/**
 * @file logger_manager_test.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the unit tests for the LoggerManager class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <gtest/gtest.h>

#include <fusion_server/http_session.hpp>
#include <fusion_server/json.hpp>

using namespace fusion_server;

TEST(LoggerManagerTest, ConfigureTestValid) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
    {"root", "/path/to/root"},
    {"extension", ".log"},
    {"level", "trace"},
    {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
    {"register_by_default", true},
    {"flush_every", 0.1}
    }, false, json::JSON::value_t::object);

  // Act
  auto success = logger_manager.Configure(json);

  // Assert
  EXPECT_TRUE(success);
}

TEST(LoggerManagerTest, ConfigureTestRoot) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
    {"root", "/path/to/root"},
    {"extension", ".log"},
    {"level", "trace"},
    {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
    {"register_by_default", true},
    {"flush_every", 0.1}
 }, false, json::JSON::value_t::object);

  auto root_is_object = decltype(json)(json);
  root_is_object["root"] = json::JSON::object({{"path", "value"}});
  auto root_is_array = decltype(json)(json);
  root_is_array["root"] = json::JSON::array({"path"});
  auto root_is_number = decltype(json)(json);
  root_is_number["root"] = 1337;
  auto root_is_boolean = decltype(json)(json);
  root_is_boolean["root"] = false;
  auto root_is_null = decltype(json)(json);
  root_is_null["root"] = nullptr;

  // Act

  // Assert
  EXPECT_FALSE(logger_manager.Configure(root_is_object));
  EXPECT_FALSE(logger_manager.Configure(root_is_array));
  EXPECT_FALSE(logger_manager.Configure(root_is_number));
  EXPECT_FALSE(logger_manager.Configure(root_is_boolean));
  EXPECT_FALSE(logger_manager.Configure(root_is_null));
}

TEST(LoggerManagerTest, ConfigureTestExtension) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"extension", ".log"},
                           {"level", "trace"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"register_by_default", true},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);

  auto extension_is_object = decltype(json)(json);
  extension_is_object["extension"] = json::JSON::object({{"path", "value"}});
  auto extension_is_array = decltype(json)(json);
  extension_is_array["extension"] = json::JSON::array({"path"});
  auto extension_is_number = decltype(json)(json);
  extension_is_number["extension"] = 1337;
  auto extension_is_boolean = decltype(json)(json);
  extension_is_boolean["extension"] = false;
  auto extension_is_null = decltype(json)(json);
  extension_is_null["extension"] = nullptr;

  // Act

  // Assert
  EXPECT_FALSE(logger_manager.Configure(extension_is_object));
  EXPECT_FALSE(logger_manager.Configure(extension_is_array));
  EXPECT_FALSE(logger_manager.Configure(extension_is_number));
  EXPECT_FALSE(logger_manager.Configure(extension_is_boolean));
  EXPECT_FALSE(logger_manager.Configure(extension_is_null));
}

TEST(LoggerManagerTest, ConfigureTestLevel) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"extension", ".log"},
                           {"level", "trace"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"register_by_default", true},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);

  auto trace = decltype(json)(json);
  trace["level"] = "trace";
  auto debug = decltype(json)(json);
  debug["level"] = "debug";
  auto info = decltype(json)(json);
  info["level"] = "info";
  auto warn = decltype(json)(json);
  warn["level"] = "warn";
  auto error = decltype(json)(json);
  error["level"] = "error";
  auto critical = decltype(json)(json);
  critical["level"] = "critical";
  auto none = decltype(json)(json);
  none["level"] = "none";

  auto not_valid = decltype(json)(json);
  not_valid["level"] = "foo";

  // Act

  // Assert
  EXPECT_TRUE(logger_manager.Configure(trace));
  EXPECT_TRUE(logger_manager.Configure(debug));
  EXPECT_TRUE(logger_manager.Configure(info));
  EXPECT_TRUE(logger_manager.Configure(warn));
  EXPECT_TRUE(logger_manager.Configure(error));
  EXPECT_TRUE(logger_manager.Configure(critical));
  EXPECT_TRUE(logger_manager.Configure(none));

  EXPECT_FALSE(logger_manager.Configure(not_valid));
}

TEST(LoggerManagerTest, ConfigureTestForPattern) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"extension", ".log"},
                           {"level", "trace"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"register_by_default", true},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);

  auto pattern_is_object = decltype(json)(json);
  pattern_is_object["pattern"] = json::JSON::object({{"path", "value"}});
  auto pattern_is_array = decltype(json)(json);
  pattern_is_array["pattern"] = json::JSON::array({"path"});
  auto pattern_is_number = decltype(json)(json);
  pattern_is_number["pattern"] = 1337;
  auto pattern_is_boolean = decltype(json)(json);
  pattern_is_boolean["pattern"] = false;
  auto pattern_is_null = decltype(json)(json);
  pattern_is_null["pattern"] = nullptr;

  // Act

  // Assert
  EXPECT_FALSE(logger_manager.Configure(pattern_is_object));
  EXPECT_FALSE(logger_manager.Configure(pattern_is_array));
  EXPECT_FALSE(logger_manager.Configure(pattern_is_number));
  EXPECT_FALSE(logger_manager.Configure(pattern_is_boolean));
  EXPECT_FALSE(logger_manager.Configure(pattern_is_null));
}

TEST(LoggerManagerTest, ConfigureTestRegisterByDefault) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"extension", ".log"},
                           {"level", "trace"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"register_by_default", true},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);

  auto register_is_string = decltype(json)(json);
  register_is_string["register_by_default"] = "foo";
  auto register_is_object = decltype(json)(json);
  register_is_object["register_by_default"] = json::JSON::object({{"path", "value"}});
  auto register_is_array = decltype(json)(json);
  register_is_array["register_by_default"] = json::JSON::array({"path"});
  auto register_is_number = decltype(json)(json);
  register_is_number["register_by_default"] = 1337;
  auto register_is_null = decltype(json)(json);
  register_is_null["register_by_default"] = nullptr;

  // Act

  // Assert
  EXPECT_FALSE(logger_manager.Configure(register_is_string));
  EXPECT_FALSE(logger_manager.Configure(register_is_object));
  EXPECT_FALSE(logger_manager.Configure(register_is_array));
  EXPECT_FALSE(logger_manager.Configure(register_is_number));
  EXPECT_FALSE(logger_manager.Configure(register_is_null));
}

TEST(LoggerManagerTest, ConfigureTestFlushEvery) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"extension", ".log"},
                           {"level", "trace"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"register_by_default", true},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);

  auto flush_is_string = decltype(json)(json);
  flush_is_string["flush_every"] = "foo";
  auto flush_is_object = decltype(json)(json);
  flush_is_object["flush_every"] = json::JSON::object({{"path", "value"}});
  auto flush_is_array = decltype(json)(json);
  flush_is_array["flush_every"] = json::JSON::array({"path"});
  auto flush_is_boolean = decltype(json)(json);
  flush_is_boolean["flush_every"] = true;
  auto flush_is_null = decltype(json)(json);
  flush_is_null["flush_every"] = nullptr;

  auto flush_int = decltype(json)(json);
  flush_int["flush_every"] = 1;
  auto flush_negative = decltype(json)(json);
  flush_negative["flush_every"] = -1;

  // Act

  // Assert
  EXPECT_FALSE(logger_manager.Configure(flush_is_string));
  EXPECT_FALSE(logger_manager.Configure(flush_is_object));
  EXPECT_FALSE(logger_manager.Configure(flush_is_array));
  EXPECT_FALSE(logger_manager.Configure(flush_is_boolean));
  EXPECT_FALSE(logger_manager.Configure(flush_is_null));

  EXPECT_TRUE(logger_manager.Configure(flush_int));
  EXPECT_FALSE(logger_manager.Configure(flush_negative));
}

TEST(LoggerManagerTest, ConfigureMissingRoot) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"extension", ".log"},
                           {"level", "trace"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"register_by_default", true},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);

  // Act
  auto ret = logger_manager.Configure(json);

  // Assert
  EXPECT_TRUE(ret);
}

TEST(LoggerManagerTest, ConfigureMissingExtension) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"level", "trace"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"register_by_default", true},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);

  // Act
  auto ret = logger_manager.Configure(json);

  // Assert
  EXPECT_TRUE(ret);
}

TEST(LoggerManagerTest, ConfigureMissingLevel) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"extension", ".log"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"register_by_default", true},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);

  // Act
  auto ret = logger_manager.Configure(json);

  // Assert
  EXPECT_TRUE(ret);
}

TEST(LoggerManagerTest, ConfigureMissingPattern) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"extension", ".log"},
                           {"level", "trace"},
                           {"register_by_default", true},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);

  // Act
  auto ret = logger_manager.Configure(json);

  // Assert
  EXPECT_TRUE(ret);
}

TEST(LoggerManagerTest, ConfigureMissingRegisterByDefault) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"extension", ".log"},
                           {"level", "trace"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"flush_every", 0.1}
                         }, false, json::JSON::value_t::object);



  // Act
  auto ret = logger_manager.Configure(json);

  // Assert
  EXPECT_TRUE(ret);
}

TEST(LoggerManagerTest, ConfigureMissingFlushEvery) {
  // Arrange
  LoggerManager logger_manager;
  auto json = json::JSON({
                           {"root", "/path/to/root"},
                           {"extension", ".log"},
                           {"level", "trace"},
                           {"pattern", "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"},
                           {"register_by_default", true}
                         }, false, json::JSON::value_t::object);

  // Act
  auto ret = logger_manager.Configure(json);

  // Assert
  EXPECT_TRUE(ret);
}
