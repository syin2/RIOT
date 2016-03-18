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


/* Dummy get_option_status for testing fake prot*/
static int fakeprot_get_option_status(cp_opt_hdr_t *opt)
{
	/* if type > 2, reject */
	if (opt->type > 2)
	{
		return CP_CREQ_REJ;
	}
	
	/* Nak every packet with u16 payload < 10 */
	uint8_t *p = (uint8_t*) opt;
	printf("Value 0: %i\n", (*p));
	printf("Value 1: %i\n", *(p+1));
	uint16_t u16 = (*(p+sizeof(cp_opt_hdr_t))<<8) + *(p+sizeof(cp_opt_hdr_t)+1);
	printf("Value type: %i\n", u16);
	if (u16 > 10)
	{
		return CP_CREQ_NAK;
	}
	return CP_CREQ_ACK;

}
static void test_gnrc_ppp_lcp_recv_cr_nak(void)
{
	/*Make fake ctrl prot*/
	ppp_cp_t fake_prot;
	fake_prot.get_option_status = &fakeprot_get_option_status;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t nak_pkt[8] = {0x01,0x00,0x00,0x08,0x01,0x04,0x00,0xF1};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(nak_pkt, 8, &cp_pkt);

	handle_cp_pkt(&fake_prot, &cp_pkt);

	/* In this case, we are expecting an E_RCRm*/
	TEST_ASSERT_EQUAL_INT(E_RCRm, fake_prot.event);
	/* See if packet has NAK */
	TEST_ASSERT_EQUAL_INT(1, (fake_prot.metadata.opts_status_content&OPT_HAS_NAK) > 0);
}

static void test_gnrc_ppp_lcp_recv_cr_rej(void)
{
	/*Make fake ctrl prot*/
	ppp_cp_t fake_prot;
	fake_prot.get_option_status = &fakeprot_get_option_status;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t rej_pkt[8] = {0x01,0x00,0x00,0x08,0x05,0x04,0x00,0xF1};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(rej_pkt, 8, &cp_pkt);

	handle_cp_pkt(&fake_prot, &cp_pkt);

	/* In this case, we are expecting an E_RCRm*/
	TEST_ASSERT_EQUAL_INT(E_RCRm, fake_prot.event);
	/* See if packet has REJ */
	TEST_ASSERT_EQUAL_INT(1, (fake_prot.metadata.opts_status_content&OPT_HAS_REJ) > 0);
}

static void test_gnrc_ppp_lcp_recv_cr_ack(void)
{
	/*Make fake ctrl prot*/
	ppp_cp_t fake_prot;
	fake_prot.get_option_status = &fakeprot_get_option_status;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t good_pkt[8] = {0x01,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(good_pkt, 8, &cp_pkt);

	handle_cp_pkt(&fake_prot, &cp_pkt);

	/* In this case, we are expecting an E_RCRp*/
	TEST_ASSERT_EQUAL_INT(E_RCRp, fake_prot.event);

}
static void test_ppp_pkt_metadata(void)
{
	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t code = 0x01;
	uint8_t id = 33;
	uint16_t length = 8;
	uint8_t pkt[8] = {code,id,0x00,length,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(pkt, 8, &cp_pkt);

	cp_pkt_metadata_t metadata;
	ppp_pkt_gen_metadata(&metadata, &cp_pkt, &fakeprot_get_option_status);

	TEST_ASSERT_EQUAL_INT(1, metadata.num_tagged_opts);
	/* In this case, data should have ACK flag*/
	TEST_ASSERT_EQUAL_INT(1, metadata.opts_status_content & OPT_HAS_ACK);
}

static void test_gnrc_ppp_lcp_recv_ack(void)
{
	/*Make fake ctrl prot*/
	ppp_cp_t fake_prot;
	fake_prot.get_option_status = &fakeprot_get_option_status;

	/* |--ConfigureReq--|--Identifier--|--Length(MSB)--|--Length(LSB)--|--Type--|--Length--|--MRU(MSB)--|--MRU(LSB)--| */
	uint8_t good_pkt[8] = {0x02,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	cp_pkt_t cp_pkt;
	ppp_pkt_init(good_pkt, 8, &cp_pkt);


	fake_prot.cr_sent_identifier = 0;
	memcpy(fake_prot.cr_sent_opts,good_pkt+4,4);
	fake_prot.cr_sent_size = 8;

	handle_cp_pkt(&fake_prot, &cp_pkt);
	
	/* In this case, we are expecting an E_RCA*/
	TEST_ASSERT_EQUAL_INT(E_RCA, fake_prot.event);
}

Test *tests_gnrc_ppp_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_gnrc_ppp_lcp_recv_cr_ack),
        new_TestFixture(test_gnrc_ppp_lcp_recv_cr_nak),
        new_TestFixture(test_gnrc_ppp_lcp_recv_cr_rej),
        new_TestFixture(test_gnrc_ppp_lcp_recv_ack),
        new_TestFixture(test_ppp_pkt_metadata),
    };

    EMB_UNIT_TESTCALLER(gnrc_ppp_tests, NULL, NULL, fixtures);

    return (Test *)&gnrc_ppp_tests;
}

void tests_gnrc_ppp(void)
{
    TESTS_RUN(tests_gnrc_ppp_tests());
}
/** @} */
