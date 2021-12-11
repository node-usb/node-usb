import { EventEmitter } from 'events';
import { LibUSBException, LIBUSB_TRANSFER_CANCELLED, Transfer, Device } from './bindings';
import { EndpointDescriptor } from './descriptors';

const isBuffer = (obj: ArrayBuffer | Buffer): obj is Uint8Array => obj && obj instanceof Uint8Array;

/** Common base for InEndpoint and OutEndpoint. */
export abstract class Endpoint extends EventEmitter {
    public address: number;

    /** Endpoint direction: `"in"` or `"out"`. */
    public abstract direction: 'in' | 'out';

    /** Endpoint type: `usb.LIBUSB_TRANSFER_TYPE_BULK`, `usb.LIBUSB_TRANSFER_TYPE_INTERRUPT`, or `usb.LIBUSB_TRANSFER_TYPE_ISOCHRONOUS`. */
    public transferType: number;

    /** Sets the timeout in milliseconds for transfers on this endpoint. The default, `0`, is infinite timeout. */
    public timeout = 0;

    /** Object with fields from the endpoint descriptor -- see libusb documentation or USB spec. */
    public descriptor: EndpointDescriptor;

    constructor(protected device: Device, descriptor: EndpointDescriptor) {
        super();
        this.descriptor = descriptor;
        this.address = descriptor.bEndpointAddress;
        this.transferType = descriptor.bmAttributes & 0x03;
    }

    /** Clear the halt/stall condition for this endpoint. */
    public clearHalt(callback: (error: LibUSBException | undefined) => void): void {
        return this.device.__clearHalt(this.address, callback);
    }

    /**
     * Create a new `Transfer` object for this endpoint.
     *
     * The passed callback will be called when the transfer is submitted and finishes. Its arguments are the error (if any), the submitted buffer, and the amount of data actually written (for
     * OUT transfers) or read (for IN transfers).
     *
     * @param timeout Timeout for the transfer (0 means unlimited).
     * @param callback Transfer completion callback.
     */
    public makeTransfer(timeout: number, callback: (error: LibUSBException | undefined, buffer: Buffer, actualLength: number) => void): Transfer {
        return new Transfer(this.device, this.address, this.transferType, timeout, callback);
    }
}

/** Endpoints in the IN direction (device->PC) have this type. */
export class InEndpoint extends Endpoint {

    /** Endpoint direction. */
    public direction: 'in' | 'out' = 'in';

    protected pollTransfers: Transfer[] | undefined;
    protected pollTransferSize = 0;
    protected pollPending = 0;
    public pollActive = false;

    constructor(device: Device, descriptor: EndpointDescriptor) {
        super(device, descriptor);
    }

    /**
     * Perform a transfer to read data from the endpoint.
     *
     * If length is greater than maxPacketSize, libusb will automatically split the transfer in multiple packets, and you will receive one callback with all data once all packets are complete.
     *
     * `this` in the callback is the InEndpoint object.
     *
     * The device must be open to use this method.
     * @param length
     * @param callback
     */
    public transfer(length: number, callback: (error: LibUSBException | undefined, data?: Buffer) => void): InEndpoint {
        const buffer = Buffer.alloc(length);

        const cb = (error: LibUSBException | undefined, _buffer?: Buffer, actualLength?: number) => {
            callback.call(this, error, buffer.slice(0, actualLength));
        };

        try {
            this.makeTransfer(this.timeout, cb).submit(buffer);
        } catch (e) {
            process.nextTick(() => callback.call(this, e));
        }
        return this;
    }

    /**
     * Start polling the endpoint.
     *
     * The library will keep `nTransfers` transfers of size `transferSize` pending in the kernel at all times to ensure continuous data flow.
     * This is handled by the libusb event thread, so it continues even if the Node v8 thread is busy. The `data` and `error` events are emitted as transfers complete.
     *
     * The device must be open to use this method.
     * @param nTransfers
     * @param transferSize
     */
    public startPoll(nTransfers?: number, transferSize?: number, _callback?: (error: LibUSBException | undefined, buffer: Buffer, actualLength: number) => void): Transfer[] {
        const transferDone = (error: LibUSBException | undefined, transfer: Transfer, buffer: Buffer, actualLength: number) => {
            if (!error) {
                this.emit('data', buffer.slice(0, actualLength));
            } else if (error.errno != LIBUSB_TRANSFER_CANCELLED) {
                this.emit('error', error);
                this.stopPoll();
            }

            if (this.pollActive) {
                startTransfer(transfer);
            } else {
                this.pollPending--;

                if (this.pollPending === 0) {
                    this.pollTransfers = [];
                    this.emit('end');
                }
            }
        };

        const startTransfer = (transfer: Transfer) => {
            try {
                transfer.submit(Buffer.alloc(this.pollTransferSize), (error, buffer, actualLength) => {
                    transferDone(error, transfer, buffer, actualLength);
                });
            } catch (e) {
                this.emit('error', e);
                this.stopPoll();
            }
        };

        this.pollTransfers = this.startPollTransfers(nTransfers, transferSize, function (this: Transfer, error, buffer, actualLength) {
            transferDone(error, this, buffer, actualLength);
        });
        this.pollTransfers.forEach(startTransfer);
        this.pollPending = this.pollTransfers.length;
        return this.pollTransfers;
    }

    protected startPollTransfers(nTransfers = 3, transferSize = this.descriptor.wMaxPacketSize, callback: (error: LibUSBException | undefined, buffer: Buffer, actualLength: number) => void): Transfer[] {
        if (this.pollTransfers) {
            throw new Error('Polling already active');
        }

        this.pollTransferSize = transferSize;
        this.pollActive = true;
        this.pollPending = 0;

        const transfers: Transfer[] = [];
        for (let i = 0; i < nTransfers; i++) {
            const transfer = this.makeTransfer(0, callback);
            transfers[i] = transfer;
        }
        return transfers;
    }

    /**
     * Stop polling.
     *
     * Further data may still be received. The `end` event is emitted and the callback is called once all transfers have completed or canceled.
     *
     * The device must be open to use this method.
     * @param callback
     */
    public stopPoll(callback?: () => void): void {
        if (!this.pollTransfers) {
            throw new Error('Polling is not active.');
        }
        for (let i = 0; i < this.pollTransfers.length; i++) {
            try {
                this.pollTransfers[i].cancel();
            } catch (error) {
                this.emit('error', error);
            }
        }
        this.pollActive = false;
        if (callback) this.once('end', callback);
    }
}

/** Endpoints in the OUT direction (PC->device) have this type. */
export class OutEndpoint extends Endpoint {

    /** Endpoint direction. */
    public direction: 'in' | 'out' = 'out';

    /**
     * Perform a transfer to write `data` to the endpoint.
     *
     * If length is greater than maxPacketSize, libusb will automatically split the transfer in multiple packets, and you will receive one callback once all packets are complete.
     *
     * `this` in the callback is the OutEndpoint object.
     *
     * The device must be open to use this method.
     * @param buffer
     * @param callback
     */
    public transfer(buffer: Buffer, callback?: (error: LibUSBException | undefined, actual: number) => void): OutEndpoint {
        if (!buffer) {
            buffer = Buffer.alloc(0);
        } else if (!isBuffer(buffer)) {
            buffer = Buffer.from(buffer);
        }

        const cb = (error: LibUSBException | undefined, _buffer?: Buffer, actual?: number) => {
            if (callback) {
                callback.call(this, error, actual || 0);
            }
        };

        try {
            this.makeTransfer(this.timeout, cb).submit(buffer);
        } catch (e) {
            process.nextTick(() => cb(e));
        }

        return this;
    }

    public transferWithZLP(buffer: Buffer, callback: (error: LibUSBException | undefined) => void): void {
        if (buffer.length % this.descriptor.wMaxPacketSize === 0) {
            this.transfer(buffer);
            this.transfer(Buffer.alloc(0), callback);
        } else {
            this.transfer(buffer, callback);
        }
    }
}
