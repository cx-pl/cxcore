# Collections

`System.Collections` provides arrays, lists, stacks, queues, linked lists,
and dictionaries. Import the namespace to use the public APIs.

## Array representation

`Array<T>` and built-in `T[]` use the same runtime type and storage.
`new Array<T>(length)`, `Length`, `Get`, and `Set` operate on that
representation without a separate wrapper allocation. The runtime helpers are
`cx_array_new`, `cx_array_new_for_owner`, and `cx_array_at`.

The public declaration is
[`src/System/Collections/Array.cx`](../src/System/Collections/Array.cx);
its native implementation is
[`platform/common/System/Collections/Array.c`](../platform/common/System/Collections/Array.c).

## Container APIs

| Type | Operations |
| --- | --- |
| `Array<T>` | Construction, `Length`, `Get`, `GetOrDefault`, `Set`, `TrySet`, `Fill`, `TryCopyTo` |
| `List<T>` | `Count`, `Get`, `GetOrDefault`, `Set`, `TrySet`, `Add`, `Insert`, `TryInsert`, `RemoveAt`, `TryRemoveAt`, `Clear`, `AddRange`, `ToArray`, `CopyTo`, `TryCopyTo` |
| `Stack<T>` | `Count`, `Push`, `Peek`, `Pop`, `Clear` |
| `Queue<T>` | `Count`, `Enqueue`, `Peek`, `Dequeue`, `Clear` |
| `LinkedList<T>` | `Count`, `AddFirst`, `AddLast`, `RemoveFirst`, `RemoveLast`, `Clear` |
| `Dictionary<TKey, TValue>` | `Count`, `Set`, `GetValueOrDefault`, `ContainsKey`, `TryAdd`, `Remove`, `Clear`, insertion-order enumeration |

Collection indices, destination ranges passed to `CopyTo`, and non-`Try`
removal/peek operations require valid ranges or non-empty containers.
`GetOrDefault` returns `default` for an out-of-range index. `TrySet`,
`TryInsert`, and `TryCopyTo` report invalid ranges without changing the
collection or destination.

`List.AddRange` supports self-append. `ToArray` returns a compact copy.
Stacks and queues use a nested list; dequeue shifts the remaining elements.
Linked lists allocate a separate managed node for each entry.

## Dictionaries and equality

Dictionary keys are currently restricted to reference types. Null keys are
accepted. Without a comparer, non-null keys use virtual `Object.GetHashCode`
and `Object.Equals`; strings therefore compare by contents.

An optional `System.IEqualityComparer<TKey>` constructor argument replaces
both hashing and equality. A null comparer selects the default virtual
behavior. The comparer is fixed for the dictionary's lifetime.

Keys must keep a stable hash while stored, and equal keys must have equal
hashes. Value-type keys are unsupported.

Lookup uses open-addressed buckets with a power-of-two bucket count.
Enumeration preserves insertion order through a separate entry array.
Removal and capacity growth rebuild the bucket index.
`KeyValuePair<TKey, TValue>` exposes the `Key` and `Value` of each entry.

## Enumeration

`ICollection<T>` inherits `IEnumerable<T>` and exposes a read-only `uint Count`.
Array, List, Stack, Queue, LinkedList, and Dictionary implement it.
Dictionary enumerates `KeyValuePair<TKey, TValue>`; built-in arrays share
`Array<T>`'s collection contract.

Collections expose `GetEnumerator`, `MoveNext`, and `Current` for structural
enumeration, alongside their interface contracts. Dispose an enumerator after
early termination. Mutating a collection during enumeration is not specified;
list and dictionary enumerators capture their count when created.

See [collection queries](collection-queries-design.md) for local synchronous
and asynchronous sequence operators and provider plans.

## Allocation and lifetimes

Growth follows the collection owner's allocation destination. A durable list
or dictionary grows on the managed heap even while an arena is active. An
arena-owned collection grows in its owning arena, including when a durable
allocation override is active. Linked-list node allocation follows the same
owner rule.

Checked stores reject references that would outlive their arena. Stale arena
extents and padding pointers are invalid allocation owners. A copied collection
does not automatically make its element references durable.

See [scoped allocation arenas](scoped-allocation-arenas-design.md) for region
lifetimes and [managed memory](managed-memory-design.md) for collection tracing.

## Limits

Container support relies on the compiler's closed generic specialization and
metadata support. It does not imply arbitrary open-generic runtime construction.
Value-type dictionary keys and mutation during enumeration have no supported
contract. Containers do not synchronize concurrent mutation; callers must
provide synchronization.
