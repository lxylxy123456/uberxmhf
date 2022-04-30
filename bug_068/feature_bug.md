#### `Feature: fix compile error found in high GCC version`
```
13f57fe5b fix compile error in subr_prf.c and peh-x86svm-main.c                 
cce2c94d0 fix compiler warning in smpg-x86svm.c                                 
b5cd8f321 fix compile error in smpg-x86vmx.c                                    
ec00dd19d fix compile error in bplt-x86.c                                       
f75760024 disable PIE for new compilers                                         
cf3629030 disable PIC                                                           
901af163e Fix compiler warning in tlsf.c                                        
c06820b13 fix compiler warning in txt_acmod.c                                   
cb4db46c1 fix typo                                                              
1bab2678d fix subsequent compiler warning in txt_acmod.c                        
93c3757eb comment unused global variable in cmdline.c                           
ecf600679 use fallthrough comment for compatibility                             
```

#### `Misc change`
```
4dfcb7b85 fix typo: suppported -> supported                                     
669ffe425 remove uberXMHF from repo                                             
9bea8ffee manually revert commit 77de23a25ecff99356122deead2e9e67734e89d0       
c2d73426a revert Makefile.in                                                    
024249c8b revert documentation to v0.2.2                                        
c1981a125 add newlines to debug output                                          
ded790409 Move newline to txt_parse_sinit()                                     
0f069fa78 Update .gitignore                                                     
c2230845e Update .gitignore                                                     
635b988fe Support debug symbols                                                 
c66ae7f6f Support debugging in x86                                              
27d968aad Increase runtime stack size                                           bug_015
36fc963ae Change some printf statements                                         bug_015
5fcac0afa Update print statements in xcph-x86_64.c                              
2a207ec98 Fix bug in 5fcac0afaa2e6774f7ee7671e93375f5f9a8bacf                   
7715f3f03 Print error code in exception handler                                 
d26673ba8 Optimize __vmx_vmread and __vmx_vmwrite by giving compiler more freed 
16095a3e7 Change some indenting in _vmx.h                                       
d63fb3b74 Follow specification in GDT limit field (should be 8*k - 1)           
3b199dbe0 Update contributors                                                   
da54b32fe Provide both decimal and hex value in exception and intercept number  
55c0082b3 Use RFLAGS instead of EFLAGS in x64                                   
5324ae8e8 Merge branch 'xmhf64-win' into xmhf64                                 
78f0eef8e Update MAX_PHYS_ADDR to 9GB                                           
2e3af773e Define debug section in ld scripts, remove linker warnings            
486583a05 Add todo                                                              
5aa835725 Phase 1 of merging x86 and x64: rename functions from *x86_64* to *x8 bug_049
ee49c5948 Modify sl-x86.c                                                       bug_049
8e0fbd03c Run phase 2 on sl-x86.c                                               bug_049
357568fac Merge _svm_getvcpu and _vmx_getvcpu in x86                            bug_049
63e942cc1 expand -t 4 xcph-x86.c                                                bug_049
c455729b1 Change types in xcph-x86.c                                            bug_049
87a497800 Add ifdef to xcph-x86.c                                               bug_049
521339fc2 Simplify ifdefs in xcph-x86.c                                         bug_049
dd266a926 Fix compile errors in xcph-x86.c                                      bug_049
cf6ff75df Merge rntm-x86-data.c                                                 bug_049
016af4a76 Add ifdef to smpg-x86vmx.c                                            bug_049
217859474 Add ifdef to part-x86vmx.c                                            bug_049
643b4fee4 Add ifdef to memp-x86vmx.c                                            bug_049
699eed0d0 Make memp-x86vmx-data.c the same                                      bug_049
6e52a0031 Update some of peh-x86vmx-main.c                                      bug_049
bf7bd770a Change some of peh-x86vmx-main.c                                      bug_049
6b4de450a Add ifdef to peh-x86vmx-main.c                                        bug_049
1d05a814a Add ifdefs to peh-x86-safemsr.c                                       bug_049
7545c2f72 Make bplt-x86vmx.c the same                                           bug_049
fa9e44776 Use DECLARE_FIELD in x86                                              bug_049
99022eb25 Add ifdef to bplt-x86vmx-data.c                                       bug_049
c2d899d2a Update bplt-x86vmx-vmcs.c                                             bug_049
6a4b0794a Add ifdef to bplt-x86vmx-vmcs.c                                       bug_049
28e42cef9 Fix compile error                                                     bug_049
c51de4165 Fix EPT misconfiguration error                                        bug_049
a6d27db2f Remove STATIC_ASSERT                                                  bug_049
a4fa44cd1 Add ifdef to bplt-x86vmx-smp.c                                        bug_049
71aa12993 Add ifdef to bplt-x86svm-smp.c                                        bug_049
77977788d Add ifdef to bplt-x86-smp.c                                           bug_049
937c8fd9b Change bplt-x86-addressing.c                                          bug_049
85d100bb2 Add ifdefs to bplt-x86-addressing.c                                   bug_049
f1051aa2c Add ifdef to xmhf-tpm-arch-x86.h                                      bug_049
31d385267 Misc change in xmhf-baseplatform-arch-x86_64.h                        bug_049
a0c690217 Add ifdef to _configx86.h                                             bug_049
17f6c6e9c Add ifdef to xmhf-baseplatform-arch-x86.h                             bug_049
9d00a4200 Make _vmx.h the same                                                  bug_049
4fa4ded1e Move x86_64/_configx86_64.h to x86_64/_configx86.h to make xmhf-basep bug_049
5876db595 Add ifdefs to _txt_config_regs.h                                      bug_049
11cd3f2b1 Add ifdef to _processor.h                                             bug_049
b36174bc9 Rename isrHigh to isrHigh16 for x86                                   bug_049
655081580 Change x86/_paging.h                                                  bug_049
f3e72dd58 Add ifdefs to _paging.h                                               bug_049
f57ce3040 Add ifdefs to _msr.h                                                  bug_049
ff3d34c26 Add ifdefs to _div64.h                                                bug_049
354e7a22c Fix typo in _cmdline.h                                                bug_049
f3841059f Fix compile bug in _div64.h                                           bug_049
de8ddcb5d Fix compile bug in _div64.h                                           bug_049
95253b2e7 Remove trailing white spaces for all files                            bug_049
15401fa2c Wrap up phase 2                                                       bug_049
896edafb9 Update Makefiles                                                      bug_049
6039c0471 Phase 3: remove x86_64 files                                          bug_049
f94369412 Moved .S files (Makefiles not changed yet)                            bug_049
2dc631f2e Change Makefiles to follow moved .S files                             bug_049
3d138008b Add i386/amd64 to artifacts                                           bug_049
9afa21550 Change build.yml                                                      bug_049
c33a79cdb Rename ia64 to amd64                                                  bug_049
049e08980 Change to FreeBSD's amd64 version, not ia64                           bug_049
03ce4152d Replace all `__X86_64__` to `__AMD64__`, and `__X86__` to `__I386__`  bug_049
046be449b Replace remaining files                                               bug_049
7585a545b __XMHF_X86_64__ -> __XMHF_AMD64__, __XMHF_X86__ -> __XMHF_I386__      bug_049
099b1239e Update comments                                                       bug_049
88226b51d Update .gitignore                                                     bug_049
ac4a81ad3 Manually change x86_64 to amd64                                       bug_049
e4ff5ad03 Manually remove useless __AMD64__                                     bug_049
f8f0a50e4 Manually replace "x86-64"                                             bug_049
65a3f5ea4 Complete merging 32-bit and 64-bit XMHF code                          bug_049
f0e16b867 Write Setting up XMHF64 documentation                                 
67bfb18d8 Use shifts in _vmx_setupEPT() instead of * and /                      
f3b723e72 Update setup-xmhf64.md docs                                           
19f71b95b Update docs for XMHF64 support status                                 
```

#### `Bug: dealwithE820() does not check grube820list_numentries`
```
bc29c25e4 auto-pushed                                                           
```

#### `Feature: disable unused features when !DRT`
```
cef6374ac Disable trustvisor_master_crypto_init() in when disable DRT           
e0fbf969f Disable utpm_extend                                                   
ea33d732f compiler warning                                                      
4e5026d35 Remove txt.c when DRT=n                                               
04514f3de Disable more DRT functions                                            
2867a2058 Disable more DRT functions                                            
31d80e6c4 Remove DRT in sl-x86.c                                                
```

#### `Feature: support XMHF in x64 (i.e. amd64), without DRT`
```
909c63e85 Add compiler flags for x64                                            
cc5d26a07 typo                                                                  
5a9adc210 Prepare Makefile in subdirectories                                    
fc67adff4 Change CCLIB                                                          
982548495 Debug CCLIB                                                           
5edcfddf1 Port x86_64_types.h from FreeBSD                                      
2b9cd71ca Port x86_64_endian.h                                                  
55cc661a1 Port x86_64 versions from <http://fxr.watson.org/fxr/source/ia64/incl 
0a12fa84d Rename to ia64 (follow name in FreeBSD)                               
081944c54 debug                                                                 
7cea4a0bf Remove 32 in string.{c,h}                                             
b016dc653 Remove 32 from relocate_kernel() and setuplinuxboot()                 
8eb6d0a7f Fix compile error in scode.c                                          
0090c6829 Fix compile warning in utpm.c                                         
873058d45 Fix compile error                                                     
940d17809 Fix assembler error                                                   
eaa80c334 Compile error peh-x86vmx-main.c                                       
7e1519348 Fix smpg-x86svm.c                                                     
a55102a50 Compiler warning in dmap and smpg                                     
fa76c0dcf Fix xcph-stubs.S                                                      
1bfc8bc21 Compiler error                                                        
6e0ec61d0 Fix compiler errror                                                   
4e58de041 Define sla_t                                                          
0036487af Update RPB and SL_PARAMETER_BLOCK to use hva_t                        
20307c0ee Port rntm-x86-data.c to 64 bit                                        
53ddc6862 Able to compile xmhf-runtime                                          
ed360badf Add comment                                                           
d3c994a06 Fix sl-x86_64-*.S                                                     
c82bf3b46 Fix sl.c                                                              
cec783e3c Fix sl-x86.c                                                          
bc9fc4c97 Fix sl-x86_64.lds.S                                                   
d15b03c35 Config Makefile to use -m32 for bootloader and -m64 for others        
50572137e Add comments                                                          
dcef43b4d Fix typo in lds                                                       
1ec06c4c8 Add BCCLIB to link bootloader correctly                               
289fcb52a Compile external libraries again in 32-bits for bootloader            
ace95e83d Compile xmhf-tpm and xmhf-baseplatform one more time in x86 for bootl 
938393f20 Can compile bootloader in x64                                         
f48abb143 Can fully compile XMHF                                                
347871da1 Add clean rules for Makefile                                          
f140098cf Fix bplt-x86-smplock.S                                                
7019266c2 __XMHF_X86_64__ -> __X86_64__ in part-x86*-sup.S                      
62c6eb8c6 More __XMHF_X86_64__ -> __X86_64__                                    
346b03703 Remove use of uintptr_t                                               
e2f192117 Remove more use of uintptr_t                                          
66cceeff7 Revert gva_t in utmp due to header file problems                      
dbf2221c7 Write some 32-bit code in sl                                          
473dcfeae Set up page table                                                     
6666c2a44 Should enter compatibility mode                                       
199be5a11 Can jump to x64 code                                                  
dfe077d3e Update comment                                                        
2c03e4243 Make VA = link address in secure loader                               
80e8d531e Duplicate bplt-x86-smplock to bplt-x86_64-smplock                     
0bd39f9f4 Use u32 in slpb                                                       
7bf294603 Fix printf because stdarg was only implemented for x86                
dc25093d1 Try to expand page table to cover 4G                                  
02c8b2b89 Fix "ACPI RSDP not found" error                                       
709f225df Fix NULL pointer error in calculating gdt_base                        
a1e43a43f can setup runtime TSS                                                 
58c615819 Write code for setting up x86_64 paging                               
ed4529e95 Debug macro in xmhf-types.h                                           
86045ac70 Can jump to x64 runtime code                                          
e45daef52 Copy xcph-stubs.S and xcph-x86.c to x86_64                            
fab4d6d01 Modifying IDT for x64                                                 
a3a16e44a Modify xmhf_xcphandler_arch_initialize()                              
662424fae Finish porting xcph-x86_64.c                                          
8441bd49b Fix typo                                                              
3410c6246 iretl -> iret                                                         
0a966ee9b Rename esp to rsp in VCPU struct                                      
7e007e76a auto-pushed                                                           
3f8a48b04 Rename Makefile to duplicate x86 files to x86_64                      
e37dfb476 Duplicate x86 source files to x86_64                                  
4e1ddca98 Duplicate x86 header files to x86_64                                  
386d791fd Merge branch 'xmhf64' of github.com:lxylxy123456/uberxmhf into xmhf64 
f7da77ff9 Prevent BSP from being interrupted by an AP                           
d027f1298 Changed too much                                                      
b1aef7960 Revert to a more stable state                                         
46baba13f Revert b1aef79605ec139c4465a05c87370a2d5ab5ffc6                       
f1845fef3 AP and BSP can enter xmhf_baseplatform_arch_x86_smpinitialize_commons 
655159848 Fix the bug that AP enters xmhf_baseplatform_arch_x86_smpinitialize_c 
2b511ec4e Rename functions in xmhf-partition.h to x86_64                        
1a22c80a5 Rename in bplt-x86_64-smptrampoline.S                                 
2c87a8af9 Disable ltr instruction, allow more than 1 APs                        
1fffabcee Rename functions in xmhf-baseplatform-arch-x86_64.h                   
5beed0696 Rename x86 functions in xmhf/src/xmhf-core/include/arch/x86_64/*.h    
88ffba9e3 Remove temporary code                                                 
2a4fb99a6 Change unhandled exception print format                               
bde373f83 Fix use of x86_64 source when compiling library for bootloader        
88d01cd48 Rename functions in xmhf/src/xmhf-core/include/xmhf-dmaprot.h from x8 
d71a4f103 Update xmhf-memprot.h                                                 
1be7c026d Rename functions in xmhf/src/xmhf-core/include/xmhf-parteventhub.h fr 
3ec02784c Rename dbg_x86_uart_putc_bare to dbg_x86_64_uart_putc_bare            
5a4b32119 restore ltr code in _vmx_initVT                                       
d9ac83707 Remove lldt and ltr code                                              
371028617 Remove "not implemented"                                              
498cc891f Implement xmhf_parteventhub_arch_x86_64vmx_entry                      
64c51c8c3 Implement __vmx_start_hvm                                             
2c7662626 Handle full and high addresses in VMCS                                
ba82a6927 Update _vmx_setupEPT                                                  
0125831b4 Update appParamBlock                                                  
5ca3e9fc6 Remove dead code in _vmx.h                                            
567a502a2 Fix VMLAUNCH error: set Host address-space size in control_VM_exit_co 
19f921c3c Remove VMX capability in VM's cpuid                                   
c2d641da3 Clear MSR_EFER for VM, solve triple fault when guest enables paging   bug_001
eca643c3c Code clean up                                                         
9b1777042 Fix incorrect copy_from_current_guest size                            bug_006
9223b099e Set "IA-32e mode guest" in control_VM_entry_controls according to EFE bug_009
16f746a73 Print full 64-bit address when reporting "Unhandled intercept"        bug_010
de16a3daf Use guest_GS_base for RDMSR, instead of forwarding                    bug_010
ae0968f19 Double runtime stack size for x64                                     bug_013
f29cabbd2 Do not use MSR load area for MSR_EFER; instead, use VMCS {host,guest} bug_012
f33f8effe Update hpt_emhf_get_guest_hpt_type() to detect long mode paging       bug_012
9b3bee219 Fix regression in x64 Debian 11 caused by f29cabbd297b63e943b8c30f796 bug_012
9b2327510 Revert "Fix regression in x64 Debian 11 caused by f29cabbd297b63e943b bug_012
66265c823 Revert "Do not use MSR load area for MSR_EFER; instead, use VMCS {hos bug_012
27fdb28d0 Implement VCPU_glm (getting EFER is tricky)                           bug_012
03c0c21de Implement xmhf_baseplatform_arch_x86_64vmx_dump_vcpu() to print full  
ff1efb9d3 Update vmx_handle_intercept_cr0access_ug() to allow switching from lo bug_014
da32a4e60 Change TR access right to 0x8b to support x64 guests                  bug_014
050a8138e Only update "IA-32e mode guest" in VMCS when CR0.PG changes           bug_015
c23d4ff6c Rename x86 -> x86_64                                                  
569141931 Prevent accessing memory below RSP in xcph                            bug_018
bda589cab Remove unsupported field from struct in __DEBUG_QEMU__ for x86_64     bug_018
231f0d65d Remove unsupported field from struct in __DEBUG_QEMU__ for x86        bug_018
0e041b70f Add membersize to vmcs field encodings for x64                        bug_018
4b8f8b277 In x64 when reading VMCS, check return value and prevent overwriting  bug_018
c1bb8959a Prevent assuming info_vmexit_instruction_length == 3 for VMCALL in pe bug_018
26d22939d Change __vmx_vmread in x86 to return error when error                 bug_018
b04ca351a Add gcc -mno-red-zone to prevent vmread fail                          bug_018
850af783b Extend runtime paging to 8GB                                          bug_020
9c5f688f3 Add error when E820 has entry exceed MAX_PHYS_ADDR                    bug_020
582a35cd5 Extend EPT to 8GB                                                     bug_020
d3862d398 Fix gpa truncated bug in _vmx_handle_intercept_eptviolation           bug_022
1c7ec32a2 Also read vcpu->vmcs.guest_paddr_high                                 bug_022
7759549ba Update _vmx.h to use union for 64-bit VMCS fields                     bug_023
c1257b9ff Use 64-bit VMCS field names in C source code                          bug_023
b406535e9 In x86-64 code, read 64-bit VMCS fields in 1 VMREAD                   bug_023
716d95b93 Remove _full and _high from x86-64 header                             bug_023
069f2d1b9 Update CR0 and CR4 intercept handler for 64-bit values                
41e8844f4 Fix typo in comment                                                   
43b3d6552 Support CR{0,4} intercept with R8-R15                                 bug_034
```

#### `Bug: Makefile parallel build`
```
7e142744b Add Makefile dependency to allow parallelize make                     
```

#### `Bug: in udelay() (old code sleeps shorter than expected)`
```
92e3e641c Fix integer overflow in xmhf_baseplatform_arch_x86_udelay()           
1e0387a58 Fix unsigned overflow in udelay() in init.c                           bug_045
72356c5a7 Check for overflow of latchregval in udelay()                         bug_045
```

#### `Feature: CI (GitHub)`
```
faf6ca0d5 Add GitHub workflow                                                   
c88c0b8ab sudo                                                                  
009705742 Fix compile error                                                     
05113224d Use GitHub Actions matrix                                             
```

#### `Feature: run in QEMU`
```
8570ed122 Add workaround for VMWRITE fail for QEMU                              
0495917ba Update GitHub Actions to compile QEMU workaround                      
d3375e43a Debug github actions                                                  
26e39e184 Debug Makefile.in to recognize __DEBUG_QEMU__                         
26cc24d99 Debug QEMU workaround                                                 
97c05fbba Copy QEMU workaround to x86                                           
79c315dab Declutter VMWRITE warning messages                                    
849956434 Fix compile error                                                     
ce93dfa23 Prevent reading VMCS fields undefined by QEMU                         
```

#### `Feature: support running Debian 11`
```
3b8955c74 allow INVPCID in guest OS (used by Debian 11)                         bug_002
81a80d216 allow RDTSCP in guest OS (used by Debian 11)                          bug_003
21b1abf4b Implement safe read msr in x86                                        bug_004
e836829f2 Implement safe read msr in x86-64                                     bug_004
f8b6f6ce4 Inject GP to x86 guest when invalid RDMSR                             bug_004
d12b97c0c Inject GP to x86_64 guest when invalid RDMSR                          bug_004
e74a28b83 Check INVPCID and RDTSCP before setting them in VMCS                  bug_005
```

#### `Feature: PAL demo`
```
53d8965dc Try to call PAL by setting up memory space in software                
d0958afba Able to call PAL in a standalone appliaction                          
0b37c866d Update pal_demo to lock PAL's pages                                   
1f0be0167 Add test.c in pal_demo to call TV_HC_TEST                             bug_018
```

#### `Bug: CR0 intercept handler`
```
801fe353f Prevent guest from setting CR0 to CPU-disallowed value                bug_008
fd4534a29 Retry MOV CR0 interceptions when CR0.PG changes                       bug_040
0efebfe87 Comments                                                              bug_040
```

#### `Bug: wrmsr intercept ignores vmx_msr_area_msrs`
```
5cae3acd8 Fix rdmsr and wrmsr to use loaded vmx_msr_area_msrs                   bug_009
```

#### `Bug: guest x2APIC not blocked`
```
e945f7b27 Disable guest's x2APIC in CPUID                                       bug_011
```

#### `Bug: incorrect assert in hpt.c`
```
565cef3cf Fix typo of `assert(lvl<=3);` for HPT_TYPE_LONG                       bug_012
```

#### `Feature: support TrustVisor in x64 (i.e. amd64), without DRT`
```
27c303501 Change scode.c and scode.h to have 64-bit data size                   bug_017
ba4cd9d7a Fix bugs caused by previous commit (27c303501)                        bug_015
c3cfa5da6 Consider CR3 as match if address bits match (allow lower bits to be d bug_015
324855612 Fix bug of sentinel_return overwriting first argument                 bug_015
18b9dd541 Prepare pal_demo for compiling in 64-bit                              bug_026
e6abf7c3b Update pointer fields in trustvisor.h to 64 bits                      bug_026
3568461cc Remove dead functions in scode.c                                      bug_026
3b6f66898 Update appmain.c and scode.c to accept 64-bit pointers                bug_026
943e1b612 Remove is_page_4K_aligned(), which does not handle 64-bit addresses w bug_026
dcf0621c5 Fix alignment problem while parsing structs from guest                bug_026
1cb915f36 Fix bug in 3b6f66898cd1ac651f6cf139121c4ee6f7994ba0                   bug_026
564109e7c Able to detect application's 64-bit mode; force mode match for whitel bug_026
1cd4c3590 Split scode_marshall() to scode_marshall{32,64}                       bug_026
2317b0cda Change return address to support x64                                  bug_026
7b569008f pal_demo: make PAL registration function more general                 bug_026
b9af57ef0 Write test to check argument passing                                  bug_026
908fb29f5 Fix bug in getting 64-bit arguments from stack                        bug_026
5bfdfb53e Implement test_10_ptr in pal_demo                                     bug_026
164fea8e9 Fix compile error caused by 1cd4c35907029f9a273670b213e7185cba2738dc  bug_026
d37c25de2 Implement test_5_ptr in pal_demo                                      bug_026
0457f41d7 Fix bug in d37c25de2e3cb744791f08ae6631247127ffc013                   bug_026
4c895f860 Add pal_demo to GitHub actions                                        bug_026
f2a054614 Fix bug in shell script                                               bug_026
f835a5710 Fix inconsistent data type in scode_unmarshall                        bug_026
```

#### `Bug: NMI quiesce handling`
```
377a235cf Prevent printf deadlock in xmhf_smpguest_arch_x86_64vmx_quiesce()     bug_015
dc0bfe092 Fix race condition in xmhf_smpguest_arch_x86_64vmx_endquiesce()       bug_018
29065357e Fix race condition in xmhf_smpguest_arch_x86vmx_endquiesce()          bug_018
aa2d65ef0 Make access to vcpu->quiesced atomic in NMI handler                   bug_018
eb98f9992 Move emhfc_putchar_lineunlock in xmhf_smpguest_arch_x86vmx_quiesce to bug_018
d66fa9d01 Add fromhvm for xmhf_smpguest_arch_x86{,_64}vmx_eventhandler_nmiexcep bug_018
8a5d542f2 Remove nmiinhvm (testing origin of NMI using VMCS)                    bug_018
1d6346255 Revert aa2d65ef0 (no need to make access to vcpu->quiesced atomic)    bug_018
040b6e1a3 If g_vmx_quiesce && vcpu->quiesced in xmhf_smpguest_arch_x86vmx_event bug_018
3cf7c7a38 Inject NMI to guest even not in guest mode                            bug_018
787a0f8d1 Implement unblocking NMI in intercept handler                         bug_018
5665b1d9b Implement virtual NMI                                                 bug_018
eed795ba3 Change lock ordering to prevent deadlock                              bug_018
fc92858ff Fix typo in NMI injection code                                        
8d0fa2d1b Check NMI interception in NMI window handler to prevent injecting NMI bug_021
ce143f21b Fix deadlock related to printf lock in xmhf_smpguest_arch_x86{,_64}vm bug_024
592fbd12c Add comment about not being able to print in certain places of interc bug_024
4cc64d2de Add assertions about maskable interrupts in scode.c                   bug_051
fbabf202a Block NMI when running PALs                                           bug_051
f0baec0bb Halt if dropping NMI                                                  bug_051
617b027c8 Update xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception() to allow bug_065
```

#### `Bug: HALT() does not halt forever`
```
7654c297b Prevent CPU from running after HALT() if receive NMI                  bug_015
```

#### `Bug: E820 last entry dropped`
```
63a196411 Fix e820 not able to detect last entry error                          bug_019
```

#### `Bug: CR4.VMXE should not be set`
```
acfb074a0 Clear VMXE from CR4 shadow                                            bug_027
274fd4f8b Update CR4 intercept handler to handle valid bits                     bug_027
```

#### `Feature: support PAE guests`
```
8b67cfbf3 For PAE paging, update guest_PDPTE{0..3} after changing guest_CR3     bug_028
ef05d91c8 Update CR0 intercept handler to halt when PAE changes to set          bug_033
```

#### `Bug: incorrect logic for 1 CPU machines`
```
793137a11 Support booting with 1 CPU in SMP mode                                bug_032
```

#### `Bug: guest state does not follow spec`
```
0f7229788 For AP's {G,I,L}DTR, DR7, RFLAGS, follow Intel's INIT specification   bug_033
0ec2e511e For AP's EDX, follow Intel's INIT specification                       bug_033
9c0f9491a For AP's CR0, follow Intel's INIT specification                       bug_033
a711db2d0 Change initial CR0 for BSP                                            bug_033
9afbe1130 Support changing boot drive (used to be hardcoded 0x80)               bug_043
```

#### `Bug: _vmx_int15_handleintercept() does not truncate RSP`
```
7b6db45be Fix bug of interpreting RSP in real mode                              bug_036
```

#### `Bug: incorrect default MTRR`
```
a62e6a79b Reduce code used to read fixed MTRRs in x86_64                        bug_036
429d0dcac Reduce code used to read fixed MTRRs in x86                           bug_036
44eb4d9c6 Remove hardcoding of memorytype=WB in _vmx_setupEPT()                 bug_036
25c2f085b Fix bug introduced in 44eb4d9c6d00c4a606da2fc9a267b173603525d8        bug_036
130022082 Refactor MTRR reading code                                            bug_036
3bbcf39a8 Dynamically determine default MTRR                                    bug_036
```

#### `Bug: did not block guest's MTRR changes`
```
1c59b4fc3 Block guest's changes to MTRR                                         bug_036
835d832f6 Workaround 1c59b4fc3aec8ae9552fe9f579460af6126703fb for Windows 10    bug_036
8336bbd89 Disallow guest to modify IA32_MTRR_DEF_TYPE                           
```

#### `Feature: support XMHF in x64 (i.e. amd64), DRT`
```
d4ef00c7a Make x64 XMHF able to compile when DRT is enabled                     bug_037
5461d4844 Use DS to access memory to work in Inel TXT mode                      bug_037
a2fe5a973 Revert incorrect definition of uintptr_t rsdtaddress                  bug_037
87a2a70a9 Reset segment descriptors in x64 sl entry                             bug_042
81563bccd Add attribute packed to _txt_acmod.h                                  bug_042
7dd9a985d Use correct way to access memory in x64 mode (FS -> paging)           bug_042
5a3bee662 Fix rdmsr64 and wrmsr64 in x64                                        bug_042
8589df17b Replace use of "A" in asm for x86_64                                  bug_042
8da04f576 Make data structures the same between x86 and x64 in _txt_mtrrs.h     bug_042
daccc6746 Fix types after changing num_var_mtrrs                                bug_042
92a9e94dc Make os_mle_data_t the same between x86 and x64 in _txt_heap.h        bug_042
d06cbc4f9 Fix word size problems in xmhf_baseplatform_arch_x86_64vmx_wakeupAPs( bug_046
3ae6989ba Fix x86_64 alignment problem for sinit_mle_data_t                     bug_046
774b9dc34 Fix missed change in 8da04f576860a7afbba6370aadb2ed024431c4cc         bug_046
a08f148a1 Fix x86_64 alignment problem for other fields in _txt_heap.h          bug_046
e7d5a6616 Split definition of xmhf_sl_arch_x86_setup_runtime_paging in xmhf-sl. 
215bff899 Make peh-x86svm-entry.S similar to peh-x86_64svm-entry.S              
966a26747 Add amd64 / i386 to banner                                            
0bb1b2bd7 Rename TARGET_WORDSIZE to TARGET_SUBARCH                              
f14b795dc Support configuring AMD64_MAX_PHYS_ADDR                               
e82b0f5fd Fix bug in configure.ac                                               
24eafa6a2 Fix bug in displaying subarch in init.c                               
516e618f6 Rearrange x64 GDT: make code32 + 8 = data32                           bug_046
763ac4121 Fix problem in types when calling hash_memory_multi() (variadic funct bug_053
fafe5feb7 Add PA_PAGE* macros in _paging.h for physical addresses               
34011324f Combine _vmx_setupEPT() between i386 and amd64                        
ec78a946a For Intel, unblock NMI when runtime starts                            bug_055
fc54fa5d2 In all occurrences of __AMD64__, require __I386__ || __AMD64__ to be  bug_059
0a7127ebc Fix bug caused by fc54fa5d2db3f0616ddc98caaa628c964d2e76e3            bug_059
430f6881b Add documentation about PA_* macros                                   
092ec73dc Refactor _paging.h macros                                             bug_060
ddb885732 Use PA_PAGE_ALIGN_UP macros in macros like P4L_NPLM4T                 bug_060
6ff794a4a Add checking in _paging.h macros                                      bug_060
bcdaface3 Add _ in PAGE_ALIGN_UP (fix typo)                                     bug_060
85141ca6b Fix incorrect use of PAGE_ALIGN_* in dmap code                        bug_060
813a64e0e Make TSS access rights the same for i386 and amd64                    
3bd65eec4 In QEMU, print warning if running amd64 guest in i386 XMHF            
```

#### `Feature: support Windows in TrustVisor`
```
a547049ec Allow RAND_MAX to be 0x7fff (to support MinGW)                        bug_038
ebd862271 Update Makefile to set Windows macro                                  bug_038
a7fa77062 Use Windows memory APIs                                               bug_038
215aea755 Build Windows PAL demo in GitHub Actions                              
69bad8e08 Debug GitHub actions                                                  
a6a4ab163 Write assembly code to translate x64 Windows calling convention to Li bug_038
057309e3f Fix bug in hpt_scode_switch_regular() that cause unaligned RSP in x64 bug_038
311a27035 Change unsigned long to uintptr_t (because Windows is LLP64)          bug_038
85415d2dd Update test                                                           bug_038
ed56470da Support munmap memory in caller.c                                     bug_038
342ad3e62 Fix bug in calling VirtualFree()                                      bug_038
5b5cd7931 Able to build PAL for all platforms easily                            bug_038
e1a98a47c Update build_pal_demo.sh message                                      
6733ca433 Build pal_demo.zip                                                    
f17a7235e Fix bug in pal_demo: unregister_pal() does not free record            
```

#### `Bug: did not block microcode update`
```
c53397d9c Ignore WRMSR to IA32_BIOS_UPDT_TRIG                                   bug_039
221e7b84b Set hypervisor present flag in CPUID                                  bug_039
```

#### `Bug: VGA driver references NULL pointer`
```
da97a50e1 Fix unexpected content on screen when debugging with VGA              bug_044
```

#### `Feature: decrease artifact size`
```
4d6217bd5 Use sparse file to save build disk space                              bug_047
549eb6838 Make xmhf-mm.o sparse                                                 
```

#### `Bug: xmhf_xcphandler_idt_start() incorrectly uses .fill`
```
3b40aa137 Fix 2 compile warnings                                                
```

#### `Feature: PR: Intel DMAP support`
```
0240f2b7a Try to add DMAP support to XMHF                                       bug_050
921e1e1e5 Miao xmhf64 (#2)                                                      
a1fdf7b2c Solve regression that PALs cannot run with DMAP (assertion error)     
1e456c131 Move ifdefs in a1fdf7b2c into hypapp                                  
8867950a1 Fix a bug when running DMAP on 32-bit XMHF on HP 2540p                
74026e537 Miao xmhf64 (#4)                                                      
2d1aa5f5f Export <xmhf_get_machine_paddr_range>, because hypapps may need to us 
```

#### `Feature: allow guest to change MTRR`
```
c9bb82a98 Implement inefficient MTRR change handling                            bug_054
70e58b6f5 Support notifying hypapp about MTRR change                            bug_054
f068c9059 Implement MTRR read / write failure handling                          bug_054
02343124d Skip EPT update when modifying MTRR for simple cases                  bug_054
```

#### `Feature: optimize build process`
```
2dfada581 Skip compiling xmhf-dmaprot when DMAP=n                               bug_056
0fc1584cb Allow parallel build of runtimecomponents                             bug_056
8af91ba58 Move .palign_data to .bss.palign_data to save space in .o files       bug_056
1fc057b46 Move .stack to .bss.stack to save more space                          bug_056
87eb884b3 No need to compile bplt-x86vmx-mtrrs.c when DRT=n                     bug_056
82bf15474 Do not compile txt_acmod.c if DRT=n                                   bug_056
b25354fad Remove unneeded header files when DRT=n                               bug_056
74ea1e2c4 Fix compile error in amd64                                            bug_056
e2a866a5f Remove libtommath.a from bootloader in amd64, because it is not used  bug_056
023c78fa0 Rename ADDL_LIBS32 to ADDL_LIBS_BOOTLOADER                            bug_056
31425b931 Create ADDL_INCLUDES_BOOTLOADER to reduce bootloader's includes       bug_056
9e1aa4b5a Split the subdir build target to more targets (to increase parallelis bug_056
84886fc6f Replace .a file names with variables                                  bug_056
cc879edbc Fix problem caused by 1fc057b46: force objcopy to include .bss-like s bug_056
6fa884f17 Update .github/build.sh                                               bug_056
cd81de2b9 Move vmx_eap_zap() to runtime.c (solves compile regression)           bug_056
04b9121c7 Remove invalid Makefile targets                                       bug_057
d9bed08ba Define runtime.mk to capture common logic in runtime component Makefi bug_057
a65ddfa27 Rename *.x86.o to *.i386.o                                            bug_057
70636de65 Update runtime.mk docs                                                bug_057
be9cba77f Generate dependency files                                             bug_057
23f92f882 Remove -mno-sse5 from CFLAGS, as this argument disappears as of gcc v bug_057
548d2a911 Fix compile error in tpm_extra.c: cannot use uninitialized stack as r bug_057
5fffd8d20 Fix compile error in rijndael.c                                       bug_057
5d8dfb8e5 Fix compile error in sha2.c                                           bug_057
16f6c8b6a Fix compile warning in part-x86vmx.c                                  bug_057
2bd0b9800 Fix linker script for compiling on Fedora                             bug_057
10a91e40c Add instructions to compile XMHF on Fedora                            bug_057
a3b624ebd Update build.sh to detect Fedora platform                             bug_057
c07296815 Update docs                                                           bug_057
6628105a5 Prevent running hypapp's autogen.sh every time remaking XMHF          
40581d213 Fix typo in sha1 computation                                          
```

#### `Feature: support -O3 compile`
```
3de515930 Support setting optimization level                                    
13ff967ee Change HALT() implementation to fix compile errors in -O3             
2a50e9668 Fix incorrect use of HALT_ON_ERRORCOND()                              
7ccb2e7ae Fix compiler error in -O1 (make sure compiler knows assert() never re bug_058
8230fe894 Fix compile error in hpt.c: not checked else condition                bug_058
f947b369c Fix -O1 compile error in part-x86vmx.c: remove inline asm of CPUID    bug_058
8ff33cbef Fix -O1 compile error in part-x86{vmx,svm}.c: use volatile to prevent bug_058
9ce9ef63e Add sections in *.lds.S to support -O3 compile                        bug_058
7263d8411 Add sections in *.lds.S to support -O3 compile for debug info         bug_058
08fcdb596 Add sections in *.lds.S to support -O3 compile for debug info         bug_058
98b48c6aa Fix Debian -O3 compile error: hint to compiler that EU_CHK functions  bug_058
1f972ecc1 Support -O3 compile in build.sh                                       bug_058
b94f3aa19 Fix build.yml                                                         bug_058
054820a3f Fix runtime error in -O3: make some global variables volatile         bug_058
58182650d Fix runtime error in -O3: make quiesce signal global variables volati bug_058
7a41602ef Make sipireceived volatile                                            bug_058
822842a83 Make g_appmain_success_counter volatile                               bug_058
```

#### `Feature: make sl+rt position independent`
```
030a30f41 Make macro __TARGET_BASE depend on __TARGET_BASE_SL                   bug_061
50ca0078f Run preprocessor on runtime lds files                                 bug_061
f71533872 Fix error in preprocessing runtime-x86-i386.lds.S                     bug_061
eb842cc3e Merge runtime LD scripts                                              bug_061
efb5f4598 Run preprocessor on sl LD script                                      bug_061
69ff2c15c Rename runtime and sl ld scripts                                      bug_061
```

#### `Feature: CI (Jenkins, CircleCI)`
```
61f4e88a7 First try of Jenkins                                                  bug_063
23f049843 Debug Jenkinsfile                                                     bug_063
595da9d54 Use git clean                                                         bug_063
28d01dbe4 Update Jenkins CI                                                     bug_063
1ca738d78 Debug test2.py for Jenkins                                            bug_063
53b2c9151 Add lock to println                                                   bug_063
f1a66b0ef Circleci project setup (#6)                                           bug_063
05943b8e0 Update Circle CI config.yml                                           bug_063
36879f183 Implement _optimize_x86vmx_intercept_handler to run common intercepts bug_064
8f4d34bef Enable Circle CI testing using 1 CPU                                  bug_064
247b937c0 Optimize EPT and #DB for handling LAPIC EOI                           bug_064
b5c4c40aa Fix compile error caused by 247b937c0                                 bug_064
afb602b39 Remove TODO                                                           bug_064
7abab5b91 Change to SMP = 2 for Circle CI                                       bug_064
016013a8c Specify CircleCI resource class                                       
5c29efd57 Fix bug in xmhf/src/xmhf-core/Makefile to correctly parallel remake   bug_066
73bd018f7 Use debugfs to install XMHF in CI testing                             bug_069
2915cb789 Force add ignored files                                               bug_069
3c793444e Update Circle CI test script                                          bug_069
a830c3af4 Remove unnecessary files                                              bug_069
e61506a5f Use debugfs -f                                                        bug_069
18ecd868e Prevent commands containing paths in debugfs                          bug_069
```

#### `Feature: support x2APIC`
```
f5c27d852 Replace #DB exception with monitor trap                               bug_065
ed054072d Disable NMI during LAPIC interception                                 bug_065
ae0d0ffea Intercept exceptions during LAPIC interception                        bug_065
9a15c9cd4 Fix OPTIMIZE_NESTED_VIRT for LAPIC logic change                       bug_065
ac146f45b Fix typo                                                              bug_065
876b85972 Modify .jenkins/test2.py to test faster locally                       bug_065
a13c6bfe1 Support x2APIC                                                        bug_065
64b45703b Revert "Replace #DB exception with monitor trap"                      bug_065
d6d3cea2d Clean up for reverting monitor trap                                   bug_065
611d3ddee Update for Circle CI                                                  bug_065
833101058 Store artifacts                                                       bug_065
02ab74d97 Clean up code                                                         bug_065
3d1952ab3 Fix i386 Circle CI                                                    bug_065
734901443 Indentation                                                           
1a3f267cd Update automated testing time                                         
```

#### `Feature: documentation for hpt.c`
```
1e4548f70 Add comments for hpt.c                                                bug_067
0bd3d8ce7 Comment hpto.c and hpt.h                                              bug_067
e8ed5593f Add documentation for hptw.c                                          bug_067
cb825af84 Add a little bit documentation for hptw_emhf.c                        bug_067
```

#### `Bug: in unused nmm function`
```
007ab17d2 Remove nmm_*_gpaddr() functions                                       bug_067
```

#### `Feature: Intel microcode update`
```
e995b81e3 Implement microcode update                                            bug_067
d0f2be0fb Enable ucode in CI                                                    bug_067
```

