/* Return whether enter if block */
extern int f_a(void);

/* Input is y, output written to (*z) */
extern void f_b(int y, int *z);

/* Input is z, output is returned */
extern int f_c(int z);

int func(int *p_r)
{
	int v_x = 0;
	if (f_a()) {
		int v_y = 0;
		int v_z;
		/* Expecting -Werror=maybe-uninitialized warning here (line 17) */
		*p_r = v_z;
		f_b(v_y, &v_z);
		v_x = f_c(v_z);
	}
	return v_x;
}

