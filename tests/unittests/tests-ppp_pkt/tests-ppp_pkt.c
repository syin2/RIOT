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

static void test_ppp_pkt_init(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 8;
	uint8_t pkt[8] = {code,id,0x00,length,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;

	ppp_pkt_init(pkt, 8, &cp_pkt);
	

	TEST_ASSERT_EQUAL_INT(code, cp_pkt.hdr->code);
	TEST_ASSERT_EQUAL_INT(id, cp_pkt.hdr->id);
	TEST_ASSERT_EQUAL_INT(length, byteorder_ntohs(cp_pkt.hdr->length));
	TEST_ASSERT_EQUAL_INT(length, ppp_pkt_get_length(&cp_pkt));
	
	TEST_ASSERT_EQUAL_INT(0,memcmp(pkt+4,cp_pkt.payload,4));
}

static void test_ppp_pkt_get_set_code(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 8;
	uint8_t pkt[8] = {code,id,0x00,length,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 8, &cp_pkt);

	uint8_t new_code = 47;

	ppp_pkt_set_code(&cp_pkt, new_code);

	TEST_ASSERT_EQUAL_INT(new_code, cp_pkt.hdr->code);
	TEST_ASSERT_EQUAL_INT(new_code, ppp_pkt_get_code(&cp_pkt));
}

static void test_ppp_pkt_get_set_id(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 8;
	uint8_t pkt[8] = {code,id,0x00,length,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 8, &cp_pkt);

	uint8_t new_id=13;
	ppp_pkt_set_id(&cp_pkt, new_id);

	TEST_ASSERT_EQUAL_INT(new_id, cp_pkt.hdr->id);
	TEST_ASSERT_EQUAL_INT(new_id, ppp_pkt_get_id(&cp_pkt));
}

static void test_ppp_pkt_get_set_length(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 8;
	uint8_t pkt[8] = {code,id,0x00,length,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 8, &cp_pkt);

	uint16_t new_length=13;
	ppp_pkt_set_length(&cp_pkt, new_length);

	TEST_ASSERT_EQUAL_INT(new_length, byteorder_ntohs(cp_pkt.hdr->length));
	TEST_ASSERT_EQUAL_INT(new_length, ppp_pkt_get_length(&cp_pkt));
	printf("Finished length\n");
}

static void test_ppp_pkt_get_set_payload(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 8;
	uint8_t pkt[8] = {code,id,0x00,length,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 8, &cp_pkt);

	uint8_t new_payload[4]={'h','o','l','a'};
	ppp_pkt_set_payload(&cp_pkt, new_payload, 4);

	TEST_ASSERT_EQUAL_INT(0, memcmp(ppp_pkt_get_payload(&cp_pkt),new_payload,4));
	printf("Finished length\n");
}


Test *tests_ppp_pkt_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_ppp_pkt_init),
        new_TestFixture(test_ppp_pkt_get_set_code),
        new_TestFixture(test_ppp_pkt_get_set_id),
        new_TestFixture(test_ppp_pkt_get_set_length),
        new_TestFixture(test_ppp_pkt_get_set_payload),
    };

    EMB_UNIT_TESTCALLER(ppp_pkt_tests, NULL, NULL, fixtures);

    return (Test *)&ppp_pkt_tests;
}


void tests_ppp_pkt(void)
{
    TESTS_RUN(tests_ppp_pkt_tests());
}
/** @} */