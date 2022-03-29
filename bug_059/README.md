# Change i386 and amd64 macros

## Scope
* All XMHF
* Git `67bfb18d8`

## Behavior
Currently XMHF64 only supports 2 architectures: i386 and amd64.

Most code I have written look like
```
#ifdef __AMD64__
	Code for amd64 (64-bit)
#else /* !__AMD64__ */
	Code for i386 (32-bit)
#endif /* __AMD64__ */
```

However, this is better to use
```
#ifdef __I386__
#elif defined(__AMD64__)
#else
	#error "Unsupported Arch" 
#endif
```

So now we want a way to automatically replace code.

## Debugging
Use `bug_049/merge_x86.py` as the starting point, write `merge.py`.

In 1:
```
#ifdef __AMD64__
	Code for amd64 (64-bit)
#else /* !__AMD64__ */
	Code for i386 (32-bit)
#endif /* __AMD64__ */
```

Out 1:
```
#ifdef __AMD64__
	Code for amd64 (64-bit)
#elif defined(__I386__)
	Code for i386 (32-bit)
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch" 
#endif /* !defined(__I386__) && !defined(__AMD64__) */
```

In 2:
```
#ifdef __AMD64__
	Code for amd64 (64-bit)
#endif /* __AMD64__ */
```

Out 2:
```
#ifdef __AMD64__
	Code for amd64 (64-bit)
#elif !defined(__I386__)
    #error "Unsupported Arch" 
#endif /* !defined(__I386__) */
```

We use 4 spaces before `#error`

## Fix

`67bfb18d8..0a7127ebc`
* In all occurrences of `__AMD64__`, require `__I386__ || __AMD64__`

