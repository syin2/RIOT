
/*Control Protocol option*/
typedef struct cp_opt_t{
	uint8_t type;
	size_t length;
	void *payload;
	cp_opt_t *next;
} cp_opt_t;

/* Status of Control Protocol options response */
typedef struct opt_stack_t
{
	uint8_t type; /* Status of the set of CP opt response (ACK, NAK, REJ)*/
	uint8_t num_opts; /* Number of options in response */
	uint8_t content_flag;
	/* CP options to be sent are stored here */
	cp_opt_t opts[MAX_CP_OPTIONS];
}opt_stack_t;

int ppp_cp_populate_options(opt_stack *o_stack, uint8_t *payload, size_t p_size);
int ppp_cp_opts_are_equal(cp_opt_t *o1, cp_opt_t *o2);
int ppp_cp_optstacks_are_equal(opt_stack_t *s1, opt_stack_t *s2);
