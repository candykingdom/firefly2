#ifndef LIB_DEVICE_DEVICE_CATALOG_HPP_
#define LIB_DEVICE_DEVICE_CATALOG_HPP_

#include <DeviceDescription.hpp>
#include <vector>

struct NamedDeviceDescription {
  const char* name;
  const DeviceDescription* description;
};

namespace DeviceCatalog {

const std::vector<NamedDeviceDescription>& All();

const NamedDeviceDescription* Find(const char* name);

}  // namespace DeviceCatalog

#endif  // LIB_DEVICE_DEVICE_CATALOG_HPP_
