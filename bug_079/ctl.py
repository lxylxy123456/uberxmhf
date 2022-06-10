if __name__ == '__main__':
	for i in open('cap.csv').read().split('\n'):
		if not i:
			continue
		human_name, cap_name, num, low_name, ctl_name = i.split(',')
		print('/* %s */' % human_name)
		print('#define VMX_%s %d' % (cap_name, int(num)))
		print('static inline bool _vmx_hasctl_%s(vmx_ctls_t *ctls)' % low_name)
		print('{')
		print('\treturn ctls->%s & (1U << VMX_%s);' % (ctl_name, cap_name))
		print('}')
		print('static inline void _vmx_setctl_%s(vmx_ctls_t *ctls)' % low_name)
		print('{')
		print('\tctls->%s |= (1U << VMX_%s);' % (ctl_name, cap_name))
		print('}')
		print('static inline void _vmx_clearctl_%s(vmx_ctls_t *ctls)' % low_name)
		print('{')
		print('\tctls->%s &= ~(1U << VMX_%s);' % (ctl_name, cap_name))
		print('}')
		print()

