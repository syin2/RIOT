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
	uint8_t cp[11] = "test_data\0\0";
	int len = 9;

	trialfcs = hdlc_fcs16( PPPINITFCS16, &cp[0], len );
	trialfcs ^= 0xffff;                 /* complement */
	cp[len] = (trialfcs & 0x00ff);      /* least significant byte first */
	cp[len+1] = ((trialfcs >> 8) & 0x00ff);

	trialfcs = hdlc_fcs16( PPPINITFCS16, &cp[0], len + 2 );

	TEST_ASSERT_EQUAL_INT(PPPGOODFCS16, trialfcs);
}
static void test_hdlc_fcs32_checksum(void)
{
	uint32_t trialfcs;

	uint8_t cp[13] = "test_data\0\0";
	int len = 9;

	/* add on output */
	trialfcs = hdlc_fcs32( PPPINITFCS32, &cp[0], len );
	trialfcs ^= 0xffffffff;             /* complement */
	cp[len] = (trialfcs & 0x00ff);      /* least significant byte first */
	cp[len+1] = ((trialfcs >>= 8) & 0x00ff);
	cp[len+2] = ((trialfcs >>= 8) & 0x00ff);
	cp[len+3] = ((trialfcs >> 8) & 0x00ff);

	/* check on input */
	trialfcs = hdlc_fcs32( PPPINITFCS32, &cp[0], len + 4 );
	TEST_ASSERT_EQUAL_INT(PPPGOODFCS32, trialfcs);

}

Test *tests_hdlc_fcs_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_hdlc_fcs_checksum),
        new_TestFixture(test_hdlc_fcs32_checksum),
    };

    EMB_UNIT_TESTCALLER(hdlc_fcs_tests, NULL, NULL, fixtures);

    return (Test *)&hdlc_fcs_tests;
}

void tests_hdlc_fcs(void)
{
    TESTS_RUN(tests_hdlc_fcs_tests());
}
