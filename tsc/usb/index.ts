import { EventEmitter } from 'events';
import { ExtendedDevice } from './device';
import * as usb from './bindings';

if (usb.INIT_ERROR) {
    /* eslint-disable no-console */
    console.warn('Failed to initialize libusb.');
}

Object.setPrototypeOf(usb, EventEmitter.prototype);
Object.defineProperty(usb, 'pollHotplug', {
    value: false,
    writable: true
});

Object.defineProperty(usb, 'pollHotplugDelay', {
    value: 500,
    writable: true
});

// `usb.Device` is not defined when `usb.INIT_ERROR` is true
if (usb.Device) {
    Object.getOwnPropertyNames(ExtendedDevice.prototype).forEach(name => {
        Object.defineProperty(usb.Device.prototype, name, Object.getOwnPropertyDescriptor(ExtendedDevice.prototype, name) || Object.create(null));
    });
}

// Devices delta support for non-libusb hotplug events
let hotPlugDevices = new Set<usb.Device>();

// This method needs to be used for attach/detach IDs (hotplugSupportType === 2) rather than a lookup because vid/pid are not unique
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

    setTimeout(() => pollHotplug(), usb.pollHotplugDelay);
};

// Devices changed event handler
const devicesChanged = () => setTimeout(() => emitHotplugEvents(), usb.pollHotplugDelay);

// Hotplug control
let hotplugSupported = 0;
const startHotplug = () => {
    hotplugSupported = usb.pollHotplug ? 0 : usb._supportedHotplugEvents();

    if (hotplugSupported !== 1) {
        // Collect initial devices when not using libusb
        hotPlugDevices = new Set(usb.getDeviceList());
    }

    if (hotplugSupported) {
        // Use hotplug event emitters
        usb._enableHotplugEvents();

        if (hotplugSupported === 2) {
            // Use hotplug ID events to trigger a change check
            usb.on('attachIds', devicesChanged);
            usb.on('detachIds', devicesChanged);
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
            usb.off('attachIds', devicesChanged);
            usb.off('detachIds', devicesChanged);
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
