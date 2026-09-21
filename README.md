# cxcore
Core System Library

## Runtime type ABI

Class type information contains a base-type link and, when needed, a private map
from implemented interface type information to the concrete interface vtable.
Concrete interface vtables identify the dynamic class in slot zero. This lets
runtime type tests and checked casts recover the original object pointer and the
correct interface dispatch table without reinterpreting an incompatible pointer.

Checked casts preserve null. A failed non-null checked cast calls `abort()`; it
never returns a guessed pointer. Type tests return false for null.
