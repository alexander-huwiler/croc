// Copyright (c) 2026 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Authors:
// - OpenAI Codex

#include "config.h"
#include "sha256_accel.h"
#include "soc_ctrl.h"
#include "util.h"

static const uint32_t empty_msg_blocks[1][16] = {
    {
        0x80000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
    },
};

static const uint32_t empty_msg_digest[8] = {
    0xe3b0c442, 0x98fc1c14, 0x9afbf4c8, 0x996fb924,
    0x27ae41e4, 0x649b934c, 0xa495991b, 0x7852b855,
};

static const uint32_t message_digest_blocks[1][16] = {
    {
        0x6d657373, 0x61676520, 0x64696765, 0x73748000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000070,
    },
};

static const uint32_t message_digest_digest[8] = {
    0xf7846f55, 0xcf23e14e, 0xebeab5b4, 0xe1550cad,
    0x5b509e33, 0x48fbc4ef, 0xa3a1413d, 0x393cb650,
};

static const uint32_t alphabet_blocks[1][16] = {
    {
        0x61626364, 0x65666768, 0x696a6b6c, 0x6d6e6f70,
        0x71727374, 0x75767778, 0x797a8000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x000000d0,
    },
};

static const uint32_t alphabet_digest[8] = {
    0x71c480df, 0x93d6ae2f, 0x1efad144, 0x7c66c952,
    0x5e316218, 0xcf51fc8d, 0x9ed832f2, 0xdaf18b73,
};

static const uint32_t a56_blocks[2][16] = {
    {
        0x61616161, 0x61616161, 0x61616161, 0x61616161,
        0x61616161, 0x61616161, 0x61616161, 0x61616161,
        0x61616161, 0x61616161, 0x61616161, 0x61616161,
        0x61616161, 0x61616161, 0x80000000, 0x00000000,
    },
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x000001c0,
    },
};

static const uint32_t a56_digest[8] = {
    0xb35439a4, 0xac6f0948, 0xb6d6f9e3, 0xc6af0f5f,
    0x590ce20f, 0x1bde7090, 0xef797068, 0x6ec6738a,
};

static void finish_with_code(uint32_t exit_code) {
    *reg32(SOCCTRL_BASE_ADDR, SOC_CTRL_CORESTATUS_REG_OFFSET) = (exit_code << 1) | 1u;
    while (1) {
        wfi();
    }
}

#define CHECK_ASSERT_EOC(ret, cond) \
    do { \
        if (!(cond)) finish_with_code(ret); \
    } while (0)

static uint32_t run_case(const uint32_t *blocks, uint32_t num_blocks,
                         const uint32_t expected_digest[8], uint32_t err_base) {
    uint32_t digest[8];
    uint32_t status = sha256_accel_hash_blocks(blocks, num_blocks, digest);

    CHECK_ASSERT_EOC(err_base + 0, (status & SHA256_ACCEL_STATUS_BUSY_BIT_MASK) == 0u);
    CHECK_ASSERT_EOC(err_base + 1, (status & SHA256_ACCEL_STATUS_DONE_BIT_MASK) != 0u);
    CHECK_ASSERT_EOC(err_base + 2, (status & SHA256_ACCEL_STATUS_DIGEST_VALID_BIT_MASK) != 0u);
    CHECK_ASSERT_EOC(err_base + 3, (status & SHA256_ACCEL_STATUS_START_ERR_BIT_MASK) == 0u);

    for (int i = 0; i < 8; ++i) {
        CHECK_ASSERT_EOC(err_base + 4 + (uint32_t)i, digest[i] == expected_digest[i]);
    }

    sha256_accel_clear_status(
        SHA256_ACCEL_STATUS_DONE_BIT_MASK |
        SHA256_ACCEL_STATUS_DIGEST_VALID_BIT_MASK |
        SHA256_ACCEL_STATUS_IRQ_PENDING_BIT_MASK |
        SHA256_ACCEL_STATUS_START_ERR_BIT_MASK);

    status = sha256_accel_get_status();
    CHECK_ASSERT_EOC(err_base + 12, (status & SHA256_ACCEL_STATUS_DONE_BIT_MASK) == 0u);
    CHECK_ASSERT_EOC(err_base + 13, (status & SHA256_ACCEL_STATUS_DIGEST_VALID_BIT_MASK) == 0u);

    return status;
}

int main() {
    uint32_t status = sha256_accel_get_status();

    CHECK_ASSERT_EOC(1, (status & SHA256_ACCEL_STATUS_READY_BIT_MASK) != 0u);
    CHECK_ASSERT_EOC(2, (status & SHA256_ACCEL_STATUS_BUSY_BIT_MASK) == 0u);

    run_case(&empty_msg_blocks[0][0], 1, empty_msg_digest, 10);
    run_case(&message_digest_blocks[0][0], 1, message_digest_digest, 30);
    run_case(&alphabet_blocks[0][0], 1, alphabet_digest, 50);
    run_case(&a56_blocks[0][0], 2, a56_digest, 70);

    finish_with_code(0);
}
