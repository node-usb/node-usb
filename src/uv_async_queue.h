#ifndef SRC_UV_ASYNC_QUEUE_H
#define SRC_UV_ASYNC_QUEUE_H

#include <uv.h>
#include <functional>
#include <queue>
#include <pthread.h>

template <class T>
class UVQueue{
	public:
		UVQueue(std::function<void(T&)> cb, bool _keep_alive=false): callback(cb), keep_alive(_keep_alive) {
			uv_async_init(uv_default_loop(), &async, UVQueue::internal_callback);
			pthread_mutex_init(&mutex, NULL);
			async.data = this;
			if (!keep_alive) uv_unref(uv_default_loop());
		}
		
		void post(T value){
			pthread_mutex_lock(&mutex);
			queue.push(value);
			pthread_mutex_unlock(&mutex);
			uv_async_send(&async);
		}
		
		~UVQueue(){
			if (!keep_alive) uv_ref(uv_default_loop());
			uv_close((uv_handle_t*)&async, NULL); //TODO: maybe we can't delete UVQueue until callback?
			pthread_mutex_destroy(&mutex);
		}
		
	private:
		std::function<void(T&)> callback;
		std::queue<T> queue;
		pthread_mutex_t mutex;
		uv_async_t async;
		bool keep_alive;
		
		static void internal_callback(uv_async_t *handle, int status){
			UVQueue* uvqueue = static_cast<UVQueue*>(handle->data);
			
			while(1){
				pthread_mutex_lock(&uvqueue->mutex);
				if (uvqueue->queue.empty()){
					pthread_mutex_unlock(&uvqueue->mutex);
					break;
				}
				T item = uvqueue->queue.front();
				uvqueue->queue.pop();
				pthread_mutex_unlock(&uvqueue->mutex);
				uvqueue->callback(item);
			}
		}
};

#endif
