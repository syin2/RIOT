/*
 * Copyright (C) 2014 José Ignacio Alamos <jialamos@uc.cl>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "embUnit.h"

#include "unittests-constants.h"
#include "tests-ppp_hdr.h"
#include "net/ppp/hdr.h"

#define PPP_CODE (TEST_UINT8)
#define PPP_ID (TEST_UINT8 >> 1)
#define PPP_LENGTH (TEST_UINT16)
#define OTHER_BYTE (TEST_UINT8 >> 4)
#define OTHER_U16 (TEST_UINT16 >> 4)

static void test_ppp_hdr_get_code(void)
{
    uint8_t val[] = { PPP_CODE };

    TEST_ASSERT_EQUAL_INT(PPP_CODE, ppp_hdr_get_code((ppp_hdr_t *) val));
}

static void test_ppp_hdr_set_code(void)
{
    uint8_t val[] = { OTHER_BYTE };

    ppp_hdr_set_code((ppp_hdr_t *) val, PPP_CODE);

    TEST_ASSERT_EQUAL_INT(PPP_CODE, val[0]);
}

static void test_ppp_hdr_get_id(void)
{
    uint8_t val[] = { PPP_CODE, PPP_ID };

    TEST_ASSERT_EQUAL_INT(PPP_ID, ppp_hdr_get_id((ppp_hdr_t *) val));
}

static void test_ppp_hdr_set_id(void)
{
    uint8_t val[] = { TEST_UINT8, PPP_ID };

    ppp_hdr_set_id((ppp_hdr_t *) val, PPP_ID);

    TEST_ASSERT_EQUAL_INT(PPP_ID, val[1]);
}

static void test_ppp_hdr_get_length(void)
{
    uint8_t val[] = { PPP_CODE, PPP_ID, PPP_LENGTH >> 8, PPP_LENGTH & 0x00FF };

    TEST_ASSERT_EQUAL_INT(PPP_LENGTH, ppp_hdr_get_length((ppp_hdr_t *) val));
}

static void test_ppp_hdr_set_length(void)
{
    uint8_t val[] = { PPP_CODE, PPP_ID, OTHER_U16 >> 8, OTHER_U16  & 0x00FF };

    ppp_hdr_set_length((ppp_hdr_t *) val, PPP_LENGTH);

    TEST_ASSERT_EQUAL_INT(PPP_LENGTH >> 8, val[2]);
    TEST_ASSERT_EQUAL_INT(PPP_LENGTH & 0x00FF, val[3]);
}


Test *tests_ppp_hdr_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_ppp_hdr_get_code),
        new_TestFixture(test_ppp_hdr_set_code),
        new_TestFixture(test_ppp_hdr_get_id),
        new_TestFixture(test_ppp_hdr_set_id),
        new_TestFixture(test_ppp_hdr_get_length),
        new_TestFixture(test_ppp_hdr_set_length),
    };

    EMB_UNIT_TESTCALLER(ppp_hdr_tests, NULL, NULL, fixtures);

    return (Test *)&ppp_hdr_tests;
}


void tests_ppp_hdr(void)
{
    TESTS_RUN(tests_ppp_hdr_tests());
}
/** @} */
