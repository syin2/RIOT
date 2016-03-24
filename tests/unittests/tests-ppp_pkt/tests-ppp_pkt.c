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
#include "net/ppp/opt.h"

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
	
	TEST_ASSERT_EQUAL_INT(0,memcmp(pkt+4,cp_pkt._buf._payload+4,4));

	/* Try to init a pkt with less than 4 bytes */
	int res;
	res = ppp_pkt_init(pkt, 2, &cp_pkt);
	TEST_ASSERT_EQUAL_INT(-ENOMEM, res);
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
	int res;
	res = ppp_pkt_set_payload(&cp_pkt, new_payload, 4);

	TEST_ASSERT_EQUAL_INT(0, memcmp(ppp_pkt_get_payload(&cp_pkt),new_payload,4));
	TEST_ASSERT_EQUAL_INT(0, res);

	/* Try overflow */
	res = ppp_pkt_set_payload(&cp_pkt, new_payload, 90);
	TEST_ASSERT_EQUAL_INT(-ENOMEM, res);
}

static void test_ppp_opts_init(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 20;
	uint8_t pkt[20] = {code,id,0x00,length,0x01,0x04,0x00,0x01,0x02,0x04,0x02,0x01,0x03,0x04,0x00,0x01,0x04,0x04,0xF0,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 20, &cp_pkt);

	opt_list_t opt_list;
	int status = ppp_opts_init(&opt_list, &cp_pkt);
	TEST_ASSERT_EQUAL_INT(0, status);

	TEST_ASSERT_EQUAL_INT(4, opt_list.num);
	TEST_ASSERT_EQUAL_INT(0, opt_list._co);
	TEST_ASSERT_EQUAL_INT(1, opt_list.head == (pkt+4));
	TEST_ASSERT_EQUAL_INT(1, opt_list.head == opt_list.current);

	/* Send bad pkt                                  v           */
	uint8_t bad_pkt[8] = {0x01,0x00,0x00,0x08,0x01,0x06,0x00,0x01};
	ppp_pkt_init(bad_pkt, 8, &cp_pkt);
	status = ppp_opts_init(&opt_list, &cp_pkt);

	TEST_ASSERT_EQUAL_INT(-1, status);
}

static void test_ppp_opts_get_head(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 20;
	uint8_t pkt[20] = {code,id,0x00,length,0x01,0x04,0x00,0x01,0x02,0x04,0x02,0x01,0x03,0x04,0x00,0x01,0x04,0x04,0xF0,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 20, &cp_pkt);

	opt_list_t opt_list;
	ppp_opts_init(&opt_list, &cp_pkt);

	/* Get head */
	TEST_ASSERT_EQUAL_INT(1, ppp_opts_get_head(&opt_list) == pkt+4);
}

static void test_ppp_opts_reset(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 20;
	uint8_t pkt[20] = {code,id,0x00,length,0x01,0x04,0x00,0x01,0x02,0x04,0x02,0x01,0x03,0x04,0x00,0x01,0x04,0x04,0xF0,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 20, &cp_pkt);

	opt_list_t opt_list;
	ppp_opts_init(&opt_list, &cp_pkt);

	opt_list.head = pkt+8;
	ppp_opts_reset(&opt_list);
	TEST_ASSERT_EQUAL_INT(1, opt_list.current == opt_list.head);
}

static void test_ppp_opts_next(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 21;
	uint8_t pkt[21] = {code,id,0x00,length,0x01,0x04,0x00,0x01,0x02,0x04,0x02,0x01,0x03,0x05,0x00,0x00,0x01,0x04,0x04,0xF0,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 21, &cp_pkt);

	opt_list_t opt_list;
	ppp_opts_init(&opt_list, &cp_pkt);

	void *p;
	p = ppp_opts_next(&opt_list);
	TEST_ASSERT_EQUAL_INT(1, p == pkt+8);
	p = ppp_opts_next(&opt_list);
	TEST_ASSERT_EQUAL_INT(1, p == pkt+12);
	p = ppp_opts_next(&opt_list);
	TEST_ASSERT_EQUAL_INT(1, p == pkt+17);
	p = ppp_opts_next(&opt_list);
	TEST_ASSERT_NULL(p);
}

static void test_ppp_opt_get_type(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 21;
	uint8_t pkt[21] = {code,id,0x00,length,0x01,0x04,0x00,0x01,0x02,0x04,0x02,0x01,0x03,0x05,0x00,0x00,0x01,0x04,0x04,0xF0,0x01};
	
	TEST_ASSERT_EQUAL_INT(1,ppp_opt_get_type((void*) (pkt+4)));
}

static void test_ppp_opt_get_length(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 21;
	uint8_t pkt[21] = {code,id,0x00,length,0x01,0x04,0x00,0x01,0x02,0x04,0x02,0x01,0x03,0x05,0x00,0x00,0x01,0x04,0x04,0xF0,0x01};
	
	TEST_ASSERT_EQUAL_INT(4,ppp_opt_get_length((void*) (pkt+4)));
}

static void test_ppp_opt_get_payload(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 21;
	uint8_t pkt[21] = {code,id,0x00,length,0x01,0x04,0x00,0x01,0x02,0x04,0x02,0x01,0x03,0x05,0x00,0x00,0x01,0x04,0x04,0xF0,0x01};
	
	TEST_ASSERT_EQUAL_INT(1,ppp_opt_get_payload((void*) (pkt+4)) == (void*)(pkt+6));
}

static void test_ppp_opts_get_num(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 21;
	uint8_t pkt[21] = {code,id,0x00,length,0x01,0x04,0x00,0x01,0x02,0x04,0x02,0x01,0x03,0x05,0x00,0x00,0x01,0x04,0x04,0xF0,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 21, &cp_pkt);

	opt_list_t opt_list;
	ppp_opts_init(&opt_list, &cp_pkt);

	TEST_ASSERT_EQUAL_INT(4, ppp_opts_get_num(&opt_list));
}

static void test_ppp_opts_get_opt_num(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 21;
	uint8_t pkt[21] = {code,id,0x00,length,0x01,0x04,0x00,0x01,0x02,0x04,0x02,0x01,0x03,0x05,0x00,0x00,0x01,0x04,0x04,0xF0,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 21, &cp_pkt);

	opt_list_t opt_list;
	ppp_opts_init(&opt_list, &cp_pkt);

	ppp_opts_next(&opt_list);

	TEST_ASSERT_EQUAL_INT(1, ppp_opts_get_opt_num(&opt_list));
}

Test *tests_ppp_pkt_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_ppp_pkt_init),
        new_TestFixture(test_ppp_pkt_get_set_code),
        new_TestFixture(test_ppp_pkt_get_set_id),
        new_TestFixture(test_ppp_pkt_get_set_length),
        new_TestFixture(test_ppp_pkt_get_set_payload),
        new_TestFixture(test_ppp_opts_init),
        new_TestFixture(test_ppp_opts_get_head),
        new_TestFixture(test_ppp_opts_reset),
        new_TestFixture(test_ppp_opts_next),
        new_TestFixture(test_ppp_opt_get_type),
        new_TestFixture(test_ppp_opt_get_length),
        new_TestFixture(test_ppp_opt_get_payload),
        new_TestFixture(test_ppp_opts_get_num),
        new_TestFixture(test_ppp_opts_get_opt_num),
    };

    EMB_UNIT_TESTCALLER(ppp_pkt_tests, NULL, NULL, fixtures);

    return (Test *)&ppp_pkt_tests;
}


void tests_ppp_pkt(void)
{
    TESTS_RUN(tests_ppp_pkt_tests());
}
/** @} */
