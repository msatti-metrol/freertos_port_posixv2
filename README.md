# FreeRTOS Port PosixV2

This is a reimplementation of the POSIX port of FreeRTOS, heavily based on the original.

Tested under Cygwin (3.5.7-1.x86_64) and Ubuntu Linux (24.10 via WSL2).

## Changes

- Builds and runs on Cygwin without issue.
- Fixed a bug where upon starting a tick could be lost due to signals / timing issues.

## License

This work is licensed under the MIT license, but some portions are licensed under a different license. See `LICENSE.md` for details.

`SPDX-License-Identifier: MIT`
