Fault Injection
===============

Testing complex software which actively interacts with system or some
library which provide interfaces for their content can be hard. The
testing of "Happy Path" is not very hard usually. But testing
scenarios with different errors returned from system/library might be
very hard because it impossible to mock interfaces (they are just
missing) or system calls. In such circumstances the "Fault Injection"
technique is used.

This library provides simple fault injection support. Its benefits:

* Small.

* Built as static library.

* Allow injections of:
  - return codes,
  - `errno`,
  - throwing exceptions.

* One-shot and multiple triggering.

* Enumerating injection points and controlling their status to
  implement external control.

* Thread support can be turned off.

* The custom build scripts and translation units with many includes
  are not required.

Platforms
---------

Tested on the following platforms:

* x86 MacOS

* x86 Linux

* PowerPC 64-bit LE Linux

Design Notes
------------

The entire library can work in 2 modes selected during compilation:

* Turned off

  The defined fault injections are collected and placed to binaries
  but the actual usage is removed completely. Fault injection points
  can't be turned on.

* Turned on

  All fault injection points check at runtime their status and perform
  actions when enabled.

This is controlled via macro `FAULT_INJECTIONS_ENABLED` when this
macro is defined and has value greater than 0 all injection points
will inject logic to corresponding places. Without this macro defined
or when its value is equal to 0 no fault logic will be injected
producing retail binary without any performance penalty.

The macro `FAULT_INJECTIONS_DEFINITIONS` controls whether injection
point definitions included to binary or not. If
`FAULT_INJECTIONS_ENABLED` is greater than 0 or
`FAULT_INJECTIONS_DEFINITIONS` is greater than 0 then points are
defined and can be listed. This allows to disable fault injections but
have point definitions in place to clearly integrate with any tools
or provide stable external API.

All fault injection point should be defined before usage at global
namespace. Every injection point has:

* namespace to be placed in,

* name of point.

The full injection point symbol name will be
`::<space>::fault_injection_point_<name>` as external symbol allowing
to quick access from any translation unit.

The definition of injection point also contains status (active or not)
and error code which should be injected when it is active.

Pointers to all injection points are collected to a separate binary
section allowing iteration and lookup by name.

The fault injection macros refer to injection point by its symbol name
so this doesn't have any penalty to search over the list. But this
requires declaration of this point to be visible at the place of
injection. Since reuse of fault injection point can make non-obvious
dependencies there is declaration macro provided. The suggested style
is to define points in translation units that uses them and use every
point only in a single place.

The injection with error code or `errno` can be used as
expressions. The exception throw injection can be used only as
statement.

Tests are written with Boost.Test and it is expected that it is
available on standard include and library paths.

The dynamic library support works via registering module's points
during process of loading. This performed via "constructor"
function. All modules chained to a list allowing search by space and
name. This "constructor" function is linked automatically when library
is used.

Thread Safety
-------------

By default the library implements thread safe access to its data via
atomic operations. The error code and activation status are atomic
variables and access to their values by all library API uses
Release-Acquire ordering.

To have safe access to injection point data 2 macros are added:
`FAULT_INJECT_READ(var)` and `FAULT_INJECT_WRITE(var, value)`. These
macros use Release-Acquire ordering when thread support is turned on
and direct access when it is turned off. Please don't use them in
client code to maintain backward compatibility. The accessors in API
should be used instead.

The thread support is turned on by default and it can be turned of by
defining `FAULT_INJECTION_HAS_THREADS` to 0.

API
---

All code except macros is placed to namespace `avm::fault_injection`.

### Define Injection Point

An injection point is represented with the structure `point_t` and has
the following properties:

`version`
: a version of the structure used for backward compatibility

`space`
: namespace of point

`name`
: name of point

`description`
: a verbose description of point

`error_code`
: the error code returned when injection point is active and has type
  or "error code" or "`errno`"

`active`
: controls whether point is active or not

Direct access to that properties breaks backward compatibility. The
accessor functions should be used to obtain information about point.

Instances of injection points should created with the following
macros:

`FAULT_INJECTION_POINT(space, name)`
: define fault injection point with `space` and `name`, the
  `error_code` will be 0.

`FAULT_INJECTION_POINT_EX(space, name, error_code)`
: define fault injection point with `space`, `name` and default
  `error_code`.

All defined injection points are turned off by default.

> NOTE: all macros above should be used outside any namespace at global scope!

`FAULT_INJECTION_POINT_REF(space, name)`
: constructs global symbol name for point with `space` and `name`
  allowing to direct access to the point definition.

  > NOTE: Do not use this macros outside shared object which defines
  > injection point. This will break compilation or One Definition
  > Rule when shared object is built with another version of library.

### Injecting

> NOTE: Do not use these macros outside shared object which defines
> injection point. This will break compilation or One Definition
> Rule when shared object is built with another version of library.

The fault can be injected via one of the following macros.

`FAULT_INJECT_ERROR_CODE(space, name, action)`
: when inactive execute `action` (it is expected to return `int`),
  when active doesn't execute `action` but return `error_code` from
  point definition directly.

`FAULT_INJECT_ERRNO(space, name, action)`
: when inactive execute `action` and return its result (it is expected
  to return `int`), when active return -1 and set `errno` to the
  `error_code` from point definition.

`FAULT_INJECT_ERRNO_EX(space, name, action, result)`
: when inactive execute `action` and return its result, when active
  return `result` and set `errno` to the `error_code` from point
  definition.

`FAULT_INJECT_EXCEPTION(space, name, exception)`
: when inactive does nothing, when active throws `exception`.

  Requires semicolon after.

`FAULT_INJECT_ACTION(space, name, exception)`
: when inactive does nothing, when active performs `action`. `action`
  can be a complex statement by using `do { ... } while (false)`
  construct.

The separate set of macros allows injection of point with additional
condition to check before trigger. If condition is true the point is
triggered. If point is not triggered the one-shot point is not
deactivated because no error injected. These macros has `IF` inside:

* `FAULT_INJECT_ERROR_CODE_IF(space, name, condition, action)`

* `FAULT_INJECT_ERRNO_IF(space, name, condition, action)`

* `FAULT_INJECT_ERRNO_IF_EX(space, name, condition, action, result)`

* `FAULT_INJECT_EXCEPTION_IF(space, name, condition, exception)`

### Manipulating

All functions that receive 2 parameters perform search over the list
of defined points.

`isActive(FAULT_INJECTION_POINT_REF(space, name))` or
`isActive("space", "name")`
: returns `true` when point active.

`activate(FAULT_INJECTION_POINT_REF(space, name), mode = mode_t::multiple)` or
`activate("space", "name", mode = mode_t::multiple)`
: activate point. If `mode` is `mode_t::multiple` the point will
  triggers every time until explicitly deactivated, if
  `mode_t::oneshot` it will self-deactivate on first trigger.

`deactivate(FAULT_INJECTION_POINT_REF(space, name))` or
`deactivate("space", "name")`
: deactivate point.

`setErrorCode(FAULT_INJECTION_POINT_REF(space, name), error = 0)` or
`activate("space", "name", error = 0)`
: set `error_code` to generate.

`find("space", "name")`
: lookup injection point by `space` and `name`, return pointer to
  point definition or `nullptr` in case when it is not found.

> WARNING: do not use access macros (`FAULT_INJECTION_READ` and
> `FAULT_INJECTION_WRITE`) to maintain backward compatibility of
> client code.

### Listing

All available injection points can be iterated via range-like
object `points`. It offers range of all registered injection points in
all loaded modules.

### Test Helpers

Since tests want to activate fault injection in specific mode and with
specific error code, run code and then deactivate injection very often
the `InjectionStateGuard` class implements scoped guard to perform
these tasks. It allows to enable point optionally setting mode and
error code. If point has been inactive at the time of guard
construction it will be deactivated back upon destruction of
guard. The specified mode and error code will be set to point on
construction and returned back to previous values on destruction.
