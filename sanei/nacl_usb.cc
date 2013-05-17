// copyright...

#include <stdio.h>
#include <string>

#include <libusb.h>
#include "ppapi/cpp/module.h"

#include "sane/nacl_jscall.h"

extern scanley::SynchronousJavaScriptCaller* g_js_caller;

using std::string;

#define ENTRY fprintf(stderr, "%s entered\n", __func__)

struct libusb_device {
  int js_id;

  uint16_t vendor_id;
  uint16_t product_id;
};

int libusb_init(libusb_context **ctx) {
  ENTRY;
  return 0;
}

void libusb_set_debug(libusb_context *ctx, int level) {
  ENTRY;
}

void libusb_exit(libusb_context *ctx) {
  ENTRY;
}

ssize_t LIBUSB_CALL libusb_get_device_list(libusb_context *ctx,
                                           libusb_device ***list) {
  ENTRY;
  if (!g_js_caller) {
    fprintf(stderr, "no JS caller\n");
    return 0;
  }
  string result = g_js_caller->Call("USB:FIND_DEVICES");
  fprintf(stderr, "get devices, got js reply: %s\n", result.c_str());

  return 0;
}
void LIBUSB_CALL libusb_free_device_list(libusb_device **list,
                                         int unref_devices) {
  ENTRY;
}

uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device *dev) {
  ENTRY;
  return 0;
}
uint8_t LIBUSB_CALL libusb_get_device_address(libusb_device *dev) {
  ENTRY;
  return 0;
}
int LIBUSB_CALL libusb_get_device_descriptor(libusb_device *dev,
                                             struct libusb_device_descriptor *desc) {
  ENTRY;
  return 0;
}
int LIBUSB_CALL libusb_open(libusb_device *dev, libusb_device_handle **handle) {
  ENTRY;
  return 0;
}
void LIBUSB_CALL libusb_close(libusb_device_handle *dev_handle) {
  ENTRY;
}
int LIBUSB_CALL libusb_get_config_descriptor(libusb_device *dev,
                                             uint8_t config_index, struct libusb_config_descriptor **config) {
  ENTRY;
  return 0;
}
void LIBUSB_CALL libusb_free_config_descriptor(
    struct libusb_config_descriptor *config) {
  ENTRY;
}
libusb_device * LIBUSB_CALL libusb_ref_device(libusb_device *dev) {
  ENTRY;
  return dev;
}

int LIBUSB_CALL libusb_claim_interface(libusb_device_handle *dev,
                                       int interface_number) {
  ENTRY;
  return 0;
}
int LIBUSB_CALL libusb_release_interface(libusb_device_handle *dev,
                                         int interface_number) {
  ENTRY;
  return 0;
}
int LIBUSB_CALL libusb_clear_halt(libusb_device_handle *dev,
                                  unsigned char endpoint) {
  ENTRY;
  return 0;
}
int LIBUSB_CALL libusb_reset_device(libusb_device_handle *dev) {
  ENTRY;
  return 0;
}
int LIBUSB_CALL libusb_bulk_transfer(libusb_device_handle *dev_handle,
                                     unsigned char endpoint, unsigned char *data, int length,
                                     int *actual_length, unsigned int timeout) {
  ENTRY;
  return 0;
}
int LIBUSB_CALL libusb_set_interface_alt_setting(libusb_device_handle *dev,
                                                 int interface_number, int alternate_setting) {
  ENTRY;
  return 0;
}

int LIBUSB_CALL libusb_get_configuration(libusb_device_handle *dev,
                                         int *config) {
  ENTRY;
  return 0;
}

int LIBUSB_CALL libusb_set_configuration(libusb_device_handle *dev,
                                         int configuration) {
  ENTRY;
  return 0;
}
int LIBUSB_CALL libusb_control_transfer(libusb_device_handle *dev_handle,
                                        uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                                        unsigned char *data, uint16_t wLength, unsigned int timeout) {
  ENTRY;
  return 0;
}

int LIBUSB_CALL libusb_interrupt_transfer(libusb_device_handle *dev_handle,
                                          unsigned char endpoint, unsigned char *data, int length,
                                          int *actual_length, unsigned int timeout) {
  ENTRY;
  return 0;
}

scanley::SynchronousJavaScriptCaller* g_js_caller;
