// what doesn't exist in nan exists here

#if (NODE_MODULE_VERSION > 0x000B)

#define EXTERNAL_NEW(x) External::New(Isolate::GetCurrent(), x)
#define UV_ASYNC_CB(x) void x(uv_async_t *handle)

#else

#define EXTERNAL_NEW(x) External::New(x)
#define UV_ASYNC_CB(x) void x(uv_async_t *handle, int status)

#endif