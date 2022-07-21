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
    newListener: [event: keyof T];
    removeListener: [event: keyof T];
}

declare module './bindings' {

    /* eslint-disable @typescript-eslint/no-empty-interface */
    interface Device extends ExtendedDevice { }

    interface DeviceEvents extends EventListeners<DeviceEvents> {
        attach: [device: Device];
        detach: [device: Device];
        attachIds: [vendorId: number | null, productId: number | null]
        detachIds: [vendorId: number | null, productId: number | null]
    }

    function addListener<K extends keyof DeviceEvents>(event: K, listener: (...args: DeviceEvents[K]) => void): void;
    function removeListener<K extends keyof DeviceEvents>(event: K, listener: (...args: DeviceEvents[K]) => void): void;
    function on<K extends keyof DeviceEvents>(event: K, listener: (...args: DeviceEvents[K]) => void): void;
    function off<K extends keyof DeviceEvents>(event: K, listener: (...args: DeviceEvents[K]) => void): void;
    function once<K extends keyof DeviceEvents>(event: K, listener: (...args: DeviceEvents[K]) => void): void;
    function listeners<K extends keyof DeviceEvents>(event: K): ((...args: DeviceEvents[K]) => void)[];
    function rawListeners<K extends keyof DeviceEvents>(event: K): ((...args: DeviceEvents[K]) => void)[];
    function removeAllListeners<K extends keyof DeviceEvents>(event?: K): void;
    function emit<K extends keyof DeviceEvents>(event: K, ...args: DeviceEvents[K]): boolean;
    function listenerCount<K extends keyof DeviceEvents>(event: K): number;
}

// Polling mechanism for discovering device changes where hotplug detection is not available
const pollTimeout = 500;
const hotplugSupported = usb._supportedHotplugEvents();
let pollingHotplug = false;
let pollDevices = new Set<usb.Device>();

const pollHotplugOnce = (start: boolean) => {
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
};

const pollHotplugLoop = (start = false) => {
    if (start) {
        pollingHotplug = true;
    } else if (!pollingHotplug) {
        return;
    }

    pollHotplugOnce(start);

    setTimeout(() => {
        pollHotplugLoop();
    }, pollTimeout);
};

const hotplugModeIsIdsOnly = hotplugSupported === 2;
if (hotplugModeIsIdsOnly) {
    // The hotplug backend doesnt emit 'attach' or 'detach', so we need to do some conversion
    const hotplugEventConversion = () => {
        // Future: This might want a debounce, to avoid doing multiple polls when attaching a usb hub or something
        pollHotplugOnce(false);
    };

    usb.on('attachIds', hotplugEventConversion);
    usb.on('detachIds', hotplugEventConversion);
}

usb.on('newListener', event => {
    if (event !== 'attach' && event !== 'detach') {
        return;
    }
    const listenerCount = usb.listenerCount('attach') + usb.listenerCount('detach');
    if (listenerCount === 0) {
        if (hotplugSupported) {
            usb._enableHotplugEvents();
        } else {
            pollHotplugLoop(true);
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
