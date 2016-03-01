/*
 * Copyright (C) 2014 Martine Lenders <mlenders@inf.fu-berlin.de>
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

#include "embUnit.h"

#include "net/ppp/hdr.h"

#include "unittests-constants.h"
#include "tests-ppp_hdr.h"

static void test_ppp_hdr_set_get_version(void)
{
	ppp_hdr_t hdr;
	ppp_hdr_set_flag(&hdr);

	TEST_ASSERT_EQUAL_INT(0x7E, hdr.flag);	
	TEST_ASSERT_EQUAL_INT(0x7E, ppp_hdr_get_flag(&hdr));	
}

static void test_ppp_hdr_set_get_address(void)
{
	ppp_hdr_t hdr;
	ppp_hdr_set_address(&hdr);

	TEST_ASSERT_EQUAL_INT(0xFF, hdr.address);
	TEST_ASSERT_EQUAL_INT(0xFF, ppp_hdr_get_address(&hdr));
}

static void test_ppp_hdr_set_get_protocol(void)
{
	ppp_hdr_t hdr;

	//Test with dummy protocol
	uint16_t protocol = 15;

	ppp_hdr_set_protocol(&hdr, protocol);
	TEST_ASSERT_EQUAL_INT(protocol, hdr.protocol);
	TEST_ASSERT_EQUAL_INT(protocol, ppp_hdr_get_protocol(&hdr));
}

static void test_ppp_hdr_set_get_fcs(void)
{
	ppp_hdr_t hdr;

	uint16_t fcs_1, fcs_2;
	fcs_1 = 0xFFFF;
	fcs_2 = 0xFFFF;

	ppp_hdr_set_fcs(&hdr, fcs_1, fcs_2);

	TEST_ASSERT_EQUAL_INT(0xFFFFFFFF, hdr.fcs.u32);
}

Test *tests_ppp_hdr_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_ppp_hdr_set_get_version),
        new_TestFixture(test_ppp_hdr_set_get_address),
        new_TestFixture(test_ppp_hdr_set_get_protocol),
        new_TestFixture(test_ppp_hdr_set_get_fcs),
    };

    EMB_UNIT_TESTCALLER(ppp_hdr_tests, NULL, NULL, fixtures);

    return (Test *)&ppp_hdr_tests;
}

void tests_ppp_hdr(void)
{
    TESTS_RUN(tests_ppp_hdr_tests());
}
/** @} */
