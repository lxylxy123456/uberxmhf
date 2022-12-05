# Windows 10 Antivirus Problem

## Scope
* Windows 10, all subarch
* Both hyper-v and non hyper-v
* Older Windows may still have this problem
* Good: Linux
* `lhv 68afc420f`
* `lhv-dev 0d1fce092`
* `xmhf64 0d55a8144`
* `xmhf64-dev f01b8ba03`

## Behavior

In Windows 10, when running PAL demo for a long time, see
`EU_CHK( hpt_error_wasInsnFetch(vcpu, errorcode)) failed` assertion error. This
is caused by Windows trying to access memory of a PAL. This was discovered in
`bug_106`, and at that time I guessed that it was antivirus software scanning
the memory.

This problem is also reproducible when a process registers PAL but never uses
it. Just sleep forever.

## Debugging

In `xmhf64-dev b86ef5b72`, serial `20221204135308`, reproduce this problem.
serial is running `test_args32 7 7 7` then `main32 7 7 7`. Can see that when
registering PAL CR3=0x1c7175001, but exception triggers when CR3=0x211e00002.
We can assume that exception is triggered by another process.

In `xmhf64-dev f95d51a72`, dump VCPU during the memory access. See that CS and
SS are at ring 0, but other registers are in ring 3. Is this a kernel thread?

Ideas to debug:
* Let the antivirus software repeat the violating instruction forever
* Randomly disable Windows security features
* Let TV display process memory space

Git `xmhf64-dev 4213042e6`. Basically reuse the code in `xmhf64-dev f95d51a72`,
but print less. This code will ignore the EPT violation and return to user
mode, re-running the same instruction. In Windows task manager can see see
"Service Host: SysMain" with high CPU usage.

This is a service called SysMain in Windows. Can see it in Windows Services
Manager. The description of this service says:
> Maintains and improves system performance over time

Well, it is not exactly antivirus, but it has a similar effect.

We stop this service in Windows, then try running PAL demo for a long time.

Also tested on HP 2540p, XMHF, Hyper-V, Windows 10. looks good.

### Windows Internals

CMU library provides access to "Windows Internals Seventh Edition Part 1:
System architecture, processes, threads, memory management, and more, Seventh
Edition" through <https://learning.oreilly.com/>.

Search "SysMain" in this book, see one match
> During system startup, the Superfetch service (sysmain.dll, hosted in a
> svchost.exe instance, described in the upcoming “Proactive memory management
> (SuperFetch)” section) instructs the Store Manager in the executive through a
> call to NtSetSystemInformation to create a single system store (always the
> first store to be created), to be used by non-UWP applications. Upon app
> startup, each UWP application communicates with the Superfetch service and
> requests the creation of a store for itself.

Can read the book in detail if needed.

### Other Windows versions

In ..., this service is ...
* Windows 11 pro: present
* Windows 8 Pro: not present
* Windows 7 ultimate: not present

On Windows 10 x86, Hyper-V cannot be enabled (even without XMHF). Windows says
the hardware lacks virtualization features. In Microsoft website
<https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/reference/hyper-v-requirements>,
it says Hyper-V requires:
> 64-bit Processor with Second Level Address Translation (SLAT).

## Result

The "SysMain" service in Windows 10 and up will access other process's memory.
Need to disable this service.

