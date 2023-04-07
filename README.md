Fault Injection
===============

Testing complex software which actively interacts with system or some
library which provide interfaces for their content can be hard. The
testing of "Happy Path" is not very hard usually. But testing
scenarios with different errors returned from system/library might be
very hard because it impossible to mock interfaces (they are just
missing) or system calls. In such circumstances the "Fault Injection"
technique is used.

This library provides simple fault injection support. It's benefits:

* Small.

* Built as static library;

* Allow injections of:
  - return codes,
  - `errno`,
  - throwing exceptions.
  
* Enumerating injection points and controlling their status to
  implement external control.
  
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

This controls via macro `FAULT_INJECTIONS_ENABLED` when this macro is
defined and has value greater than 0 all injection points will inject
logic to corresponding places. Without this macro defined or when its
value is equal to 0 no fault logic will be injected producing retail
binary without any performance penalty.

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

API
---

All code except macros is placed to namespace `avm::fault_injection`.

### Define Injection Point

An injection point is represented with the following structure:

``` c++
struct point_t {
    const char * space;
    const char * name;
    int error_code;
    bool active;
};
```

`space`
: namespace of point

`name`
: name of point

`error_code`
: the error code returned when injection point is active and has type
  or "error code" or "`errno`"
  
`active`
: controls whether point is active or not

Instances of injection points should created with the following
macros:

`FAULT_INJECTION_POINT(space, name)`
: define fault injection point with `space` and `name`, the
  `error_code` will be 0.
  
`FAULT_INJECTION_POINT_E(space, name, error_code)`
: define fault injection point with `space`, `name` and default
  `error_code`.
  
All defined injection points are turned off by default.

> NOTE: all these macros should be used outside any namespace!

`FAULT_INJECTION_POINT_REF(space, name)`
: constructs global symbol name for point with `space` and `name`
  allowing to direct access to the point definition.
  
### Injecting

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
  
### Manipulating

All functions that receive 2 parameters perform search over the list
of defined points.

`isActive(FAULT_INJECTION_POINT_REF(space, name))` or
`isActive("space", "name")`
: returns `true` when point active.

`activate(FAULT_INJECTION_POINT_REF(space, name), active = true)` or
`activate("space", "name", active = true)`
: activate (when `active` is `true`) or deactivate point.

`setErrorCode(FAULT_INJECTION_POINT_REF(space, name), error = 0)` or
`activate("space", "name", error = 0)`
: set `error_code` to generate.

`find("space", "name")`
: lookup injection point by `space` and `name`, return pointer to
  point definition or `nullptr` in case when it is not found.

### Listing

All available injection points can be iterated via range-like
object `points`. It offers range of all registered injection points in
all loaded modules.
