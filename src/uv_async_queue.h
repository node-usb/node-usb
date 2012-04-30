#include <uv.h>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>

template <class T>
class UVQueue{
	public:
		UVQueue(std::function<void(T&)> cb): callback(cb) {
			uv_async_init(uv_default_loop(), &async, UVQueue::internal_callback);
			async.data = this;
		}
		
		void post(T value){
			mutex.lock();
			queue.push(value);
			mutex.unlock();
			uv_async_send(&async);
		}
		
	private:
		std::function<void(T&)> callback;
		std::queue<T> queue;
		std::mutex mutex;
		uv_async_t async;
		
		static void internal_callback(uv_async_t *handle, int status){
			UVQueue* uvqueue = static_cast<UVQueue*>(handle->data);
			
			while(1){
				uvqueue->mutex.lock();
				if (uvqueue->queue.empty()){
					uvqueue->mutex.unlock();
					break;
				}
				T item = uvqueue->queue.front();
				uvqueue->queue.pop();
				uvqueue->mutex.unlock();
				uvqueue->callback(item);
			}
		}
};
