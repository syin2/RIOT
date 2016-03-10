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

#include "embUnit.h"

#include "unittests-constants.h"
#include "tests-gnrc_ppp.h"


static void test_gnrc_ppp_lcp_recv_pkt(void)
{
	/*TEST_ASSERT_EQUAL_INT(0xFF, hdr.address);*/
}

Test *tests_gnrc_ppp_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_gnrc_ppp_lcp_recv_pkt),
    };

    EMB_UNIT_TESTCALLER(gnrc_ppp_tests, NULL, NULL, fixtures);

    return (Test *)&gnrc_ppp_tests;
}

void tests_gnrc_ppp(void)
{
    TESTS_RUN(tests_gnrc_ppp_tests());
}
/** @} */
