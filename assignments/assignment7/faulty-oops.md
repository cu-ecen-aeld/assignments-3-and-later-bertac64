# Analysis of the oops error caused by the command: echo "Hello world" > /dev/faulty

## The Error
The operative system detected the tentative to write something using a null pointer:
> Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000

## The message reported:
- the memory abort info:
```
	ESR = 0x96000045
	EC = 0x25: DABT (current EL), IL = 32 bits
	SET = 0, FnV = 0
	EA = 0, S1PTW = 0
	FSC = 0x05: level 1 translation fault
```
- the abort info:
```
	ISV = 0, ISS = 0x00000045
	CM = 0, WnR = 1
```
- the user pgtable info:
```
	4k pages, 39-bit VAs, pgdp=0000000042306000
```
- the type of internal error:
```
	Oops: 96000045 [#1] SMP
```
- the list of the active loaded modules:
```
	scull(0) faulty(0) hello(0)
```
- The cpu involved (0), the PID (160), the Comm (sh) the Tainted (G 0 5.15.18 #1)
- the reference to the hardware:
```
	Hardware name: linux,dummy-virt
```
- the pstate, the program counter and the registers
```
	pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
	pc : faulty_write+0x14/0x20 [faulty]
	lr : vfs_write+0xa8/0x2b0
	sp : ffffffc008cb3d80
	x29: ffffffc008cb3d80 x28: ffffff80020f72c0 x27: 0000000000000000
	x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
	x23: 0000000040001000 x22: 000000000000000c x21: 0000005584bb2670
	x20: 0000005584bb2670 x19: ffffff80020ad500 x18: 0000000000000000
	x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
	x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
	x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
	x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
	x5 : 0000000000000001 x4 : ffffffc000705000 x3 : ffffffc008cb3df0
	x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
```
- the result of a call trace is shown:
```
	faulty_write+0x14/0x20 [faulty]
	ksys_write+0x68/0x100
	__arm64_sys_write+0x20/0x30
	invoke_syscall+0x54/0x130
	el0_svc_common.constprop.0+0x44/0xf0
	do_el0_svc+0x40/0xa0
	el0_svc+0x20/0x60
	el0t_64_sync_handler+0xe8/0xf0
	el0t_64_sync+0x1a0/0x1a4
	Code: d2800001 d2800000 d503233f d50323bf (b900003f)
```
At the end the operative system exited the user and restarted asking for a new login


