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
#include "tests-gnrc_ppp_lcp.h"
#include "net/gnrc/ppp/cp.h"
#include "net/gnrc/ppp/lcp.h"
#include "net/ppp/opt.h"



static void test_lcp_recv_rcr(void)
{
	ppp_cp_t lcp;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t good_pkt[8] = {0x01,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	uint8_t rej_pkt[8] = {0x01,0x00,0x00,0x08,0x05,0x04,0x00,0xF1};
	uint8_t nak_pkt[8] = {0x01,0x00,0x00,0x08,0x01,0x04,0xFF,0xFF};

	int event;

	cp_pkt_t cp_good_pkt;
	cp_pkt_t cp_nak_pkt;
	cp_pkt_t cp_rej_pkt;

	ppp_pkt_init(good_pkt, 8, &cp_good_pkt);
	ppp_pkt_init(rej_pkt, 8, &cp_rej_pkt);
	ppp_pkt_init(nak_pkt, 8, &cp_nak_pkt);

	/* Try with a good pkt */
	event = lcp_handle_pkt(&lcp, &cp_good_pkt);
	TEST_ASSERT_EQUAL_INT(E_RCRp, event);
	
	/* Try with a rejected pkt */
	event = lcp_handle_pkt(&lcp, &cp_rej_pkt);
	TEST_ASSERT_EQUAL_INT(E_RCRm, event);

	/* Try with a NAK pkt */
	event = lcp_handle_pkt(&lcp, &cp_nak_pkt);
	TEST_ASSERT_EQUAL_INT(E_RCRm, event);
}

static void test_lcp_recv_ack(void)
{
	ppp_cp_t lcp;

	/* |--ACK--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t good_pkt[8] = {0x02,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	int event;

	cp_pkt_t cp_pkt;
	ppp_pkt_init(good_pkt, 8, &cp_pkt);

	lcp.cr_sent_identifier = 0;
	memcpy(lcp.cr_sent_opts,good_pkt+4,4);
	lcp.cr_sent_size = 8;

	event = lcp_handle_pkt(&lcp, &cp_pkt);
	
	/* In this case, we are expecting an E_RCA*/
	TEST_ASSERT_EQUAL_INT(E_RCA, event);


	lcp.cr_sent_identifier = 8;
	event = lcp_handle_pkt(&lcp, &cp_pkt);
	/* In this case, we are not expecting an E_RCA*/
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);

	/* Payload mismatch */
	good_pkt[7] = 0;
	lcp.cr_sent_identifier = 0;
	event = lcp_handle_pkt(&lcp, &cp_pkt);
	/* In this case, we are not expecting an E_RCA*/
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);
}

static void test_lcp_recv_nakrej(void)
{
	/*Make fake ctrl prot*/
	ppp_cp_t lcp;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t nak_pkt[8] = {0x03,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	uint8_t rej_pkt[8] = {0x04,0x00,0x00,0x08,0x01,0x04,0x00,0x01};

	int event;
	cp_pkt_t cp_nak_pkt;
	cp_pkt_t cp_rej_pkt;
	ppp_pkt_init(nak_pkt, 8, &cp_nak_pkt);
	ppp_pkt_init(rej_pkt, 8, &cp_rej_pkt);

	event = lcp_handle_pkt(&lcp, &cp_nak_pkt);
	TEST_ASSERT_EQUAL_INT(E_RCN, event);

	event = lcp_handle_pkt(&lcp, &cp_rej_pkt);
	TEST_ASSERT_EQUAL_INT(E_RCN, event);
}

Test *tests_gnrc_ppp_lcp_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_lcp_recv_rcr),
        new_TestFixture(test_lcp_recv_ack),
        new_TestFixture(test_lcp_recv_nakrej),
    };

    EMB_UNIT_TESTCALLER(gnrc_ppp_lcp_tests, NULL, NULL, fixtures);

    return (Test *)&gnrc_ppp_lcp_tests;
}

void tests_gnrc_ppp_lcp(void)
{
    TESTS_RUN(tests_gnrc_ppp_lcp_tests());
}
/** @} */
