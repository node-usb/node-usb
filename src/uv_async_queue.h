#ifndef SRC_UV_ASYNC_QUEUE_H
#define SRC_UV_ASYNC_QUEUE_H

#include <uv.h>
#include <node_version.h>
#include <queue>
#include "polyfill.h"

template <class T>
class UVQueue{
	public:
		typedef void (*fptr)(T);

		UVQueue(fptr cb, int _ref_count=0): callback(cb), ref_count(_ref_count) {
			uv_mutex_init(&mutex);
			uv_async_init(uv_default_loop(), &async, UVQueue::internal_callback);
			async.data = this;
			if (ref_count < 1) {
				uv_unref((uv_handle_t*)&async);
			}
		}
		
		void post(T value){
			uv_mutex_lock(&mutex);
			queue.push(value);
			uv_mutex_unlock(&mutex);
			uv_async_send(&async);
		}
		
		~UVQueue(){
			uv_mutex_destroy(&mutex);
			uv_close((uv_handle_t*)&async, NULL); //TODO: maybe we can't delete UVQueue until callback?
		}

		void ref(){
			ref_count++;
			if (ref_count == 1) {
				uv_ref((uv_handle_t*)&async);
			}
		}

		void unref(){
			ref_count--;
			if (ref_count == 0) {
				uv_unref((uv_handle_t*)&async);
			}
		}
		
	private:
		fptr callback;
		std::queue<T> queue;
		uv_mutex_t mutex;
		uv_async_t async;
		int ref_count;
		
		static UV_ASYNC_CB(internal_callback){
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
