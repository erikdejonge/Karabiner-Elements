#pragma once

#include "DiagnosticMacros.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/hid/IOHIDDevice.h>
END_IOKIT_INCLUDE;

class org_pqrs_driver_VirtualHIDPointing final : public IOHIDDevice {
  OSDeclareDefaultStructors(org_pqrs_driver_VirtualHIDPointing);

public:
  virtual bool start(IOService* provider) override;

  virtual OSString* newManufacturerString() const override;
  virtual OSString* newProductString() const override;
  virtual OSNumber* newVendorIDNumber() const override;
  virtual OSNumber* newProductIDNumber() const override;
  virtual OSNumber* newPrimaryUsageNumber() const override;
  virtual OSNumber* newPrimaryUsagePageNumber() const override;
  virtual IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
  virtual OSString* newSerialNumberString() const override;
  virtual OSNumber* newLocationIDNumber() const override;
};
