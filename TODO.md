# CUSS TODO

A collection of things to *eventually* take care of in CUSS. Things are added
to this document as I encounter issues and remember features yet to be
implemented. Things are removed from this document as and when they are taken
care of.

## Executor

### CUP

* Fix unsigned multiply and divide instructions.
* Support a `HALT` instruction.
* Improve the performance of the simulation.
* How to interact with hardware peripherals for input/output.
  * Raising and handling traps.
* User versus supervisor mode and privileged instructions.
* Memory-management unit and support for virtual memory.
* Floating-point arithmetic.
* Simulation of a memory-hierarchy, including appropriate latency-hits.
* Calling-convention for C programs.
* Pipelined execution of instructions.
* Multi-core support, including having a well-defined memory-model.
* Vector instructions for multimedia programs.

### CUS

* Frame-buffer for video-display.
  * Text-mode display.
  * Colors and bold/blink/etc. attributes in text-mode display.
  * Support setting different display-modes.
* Keyboard-based input.
  * Support for input-methods.
* Persistent storage support.
* Allow the user to tweak the initial parameters.
  * Memory-size
  * Disk-size
  * Display-resolution

### Bootstrap

* Bootstrap CUS into a usable system within CUSS.

## Monitor

### Core

* Command to inspect memory-locations.
  * Specify how much of the memory-location to inspect.
* Support to dissassemble a particular memory-location.
  * Specify how much of the memory-location to disassemble.
* Support to transfer data in/out of CUSS.
* Start the Monitor only on breakpoints or explicit user-request.

### CLI

* Suppress Monitor-interaction for headless execution.

### GUI

* Fix dangling CUSS on `quit` command.
* Only redraw portions of the display that actually need to be redrawn.

## User-Interface

* Render the frame-buffer from display-memory.
* Support switching between the simulated display and the Monitor.
* Allow switching to full-screen and back.
* Allow the capture and release of keyboard and mouse-events.

## Miscellaneous

* Support headless execution for benchmarking, etc.
* Add a way to write unit-tests for various modules.
* Add a way to write integration-tests for end-to-end testing.
* Maybe add support for CMake-based build-configuration.
