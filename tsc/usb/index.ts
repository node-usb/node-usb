import { EventEmitter } from 'events';
import { ExtendedDevice } from './device';
import * as usb from './bindings';

if (usb.INIT_ERROR) {
    /* eslint-disable no-console */
    console.warn('Failed to initialize libusb.');
}

Object.setPrototypeOf(usb, EventEmitter.prototype);

Object.getOwnPropertyNames(ExtendedDevice.prototype).forEach(name => {
    Object.defineProperty(usb.Device.prototype, name, Object.getOwnPropertyDescriptor(ExtendedDevice.prototype, name) || Object.create(null));
});

interface EventListeners<T> {
    newListener: keyof T;
    removeListener: keyof T;
}

declare module './bindings' {

    /* eslint-disable @typescript-eslint/no-empty-interface */
    interface Device extends ExtendedDevice { }

    interface DeviceEvents extends EventListeners<DeviceEvents> {
        attach: Device;
        detach: Device;
    }

    function addListener<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
    function removeListener<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
    function on<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
    function off<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
    function once<K extends keyof DeviceEvents>(event: K, listener: (arg: DeviceEvents[K]) => void): void;
    function listeners<K extends keyof DeviceEvents>(event: K): ((arg: DeviceEvents[K]) => void)[];
    function rawListeners<K extends keyof DeviceEvents>(event: K): ((arg: DeviceEvents[K]) => void)[];
    function removeAllListeners<K extends keyof DeviceEvents>(event?: K): void;
    function emit<K extends keyof DeviceEvents>(event: K, arg: DeviceEvents[K]): boolean;
    function listenerCount<K extends keyof DeviceEvents>(event: K): number;
}

// Polling mechanism for discovering device changes until this is fixed:
// https://github.com/libusb/libusb/issues/86
const pollTimeout = 500;
const hotplugSupported = usb._getLibusbCapability(usb.LIBUSB_CAP_HAS_HOTPLUG) > 0;
let pollingHotplug = false;
let pollDevices = new Set<usb.Device>();

const pollHotplug = (start = false) => {
    if (start) {
        pollingHotplug = true;
    } else if (!pollingHotplug) {
        return;
    }

    // Collect current devices
    const devices = new Set(usb.getDeviceList());

    if (!start) {
        // Find attached devices
        for (const device of devices) {
            if (!pollDevices.has(device))
                usb.emit('attach', device);
        }

        // Find detached devices
        for (const device of pollDevices) {
            if (!devices.has(device))
                usb.emit('detach', device);
        }
    }

    pollDevices = devices;
    setTimeout(() => {
        pollHotplug();
    }, pollTimeout);
};

usb.on('newListener', event => {
    if (event !== 'attach' && event !== 'detach') {
        return;
    }
    const listenerCount = usb.listenerCount('attach') + usb.listenerCount('detach');
    if (listenerCount === 0) {
        if (hotplugSupported) {
            usb._enableHotplugEvents();
        } else {
            pollHotplug(true);
        }
    }
});

usb.on('removeListener', event => {
    if (event !== 'attach' && event !== 'detach') {
        return;
    }
    const listenerCount = usb.listenerCount('attach') + usb.listenerCount('detach');
    if (listenerCount === 0) {
        if (hotplugSupported) {
            usb._disableHotplugEvents();
        } else {
            pollingHotplug = false;
        }
    }
});

export = usb;
