/**
 * Sets the name of the calling thread to `name`, which is
 * a NUL terminated UTF-8 string. Returns true if success,
 * false if error or unsupported platform.
 *
 * Since this is an OS-dependent operation, the requirements
 * of `name` may vary depending on platform. For example on
 * Linux, the maximum length (excluding the terminator) is
 * 16 bytes, and UTF-8 isn't strictly required (only conventional).
 */
bool SetThreadName(const char* name);
