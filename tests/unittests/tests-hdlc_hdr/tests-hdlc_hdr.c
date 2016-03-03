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

#include "net/hdlc/hdr.h"

#include "unittests-constants.h"
#include "tests-hdlc_hdr.h"


static void test_hdlc_hdr_set_get_address(void)
{
	hdlc_hdr_t hdr;
	hdlc_hdr_set_address(&hdr,0xFF);

	TEST_ASSERT_EQUAL_INT(0xFF, hdr.address);
	TEST_ASSERT_EQUAL_INT(0xFF, hdlc_hdr_get_address(&hdr));
}

static void test_hdlc_hdr_set_get_protocol(void)
{
	hdlc_hdr_t hdr;

	//Test with dummy protocol
	uint16_t protocol = 15;

	hdlc_hdr_set_protocol(&hdr, protocol);
	TEST_ASSERT_EQUAL_INT(protocol, hdr.protocol);
	TEST_ASSERT_EQUAL_INT(protocol, hdlc_hdr_get_protocol(&hdr));
}


Test *tests_hdlc_hdr_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_hdlc_hdr_set_get_address),
        new_TestFixture(test_hdlc_hdr_set_get_protocol),
    };

    EMB_UNIT_TESTCALLER(hdlc_hdr_tests, NULL, NULL, fixtures);

    return (Test *)&hdlc_hdr_tests;
}

void tests_hdlc_hdr(void)
{
    TESTS_RUN(tests_hdlc_hdr_tests());
}
/** @} */
