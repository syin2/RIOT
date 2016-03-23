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
#include "net/gnrc/ppp/cp.h"
#include "net/gnrc/ppp/cp_fsm.h"
#include "net/gnrc/ppp/lcp.h"
#include "net/ppp/opt.h"



static void test_lcp_recv_cr_nak(void)
{
	ppp_cp_t lcp;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t nak_pkt[8] = {0x01,0x00,0x00,0x08,0x01,0x04,0x00,0xF1};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(nak_pkt, 8, &cp_pkt);

	uint8_t event;
	event = handle_lcp_pkt(&lcp, &cp_pkt);

	/* In this case, we are expecting an E_RCRm*/
	TEST_ASSERT_EQUAL_INT(E_RCRm, event);
}

static void test_lcp_recv_cr_rej(void)
{
	ppp_cp_t lcp;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t rej_pkt[8] = {0x01,0x00,0x00,0x08,0x05,0x04,0x00,0xF1};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(rej_pkt, 8, &cp_pkt);

	uint8_t event;
	event = handle_lcp_pkt(&lcp, &cp_pkt);

	/* In this case, we are expecting an E_RCRm*/
	TEST_ASSERT_EQUAL_INT(E_RCRm, event);
}

static void test_lcp_recv_cr_ack(void)
{
	ppp_cp_t lcp;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t good_pkt[8] = {0x01,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(good_pkt, 8, &cp_pkt);

	uint8_t event;
	event = handle_lcp_pkt(&lcp, &cp_pkt);

	/* In this case, we are expecting an E_RCRp*/
	TEST_ASSERT_EQUAL_INT(E_RCRp, event);

}

static void test_lcp_recv_ack(void)
{
	/*Make fake ctrl prot*/
	ppp_cp_t lcp;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t good_pkt[8] = {0x02,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(good_pkt, 8, &cp_pkt);


	lcp.cr_sent_identifier = 0;
	memcpy(lcp.cr_sent_opts,good_pkt+4,4);
	lcp.cr_sent_size = 8;

	uint8_t event;
	event = handle_lcp_pkt(&lcp, &cp_pkt);
	
	/* In this case, we are expecting an E_RCA*/
	TEST_ASSERT_EQUAL_INT(E_RCA, event);


	lcp.cr_sent_identifier = 8;
	event = handle_lcp_pkt(&lcp, &cp_pkt);
	/* In this case, we are not expecting an E_RCA*/
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);

	/* Payload mismatch */
	good_pkt[7] = 0;
	lcp.cr_sent_identifier = 0;
	event = handle_lcp_pkt(&fake_prot, &cp_pkt);
	/* In this case, we are not expecting an E_RCA*/
	TEST_ASSERT_EQUAL_INT(E_RCA, event);
}

static void test_lcp_recv_nak(void)
{
	/*Make fake ctrl prot*/
	ppp_cp_t lcp;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t nak_pkt[8] = {0x03,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(nak_pkt, 8, &cp_pkt);

	uint8_t event;
	event = handle_cp_pkt(&fake_prot, &cp_pkt);
	
	TEST_ASSERT_EQUAL_INT(E_RCN, event);
}

Test *tests_gnrc_ppp_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_lcp_recv_cr_ack),
        new_TestFixture(test_lcp_recv_cr_nak),
        new_TestFixture(test_lcp_recv_cr_rej),
        new_TestFixture(test_lcp_recv_ack),
        new_TestFixture(test_lcp_recv_nak),
    };

    EMB_UNIT_TESTCALLER(gnrc_ppp_tests, NULL, NULL, fixtures);

    return (Test *)&gnrc_ppp_tests;
}

void tests_gnrc_ppp(void)
{
    TESTS_RUN(tests_gnrc_ppp_tests());
}
/** @} */
