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
#include "net/gnrc/ppp/opt.h"
#include "net/ppp/hdr.h"



static void set_up(void)
{
    gnrc_pktbuf_init();
}

static void test_lcp_rcr_ack(void)
{
	ppp_cp_t lcp;

	uint8_t ack[10] = {0x01,0x00,0x00,0x08,0x01,0x04,0x00,0x01,0x00,0x00};

    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, ack, sizeof(ack),
                                           GNRC_NETTYPE_UNDEF);

	int event;

	/* Try with a good pkt */
	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RCRp, event);
}

static void test_lcp_rcr_nak(void)
{
	ppp_cp_t lcp;
	uint8_t nak[8] = {0x01,0x00,0x00,0x08,0x01,0x04,0xFF,0xFF};
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, nak, sizeof(nak),
                                           GNRC_NETTYPE_UNDEF);
	int event;

	/* Try with a NAK pkt */
	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RCRm, event);
}

static void test_lcp_rcr_rej(void)
{
	ppp_cp_t lcp;
	uint8_t rej[8] = {0x01,0x00,0x00,0x08,0x05,0x04,0x00,0xF1};
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, rej, sizeof(rej),
                                           GNRC_NETTYPE_UNDEF);
	int event;

	/* Try with a REJ pkt */
	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RCRm, event);
}
static void test_lcp_rcr_ack_no_options(void)
{
	ppp_cp_t lcp;

	uint8_t ack_noopt[4] = {0x01,0x00,0x00,0x04};

    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, ack_noopt, sizeof(ack_noopt),
                                           GNRC_NETTYPE_UNDEF);

	int event;

	/* Try with a good pkt */
	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RCRp, event);
}

static void test_lcp_rcr_malformed_opts(void)
{
	ppp_cp_t lcp;

	uint8_t malformed[] = {0x01,0x00,0x00,0x08,0xD2,0x03,0x00,0x00};
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, malformed, sizeof(malformed),
                                           GNRC_NETTYPE_UNDEF);
	int event;
	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);
}


static void test_lcp_rca(void)
{
	ppp_cp_t lcp;

	uint8_t good_pkt[9] = {0x02,0x00,0x00,0x08,0x01,0x04,0x00,0x01, 0x00};
	int event;

    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, good_pkt, sizeof(good_pkt),
                                           GNRC_NETTYPE_UNDEF);
	lcp.cr_sent_identifier = 0;
	memcpy(lcp.cr_sent_opts,good_pkt+4,4);
	lcp.cr_sent_size = 8;

	event = lcp_handle_pkt(&lcp, payload);
	
	/* In this case, we are expecting an E_RCA*/
	TEST_ASSERT_EQUAL_INT(E_RCA, event);
}

static void test_lcp_rca_mismatch_id(void)
{
	ppp_cp_t lcp;

	uint8_t good_pkt[8] = {0x02,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	int event;

    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, good_pkt, sizeof(good_pkt),
                                           GNRC_NETTYPE_UNDEF);
	lcp.cr_sent_identifier = 1;

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);
}

static void test_lcp_rca_mismatch_payload(void)
{
	ppp_cp_t lcp;

	uint8_t good_pkt[8] = {0x02,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	uint8_t mismatched[4] = {0x01,0x03,0x00,0x01};
	int event;

    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, good_pkt, sizeof(good_pkt),
                                           GNRC_NETTYPE_UNDEF);
	lcp.cr_sent_identifier = 0;
	memcpy(lcp.cr_sent_opts,mismatched,4);

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);

	/* Payload mismatch */
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);
}

static void test_lcp_rca_no_options(void)
{
	ppp_cp_t lcp;

	uint8_t ack_noopt[8] = {0x02,0x00,0x00,0x04};

	int event;

    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, ack_noopt, sizeof(ack_noopt),
                                           GNRC_NETTYPE_UNDEF);
	lcp.cr_sent_identifier = 0;

	event = lcp_handle_pkt(&lcp, payload);
	
	/* In this case, we are expecting an E_RCA*/
	TEST_ASSERT_EQUAL_INT(E_RCA, event);
}



static void test_lcp_rcn_nak(void)
{
	ppp_cp_t lcp;

	uint8_t nak[8] = {0x03,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, nak, sizeof(nak),
                                           GNRC_NETTYPE_UNDEF);
	lcp.cr_sent_identifier = 0;
	int event;

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RCN, event);
}


static void test_lcp_rcn_nak_no_options(void)
{
	ppp_cp_t lcp;

	uint8_t nak[8] = {0x03,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, nak, sizeof(nak),
                                           GNRC_NETTYPE_UNDEF);
	int event;

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RCN, event);

}



static void test_lcp_rcn_nak_mismatch_id(void)
{
	ppp_cp_t lcp;

	uint8_t nak[8] = {0x03,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, nak, sizeof(nak),
                                           GNRC_NETTYPE_UNDEF);
	int event;
	lcp.cr_sent_identifier = 1;

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);
}


static void test_lcp_rcn_rej(void)
{
	ppp_cp_t lcp;

	uint8_t rej[8] = {0x04,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, rej, sizeof(rej),
                                           GNRC_NETTYPE_UNDEF);
	lcp.cr_sent_identifier = 0;
	lcp.cr_sent_size = 4;
	memcpy(lcp.cr_sent_opts,rej+4,4);
	int event;

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RCN, event);
}


static void test_lcp_rcn_rej_no_options(void)
{
	ppp_cp_t lcp;

	uint8_t rej[8] = {0x04,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, rej, sizeof(rej),
                                           GNRC_NETTYPE_UNDEF);
	int event;

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);

}



static void test_lcp_rcn_rej_mismatch_id(void)
{
	ppp_cp_t lcp;

	uint8_t rej[8] = {0x04,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, rej, sizeof(rej),
                                           GNRC_NETTYPE_UNDEF);
	int event;
	lcp.cr_sent_identifier = 1;

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);
}

static void test_lcp_rcn_rej_mismatch_subset(void)
{
	ppp_cp_t lcp;

	uint8_t rej[8] = {0x04,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	uint8_t sent_set[8] = {0x04,0x03,0x00,0x08,0x04,0x04,0x02,0x01};

    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, rej, sizeof(rej),
                                           GNRC_NETTYPE_UNDEF);
	int event;
	memcpy(lcp.cr_sent_opts, sent_set, 8);
	lcp.cr_sent_identifier = 0;

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);
}


static void test_lcp_coderej_notcritical(void)
{
	ppp_cp_t lcp;
	uint8_t notcritical[8] = {0x07,0x00,0x00,0x08,0x09,0x04,0x00,0x01};
	int event;
	
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, notcritical, sizeof(notcritical),
                                           GNRC_NETTYPE_UNDEF);

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RXJp, event);
}

static void test_lcp_coderej_critical(void)
{
	ppp_cp_t lcp;
	uint8_t critical[8] = {0x07,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	int event;
	
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, critical, sizeof(critical),
                                           GNRC_NETTYPE_UNDEF);

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RXJm, event);
}

static void test_lcp_term_req(void)
{
	ppp_cp_t lcp;
	uint8_t term_req[8] = {0x05,0x00,0x00,0x08,0x09,0x04,0x00,0x01};

	int event;
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, term_req, sizeof(term_req),
                                           GNRC_NETTYPE_UNDEF);


	/*Test received term req*/
	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RTR, event);
}

static void test_lcp_term_ack(void)
{
	ppp_cp_t lcp;
	int sent_treq_id = 9;
	lcp.tr_sent_identifier = sent_treq_id;
	uint8_t term_ack[8] = {0x06,sent_treq_id,0x00,0x08,0x09,0x04,0x00,0x01};
	int event;

    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, term_ack, sizeof(term_ack),
                                           GNRC_NETTYPE_UNDEF);

	/*Test received term ack with corresponding to the sent term req*/
	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RTA, event);
}

static void test_lcp_term_ack_bad_id(void)
{
	ppp_cp_t lcp;
	int sent_treq_id = 9;
	uint8_t term_ack[8] = {0x06,sent_treq_id,0x00,0x08,0x09,0x04,0x00,0x01};
	int event;

    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, term_ack, sizeof(term_ack),
                                           GNRC_NETTYPE_UNDEF);

	/*Test received term ack with bad id */
	lcp.tr_sent_identifier = 0;
	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(-EBADMSG, event);

}

static void test_lcp_unknown(void)
{
	ppp_cp_t lcp;
	uint8_t unknown_code[8] = {0xF7,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	int event;
	
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, unknown_code, sizeof(unknown_code),
                                           GNRC_NETTYPE_UNDEF);

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RUC, event);
}


static void test_lcp_rxr_echo_request(void)
{
	ppp_cp_t lcp;
	uint8_t echo_request[8] = {0x09,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	int event;
	
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, echo_request, sizeof(echo_request),
                                           GNRC_NETTYPE_UNDEF);

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RXR, event);
}


static void test_lcp_rxr_echo_reply(void)
{
	ppp_cp_t lcp;
	uint8_t echo_reply[8] = {0x0A,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	int event;
	
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, echo_reply, sizeof(echo_reply),
                                           GNRC_NETTYPE_UNDEF);

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RXR, event);
}


static void test_lcp_rxr_discard_request(void)
{
	ppp_cp_t lcp;
	uint8_t discard_request[8] = {0x0B,0x00,0x00,0x08,0x01,0x04,0x00,0x01};
	int event;
	
    gnrc_pktsnip_t *payload = gnrc_pktbuf_add(NULL, discard_request, sizeof(discard_request),
                                           GNRC_NETTYPE_UNDEF);

	event = lcp_handle_pkt(&lcp, payload);
	TEST_ASSERT_EQUAL_INT(E_RXR, event);
}

Test *tests_gnrc_ppp_lcp_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_lcp_rcr_ack_no_options),
        new_TestFixture(test_lcp_rcr_malformed_opts),
        new_TestFixture(test_lcp_rcr_ack),
        new_TestFixture(test_lcp_rcr_nak),
        new_TestFixture(test_lcp_rcr_rej),
        new_TestFixture(test_lcp_rca),
        new_TestFixture(test_lcp_rca_no_options),
        new_TestFixture(test_lcp_rca_mismatch_id),
        new_TestFixture(test_lcp_rca_mismatch_payload),
        new_TestFixture(test_lcp_rcn_nak),
        new_TestFixture(test_lcp_rcn_nak_no_options),
        new_TestFixture(test_lcp_rcn_nak_mismatch_id),
        new_TestFixture(test_lcp_rcn_rej),
        new_TestFixture(test_lcp_rcn_rej_no_options),
        new_TestFixture(test_lcp_rcn_rej_mismatch_id),
        new_TestFixture(test_lcp_rcn_rej_mismatch_subset),
        new_TestFixture(test_lcp_coderej_critical),
        new_TestFixture(test_lcp_coderej_notcritical),
        new_TestFixture(test_lcp_term_req),
        new_TestFixture(test_lcp_term_ack),
        new_TestFixture(test_lcp_term_ack_bad_id),
        new_TestFixture(test_lcp_unknown),
        new_TestFixture(test_lcp_rxr_echo_request),
        new_TestFixture(test_lcp_rxr_echo_reply),
        new_TestFixture(test_lcp_rxr_discard_request),
    };

    EMB_UNIT_TESTCALLER(gnrc_ppp_lcp_tests, set_up, NULL, fixtures);

    return (Test *)&gnrc_ppp_lcp_tests;
}

void tests_gnrc_ppp_lcp(void)
{
    TESTS_RUN(tests_gnrc_ppp_lcp_tests());
}
/** @} */
