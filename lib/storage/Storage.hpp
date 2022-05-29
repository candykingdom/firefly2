#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "../device/DeviceDescription.hpp"

/**
 * @brief Retrieves the DeviceDescription from storage and returns a boolean if reading was successful.
 * 
 * @param device A pointer to write the DeviceDescription into.
 * @return If retrieving the DeviceDescription was successful.
 */
bool GetDeviceDescription(const DeviceDescription* device);

/**
 * @brief Stores the DeviceDescription to storage and returns a boolean if reading was successful.
 * 
 * @param device A pointer to the DeviceDescription to store.
 * @return If storing the DeviceDescription was successful.
 */
bool PutDeviceDescription(const DeviceDescription* device);

#endif  // __STORAGE_H__
