# Developing XMHF

## Compiling

When developing we frequently need to compile the same code in multiple
configurations (e.g. x86, x86_64). We have 3 ways to synchronize code between
different build directories.

1. Use Git. Bug this requires committing the code and push/pull to/from the
   GitHub server.
2. Use overlayfs. Mount source directory as lower file system, and a tmpfs as
   upper file system. Use `overlay.sh` to mount and `unoverlay.sh` to unmount.
   This method is deprecated because changing lower file system in overlay is
   theoreotically undefined behavior.
3. Use symbolic links on tmpfs. `linksrc.sh` will create symbolic links for
   each file to the source directory. The only disadvantage is that creating
   symbolic links takes a little bit of time.

Also consider that the compiled files are a few hundred MBs large (no need to
store them on disk), and gdb commands favors known path (3's code can easily be
changed to link gdb directories). So 3 is the best choice.

The build environment is Ubuntu / Debian. The build tools to be installed is
in `docker/Dockerfile`. In Fedora, a Docker container can be created to build
XMHF.

For example, I use `$XMHF/xmhf64` as source directory. `$XMHF/build32` is build
directory for i386, and `$XMHF/build32` is build directory for amd64.

In a build directory, there are symbolic links to all files in the source
directory (not including `.git`). Also `gdb` links to the directory containing
gdb scripts.

## Using HP machine

This is the same as discussed in `setup.md`. AMT term provides serial output,
and only print debugging can be performed (i.e. no hardware break points etc.)

## Using QEMU

QEMU can be used to assist debugging the booting process of XMHF.
`qemu-system-x86_64` is for 64-bits, and `qemu-system-i386` is for 32-bits.
When using GDB, `qemu-system-x86_64` will cause problems for 32-bits binaries.

Use `-device e1000,netdev=net0 -netdev user,id=net0,hostfwd=tcp::2222-:22` to
map virtual machine's SSH port to host machine.

Use `-smp 4` to control number of cores.

Use `-cpu Haswell,vmx=yes --enable-kvm` to enable VMX instructions.

Use `-serial stdio` to print serial output to stdout. This can be piped to a
file. By grepping for keywords in this file, a shell script can know the
current progress of XMHF's initialization. This can be used to control GDB.

Use `-s` to allow gdb connections.

## Using GDB on QEMU

GDB can be used after serial output prints "eXtensible Modular Hypervisor
Framework". Before that, the CPU is likely to be running GRUB code. If QEMU's
serial output is redirected to a file, a shell script can easily start QEMU
after seeing this line.

Add `--ex 'target remote :::1234'` to command line to connect to QEMU

Add `-x gdb/x64_rt_pre.gdb` to command line can stop 64-bit target at start of
xmhf-runtime. There are other stop points available in the `gdb/` script
direcory.

## Browsing source

OpenGrok can be used to browse the source code.
<https://hub.docker.com/r/opengrok/docker/> can be used to run OpenGrok in
Docker.

Sample Docker command:
```sh
SCRIPT_DIR="$(dirname "$(realpath $BASH_SOURCE)")"
docker run --name 'opengrok-xmhf' -d \
	-v "$SCRIPT_DIR/src:/opengrok/src" \
	-v "$SCRIPT_DIR/data:/opengrok/data" \
	-p "127.0.0.1:8080:8080" \
	-e "INDEXER_OPT=--threads 1" \
	-e "WORKERS=1" \
	--cpus 0.75 \
	opengrok/docker:1.7.30
echo "http://127.0.0.1:8080/"
```

## Editing source

`replace_x86.py` can be used to change `x86` to `x86_64` in the source
directory. It will change if the file path contains `x86_64`, it warn if the
file path does not contain `x86` (in this case, likely need to manually add
ifdefs in source code).

This change is reverted by `bug_050`, see scripts there.

