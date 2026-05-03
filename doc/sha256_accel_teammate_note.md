# SHA-256 Accelerator Progress Note

Hi,

Here is the current state of the SHA-256 accelerator work in this Croc repo, so we can both start from the same point.

## What Is Implemented

- `rtl/sha256/sha256_accel.sv`
  SHA-256 accelerator in the user domain.
  It hashes one already padded 512-bit block per `START` command and now also supports chained multi-block hashing through a `CHAIN` command bit.

- `rtl/sha256/sha256_accel_pkg.sv`
  Register offsets and basic package constants for the accelerator.

- `rtl/user_pkg.sv`
  Maps the accelerator into the user-domain address space at `0x2000_1000`.

- `rtl/user_domain.sv`
  Instantiates the accelerator and routes its interrupt to `interrupts_o[0]`.

- `sw/lib/inc/sha256_accel.h`
- `sw/lib/src/sha256_accel.c`
  Small software driver layer for the MMIO interface, including helper functions for chained multi-block hashing.

- `sw/test/test_sha256_accel.c`
  Self-test for the one-block `"abc"` SHA-256 test vector.

- `sw/test/test_sha256_accel_multiblock.c`
  Self-test for the standard two-block SHA-256 message
  `"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"`.

- `sw/test/test_sha256_accel_bench.c`
  Small cycle benchmark that compares the accelerator against a plain C SHA-256 baseline on the one-block `"abc"` case.

## Current Programming Model

Base address: `0x2000_1000`

Register layout:

| Offset | Name | Notes |
|---|---|---|
| `0x00` | `CTRL` | bit 0 = `START`, bit 1 = `IRQ_EN`, bit 2 = `CHAIN` |
| `0x04` | `STATUS` | bit 0 = `READY`, bit 1 = `BUSY`, bit 2 = `DONE`, bit 3 = `DIGEST_VALID`, bit 4 = `IRQ_PENDING`, bit 5 = `START_ERR` |
| `0x08` | `INFO` | version/info word |
| `0x40` - `0x7C` | `BLOCK[0..15]` | 16 x 32-bit words for one 512-bit message block |
| `0x80` - `0x9C` | `DIGEST[0..7]` | 8 x 32-bit digest words |

Current assumptions:

- software pads the message
- hardware performs one SHA-256 compression over 64 rounds per command
- a plain `START` begins from the standard SHA-256 IV
- `START | CHAIN` continues from the previous digest state
- hardware exposes the current digest via `DIGEST[0..7]`

## Verification Status

The accelerator passes the current Verilator testbench end-to-end.

Passing test:

- `sw/test/test_sha256_accel.c`
- `sw/test/test_sha256_accel_multiblock.c`

Current known-good result:

- `verilator/run_verilator.sh --run ../sw/bin/test/test_sha256_accel.hex`
- `verilator/run_verilator.sh --run ../sw/bin/test/test_sha256_accel_multiblock.hex`
- `verilator/run_verilator.sh --run ../sw/bin/test/test_sha256_accel_bench.hex`
- simulator prints `Simulation finished: SUCCESS`
- after the same `crt0.S` startup fix, `test_soc_ctrl.hex` also passes in Verilator, so the bare-metal startup path looks healthy beyond the SHA-specific test

Current benchmark result on the one-block `"abc"` case (`64` iterations, pre-padded block):

- hardware cycles per hash: `0x183` = `387`
- software cycles per hash: `0x15C1` = `5569`
- measured speedup: about `14.4x`

## Local Flow That Worked

We got the verification flow working in native `Ubuntu-22.04` under WSL, not through Docker.

Useful commands:

```sh
cd /mnt/c/Users/20221478/Desktop/vlsi2/croc
bender checkout
```

```sh
cd /mnt/c/Users/20221478/Desktop/vlsi2/croc/sw
make RISCV_MARCH=rv32i RISCV_LIBDIR=/usr/lib/picolibc/riscv64-unknown-elf/lib/rv32i/ilp32 \
  bin/test/test_sha256_accel.hex
```

```sh
cd /mnt/c/Users/20221478/Desktop/vlsi2/croc/verilator
./run_verilator.sh --flist --build
./run_verilator.sh --run ../sw/bin/test/test_sha256_accel.hex
```

## Repo/Environment Notes

- `sw/Makefile` was extended with optional `RISCV_LIBDIR` support so the Ubuntu bare-metal libraries can link cleanly.
- `sw/Makefile` now also uses `-ffunction-sections`, `-fdata-sections`, and `--gc-sections` so larger tests still fit in the small SRAM image.
- `sw/crt0.S` now keeps both `sp` and `gp` setup under `.option norelax`.
  Without that, the linker relaxed `la sp, __stack_pointer$` into a `gp`-relative sequence before `gp` was initialized, which caused the core to jump into garbage immediately after resume in Verilator.
- `env.sh` and `verilator/run_verilator.sh` had their line endings normalized to LF so they execute correctly inside WSL.
- Verilator `5.048` is installed in the working WSL environment. The older Ubuntu package was too old for this Croc testbench.

## Important Debugging Note

One annoying simulation quirk came up in the software test:

- the polled MMIO `status` variable in `test_sha256_accel.c` should stay a normal `uint32_t`
- when it was forced through a `volatile` local stack slot, the first status read produced a false failure in this setup

Also, for the testbench we currently report pass/fail directly through `SOC_CTRL_CORESTATUS` in the test, instead of relying on a plain `return` path.

That is why `test_sha256_accel.c` contains a local `finish_with_code(...)` helper.

One more subtle bug we already hit:

- if the startup stub in `sw/crt0.S` is relaxed incorrectly, the core can start throwing repeated illegal instructions at `PC 0x00000000`
- the fix is already in the repo, but if startup ever regresses again, `crt0.S` is the first place to inspect

## Suggested Next Steps

1. Decide whether we keep padding in software or move it into hardware for later milestones.
2. Add an interrupt-driven software test, not only polling.
3. Measure cycle count versus a software-only SHA-256 baseline.
4. Decide whether we want explicit state reset/load registers instead of only `START` and `CHAIN`.
5. Once functionality is stable, run synthesis/area checks on the accelerator path.

## Suggested Team Split

Option A:

- one person owns RTL and register-map evolution
- one person owns software driver, tests, and benchmark/demo code

Option B:

- one person owns multi-block hardware support
- one person owns verification, software API, and system-level integration

## Files To Look At First

- `rtl/sha256/sha256_accel.sv`
- `rtl/sha256/sha256_accel_pkg.sv`
- `rtl/user_domain.sv`
- `rtl/user_pkg.sv`
- `sw/lib/inc/sha256_accel.h`
- `sw/lib/src/sha256_accel.c`
- `sw/test/test_sha256_accel.c`
