#ifndef SRC_INTERFACE_H_
#define SRC_INTERFACE_H

#include "bindings.h"

namespace NodeUsb {
	class Interface : public ObjectWrap {
		public:
			static void Initalize(Handle<Object> target);
			static Persistent<FunctionTemplate> constructor_template;
			Interface(nodeusb_device_container*, const libusb_interface_descriptor*, uint32_t idx_interface, uint32_t idx_alt_setting);
			~Interface();

		protected:
			// members
			struct nodeusb_device_container *device_container;
			const struct libusb_interface_descriptor *descriptor;
			uint32_t idx_interface;
			uint32_t idx_alt_setting;

			// V8 getter
			static Handle<Value> IdxInterfaceGetter(Local<String> property, const AccessorInfo &info);
			static Handle<Value> IdxAltSettingGetter(Local<String> property, const AccessorInfo &info);

			// exposed to V8
			static Handle<Value> New(const Arguments& args);
			static Handle<Value> IsKernelDriverActive(const Arguments& args);
			static Handle<Value> DetachKernelDriver(const Arguments& args);
			static Handle<Value> AttachKernelDriver(const Arguments& args);
			static Handle<Value> Claim(const Arguments& args);
			static Handle<Value> GetExtraData(const Arguments& args);
			static Handle<Value> GetEndpoints(const Arguments& args);

			struct release_request:device_handle_request {
				int interface_number;
			};
			static Handle<Value> Release(const Arguments& args);
			static void EIO_Release(uv_work_t *req);
			static void EIO_After_Release(uv_work_t *req);
			
			struct alternate_setting_request:release_request {
				int alternate_setting;
			};
			static Handle<Value> AlternateSetting(const Arguments& args);
			static void EIO_AlternateSetting(uv_work_t *req);
			static void EIO_After_AlternateSetting(uv_work_t *req);
	};
}

#endif
