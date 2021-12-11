/**
 * A mutex implementation that can be used to lock access between concurrent async functions
 */
export class Mutex {
    private locked: boolean;

    /**
     * Create a new Mutex
     */
    constructor() {
        this.locked = false;
    }

    /**
     *  Yield the current execution context, effectively moving it to the back of the promise queue
     */
    private async sleep(): Promise<void> {
        return new Promise(resolve => setTimeout(resolve, 1));
    }

    /**
     * Wait until the Mutex is available and claim it
     */
    public async lock(): Promise<void> {
        while (this.locked) {
            await this.sleep();
        }

        this.locked = true;
    }

    /**
     * Unlock the Mutex
     */
    public unlock(): void {
        this.locked = false;
    }
}
