

#include "principal.h"
#include "image.h"
#include "accessor_object.h"
#include "cpprest/json.h"
#include "utils.h"
#include <ios>
#include <string>

namespace latte {

Principal Principal::parse(const std::string &data) {
  return from_json(web::json::value::parse(data));
}

Principal Principal::from_json(web::json::value p) {
  return Principal(utils::atoi<uint64_t>(p["id"].as_string()),
        utils::atoi<uint32_t>(p["lo"].as_string()), utils::atoi<uint32_t>(p["hi"].as_string()),
        p["image"].as_string(), p["configs"].as_string(), p["bearer"].as_string());
}

web::json::value Principal::to_json() const {
  web::json::value v;
  v["id"] = web::json::value::string(utils::itoa<uint64_t, utils::HexType>(this->id()));
  v["lo"] = web::json::value::string(utils::itoa(this->lo()));
  v["hi"] = web::json::value::string(utils::itoa(this->hi()));
  v["image"] = web::json::value::string(this->image());
  v["configs"] = web::json::value::string(this->configs());
  v["bearer"] = web::json::value::string(this->bearer());
  return v;
}

Image Image::parse(const std::string &data) {
  return from_json(web::json::value::parse(data));
}

Image Image::from_json(web::json::value&& p) {
  return Image(p["hash"].as_string(), p["url"].as_string(),
      p["revision"].as_string(), p["configs"].as_string());
}

web::json::value Image::to_json() const {
  web::json::value v;
  v["hash"] = web::json::value::string(this->hash());
  v["url"] = web::json::value::string(this->url());
  v["revision"] = web::json::value::string(this->revision());
  v["configs"] = web::json::value::string(this->configs());
  return v;
}


web::json::value AccessorObject::to_json() const {
  web::json::value v;
  v["name"] = web::json::value::string(this->name());
  auto acls = this->acls();
  auto converted_acls = web::json::value::array();
  size_t i = 0;
  for (auto acl = acls.cbegin(); acl != acls.cend(); ++acl) {
    converted_acls[i++] = web::json::value::string(*acl);
  }
  v["acls"] = converted_acls;
  return v;
}

AccessorObject AccessorObject::parse(const std::string &data) {
  return from_json(web::json::value::parse(data));
}

AccessorObject AccessorObject::from_json(web::json::value&& p) {
  auto acls = p["acls"].as_array();
  AccessorObject o (p["name"].as_string());
  for (auto acl = acls.cbegin(); acl != acls.cend(); ++acl) {
    o.add_acl(acl->as_string());
  }
  return o;
}


}
