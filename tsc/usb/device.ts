import * as usb from './bindings';
import { Interface } from './interface';
import { Capability } from './capability';
import { BosDescriptor, ConfigDescriptor } from './descriptors';

const isBuffer = (obj: number | Uint8Array | undefined): obj is Uint8Array => !!obj && obj instanceof Uint8Array;
const DEFAULT_TIMEOUT = 1000;

export class ExtendedDevice {
    /**
     * List of Interface objects for the interfaces of the default configuration of the device.
     */
    public interfaces: Interface[] | undefined;

    private _timeout = DEFAULT_TIMEOUT;
    /**
     * Timeout in milliseconds to use for control transfers.
     */
    public get timeout(): number {
        return this._timeout || DEFAULT_TIMEOUT;
    }
    public set timeout(value: number) {
        this._timeout = value;
    }

    /**
     * Object with properties for the fields of the active configuration descriptor.
     */
    public get configDescriptor(): ConfigDescriptor | undefined {
        try {
            return (this as unknown as usb.Device).__getConfigDescriptor();
        } catch (e) {
            // Check descriptor exists
            if (e.errno === usb.LIBUSB_ERROR_NOT_FOUND) {
                return undefined;
            }
            throw e;
        }
    }

    /**
     * Contains all config descriptors of the device (same structure as .configDescriptor above)
     */
    public get allConfigDescriptors(): ConfigDescriptor[] {
        try {
            return (this as unknown as usb.Device).__getAllConfigDescriptors();
        } catch (e) {
            // Check descriptors exist
            if (e.errno === usb.LIBUSB_ERROR_NOT_FOUND) {
                return [];
            }
            throw e;
        }
    }

    /**
     * Contains the parent of the device, such as a hub. If there is no parent this property is set to `null`.
     */
    public get parent(): usb.Device {
        return (this as unknown as usb.Device).__getParent();
    }

    /**
     * Open the device.
     * @param defaultConfig
     */
    public open(this: usb.Device, defaultConfig = true): void {
        this.__open();
        if (defaultConfig === false) {
            return;
        }
        this.interfaces = [];
        const len = this.configDescriptor ? this.configDescriptor.interfaces.length : 0;
        for (let i = 0; i < len; i++) {
            this.interfaces[i] = new Interface(this, i);
        }
    }

    /**
     * Close the device.
     *
     * The device must be open to use this method.
     */
    public close(this: usb.Device): void {
        this.__close();
        this.interfaces = undefined;
    }

    /**
     * Set the device configuration to something other than the default (0). To use this, first call `.open(false)` (which tells it not to auto configure),
     * then before claiming an interface, call this method.
     *
     * The device must be open to use this method.
     * @param desired
     * @param callback
     */
    public setConfiguration(this: usb.Device, desired: number, callback?: (error: usb.LibUSBException | undefined) => void): void {
        this.__setConfiguration(desired, error => {
            if (!error) {
                this.interfaces = [];
                const len = this.configDescriptor ? this.configDescriptor.interfaces.length : 0;
                for (let i = 0; i < len; i++) {
                    this.interfaces[i] = new Interface(this, i);
                }
            }
            if (callback) {
                callback.call(this, error);
            }
        });
    }

    /**
     * Perform a control transfer with `libusb_control_transfer`.
     *
     * Parameter `data_or_length` can be an integer length for an IN transfer, or a `Buffer` for an OUT transfer. The type must match the direction specified in the MSB of bmRequestType.
     *
     * The `data` parameter of the callback is actual transferred for OUT transfers, or will be passed a Buffer for IN transfers.
     *
     * The device must be open to use this method.
     * @param bmRequestType
     * @param bRequest
     * @param wValue
     * @param wIndex
     * @param data_or_length
     * @param callback
     */
    public controlTransfer(this: usb.Device, bmRequestType: number, bRequest: number, wValue: number, wIndex: number, data_or_length: number | Buffer,
        callback?: (error: usb.LibUSBException | undefined, buffer: Buffer | number | undefined) => void): usb.Device {
        const isIn = !!(bmRequestType & usb.LIBUSB_ENDPOINT_IN);
        const wLength = isIn ? data_or_length as number : (data_or_length as Buffer).length;

        if (isIn) {
            if (!(data_or_length >= 0)) {
                throw new TypeError('Expected size number for IN transfer (based on bmRequestType)');
            }
        } else {
            if (!isBuffer(data_or_length)) {
                throw new TypeError('Expected buffer for OUT transfer (based on bmRequestType)');
            }
        }

        // Buffer for the setup packet
        // http://libusbx.sourceforge.net/api-1.0/structlibusb__control__setup.html
        const buf = Buffer.alloc(wLength + usb.LIBUSB_CONTROL_SETUP_SIZE);
        buf.writeUInt8(bmRequestType, 0);
        buf.writeUInt8(bRequest, 1);
        buf.writeUInt16LE(wValue, 2);
        buf.writeUInt16LE(wIndex, 4);
        buf.writeUInt16LE(wLength, 6);

        if (!isIn) {
            buf.set(data_or_length as Buffer, usb.LIBUSB_CONTROL_SETUP_SIZE);
        }

        const transfer = new usb.Transfer(this, 0, usb.LIBUSB_TRANSFER_TYPE_CONTROL, this.timeout,
            (error, buf, actual) => {
                if (callback) {
                    if (isIn) {
                        callback.call(this, error, buf.slice(usb.LIBUSB_CONTROL_SETUP_SIZE, usb.LIBUSB_CONTROL_SETUP_SIZE + actual));
                    } else {
                        callback.call(this, error, actual);
                    }
                }
            }
        );

        try {
            transfer.submit(buf);
        } catch (e) {
            if (callback) {
                process.nextTick(() => callback.call(this, e, undefined));
            }
        }
        return this;
    }

    /**
     * Return the interface with the specified interface number.
     *
     * The device must be open to use this method.
     * @param addr
     */
    public interface(this: usb.Device, addr: number): Interface {
        if (!this.interfaces) {
            throw new Error('Device must be open before searching for interfaces');
        }

        addr = addr || 0;
        for (let i = 0; i < this.interfaces.length; i++) {
            if (this.interfaces[i].interfaceNumber === addr) {
                return this.interfaces[i];
            }
        }

        throw new Error(`Interface not found for address: ${addr}`);
    }

    /**
     * Perform a control transfer to retrieve a string descriptor
     *
     * The device must be open to use this method.
     * @param desc_index
     * @param callback
     */
    public getStringDescriptor(this: usb.Device, desc_index: number, callback: (error?: usb.LibUSBException, value?: string) => void): void {
        // Index 0 indicates null
        if (desc_index === 0) {
            callback();
            return;
        }

        const langid = 0x0409;
        const length = 255;
        this.controlTransfer(
            usb.LIBUSB_ENDPOINT_IN,
            usb.LIBUSB_REQUEST_GET_DESCRIPTOR,
            ((usb.LIBUSB_DT_STRING << 8) | desc_index),
            langid,
            length,
            (error, buffer) => {
                if (error) {
                    return callback(error);
                }
                callback(undefined, isBuffer(buffer) ? buffer.toString('utf16le', 2) : undefined);
            }
        );
    }

    /**
     * Perform a control transfer to retrieve an object with properties for the fields of the Binary Object Store descriptor.
     *
     * The device must be open to use this method.
     * @param callback
     */
    public getBosDescriptor(this: usb.Device, callback: (error?: usb.LibUSBException, descriptor?: BosDescriptor) => void): void {
        if (this._bosDescriptor) {
            // Cached descriptor
            return callback(undefined, this._bosDescriptor);
        }

        if (this.deviceDescriptor.bcdUSB < 0x201) {
            // BOS is only supported from USB 2.0.1
            return callback(undefined, undefined);
        }

        this.controlTransfer(
            usb.LIBUSB_ENDPOINT_IN,
            usb.LIBUSB_REQUEST_GET_DESCRIPTOR,
            (usb.LIBUSB_DT_BOS << 8),
            0,
            usb.LIBUSB_DT_BOS_SIZE,
            (error, buffer) => {
                if (error) {
                    // Check BOS descriptor exists
                    if (error.errno === usb.LIBUSB_TRANSFER_STALL) return callback(undefined, undefined);
                    return callback(error, undefined);
                }

                if (!isBuffer(buffer)) {
                    return callback(undefined, undefined);
                }

                const totalLength = buffer.readUInt16LE(2);
                this.controlTransfer(
                    usb.LIBUSB_ENDPOINT_IN,
                    usb.LIBUSB_REQUEST_GET_DESCRIPTOR,
                    (usb.LIBUSB_DT_BOS << 8),
                    0,
                    totalLength,
                    (error, buffer) => {
                        if (error) {
                            // Check BOS descriptor exists
                            if (error.errno === usb.LIBUSB_TRANSFER_STALL) return callback(undefined, undefined);
                            return callback(error, undefined);
                        }

                        if (!isBuffer(buffer)) {
                            return callback(undefined, undefined);
                        }

                        const descriptor: BosDescriptor = {
                            bLength: buffer.readUInt8(0),
                            bDescriptorType: buffer.readUInt8(1),
                            wTotalLength: buffer.readUInt16LE(2),
                            bNumDeviceCaps: buffer.readUInt8(4),
                            capabilities: []
                        };

                        let i = usb.LIBUSB_DT_BOS_SIZE;
                        while (i < descriptor.wTotalLength) {
                            const capability = {
                                bLength: buffer.readUInt8(i + 0),
                                bDescriptorType: buffer.readUInt8(i + 1),
                                bDevCapabilityType: buffer.readUInt8(i + 2),
                                dev_capability_data: buffer.slice(i + 3, i + buffer.readUInt8(i + 0))
                            };

                            descriptor.capabilities.push(capability);
                            i += capability.bLength;
                        }

                        // Cache descriptor
                        this._bosDescriptor = descriptor;
                        callback(undefined, this._bosDescriptor);
                    }
                );
            }
        );
    }

    /**
     * Retrieve a list of Capability objects for the Binary Object Store capabilities of the device.
     *
     * The device must be open to use this method.
     * @param callback
     */
    public getCapabilities(this: usb.Device, callback: (error: usb.LibUSBException | undefined, capabilities?: Capability[]) => void): void {
        const capabilities: Capability[] = [];

        this.getBosDescriptor((error, descriptor) => {
            if (error) return callback(error, undefined);

            const len = descriptor ? descriptor.capabilities.length : 0;
            for (let i = 0; i < len; i++) {
                capabilities.push(new Capability(this, i));
            }

            callback(undefined, capabilities);
        });
    }
}
