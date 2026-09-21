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

### Reflection stability

The canonical `TypeInfo` address, hash, flags, size, base-type link, and generic
arity form the stable read-only identity layer. Type hashes are derived from the
module-qualified nested type name plus a backtick and generic arity when nonzero.

Implemented-interface, field, function, and parameter tables are exposed through
the `cx_reflection_*` accessors. Their current C layouts are experimental: they
support enumeration only, and do not promise invocation, mutation, or dynamic
construction. Fields declared on base classes are enumerated through the base
type's table. Static fields use `CX_REFLECTION_NO_OFFSET`; non-virtual functions
use `CX_REFLECTION_NO_SLOT`.
