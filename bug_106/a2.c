extern int f_a(void);
extern void f_b(int *x);

void func(int *p)
{
	if (f_a()) {
		int v;
		*p = v;
		f_b(&v);
	}
}

