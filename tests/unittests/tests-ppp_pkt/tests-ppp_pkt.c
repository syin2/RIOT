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
#include "tests-ppp_pkt.h"
#include "net/ppp/pkt.h"
#if 0
/* Dummy get_option_status for testing fake prot*/
static int fakeprot_get_option_status(cp_opt_t *opt)
{
	/* if type < 2, reject */
	if (opt->type < 2)
	{
		return CP_CREQ_REJ;
	}
	
	/* Nak every packet with u16 payload < 10 */
	uint16_t u16 = (*(opt->payload)<<8) + *(opt->payload+1);
	if (u16 < 10)
	{
		return CP_CREQ_NAK;
	}
	return CP_CREQ_ACK;

}
#endif

static void test_ppp_pkt_populate(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 8;
	uint8_t pkt[8] = {code,id,0x00,length,0x01,0x04,0x00,0x01};
	cp_pkt_t *cp_pkt;

	printf("Size of header%i\n", (int)sizeof(cp_hdr_t));
	printf("Size of pkt %i\n", (int)sizeof(cp_pkt_t));
	printf("Size of uint8_t* %i\n", (int)sizeof(uint8_t*));
	cp_pkt = ppp_pkt_populate(pkt, 8);
	

	TEST_ASSERT_EQUAL_INT(code, cp_pkt->hdr.code);
	/*
	TEST_ASSERT_EQUAL_INT(id, cp_pkt->hdr.id);
	TEST_ASSERT_EQUAL_INT(length, byteorder_ntohs(cp_pkt->hdr.length));
	TEST_ASSERT_EQUAL_INT(length, ppp_pkt_get_length(cp_pkt));
	
	TEST_ASSERT_EQUAL_INT(0,memcmp(pkt+4,cp_pkt->payload,4));
	*/
}

#if 0
static void test_ppp_pkt_get_set_code(void)
{
	cp_pkt_t cp_pkt;
	uint8_t code=33;
	ppp_pkt_set_code(&cp_pkt, code);

	TEST_ASSERT_EQUAL_INT(code, cp_pkt.hdr.code);
	TEST_ASSERT_EQUAL_INT(code, ppp_pkt_get_code(&cp_pkt));
}

static void test_ppp_pkt_get_set_id(void)
{
	cp_pkt_t cp_pkt;
	uint8_t id=13;
	ppp_pkt_set_id(&cp_pkt, id);

	TEST_ASSERT_EQUAL_INT(id, cp_pkt.hdr.id);
	TEST_ASSERT_EQUAL_INT(id, ppp_pkt_get_id(&cp_pkt));
}

static void test_ppp_pkt_get_set_length(void)
{
	cp_pkt_t cp_pkt;
	uint16_t length=13;
	ppp_pkt_set_length(&cp_pkt, length);

	TEST_ASSERT_EQUAL_INT(length, byteorder_ntohs(cp_pkt.hdr.length));
	TEST_ASSERT_EQUAL_INT(length, ppp_pkt_get_length(&cp_pkt));
}

static void test_ppp_pkt_get_set_payload(void)
{
	cp_pkt_t cp_pkt;
	uint8_t payload[5] = {'h','e','l','l','o'};

	memcpy(cp_pkt.payload, payload, 5);
	uint8_t *p = cp_pkt.payload;
	TEST_ASSERT_EQUAL_INT(0, memcmp(p,payload,5));
}
#endif
Test *tests_ppp_pkt_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_ppp_pkt_populate),
    };

    EMB_UNIT_TESTCALLER(ppp_pkt_tests, NULL, NULL, fixtures);

    return (Test *)&ppp_pkt_tests;
}


void tests_ppp_pkt(void)
{
    TESTS_RUN(tests_ppp_pkt_tests());
}
/** @} */
