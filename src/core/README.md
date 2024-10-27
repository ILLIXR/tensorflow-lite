This directory contains the "core" part of the TensorFlow Lite runtime library.
The header files in this `core/` directory fall into several
categories.

1.  Public API headers, in the `api` subdirectory `core/api/`

    These are in addition to the other public API headers in ``.

    For example:
    - `core/api/error_reporter.h`
    - `core/api/op_resolver.h`

2.  Private headers that define public API types and functions.
    These headers are each `#include`d from a corresponding public "shim" header
    in `` that forwards to the private header.

    For example:
    - `core/interpreter.h` is a private header file that is
      included from the public "shim" header file `interpeter.h`.

    These private header files should be used as follows: `#include`s from `.cc`
    files in TF Lite itself that are _implementing_ the TF Lite APIs should
    include the "core" TF Lite API headers.  `#include`s from files that are
    just _using_ the regular TF Lite APIs should include the regular public
    headers.

3.  The header file `core/subgraph.h`. This contains
    some experimental APIs.