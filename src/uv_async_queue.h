#ifndef SRC_UV_ASYNC_QUEUE_H
#define SRC_UV_ASYNC_QUEUE_H

#include <uv.h>
#include <node_version.h>
#include <queue>

template <class T>
class UVQueue{
	public:
		typedef void (*fptr)(T);

		UVQueue(fptr cb, bool _keep_alive=false): callback(cb), keep_alive(_keep_alive) {
			uv_mutex_init(&mutex);
			uv_async_init(uv_default_loop(), &async, UVQueue::internal_callback);
			async.data = this;
			if (!keep_alive) unref();
		}
		
		void post(T value){
			uv_mutex_lock(&mutex);
			queue.push(value);
			uv_mutex_unlock(&mutex);
			uv_async_send(&async);
		}
		
		~UVQueue(){
			if (!keep_alive) ref();
			uv_mutex_destroy(&mutex);
			uv_close((uv_handle_t*)&async, NULL); //TODO: maybe we can't delete UVQueue until callback?
		}

		void ref(){
			#if NODE_VERSION_AT_LEAST(0, 7, 9)
			uv_ref((uv_handle_t*)&async);
			#else
			uv_ref(uv_default_loop());
			#endif
		}

		void unref(){
			#if NODE_VERSION_AT_LEAST(0, 7, 9)
			uv_unref((uv_handle_t*)&async);
			#else
			uv_unref(uv_default_loop());
			#endif
		}
		
	private:
		fptr callback;
		std::queue<T> queue;
		uv_mutex_t mutex;
		uv_async_t async;
		bool keep_alive;
		
		static void internal_callback(uv_async_t *handle, int status){
			UVQueue* uvqueue = static_cast<UVQueue*>(handle->data);
			
			while(1){
				uv_mutex_lock(&uvqueue->mutex);
				if (uvqueue->queue.empty()){
					uv_mutex_unlock(&uvqueue->mutex);
					break;
				}
				T item = uvqueue->queue.front();
				uvqueue->queue.pop();
				uv_mutex_unlock(&uvqueue->mutex);
				uvqueue->callback(item);
			}
		}
};

#endif
