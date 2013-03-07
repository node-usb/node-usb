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

Local<Value> libusbException(int errorno);

#define CHECK_USB(r) \
	if (r < LIBUSB_SUCCESS) { \
		ThrowException(libusbException(r)); \
		return scope.Close(Undefined());   \
	}

#define CALLBACK_ARG(CALLBACK_ARG_IDX) \
	Local<Function> callback; \
	if (args.Length() > (CALLBACK_ARG_IDX)) { \
		if (!args[CALLBACK_ARG_IDX]->IsFunction()) { \
			return ThrowException(Exception::TypeError( String::New("Argument " #CALLBACK_ARG_IDX " must be a function"))); \
		} \
		callback = Local<Function>::Cast(args[CALLBACK_ARG_IDX]); \
	} \

#ifdef ENABLE_DEBUG
  #define DEBUG_HEADER fprintf(stderr, "node-usb [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__); 
  #define DEBUG_FOOTER fprintf(stderr, "\n");
  #define DEBUG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#else
  #define DEBUG(...)
#endif

#endif
