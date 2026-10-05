# Standard-library runtime contracts

These contracts describe the current C runtime. CX allocations are managed by a
non-moving conservative collector under the initial single-threaded Windows
runtime model. The collector design and limits are documented in
[`managed-memory-design.md`](managed-memory-design.md). The ABI and ownership
rules remain experimental.

## Allocation and ownership

- `Memory.Alloc` returns zero-initialized, collector-tracked storage. A zero-byte request returns a
  non-null allocation that can be passed to `Memory.Free`.
- `Memory.Realloc(block, 0)` frees `block` and returns null. For a nonzero size, a
  failed reallocation terminates the process; it never returns a lost old pointer.
- `Memory.Copy` is a no-op for size zero. Nonzero copies require valid, non-overlapping
  ranges, as with C `memcpy`.
- `Array` owns a zero-initialized element buffer allocated by `cx_array_new`. The
  current explicit cleanup is `Memory.Free(array->_data)` followed by
  `Memory.Free(array)`. `cx_array_at` returns null for a null array, an out-of-range
  index, invalid zero-sized elements, or missing backing storage.
- Runtime-owned allocations go through `Memory.Alloc` and are eligible for collection
  when unreachable. `Memory.Free` immediately unregisters and releases a block; freeing
  a block while a live reference may be used is invalid. `Memory.Realloc` tracks the
  resized block, but references invalidated by a move must not be used.
- `Nullable` is a one-pointer value. Its constructor and `Value` setter store a
  non-owning pointer; compiler-generated lifts call `Nullable<T>.CreateValueStorage`,
  which allocates a copy of the supplied bytes through `Memory.Alloc`; the payload is
  traced conservatively with the rest of the managed heap.
- `Object` initializes its runtime vtable and leaves `_gc` reserved and null. No
  finalizers run. The non-moving collector may retain unreachable blocks when stale
  pointer-shaped values remain on a scanned stack or in a registered root.
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
  callers may release it with `Memory.Free` only when no live reference can use it.
- `Char` is a Unicode scalar value in a 32-bit integer. `Char.NumBytes` returns its
  UTF-8 encoded width, or `-1` for a surrogate or value above U+10FFFF. Current
  `IsDigit`, `IsLetter`, case conversion, and whitespace helpers are ASCII-only.
- `Console.Read(length)` reads at most `length` bytes. `ReadLine()` reads through a
  newline or EOF and grows its buffer as needed. Both return a heap-allocated String
  object with heap-allocated contents; those allocations are traced. Callers may
  release them with `Memory.Free` when no live reference can use them. `Console.Write`
  writes the stored UTF-8 bytes
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

## Managed-memory limits

The collector scans the current thread stack, saved registers, managed blocks, and
registered static-field regions. Generated modules register their static fields;
native C code must register static storage that retains managed pointers. Heap
operations from a second OS thread fail. Native calls on the collector thread may
call CX synchronously; references retained beyond that call must be kept in a
registered root or another managed block.

## Platform support

The current runtime implementation supports Windows through MSVC. `platform.h` rejects
Linux and macOS builds because their environment and system-clock providers are not
implemented. The common runtime paths are shared C code, but only Windows platform behavior
has been built and tested so far.
