#ifndef SRC_NODE_USB_H
#define SRC_NODE_USB_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <iostream>

#include <cstring>
#include <string>
#include <cstdlib>

#include <libusb-1.0/libusb.h>
#include <v8.h>

#include <node.h>
#include <node_version.h>
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
