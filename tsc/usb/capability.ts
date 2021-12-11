import type { Device } from './bindings';
import { CapabilityDescriptor } from './descriptors';

export class Capability {
    /** Object with fields from the capability descriptor -- see libusb documentation or USB spec. */
    public descriptor: CapabilityDescriptor;

    /** Integer capability type. */
    public type: number;

    /** Buffer capability data. */
    public data: Buffer;

    constructor(protected device: Device, protected id: number) {
        if (!device._bosDescriptor) {
            throw new Error('bosDescriptor not found');
        }
        this.descriptor = device._bosDescriptor.capabilities[this.id];
        this.type = this.descriptor.bDevCapabilityType;
        this.data = this.descriptor.dev_capability_data;
    }
}
