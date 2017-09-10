

#include "principal.h"
#include "image.h"
#include "accessor_object.h"
#include "cpprest/json.h"
#include <ios>
#include <string>
#include <iomanip>

namespace latte {

typedef struct {} DecType;
typedef struct {} HexType;
typedef decltype(std::dec) ModeType;

ModeType&& mode(DecType) {
  return std::dec;
}

ModeType&& mode(HexType) {
  return std::hex;
}

template<typename I, typename BaseType=DecType>
std::string itoa(I&& i) {
  std::stringstream ss;
  ss.exceptions(std::stringstream::failbit);
  ss << mode(BaseType()) << std::showbase << i;
  return ss.str();
}


template<typename I>
I atoi(const std::string& s) {
  std::stringstream ss(s);
  ss.exceptions(std::stringstream::failbit);
  I result;
  ss >> std::setbase(0) >> result;
  return result;
}

Principal Principal::parse(const std::string &data) {
  auto p = web::json::value::parse(data);
  return Principal(atoi<uint64_t>(p["id"].as_string()),
        atoi<int>(p["lo"].as_string()), atoi<int>(p["hi"].as_string()),
        p["image"].as_string(), p["configs"].as_string());
}

web::json::value Principal::to_json() const {
  web::json::value v;
  v["id"] = web::json::value::string(itoa<uint64_t, HexType>(this->id()));
  v["lo"] = web::json::value::string(itoa(this->lo()));
  v["hi"] = web::json::value::string(itoa(this->hi()));
  v["image"] = web::json::value::string(this->image());
  v["configs"] = web::json::value::string(this->configs());
  return v;
}

Image Image::parse(const std::string &data) {
  auto p = web::json::value::parse(data);
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
  auto p = web::json::value::parse(data);
  auto acls = p["acls"].as_array();
  AccessorObject o (p["name"].as_string());
  for (auto acl = acls.cbegin(); acl != acls.cend(); ++acl) {
    o.add_acl(acl->as_string());
  }
  return o;
}


}
