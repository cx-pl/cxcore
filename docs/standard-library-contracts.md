# Standard-library runtime contracts

These contracts describe the current C runtime. The collector and implicit object
destruction are not implemented, so callers must manage heap storage explicitly. These
manual ownership rules are provisional until the garbage-collection milestone defines
managed versus explicitly allocated storage.

## Allocation and ownership

- `Memory.Alloc` returns zero-initialized storage. A zero-byte request returns a
  non-null allocation that can be passed to `Memory.Free`.
- `Memory.Realloc(block, 0)` frees `block` and returns null. For a nonzero size, a
  failed reallocation terminates the process; it never returns a lost old pointer.
- `Memory.Copy` is a no-op for size zero. Nonzero copies require valid, non-overlapping
  ranges, as with C `memcpy`.
- `Array` owns a zero-initialized element buffer allocated by `cx_array_new`. The
  current explicit cleanup is `Memory.Free(array->_data)` followed by
  `Memory.Free(array)`. `cx_array_at` returns null for a null array, an out-of-range
  index, invalid zero-sized elements, or missing backing storage.
- Runtime-owned allocations go through `Memory.Alloc` and can be released through
  `Memory.Free`. Keeping allocation behind this API is intended to leave a single
  integration point for a future collector; it does not imply that current allocations
  are traced or collected.
- `Nullable` is a one-pointer value. Its constructor and `Value` setter store a
  non-owning pointer; compiler-generated lifts call `Nullable<T>.CreateValueStorage`,
  which allocates a copy of the supplied bytes through `Memory.Alloc`. The caller owns
  that allocation.
- `Object` initializes its runtime vtable and leaves `_gc` null. No finalizer,
  reference counting, or automatic collection runs.
- `DateTime` component construction validates the Gregorian date and time fields. `AddMonths`
  uses calendar months, clamps the day to the last day of the destination month, and keeps
  the time-of-day and offset. Invalid components and results outside the stored millisecond
  range write a diagnostic and exit with failure.

## Strings, characters, and console

- String data is UTF-8 and `_length` is a byte count. `String.Length` counts Unicode
  scalar values, and `String.Item` indexes those scalar values. The item getter returns
  `-1` for an invalid index or malformed UTF-8 sequence. `Length` counts each malformed
  byte as one replacement position so it stays bounded by the stored byte length.
- `String.__constructor(length, data)` borrows an immutable byte buffer. The buffer
  must remain alive and unchanged while the string uses it. String literals borrow
  static storage. The character-fill constructor allocates its encoded UTF-8 buffer;
  callers release it through `Memory.Free(string._data)` when appropriate.
- `Char` is a Unicode scalar value in a 32-bit integer. `Char.NumBytes` returns its
  UTF-8 encoded width, or `-1` for a surrogate or value above U+10FFFF. Current
  `IsDigit`, `IsLetter`, case conversion, and whitespace helpers are ASCII-only.
- `Console.Read(length)` reads at most `length` bytes. `ReadLine()` reads through a
  newline or EOF and grows its buffer as needed. Both return a heap-allocated String
  object with heap-allocated contents; callers own both allocations and can free the
  buffer and object with `Memory.Free`. `Console.Write` writes the stored UTF-8 bytes
  unchanged and returns the number of bytes written.

Allocation failures in `Memory.Alloc` and nonzero `Memory.Realloc` write `Out of memory`
to standard error and exit with failure. The old allocation remains untouched when
reallocation fails before process termination. Bounds,
ownership, and UTF-8 errors do not currently raise typed CX exceptions.

## Shared-library declarations

`CX_API` marks runtime functions as exports while building `cxcore`, imports for dynamic
consumers, and undecorated declarations for static consumers. Public runtime data uses
`CX_CXCORE_DATA_API` with the same producer/consumer/static behavior. CMake propagates
`CX_STATIC_LINK` from static `cxcore` targets to their consumers.

## Platform support

The current runtime implementation supports Windows through MSVC. `platform.h` rejects
Linux and macOS builds because their environment and system-clock providers are not
implemented. The common runtime paths are shared C code, but only Windows platform behavior
has been built and tested so far.
