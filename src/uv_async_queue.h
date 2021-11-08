#ifndef SRC_UV_ASYNC_QUEUE_H
#define SRC_UV_ASYNC_QUEUE_H

template <class T>
class UVQueue{
    public:
        typedef void (*fptr)(T);

        Napi::ThreadSafeFunction tsfn;

        UVQueue(fptr cb): callback(cb)  {}

        void start(Napi::Env env) {
            Napi::Function empty_func = Napi::Function::New(env, [](const Napi::CallbackInfo& cb) {});
            tsfn = Napi::ThreadSafeFunction::New(env, empty_func, "libusb", 0, 1);
        }

        void stop() {
            tsfn.Release();
        }
 
        void ref(Napi::Env env) {
            tsfn.Ref(env);
        }

        void unref(Napi::Env env) {
            tsfn.Unref(env);
        }

        void post(T value){
            auto cb = callback;
            tsfn.BlockingCall( value, [cb](Napi::Env _env, Napi::Function _jsCallback, T val) {
                cb(val);
            });
        }

    private:
        fptr callback;
};

#endif
