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

interface DeviceIds {
    idVendor: number;
    idProduct: number;
}

declare module './bindings' {

    /* eslint-disable @typescript-eslint/no-empty-interface */
    interface Device extends ExtendedDevice { }

    interface DeviceEvents extends EventListeners<DeviceEvents> {
        attach: Device;
        detach: Device;
        attachIds: DeviceIds;
        detachIds: DeviceIds;
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

// Hotplug support
const hotplugSupported = usb._supportedHotplugEvents();

// Devices delta support for non-libusb hotplug events
// This methd needs to be used for attach/detach IDs (hotplugSupportType === 2) rather than a lookup because vid/pid are not unique
let hotPlugDevices = new Set<usb.Device>();
const emitHotplugEvents = () => {
    // Collect current devices
    const devices = new Set(usb.getDeviceList());

    // Find attached devices
    for (const device of devices) {
        if (!hotPlugDevices.has(device)) {
            usb.emit('attach', device);
        }
    }

    // Find detached devices
    for (const device of hotPlugDevices) {
        if (!devices.has(device)) {
            usb.emit('detach', device);
        }
    }

    hotPlugDevices = devices;
};

// Polling mechanism for checking device changes where hotplug detection is not available
let pollingHotplug = false;
const pollHotplug = (start = false) => {
    if (start) {
        pollingHotplug = true;
    } else if (!pollingHotplug) {
        return;
    } else {
        emitHotplugEvents();
    }

    setTimeout(() => pollHotplug(), 500);
};

// Hotplug control
const startHotplug = () => {
    if (hotplugSupported !== 1) {
        // Collect initial devices when not using libusb
        hotPlugDevices = new Set(usb.getDeviceList());
    }

    if (hotplugSupported) {
        // Use hotplug event emitters
        usb._enableHotplugEvents();

        if (hotplugSupported === 2) {
            // Use hotplug ID events to trigger a change check
            usb.on('attachIds', emitHotplugEvents);
            usb.on('detachIds', emitHotplugEvents);
        }
    } else {
        // Fallback to using polling to check for changes
        pollHotplug(true);
    }
};

const stopHotplug = () => {
    if (hotplugSupported) {
        // Disable hotplug events
        usb._disableHotplugEvents();

        if (hotplugSupported === 2) {
            // Remove hotplug ID event listeners
            usb.off('attachIds', emitHotplugEvents);
            usb.off('detachIds', emitHotplugEvents);
        }
    } else {
        // Stop polling
        pollingHotplug = false;
    }
};

usb.on('newListener', event => {
    if (event !== 'attach' && event !== 'detach') {
        return;
    }
    const listenerCount = usb.listenerCount('attach') + usb.listenerCount('detach');
    if (listenerCount === 0) {
        startHotplug();
    }
});

usb.on('removeListener', event => {
    if (event !== 'attach' && event !== 'detach') {
        return;
    }
    const listenerCount = usb.listenerCount('attach') + usb.listenerCount('detach');
    if (listenerCount === 0) {
        stopHotplug();
    }
});

export = usb;
