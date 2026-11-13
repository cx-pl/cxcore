# Dependency injection

`System.DependencyInjection` provides a synchronous service container for CX
applications. The current implementation supports ordered registrations,
provider snapshots, constructor and factory activation, transient/scoped/singleton
caches, explicit scopes, graph validation, ordered `IEnumerable<T>` constructor
injection, and deterministic synchronous disposal.

```cx
import System;
import System.DependencyInjection;

IServiceCollection services = new ServiceCollection();
services.AddSingleton<IClock, SystemClock>();
services.AddScoped<IRepository, Repository>();
services.AddTransient<ReportService>();

ServiceProvider provider = services.BuildServiceProvider();
try {
    IServiceScope scope = provider.CreateScope();
    try {
        ReportService report = scope.ServiceProvider.GetRequiredService<ReportService>();
        report.Run();
    } finally {
        scope.Dispose();
    }
} finally {
    provider.Dispose();
}
```

Registration extensions are available on `IServiceCollection`, so ordinary
module registration functions can accept the interface. Each provider copies
the collection at build time. A later collection edit affects only providers
built afterward. Repeated service keys are allowed: single resolution selects
the last registration, and `GetServices<T>()` resolves every registration in
insertion order into an eager sequence.
Public constructors can receive `IEnumerable<T>` directly; the provider resolves
each matching registration in order and passes an empty typed sequence when none
are registered. The compiler emits reflection adapters for supported ordinary
and closed generic class constructors, including constructed interface
parameters.

Scopes are explicit and independent. Scoped instances are shared within one
scope and differ between scopes. Singletons belong to the built root provider,
including when first resolved through a scope. With the default
`ValidateScopes = true`, resolving a scoped service from the root or capturing
one in a singleton is rejected. `ValidateOnBuild = true` checks constructor
graphs without executing factories or constructing services.

The provider owns disposable instances created through implementation and
factory activation. It disposes them in reverse activation order, continues
after individual cleanup failures, and reports the failures together. Supplied
singleton instances remain externally owned. Root disposal first disposes
outstanding scopes; disposal is idempotent, and providers reject resolution
after disposal. A provider remains a GC root for its owned graph until
`Dispose()` removes the root, so dispose root providers and scopes when their
lifetimes end.

## Activation and type identity

Constructor activation plans are cached lazily per descriptor on the root
provider and shared by its scopes. Build validation reuses those plans.
Typed generic factories are available for all three lifetimes.

The runtime invokes the constructor selected by DI. Closed generic classes use
their closed constructor adapter, preserving specialized field and element
layouts. Type-key comparisons include the hash, name, namespace, generic
arguments, and runtime metadata identity so hash collisions do not merge types.

Constructor and enumerable-mediated dependency cycles are rejected with
path diagnostics during build validation and resolution. Factory reentry
cycles are also rejected. Providers retain their owned service graph and
factory closures until disposal.

## Current limits

Optional/default constructor parameters, property injection, open-generic
registrations, asynchronous factories/disposal, and concurrent resolution are
outside the current implementation.
