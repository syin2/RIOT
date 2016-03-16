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
#include "net/gnrc/ppp/ppp.h"
#include "net/gnrc/ppp/lcp.h"
#include "net/gnrc.h"

static void test_gnrc_ppp_lcp_recv_cr_nak(void)
{
	printf("Hello");
	/*Make fake ctrl prot*/
	ppp_cp_t fake_prot;

	/* Same packet, wrong value of MRU*/
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t nak_packet[8] = {0x01,0x00,0x00,0x08,0x01,0x04,0xFF,0xFF};

	/* Test nak packet */
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL,nak_packet,8,GNRC_NETTYPE_TEST);

	test_handle_cp_rcr(&fake_prot, pkt);
	gnrc_pktbuf_release(pkt);

	/* Event should be RCRp */
	TEST_ASSERT_EQUAL_INT(E_RCRm,fake_prot.event);
	/* Response should be ACK */
	TEST_ASSERT_EQUAL_INT(1, (fake_prot.outgoing_opts.content_flag & OPT_HAS_NAK) > 0);
	/* There should be only 1 options in opt_buffer*/
	TEST_ASSERT_EQUAL_INT(1, fake_prot.outgoing_opts.num_opts);
	/* And that option should have the right values */
	TEST_ASSERT_EQUAL_INT(LCP_OPT_MRU, fake_prot.outgoing_opts._opt_buf[0].type);
	TEST_ASSERT_EQUAL_INT(0xFF, fake_prot.outgoing_opts._opt_buf[0].payload[0]);
	TEST_ASSERT_EQUAL_INT(0xFF, fake_prot.outgoing_opts._opt_buf[0].payload[1]);
	TEST_ASSERT_EQUAL_INT(2, (int) fake_prot.outgoing_opts._opt_buf[0].p_size);
}

static void test_gnrc_ppp_lcp_recv_cr_rej(void)
{
	/*Make fake ctrl prot*/
	ppp_cp_t fake_prot;

	/* Packet with unrecognized lcp opt type */
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t rej_packet[8] = {0x01,0x00,0x00,0x08,0x02,0x04,0xFF,0xFF};

	/* Test rej packet */
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL,rej_packet,8,GNRC_NETTYPE_TEST);

	test_handle_cp_rcr(&fake_prot, pkt);
	gnrc_pktbuf_release(pkt);

	/* Event should be RCRp */
	TEST_ASSERT_EQUAL_INT(E_RCRm,fake_prot.event);
	/* Response should be ACK */
	TEST_ASSERT_EQUAL_INT(1, (fake_prot.outgoing_opts.content_flag & OPT_HAS_REJ) > 0);
	/* There should be only 1 options in opt_buffer*/
	TEST_ASSERT_EQUAL_INT(1, fake_prot.outgoing_opts.num_opts);
	/* And that option should have the right values */
	TEST_ASSERT_EQUAL_INT(2, fake_prot.outgoing_opts._opt_buf[0].type);
	TEST_ASSERT_EQUAL_INT(0xFF, fake_prot.outgoing_opts._opt_buf[0].payload[0]);
	TEST_ASSERT_EQUAL_INT(0xFF, fake_prot.outgoing_opts._opt_buf[0].payload[1]);
	TEST_ASSERT_EQUAL_INT(2, (int) fake_prot.outgoing_opts._opt_buf[0].p_size);

}

/* Dummy get_option_status for testing fake prot*/
static int *fakeprot_get_option_status(cp_opt_t *opt)
{
	/* if type < 2, reject */
	if (opt->type < 2)
	{
		return CP_CREQ_REJ;
	}
	
	/* Nak every packet with u16 payload < 10 */
	uint16_t u16 = *(opt->payload)<<8 + *(opt->payload+1)
	if (u16 < 10)
	{
		return CP_CREQ_NAK;
	}
	return CP_CREQ_ACK;

}
static void test_gnrc_ppp_lcp_recv_cr_ack(void)
{
	/*Make fake ctrl prot*/
	ppp_cp_t fake_prot;



	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t good_packet[8] = {0x01,0x00,0x00,0x08,0x01,0x04,0x00,0x01};

	/* Test good configure request packet */
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL,good_packet,8,GNRC_NETTYPE_TEST);
	TEST_ASSERT_NOT_NULL(pkt);

	test_handle_cp_rcr(&fake_prot, pkt);
	gnrc_pktbuf_release(pkt);

	/* Event should be RCRp */
	TEST_ASSERT_EQUAL_INT(E_RCRp,fake_prot.event);
	/* Response should be ACK */
	TEST_ASSERT_EQUAL_INT(1, (fake_prot.outgoing_opts.content_flag & OPT_HAS_ACK) > 0);
	/* There should be only 1 options in opt_buffer*/
	TEST_ASSERT_EQUAL_INT(1, fake_prot.outgoing_opts.num_opts);
	/* And that option should have the right values */
	TEST_ASSERT_EQUAL_INT(LCP_OPT_MRU, fake_prot.outgoing_opts._opt_buf[0].type);
	TEST_ASSERT_EQUAL_INT(0, fake_prot.outgoing_opts._opt_buf[0].payload[0]);
	TEST_ASSERT_EQUAL_INT(1, fake_prot.outgoing_opts._opt_buf[0].payload[1]);
	TEST_ASSERT_EQUAL_INT(2, (int) fake_prot.outgoing_opts._opt_buf[0].p_size);
}


Test *tests_gnrc_ppp_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_gnrc_ppp_lcp_recv_cr_ack),
        new_TestFixture(test_gnrc_ppp_lcp_recv_cr_nak),
        new_TestFixture(test_gnrc_ppp_lcp_recv_cr_rej),
    };

    EMB_UNIT_TESTCALLER(gnrc_ppp_tests, NULL, NULL, fixtures);

    return (Test *)&gnrc_ppp_tests;
}

void tests_gnrc_ppp(void)
{
    gnrc_pktbuf_init();
    TESTS_RUN(tests_gnrc_ppp_tests());
}
/** @} */
