#ifndef SRC_NODE_USB_H
#define SRC_NODE_USB_H

#include <libusb.h>
#include <v8.h>
#include <string.h>
#include <errno.h>
#include <iostream>

#include <stdio.h>
#include <unistd.h>

#include <node.h>
#include <node_version.h>
#include <node_buffer.h>
#include <uv.h>
#include <uv-private/ev.h>
#include <cstring>
#include <string>
#include <cstdlib>

#define NODE_USB_VERSION "0.1"

#ifndef NODE_USB_REVISION
  #define NODE_USB_REVISION "unknown"
#endif

using namespace v8;
using namespace node;

namespace NodeUsb {
}

#endif
