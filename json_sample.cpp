#include <iostream>
#include <cassert>

#include "jsoncpp/json/json.h"


/*
  icfpc2017$ g++ json_sample.cpp jsoncpp/jsoncpp.cpp
*/

int main() {
  Json::Value json;
  Json::Reader reader;

  // number
  assert(reader.parse("1", json));
  std::cout << json << std::endl;

  assert(json.isInt() && json.asInt() == 1);

  
  // object
  assert(reader.parse("{\"a\": 2}", json));
  std::cout << json << std::endl;

  assert(json.isObject());
  assert(json.isMember("a"));
  assert(json["a"].isInt());
  assert(json["a"].asInt() == 2);


  // array, string
  assert(reader.parse("[\"a\", \"b\"]", json));
  std::cout << json << std::endl;

  assert(json.isArray());
  assert(json.size() == 2);
  assert(json[0].isString());
  assert(json[0].asString() == "a");
  assert(json[1].isString());
  assert(json[1].asString() == "b");
}
