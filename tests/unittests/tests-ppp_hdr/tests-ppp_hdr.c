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

static void test_hello(void)
{
TEST_ASSERT_EQUAL_INT(1,1);	
}

Test *tests_ppp_hdr_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_hello),
    };

    EMB_UNIT_TESTCALLER(ppp_hdr_tests, NULL, NULL, fixtures);

    return (Test *)&ppp_hdr_tests;
}

void tests_ppp_hdr(void)
{
    TESTS_RUN(tests_ppp_hdr_tests());
}
/** @} */
