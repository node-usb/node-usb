#ifndef SRC_NODE_USB_H
#define SRC_NODE_USB_H

#include <assert.h>
#include <string>

#include <libusb-1.0/libusb.h>
#include <v8.h>

#include <node.h>
#include <node_buffer.h>
#include <uv.h>

using namespace v8;
using namespace node;

#include "protobuilder.h"
#include "device.h"
#include "transfer.h"

extern Proto<Device> pDevice;
extern Proto<Transfer> pTransfer;

#endif
