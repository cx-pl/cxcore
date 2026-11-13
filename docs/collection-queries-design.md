# Collection queries

`System.Collections.Query` provides local sequence operators and inspectable
query plans for external providers. Local queries execute over
`IEnumerable<T>` or `IAsyncEnumerable<T>`. Provider plans describe an operation;
a provider decides how to translate and execute it.

Import `System.Collections.Query` to use the extension methods. The API is
experimental, and supported operations depend on the source and callback types.

## Synchronous local queries

`Filter`, `Map`, `Skip`, `Take`, and `Page` are deferred. Constructing a query
does not enumerate the source. Each enumeration obtains a fresh source
enumerator; replaying a query therefore depends on the source being replayable.

| Operator | Behavior |
| --- | --- |
| `Filter(predicate)` | Preserve source order and yield matching items |
| `Map(selector)` | Project each consumed item |
| `Skip(count)` | Discard the requested prefix |
| `Take(count)` | Stop after the requested number of items |
| `Page(pageIndex, pageSize)` | Compose checked `Skip` and `Take` |
| `Sort(keySelector, comparer)` | Buffer and stably sort by an ascending key |
| `SortDescending(keySelector, comparer)` | Buffer and stably sort by a descending key |
| `ThenSort`, `ThenSortDescending` | Add secondary keys to an ordered query |

`Take(0)` and zero-size pages do not open the source. `Page` checks
`pageIndex * pageSize` for overflow before enumeration.

Local operators validate null sources and required callbacks. Invalid null
arguments throw `ArgumentNullException`.

### Sorting

Sorting buffers on the first `MoveNext`, evaluates each key selector once per
item per enumeration, and uses a stable merge sort. Equal keys preserve source
order. One primary key and at most seven secondary keys are supported.

Every ordering call requires an explicit comparer with negative, zero, and
positive results for less-than, equality, and greater-than. There is no implicit
default comparer: the caller chooses null placement and string collation.
Comparers must define a consistent ordering for their key domain.

Sorting requires temporary buffers for values, keys, and merge work. Buffers
and callback references are released when the enumerator is disposed.

### Immediate terminals

`ToArray` and `ToList` materialize the sequence immediately.

| Terminal | Behavior |
| --- | --- |
| `Any` | Stop at the first item or predicate match |
| `Count` | Read the source and return a checked `uint` count |
| `First`, `FirstOrDefault` | Return the first item or predicate match |
| `Last`, `LastOrDefault` | Read through the source and return the last item or match |
| `Single`, `SingleOrDefault` | Require at most one item or match; stop on a second match |
| `ElementAt`, `ElementAtOrDefault` | Read the item at a zero-based position |

`Any`, `Count`, and the First/Last/Single families have predicate overloads.
`OrDefault` returns `default` when no item matches. Required-result and
multiple-match failures throw `InvalidOperationException`; out-of-range
`ElementAt` throws `ArgumentOutOfRangeException`. Count overflow throws
`InvalidOperationException` instead of wrapping.

### Enumeration and cleanup

Wrapper enumerators release their source when exhausted, when a `Take` limit
is reached, or when the consumer disposes them. Consumers must dispose
enumerators after early termination.

Immediate terminals dispose their enumerator in `finally`. Source, `Current`,
selector, predicate, and comparer failures follow the same cleanup path. If
cleanup itself throws while another exception is active, the cleanup exception
takes precedence.

## Asynchronous local queries

Async local operators consume `IAsyncEnumerable<T>` and forward the consumer's
cancellation token to the source enumerator.

| Operator | Callback or behavior |
| --- | --- |
| `Map` | Synchronous selector; compute and cache the result on first `Current` access after a successful move |
| `MapAsync` | Await a `delegate(T) => Task<TResult>` selector |
| `Filter` | Synchronous predicate |
| `FilterAsync` | Await a `delegate(T) => Task<bool>` predicate |
| `Skip`, `Take`, `Page` | Deferred windowing with the same checked offset rules as synchronous queries |
| `ToArrayAsync`, `ToListAsync` | Materialize and complete only after source cleanup |
| `AnyAsync`, `CountAsync` | Short-circuit or count, with synchronous and awaited predicate overloads |
| `FirstAsync`, `LastAsync`, `SingleAsync` and their `OrDefaultAsync` forms | Position and cardinality terminals, with synchronous and awaited predicates |
| `ElementAtAsync`, `ElementAtOrDefaultAsync` | Zero-based position terminals |

Repeated `Current` reads on async `Map` invoke the selector only once per
successful move. `MapAsync` processes one source element while its selector is
pending and rejects overlapping `MoveNextAsync` calls.

The consumer owns its async enumerator and must call and await `DisposeAsync`
after exhaustion or early termination. Terminals await pending moves and
disposal before completing their tasks. Cancellation, source faults, and
callback faults still require cleanup. A disposal failure takes precedence
over an active source or callback failure.

Direct disposal during pending movement waits for that operation before
disposing upstream. When disposal wins during source advancement, the move
finishes as false and the callback is skipped. When disposal is requested
during an awaited callback, the callback settles before cleanup and the move
finishes as false. Pending upstream disposal must settle before the disposal
task completes.

These operators do not provide concurrent enumeration or a worker-pool
executor. The runtime task scheduler's ownership and execution rules still
apply; see [threading and GC coordination](threading-gc-design.md).

## Source-specific execution paths

Interface-based sources use wrapper enumerators and delegate calls. Selected
concrete Array/List pipelines use indexed enumerators, and consecutive filters
can fuse. These paths preserve deferred execution, callback order,
short-circuiting, disposal, and reference retention.

Specialization depends on the exact source and operator chain. A pipeline may
use the general interface path when composed differently; API semantics do
not promise zero allocation or handwritten-loop performance. Mutating a source
during enumeration is not specified.

## Provider expressions

`QueryExpression<T>`, `QueryLambda<TParameter, TResult>`, `QueryConstant<T>`,
and `QueryCapture<T>` describe provider-inspectable expressions rather than
opaque local delegates.

Expression nodes cover parameters, members, constants, comparisons, arithmetic,
boolean composition, conditionals, local bindings, and calls. A call node
retains the resolved qualified method name, overload index, receiver when
present, and ordered argument nodes. Representing a call does not mean a
provider can translate it.

Supported lambda bodies include:

- Expression bodies.
- Initialized locals and straight-line assignments to declared locals.
- Built-in `+=`, `-=`, `*=`, `/=`, and `%=` assignments to those locals.
- Guard clauses with one direct return and no `else`, followed by a final return.
- A terminal `if/else` with one direct return per arm.
- Nonterminal `if/else` arms with straight-line assignments to the same local.

Locals lower to nested `Let` bindings and scoped `Variable` reads, preserving
evaluation order. Conditional branches lower to typed `Conditional` nodes.

Assignments to parameters or members, user-defined compound assignments, loops,
uninitialized locals, and other unsupported statement forms produce compiler
diagnostics. This expression model does not represent arbitrary CX code.

### Captured parameters

Place outer values in explicit `QueryCapture<T>` cells. Implicit lexical
capture in provider expressions is diagnosed.

A capture cell remains live after plan construction. A provider should snapshot
each distinct cell once at the start of an execution and bind that snapshot as
a parameter. Re-execution may see updated cell values; one execution should not
observe inconsistent values from repeated reads of the same cell.

## Provider plans

`QueryPlan<T>` represents a named source followed by filtering, projection,
ordering, and windowing operations. It exposes `Filter`, `Map`, `Sort`,
`SortDescending`, `ThenSort`, `ThenSortDescending`, `Skip`, `Take`, and
`Page`. The `Select` and `OrderBy` method families are aliases for the
corresponding projection and ordering nodes.

Projection changes the result type. Ordering retains inspectable key
expressions; providers define comparison, null, and collation behavior. A sort
chain supports at most eight keys, matching the local API.

On a source node, `Source` may contain an opaque provider-owned payload. On an
operator node, it contains the preceding plan. The plan is not an
`IEnumerable<T>` and cannot implicitly switch to local enumeration.

### Validation

Providers must validate plans before execution. `QueryPlanValidator.Validate`
rejects unknown node kinds, missing predecessors or expressions, malformed
source nodes, invalid secondary-order ancestry, and excessive ordering keys.
The public constructor can represent malformed plans, so construction alone
does not establish validity.

Validation checks plan structure. It does not prove expression compatibility
or provider translatability; each provider must reject unsupported operations
and callback forms before executing them.

### Execution boundaries

| Interface | Responsibility |
| --- | --- |
| `IQueryProvider` | Materialize a `QueryPlan<T>` synchronously through `Execute<T>` |
| `IAsyncQueryProvider` | Materialize through `ExecuteAsync<T>` or expose local async consumption through `Stream<T>` |
| `IQueryScalarProvider`, `IAsyncQueryScalarProvider` | Execute boolean and count scalar plans |
| `IQueryPositionScalarProvider<T>`, `IAsyncQueryPositionScalarProvider<T>` | Execute typed position and cardinality plans |

Scalar plan builders include `Any`, `Count`, First/Last/Single families, and
ElementAt families. Predicate Any/Count plans compose a filter. Providers should
also use `QueryScalarPlanValidator` for scalar-plan structure.

`Stream<T>` returns an `IAsyncEnumerable<T>`. Cancellation arrives when its
consumer requests an enumerator. Providers must observe cancellation while
advancing and release resources after completion, faults, cancellation, or
partial-consumer disposal. `ExecuteAsync` settles only after cleanup and must
not block waiting for async work.

`cxcore` supplies the plan model, task types, and async-disposal contracts. It
does not ship a database provider, generate SQL, bind database parameters, or
perform database I/O. External provider libraries own translation, execution,
resource management, and provider-specific error behavior.

## Implementation and limits

The local operators live in
[`EnumerableQueryExtensions.cx`](../src/System/Collections/Query/EnumerableQueryExtensions.cx)
and
[`AsyncEnumerableQueryExtensions.cx`](../src/System/Collections/Query/AsyncEnumerableQueryExtensions.cx).
Provider types and validators live in
[`QueryPlan.cx`](../src/System/Collections/Query/QueryPlan.cx).

Provider plans use constructors and instance transformations. Static generic
factory methods for these closed generic plan types are not supported.
Provider execution depends on the external implementation's supported node and
type combinations; no arbitrary translation or expression execution is implied.

See [collections](collections-design.md) for source container contracts and the
[benchmark guide](../benchmarks/README.md) for measurement practices.
