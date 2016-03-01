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
#include <stdio.h>


#include "embUnit.h"

#include "net/hdlc/fcs.h"

#include "unittests-constants.h"
#include "tests-hdlc_fcs.h"

#define PPPINITFCS16    0xffff  /* Initial FCS16 value */
#define PPPGOODFCS16    0xf0b8  /* Good final FCS16 value */

#define PPPINITFCS32  0xffffffff   /* Initial FCS32 value */
#define PPPGOODFCS32  0xdebb20e3   /* Good final FCS32 value */

//Adapted test from RFC1662
static void test_hdlc_fcs_checksum(void)
{
	uint16_t trialfcs;
	uint8_t cp[11] = "test_dataxx";
	int len = 9;
	uint8_t *p = cp;

	trialfcs = PPPINITFCS16;
	while(len--)
		trialfcs = fcs16_bit( trialfcs, *p++);
	trialfcs ^= 0xffff;                 /* complement */
	cp[9] = (trialfcs & 0x00ff);      /* least significant byte first */
	cp[10] = ((trialfcs >> 8) & 0x00ff);

	p = cp;
	len = 11;
	trialfcs = PPPINITFCS16;
	while(len--)
		trialfcs = fcs16_bit( trialfcs, *p++);

	TEST_ASSERT_EQUAL_INT(PPPGOODFCS16, trialfcs);
}

Test *tests_hdlc_fcs_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_hdlc_fcs_checksum),
    };

    EMB_UNIT_TESTCALLER(hdlc_fcs_tests, NULL, NULL, fixtures);

    return (Test *)&hdlc_fcs_tests;
}

void tests_hdlc_fcs(void)
{
    TESTS_RUN(tests_hdlc_fcs_tests());
}
