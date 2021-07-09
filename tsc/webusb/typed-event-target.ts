import { EventEmitter } from 'events';

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
