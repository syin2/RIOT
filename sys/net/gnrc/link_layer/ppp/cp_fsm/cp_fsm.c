
/*Send Control Protocol. Assumes the opt payload is loaded in HDLC Control Protocol Buffer. */
static int send_cp(ppp_ctrl_prot_t  *cp, cp_pkt_t *pkt)
{
	ppp_dev_t *dev = cp->dev;

	/* Set code, identifier, length*/
	dev->_payload_buf[0] = code;
	dev->_payload_buf[1] = identifier;
	uint32_t length;
	/* if size is not zero, the hdlc cp buffer was preloaded */
	/*TODO: Change number to labels*/
	uint16_t cursor;
	/* Generate payload with corresponding options */
	cursor = 0;
	cp_opt_t *copt;
	for(int i=0;i<l_lcp->_num_opt;i++)
	{
		copt = &(opt_stack->_opt_buf[i]);
		*(dst+cursor) = copt->type;
		*(dst+cursor+1) = copt->p_size+2;
		for(int j=0;j<copt->opt_size;j++)
		{
			*(dst+cursor+2+i) = copt->payload[j];
		}
		cursor += copt->opt_size+2;
	}
	length =  opt_size+4;
	dev->_payload_buf[2] = length & 0xFF00;
	dev->_payload_buf[3] = length & 0x00FF;

	/* Create pkt snip */
	gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, dev->_payload_buf, length, GNRC_NETTYPE_UNDEF);
	if (pkt == NULL){
		DEBUG("PPP: not enough space in pkt buffer");
		return 0; /*TODO Fix*/
	}
	
	/* Send pkt to ppp_send*/
	ppp_send(dev, pkt);
	return 0; /*TODO*/
}

static void _tlu(ppp_cp_t *cp)
{
	cp->l_upper_msg |= PPP_MSG_UP;
}

static void _tld(ppp_cp_t *cp)
{
	cp->l_upper_msg |= PPP_MSG_DOWN;
}

static void _tls(ppp_cp_t *cp)
{
	cp->l_lower_msg |= PPP_MSG_UP;
}

static void _tlf(ppp_cp_t *cp)
{
	cp->l_lower_msg |= PPP_MSG_DOWN;
}

static void _irc(ppp_cp_t *cp)
{
	/*Depending on the state, set the right value for restart counter*/
	/* TODO: Activate restart timer with corresponding time out */
	switch(cp->timer_select)
	{
		case RC_SEL_CONF:
			cp->counter_term = PPP_MAX_TERMINATE;
			break;
		case RC_SEL_TERM:
			cp->counter_conf = PPP_MAX_CONFIG;
			break;
		default:
			/* Shouldn't be here */
			break;
	}
}
static void _zrc(ppp_cp_t *cp)
{
	cp->restart_counter = 0;
	/* Set timer to appropiate value TODO*/
}

static void _src(ppp_cp_t *cp)
{
	/* Decrement configure counter */
	cp->counter_conf -= 1;

	int id=666; /* TODO */
	cp_pkt_t pkt;
	pkt.hdr.code = PPP_CP_REQUEST_CONFIGURE;
	pkt.hdr.id = id;
	cp->populate_opt_stack(cp->cp_options, &(pkt.opt_stack));
	
	send_cp(cp, &pkt);
	/* TODO: Set timeout for SRC */
}
static void _sca(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	pkt->hdr->code = PPP_CP_REQUEST_ACK;
	send_cp(cp, pkt);
}
static void _scn(ppp_cp_t *cp, cp_pkt_t *pkt)
{
	/* Check the content of received options */
	if(pkt->opts->content_flag & OPT_HAS_REJ)
	{
		_remove_opts_by_status(PPP_CP_REQUEST_REJ, pkt->opts);
		pkt->hdr->code = PPP_CP_REQUEST_REJ;
		send_cp(cp, pkt);
	}
	else
	{
		_remove_opts_by_status(PPP_CP_REQUEST_NAK, pkt->opts);
		pkt->hdr->code = PPP_CP_REQUEST_NAK;
		send_cp(cp, pkt);
	}
}
static void _str(ppp_cp_t *cp)
{
	int id = 666; /*TODO*/
	cp_pkt_t pkt;
	pkt->hdr->code = PPP_CP_TERM_REQUEST;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
}
static void _sta(ppp_cp_t *cp)
{
	int id = 666; /*TODO*/
	cp_pkt_t pkt;
	pkt->hdr->code = PPP_CP_TERM_ACK;
	pkt->hdr->id = id;
	pkt->hdr->length = 4;
	pkt->opts->num_opts = 0;
	send_cp(cp, pkt);
}
static void _scj(ppp_cp_t *cp)
{
	send_cp(cp, PPP_CP_CODE_REJ);
}
static void _ser(ppp_cp_t *cp)
{
	send_cp(cp,PPP_CP_SER);
}


/* Call functions depending on function flag*/
static void _event_action(ppp_cp_t *cp) 
{
	uint8_t flags;
	/* Reset link status */
	cp->l_upper_msg = 0;
	cp->l_lower_msg = 0;

	flags = actions[cp->state][cp->event];

	if(flags & F_TLU) _tlu(cp);
	if(flags & F_TLD) _tld(cp);
	if(flags & F_TLS) _tls(cp);
	if(flags & F_TLF) _tlf(cp);
	if(flags & F_IRC) _irc(cp);
	if(flags & F_ZRC) _zrc(cp);
	if(flags & F_SRC) _src(cp);
	if(flags & F_SCA) _sca(cp);
	if(flags & F_SCN) _scn(cp);
	if(flags & F_STR) _str(cp);
	if(flags & F_STA) _sta(cp);
	if(flags & F_SCJ) _scj(cp);
	if(flags & F_SER) _ser(cp);
}

static int fsm(ppp_cp_t *cp)
{
	int8_t next_state;

	/* Call the appropiate functions for each state */
	_event_action(cp);
	/* Set next state */
	next_state = state_trans[cp->state][cp->event];

	/* Keep in same state if there's something wrong (RFC 1661) */
	if(next_state != S_UNDEF){
		cp->state = next_state;
	}
	return 0; /*TODO*/
}
