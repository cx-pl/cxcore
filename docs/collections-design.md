# System.Collections starter API

`System.Collections.Array<T>` is the public collection API over the same value
as built-in `T[]`. The runtime storage type and reflection identity are both
`System.Collections.Array`; `cx_array_new`/`cx_array_at` remain the backing
helpers and ABI version remains 1. The compiler resolves both spellings to the same type identity and
lowers `new Array<T>(length)`, `Length`, `Get`, and `Set` to the built-in array
representation. The source declaration has no recursive `T[]` backing field
or wrapper allocation. See
[`array-unification-design.md`](../../cxc/docs/array-unification-design.md)
for implementation details and remaining reflection/coverage work.

`Array<T>` now has a supported native path for closed instances: they allocate
their backing `T[]`, expose `Length`, and read and write indexed elements.
Native coverage exercises `Array<int>`, `Array<long>`, and `Array<string>`.
This support is deliberately narrow; it does not make all other collection
classes natively usable.

`List<T>` now runs natively for closed `int` and `string` elements. Its methods
exercise closed generic owner bodies, nested `Array<T>` allocation, helper
calls, loops, default clearing, and `foreach` through a `GetEnumerator`/
`MoveNext`/`Current` pattern (with `Current` currently exposed as a method for
generic collection enumerators). `AddRange` handles self-append, `ToArray`
returns a compact copy, and `CopyTo` writes into a caller-provided array at a
destination offset. The enumerator captures the list count at creation;
mutating the list during enumeration is not yet specified. `Stack<T>` and
`Queue<T>` delegate to a nested `List<T>` field and pass native LIFO/FIFO
behavior checks for `int` and `string`. `LinkedList<T>` passes native
end-insertion/removal checks for `int` and `string`. All five collection
families currently support the structural enumerator pattern. `Dictionary<TKey,
TValue>` enumerates `KeyValuePair<TKey, TValue>` values in insertion order; its
enumerator captures the count when created, and mutation during enumeration is
not specified. `Dictionary<string, int>` passes native lookup, overwrite,
growth, removal, clear, and entry-enumeration checks. Native coverage also
includes `Dictionary<string, long>`, `Dictionary<CollisionKey, long>`, and
`Dictionary<string, string>`; the reference-value fixture checks replacement,
entry enumeration, removal, and clearing retained values.
It accepts null keys and
uses virtual `Object.GetHashCode` and `Object.Equals` for non-null reference
keys, so strings compare by contents. Lookup probes open-addressed buckets,
while entry enumeration retains insertion order. Keys must keep a stable hash
while stored, and keys considered equal must return the same hash. The optional
`IEqualityComparer<TKey>` constructor argument can replace both hashing and
equality; a null comparer keeps the virtual `Object` behavior. The comparer is
fixed for the dictionary lifetime so existing bucket placement remains valid.
The current key constraint remains reference-only. The bucket count remains a
power of two.

The public declaration and runtime C provider live under
`System/Collections/Array`. Built-in
array storage uses that same public runtime type. Since
CX does not currently declare user-defined indexers or `out` parameters;
`Get`/`Set` require prevalidated indices. `GetOrDefault` now returns `default`
for an index outside `Length`, and `TrySet` reports whether it wrote the value.
An empty factory can follow once generic static initialization has a settled
contract. Preserve this path when changing the declaration location.

The source implementations provide:

- `Array<T>`: construction, `Length`, `Get`, `GetOrDefault`, `Set`, `TrySet`,
  `Fill`, and range-checked `TryCopyTo`.
- `List<T>`: `Count`, `Get`, `GetOrDefault`, `Set`, `TrySet`, `Add`, `Insert`,
  `TryInsert`, `RemoveAt`, `TryRemoveAt`, `Clear`, `AddRange`, `ToArray`,
  `CopyTo`, and range-checked `TryCopyTo`.
- `Stack<T>`: `Count`, `Push`, `Peek`, `Pop`, and `Clear`.
- `Queue<T>`: `Count`, `Enqueue`, `Peek`, `Dequeue`, and `Clear`.
- `LinkedList<T>`: `Count`, `AddFirst`, `AddLast`, `RemoveFirst`, `RemoveLast`,
  and `Clear`.
- `KeyValuePair<TKey, TValue>`: public `Key` and `Value` fields initialized by
  its constructor; yielded by dictionary enumeration.
- `Dictionary<TKey, TValue>`: `Set`, `GetValueOrDefault`, `ContainsKey`,
  `TryAdd`, `Remove`, `Clear`, and insertion-order enumeration for reference-type keys,
  using open-addressed buckets. With no comparer, it uses virtual `Object`
  hashing and equality; a supplied `IEqualityComparer<TKey>` controls both.
  Non-null string keys compare by contents, and two null keys compare equal.
  Native key/value coverage includes `string,int`, `string,long`, and a class
  key with `long` values. Keys must obey the usual stable hash/equality
  contract. Value-type keys remain unsupported.

Collection indices, the destination range passed to `CopyTo`, and non-`Try`
removal/peek operations have valid-range and non-empty preconditions. The new
`TrySet`, `TryInsert`, and `TryCopyTo` variants report invalid ranges without
modifying the collection or destination. The queue uses a list and shifts
elements on dequeue; the dictionary stores bucket indices separately from its
ordered entries and rebuilds the buckets after removals and capacity growth.

`IEnumerable<T>` and `IEnumerator<T>` are declared. Closed generic enumerator
metadata now supports `ArrayEnumerator<T>`, `LinkedListEnumerator<T>`, and
`DictionaryEnumerator<TKey, TValue>`. Array, List, Stack, Queue, LinkedList,
and Dictionary expose `IEnumerable<T>` contracts; native fixtures cover
interface dispatch for List, Stack, Queue, LinkedList, and Dictionary. The
Array fixture covers `IEnumerator<T>` dispatch and structural `foreach`; a
same-module native fixture also verifies built-in `T[]` conversion to
`IEnumerable<T>` and interface enumeration. Next, broaden dictionary key/value
combinations, including reference values and value-type keys once their equality
and hashing support is in place. Add
checked index/empty-operation variants where they fit the public API. Do not add a
`Hashtable<T>` facade until its intended semantics are specified (set, map, or
legacy non-generic table). Defer
`IAsyncEnumerable<T>` to Milestone 18, which owns asynchronous sequences and
their runtime contract.
