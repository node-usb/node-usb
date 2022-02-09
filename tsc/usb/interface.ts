import { LibUSBException, LIBUSB_ENDPOINT_IN, Device } from './bindings';
import { InterfaceDescriptor } from './descriptors';
import { Endpoint, InEndpoint, OutEndpoint } from './endpoint';

export class Interface {
    /** Integer interface number. */
    public interfaceNumber!: number;

    /** Integer alternate setting number. */
    public altSetting = 0;

    /** Object with fields from the interface descriptor -- see libusb documentation or USB spec. */
    public descriptor!: InterfaceDescriptor;

    /** List of endpoints on this interface: InEndpoint and OutEndpoint objects. */
    public endpoints!: Endpoint[];

    constructor(protected device: Device, protected id: number) {
        this.refresh();
    }

    protected refresh(): void {
        if (!this.device.configDescriptor) {
            return;
        }

        this.descriptor = this.device.configDescriptor.interfaces[this.id][this.altSetting];
        this.interfaceNumber = this.descriptor.bInterfaceNumber;
        this.endpoints = [];
        const len = this.descriptor.endpoints.length;
        for (let i = 0; i < len; i++) {
            const desc = this.descriptor.endpoints[i];
            const c = (desc.bEndpointAddress & LIBUSB_ENDPOINT_IN) ? InEndpoint : OutEndpoint;
            this.endpoints[i] = new c(this.device, desc);
        }
    }

    /**
     * Claims the interface. This method must be called before using any endpoints of this interface.
     *
     * The device must be open to use this method.
     */
    public claim(): void {
        this.device.__claimInterface(this.id);
    }

    /**
     * Releases the interface and resets the alternate setting. Calls callback when complete.
     *
     * It is an error to release an interface with pending transfers.
     *
     * The device must be open to use this method.
     * @param callback
     */
    public release(callback?: (error?: LibUSBException) => void): void;

    /**
     * Releases the interface and resets the alternate setting. Calls callback when complete.
     *
     * It is an error to release an interface with pending transfers. If the optional closeEndpoints
     * parameter is true, any active endpoint streams are stopped (see `Endpoint.stopStream`),
     * and the interface is released after the stream transfers are cancelled. Transfers submitted
     * individually with `Endpoint.transfer` are not affected by this parameter.
     *
     * The device must be open to use this method.
     * @param closeEndpoints
     * @param callback
     */
    public release(closeEndpoints?: boolean, callback?: (error?: LibUSBException) => void): void;
    public release(closeEndpointsOrCallback?: boolean | ((error?: LibUSBException) => void), callback?: (error: LibUSBException | undefined) => void): void {

        let closeEndpoints = false;
        if (typeof closeEndpointsOrCallback === 'boolean') {
            closeEndpoints = closeEndpointsOrCallback;
        } else {
            callback = closeEndpointsOrCallback;
        }

        const next = () => {
            this.device.__releaseInterface(this.id, error => {
                if (!error) {
                    this.altSetting = 0;
                    this.refresh();
                }
                if (callback) {
                    callback.call(this, error);
                }
            });
        };

        if (!closeEndpoints || this.endpoints.length === 0) {
            next();
        } else {
            let n = this.endpoints.length;
            this.endpoints.forEach(ep => {
                if (ep.direction === 'in' && (ep as InEndpoint).pollActive) {
                    ep.once('end', () => {
                        if (--n === 0) {
                            next();
                        }
                    });
                    (ep as InEndpoint).stopPoll();
                } else {
                    if (--n === 0) {
                        next();
                    }
                }
            });
        }
    }

    /**
     * Returns `false` if a kernel driver is not active; `true` if active.
     *
     * The device must be open to use this method.
     */
    public isKernelDriverActive(): boolean {
        return this.device.__isKernelDriverActive(this.id);
    }

    /**
     * Detaches the kernel driver from the interface.
     *
     * The device must be open to use this method.
     */
    public detachKernelDriver(): void {
        return this.device.__detachKernelDriver(this.id);
    }

    /**
     * Re-attaches the kernel driver for the interface.
     *
     * The device must be open to use this method.
     */
    public attachKernelDriver(): void {
        return this.device.__attachKernelDriver(this.id);
    }

    /**
     * Sets the alternate setting. It updates the `interface.endpoints` array to reflect the endpoints found in the alternate setting.
     *
     * The device must be open to use this method.
     * @param altSetting
     * @param callback
     */
    public setAltSetting(altSetting: number, callback?: (error: LibUSBException | undefined) => void): void {
        this.device.__setInterface(this.id, altSetting, error => {
            if (!error) {
                this.altSetting = altSetting;
                this.refresh();
            }
            if (callback) {
                callback.call(this, error);
            }
        });
    }

    /**
     * Return the InEndpoint or OutEndpoint with the specified address.
     *
     * The device must be open to use this method.
     * @param addr
     */
    public endpoint(addr: number): Endpoint | undefined {
        return this.endpoints.find(item => item.address === addr);
    }
}
