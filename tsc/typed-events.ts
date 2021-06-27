import { EventEmitter } from 'events';

export declare interface EventListeners<T> {
    newListener: keyof T;
    removeListener: keyof T;
}

export interface TypedEventEmitter<T> {
    addListener<K extends keyof T>(event: K, listener: (arg: T[K]) => void): void;
    removeListener<K extends keyof T>(event: K, listener: (arg: T[K]) => void): void;
    on<K extends keyof T>(event: K, listener: (arg: T[K]) => void): void;
    off<K extends keyof T>(event: K, listener: (arg: T[K]) => void): void;
    once<K extends keyof T>(event: K, listener: (arg: T[K]) => void): void;
    listeners<K extends keyof T>(event: K): Function[];
    rawListeners<K extends keyof T>(event: K): Function[];
    removeAllListeners<K extends keyof T>(event?: K): void;
    emit<K extends keyof T>(event: K, arg: T[K]): boolean;
    listenerCount<K extends keyof T>(event: K): number;
}

export class TypedEventTarget<T extends { [key: string]: Event }> implements EventTarget {
    private emitter = new EventEmitter();

    public addEventListener(type: keyof T, listener: (event: T[keyof T]) => void): void;
    public addEventListener(type: string, listener: (event: Event) => void): void;
    public addEventListener(type: string, listener: (event: T[keyof T]) => void): void {
        this.emitter.addListener(type, listener);
    }

    public removeEventListener(type: keyof T, listener: (event: T[keyof T]) => void): void;
    public removeEventListener(type: string, listener: (event: Event) => void): void;
    public removeEventListener(type: string, listener: (event: T[keyof T]) => void): void {
        this.emitter.removeListener(type, listener);
    }
 
    public dispatchEvent(event: T[keyof T]): boolean;
    public dispatchEvent(event: Event): boolean;
    public dispatchEvent(event: T[keyof T]): boolean {
        return this.emitter.emit(event.type, event);
    }
}
