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
        attachIds: undefined;
        detachIds: undefined;
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
const hotplugSupportType = usb._supportedHotplugEvents();

// Devices delta support for non-libusb hotplug events
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
    if (hotplugSupportType === 1) {
        // Use libusb event emitters
        usb._enableHotplugEvents();
    } else {
        // Collect initial devices
        hotPlugDevices = new Set(usb.getDeviceList());

        if (hotplugSupportType === 2) {
            // Use hotplug ID events to trigger a change check
            usb.on('attachIds', emitHotplugEvents);
            usb.on('detachIds', emitHotplugEvents);
        } else {
            // Fallback to using polling to check for changes
            pollHotplug(true);
        }
    }
};

const stopHotplug = () => {
    if (hotplugSupportType === 1) {
        // Disable libusb events
        usb._disableHotplugEvents();
    } else {
        if (hotplugSupportType === 2) {
            // Remove hotplug ID event listeners
            usb.off('attachIds', emitHotplugEvents);
            usb.off('detachIds', emitHotplugEvents);
        } else {
            // Stop polling
            pollingHotplug = false;
        }
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
