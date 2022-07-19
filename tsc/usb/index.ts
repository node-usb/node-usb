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

const pollOnce = (start: boolean) => {
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

const pollHotplug = (start = false) => {
    if (start) {
        pollingHotplug = true;
    } else if (!pollingHotplug) {
        return;
    }

    pollOnce(start);

    setTimeout(() => {
        pollHotplug();
    }, pollTimeout);
};


let hotplugEventConversionRunning = false;
const hotplugEventConversion = () => {
    // Future: This might want a debounce, to avoid doing multiple polls when attaching a usb hub or something
    pollOnce(false);
};

function checkDeviceListeners(event: string, isAdding: boolean): void {
    const isDeviceEvent = event === 'attach' || event === 'detach';
    const isIdEvent = event === 'attachIds' || event === 'detachIds';

    if (!isDeviceEvent && !isIdEvent) {
        return;
    }

    const deviceListenerCount = usb.listenerCount('attach') + usb.listenerCount('detach');
    const idsListenerCount = usb.listenerCount('attachIds') + usb.listenerCount('detachIds');

    const hotplugModeIsIdsOnly = hotplugSupported === 2;
    if (isDeviceEvent && deviceListenerCount === 0 && hotplugModeIsIdsOnly) {
        if (isAdding && !hotplugEventConversionRunning) {
            hotplugEventConversionRunning = true;
            pollOnce(true);

            usb.on('attachIds', hotplugEventConversion);
            usb.on('detachIds', hotplugEventConversion);
        } else if (!isAdding && hotplugEventConversionRunning) {
            hotplugEventConversionRunning = false;

            usb.off('attachIds', hotplugEventConversion);
            usb.off('detachIds', hotplugEventConversion);

            pollDevices.clear();
        }
    } else if (deviceListenerCount === 0 && idsListenerCount === 0) {
        if (isAdding) {
            if (hotplugSupported > 0) {
                usb._enableHotplugEvents();
            } else {
                pollHotplug(true);
            }
        } else {
            if (hotplugSupported > 0) {
                usb._disableHotplugEvents();
            } else {
                pollingHotplug = false;
            }

            pollDevices.clear();
        }
    }
}

usb.on('newListener', event => {
    checkDeviceListeners(event, true);
});

usb.on('removeListener', event => {
    checkDeviceListeners(event, false);
});

export = usb;
