/*
 * Test harness for bhyve instruction emulator
 */

#include <sys/types.h>
#include <sys/errno.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "vmm_stubs.h"

static uint64_t vm_regs[VM_REG_LAST];

struct mem_cell {
	uint64_t   addr;
	uint64_t   val;
};

int
vm_get_register(void *ctx, int vcpu, int reg, uint64_t *retval)
{

	if (reg >= VM_REG_GUEST_RAX &&
	    reg < VM_REG_LAST) {
		*retval = vm_regs[reg];
		return (0);
	}
		
	return (EINVAL);
}

int
vm_set_register(void *ctx, int vcpu, int reg, uint64_t val)
{

	if (reg >= VM_REG_GUEST_RAX &&
	    reg < VM_REG_LAST) {
		vm_regs[reg] = reg;
		return (0);
	}

	return (EINVAL);
}

/*
 * Memory r/w callback functions
 */
int
test_mread(void *vm, int cpuid, uint64_t gpa, uint64_t *rval, int rsize,
    void *arg)
{
	struct mem_cell *mc;

	mc = arg;

	if (mc->addr == gpa) {
		*rval = mc->val;
		return (0);
	}

	return (EINVAL);
}

int
test_mwrite(void *vm, int cpuid, uint64_t gpa, uint64_t wval,
    int wsize, void *arg)
{
	struct mem_cell *mc;

	mc = arg;

	if (mc->addr == gpa) {
		mc->val = wval;
		return (0);
	}
	
	return (EINVAL);
}

void
panic(char *str, ...)
{

	printf("panic: %s\n", str);
}


main()
{
	struct mem_cell mc;
	struct vie vie;
	uint64_t gla, gpa;
	int err;

	/*
	 * ICLASS: AND         CATEGORY: LOGICAL               EXTENSION: BASE              IFORM: AND_GPRv_MEMv           ISA_SET: I86
     
     * SHORT: and eax, dword ptr [ecx+0xf0]                  AND r16/32, r/m16/32
	 * (rcx = lapic address, 0xff000000)
	 * and    0xf0(%rcx),%eax                         
	 * 0x23 0x81 0xf0 0x00 0x00 0x00
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0x23;
	vie.inst[1] = 0x81;
	vie.inst[2] = 0xf0;
	vie.inst[3] = 0x00;
	vie.inst[4] = 0x00;
	vie.inst[5] = 0x00;
	vie.num_valid = 6;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);


	/*
	 * ICLASS: AND         CATEGORY: LOGICAL              EXTENSION: BASE            IFORM: AND_MEMv_IMMz              ISA_SET: I86

     * SHORT: and dword ptr [eax+0xf0], 0xfffffeff        AND r/m32, imm32
     * (rax = lapic address, 0xff000000)
     * andl   $0xfffffeff,0xf0(%rax)
     * 0x81 0xa0 0xf0 0x00 0x00 0x00 0xff 0xfe 0xff 0xff
     */

	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0xff000000;
	vie.inst[0] = 0x81;
	vie.inst[1] = 0xa0;
	vie.inst[2] = 0xf0;
	vie.inst[3] = 0x00;
	vie.inst[4] = 0x00;
	vie.inst[5] = 0x00;
	vie.inst[6] = 0xff;
	vie.inst[7] = 0xfe;
	vie.inst[8] = 0xff;
	vie.inst[9] = 0xff;
	vie.num_valid = 10;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000a1aa;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xa0aa);

	/*
	 * SHORT: and dword ptr [eax+0xf0], 0xffef0000             AND r/m16, imm16
     * (rax = lapic address, 0xff000000)
     * andl   $0x0000feff,0xf0(%rax)
     * 0x81 0xa0 0xf0 0x00 0x00 0x00 0xff 0xfe 0x00 0x00
     */

	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0xff000000;
	vie.inst[0] = 0x81;
	vie.inst[1] = 0xa0;
	vie.inst[2] = 0xf0;
	vie.inst[3] = 0x00;
	vie.inst[4] = 0x00;
	vie.inst[5] = 0x00;
	vie.inst[6] = 0xff;
	vie.inst[7] = 0xfe;
	vie.inst[8] = 0x00;
	vie.inst[9] = 0x00;
	vie.num_valid = 10;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000a1aa;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xa0aa);

    /*
     * ICLASS: AND          CATEGORY: LOGICAL           EXTENSION: BASE         IFORM: AND_MEMb_IMMb_80r4               ISA_SET: I86
     
     * SHORT: and byte ptr [eax+0xf0], 0xff                AND r/m8, imm8
     * (rax = lapic address, 0xff000000)
     * andl   $0xff,0xf0(%rax)
     * 0x80 0xa0 0xf0 0x00 0x00 0x00 0xff 
     */


     memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0xff000000;
	vie.inst[0] = 0x80;
	vie.inst[1] = 0xa0;
	vie.inst[2] = 0xf0;
	vie.inst[3] = 0x00;
	vie.inst[4] = 0x00;
	vie.inst[5] = 0x00;
	vie.inst[6] = 0xff;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000a1aa;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xa0aa);

    /*
	 * SHORT: and byte ptr [eax+0xf0], 0xff                AND r/m16/32, imm8
     * (rax = lapic address, 0xff000000)
     * andl   $0xff,0xf0(%rax)
     * 0x83 0xa0 0xf0 0x00 0x00 0x00 0xff 
     */


     memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0xff000000;
	vie.inst[0] = 0x83;
	vie.inst[1] = 0xa0;
	vie.inst[2] = 0xf0;
	vie.inst[3] = 0x00;
	vie.inst[4] = 0x00;
	vie.inst[5] = 0x00;
	vie.inst[6] = 0xff;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000a1aa;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xa0aa);


	/*
	 * ICLASS: MOV             CATEGORY: DATAXFER            EXTENSION: BASE           IFORM: MOV_MEMb_GPR8             ISA_SET: I86

     * SHORT: mov byte ptr [ecx+0x58ecdc05], cl               MOV r/m8, r8
     * mov    %r8d,5827804(%rip)
	 * 88 89 05 dc ec 58 00
	 * rip -> 0xffffffff8046539d
	 * var -> 0xffffffff809f4080
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 7;	
	vm_regs[VM_REG_GUEST_R8] = 0xa5a5a5a5deadbeefULL;
	vie.inst[0] = 0x88;
	vie.inst[1] = 0x89;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.inst[6] = 0x00;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff000080;
	mc.val  = 0;
	gpa = 0xff000080;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);


    /* 
     *ICLASS: MOV              CATEGORY: DATAXFER              EXTENSION: BASE             IFORM: MOV_MEMv_GPRv      ISA_SET: I86

     * SHORT: mov dword ptr [eax+0x58ecdc05], ecx               MOV r/m16/32, r16/32
     * mov    %r8d,5827804(%rip)
	 * 89 88 05 dc ec 58 00
	 * rip -> 0xffffffff8046539d
	 * var -> 0xffffffff809f4080
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 7;	
	vm_regs[VM_REG_GUEST_R8] = 0xa5a5a5a5deadbeefULL;
	vie.inst[0] = 0x89;
	vie.inst[1] = 0x88;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.inst[6] = 0x00;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff000080;
	mc.val  = 0;
	gpa = 0xff000080;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);
 
	/*
	 * ICLASS: MOV          CATEGORY: DATAXFER             EXTENSION: BASE             IFORM: MOV_GPR8_MEMb                ISA_SET: I86

     * SHORT: mov cl, byte ptr [ecx+0x58ecdc05]                 MOV r8, r/m8
     * mov    %r8d,5827804(%rip)
	 * 8a 89 05 dc ec 58 00
	 * rip -> 0xffffffff8046539d
	 * var -> 0xffffffff809f4080
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 7;	
	vm_regs[VM_REG_GUEST_R8] = 0xa5a5a5a5deadbeefULL;
	vie.inst[0] = 0x8a;
	vie.inst[1] = 0x89;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.inst[6] = 0x00;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff000080;
	mc.val  = 0;
	gpa = 0xff000080;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/* 
	 * ICLASS: MOV             CATEGORY: DATAXFER               EXTENSION: BASE            IFORM: MOV_GPRv_MEMv           ISA_SET: I86
     * SHORT: mov ecx, dword ptr [eax+0x58ecdc05]               MOV r16/32, r/m16/32
     * mov    %r8d,5827804(%rip)
	 * 8b 88 05 dc ec 58 00
	 * rip -> 0xffffffff8046539d
	 * var -> 0xffffffff809f4080
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 7;	
	vm_regs[VM_REG_GUEST_R8] = 0xa5a5a5a5deadbeefULL;
	vie.inst[0] = 0x8b;
	vie.inst[1] = 0x88;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.inst[6] = 0x00;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff000080;
	mc.val  = 0;
	gpa = 0xff000080;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/* 
	 *ICLASS: MOV          CATEGORY: DATAXFER              EXTENSION: BASE              IFORM: MOV_OrAX_MEMv         ISA_SET: I86
     * SHORT: mov eax, dword ptr [0x0]                      MOV eax, moffs 16/32
     * mov    %r8d,5827804(%rip)  
	 * a1 00 00 00 00 
	 * rip -> 0xffffffff8046539d
	 * var -> 0xffffffff809f4080
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 5;	
	vm_regs[VM_REG_GUEST_R8] = 0xa5a5a5a5deadbeefULL;
	vie.inst[0] = 0xa1;
	vie.inst[1] = 0x00;
	vie.inst[2] = 0x00;
	vie.inst[3] = 0x00;
	vie.inst[4] = 0x00;
	vie.num_valid = 5;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff000080;
	mc.val  = 0;
	gpa = 0xff000080;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);


    /* 
     *ICLASS: MOV             CATEGORY: DATAXFER             EXTENSION: BASE               IFORM: MOV_MEMv_OrAX          ISA_SET: I86
     * SHORT: mov dword ptr [0x0], eax                        MOV moffs16/32, eax
     * mov    %r8d,5827804(%rip)  
	 * a3 00 00 00 00 
	 * rip -> 0xffffffff8046539d
	 * var -> 0xffffffff809f4080
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 5;	
	vm_regs[VM_REG_GUEST_R8] = 0xa5a5a5a5deadbeefULL;
	vie.inst[0] = 0xa3;
	vie.inst[1] = 0x00;
	vie.inst[2] = 0x00;
	vie.inst[3] = 0x00;
	vie.inst[4] = 0x00;
	vie.num_valid = 5;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff000080;
	mc.val  = 0;
	gpa = 0xff000080;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);




	/*
	 * ICLASS: MOV             CATEGORY: DATAXFER            EXTENSION: BASE           IFORM: MOV_MEMv_IMMz             ISA_SET: I86
     * SHORT: mov dword ptr [eax+0x58ecdc05], 0xffff                         MOV r/m 16, imm16
	 * movl   $0x1ef,5872021(%rip)
	 * c7 05 95 99 59 00 ef
	 * c7 80 05 dc ec 58 ff ff 00 00
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 9;	
	vie.inst[0] = 0xc7;
	vie.inst[1] = 0x80;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.inst[6] = 0xff;
	vie.inst[7] = 0xff;
	vie.inst[7] = 0x00;
	vie.inst[8] = 0x00;
	vie.num_valid = 9;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/*
	 * ICLASS: MOV             CATEGORY: DATAXFER            EXTENSION: BASE           IFORM: MOV_MEMv_IMMz             ISA_SET: I86
     * SHORT: mov dword ptr [eax+0x58ecdc05], 0xffffffff                        MOV r/m 32, imm32
	 * movl   $0x1ef,5872021(%rip)
	 * c7 05 95 99 59 00 ef
	 * c7 80 05 dc ec 58 ff ff ff ff
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 9;	
	vie.inst[0] = 0xc7;
	vie.inst[1] = 0x80;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.inst[6] = 0xff;
	vie.inst[7] = 0xff;
	vie.inst[7] = 0xff;
	vie.inst[8] = 0xff;
	vie.num_valid = 9;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/*
	 * ICLASS: MOV             CATEGORY: DATAXFER              EXTENSION: BASE              IFORM: MOV_MEMb_IMMb            ISA_SET: I86
     * SHORT: mov byte ptr [ecx+0x58ecdc05], 0xff              MOV r/m8. imm8
	 * c6 81 05 dc ec 58 ff
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 7;	
	vie.inst[0] = 0xc6;
	vie.inst[1] = 0x81;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.inst[6] = 0xff;
	vie.num_valid = 7;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/* 
	 * ICLASS: MOVZX            CATEGORY: DATAXFER            EXTENSION: BASE          IFORM: MOVZX_GPRv_MEMb           ISA_SET: I386
     * SHORT: movzx ax, byte ptr [ecx+0x58ecdc05]            MoVZX r16, r/m8 (Mov with Zero-extend)
     * 66 0f b6 81 05 dc ec 58
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 8;	
	vie.inst[0] = 0x66;
	vie.inst[1] = 0x0f;
	vie.inst[2] = 0xb6;
	vie.inst[3] = 0x81;
	vie.inst[4] = 0x05;
	vie.inst[5] = 0xdc;
	vie.inst[6] = 0xec;
	vie.inst[7] = 0x58;
	vie.num_valid = 8;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/* 
	 * ICLASS: MOVZX            CATEGORY: DATAXFER            EXTENSION: BASE          IFORM: MOVZX_GPRv_MEMb           ISA_SET: I386
     * SHORT: movzx eax, byte ptr [ecx+0x58ecdc05]            MoVZX r32, r/m8 (Mov with Zero-extend)
     * 0f b6 81 05 dc ec 58
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 7;	
	vie.inst[0] = 0x0f;
	vie.inst[1] = 0xb6;
	vie.inst[2] = 0x81;
	vie.inst[3] = 0x05;
	vie.inst[4] = 0xdc;
	vie.inst[5] = 0xec;
	vie.inst[6] = 0x58;
	vie.num_valid = 7;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);
 
    /*  
	 * ICLASS: MOVZX            CATEGORY: DATAXFER           EXTENSION: BASE           IFORM: MOVZX_GPRv_MEMw         ISA_SET: I386
     * SHORT: movzx eax, word ptr [ecx+0x58ecdc05]          MOV r16/32, r/m16
     * 0f b7 81 05 dc ec 58
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 7;	
	vie.inst[0] = 0x0f;
	vie.inst[1] = 0xb7;
	vie.inst[2] = 0x81;
	vie.inst[3] = 0x05;
	vie.inst[4] = 0xdc;	
	vie.inst[5] = 0xec;
	vie.inst[6] = 0x58;
	vie.num_valid = 7;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/*
	 * ICLASS: MOVSX             CATEGORY: DATAXFER             EXTENSION: BASE            IFORM: MOVSX_GPRv_MEMb         ISA_SET: I386
     * SHORT: movsx ax, byte ptr [ecx+0x58ecdc05]               MOVSX r16, r/m8
     * 66 0f be 81 05 dc ec 58
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 8;	
	vie.inst[0] = 0x66;
	vie.inst[1] = 0x0f;
	vie.inst[2] = 0xbe;
	vie.inst[3] = 0x81;
	vie.inst[4] = 0x05;
	vie.inst[5] = 0xdc;
	vie.inst[6] = 0xec;
	vie.inst[7] = 0x58;
	vie.num_valid = 8;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/* 
	 * ICLASS: MOVSX               CATEGORY: DATAXFER            EXTENSION: BASE           IFORM: MOVSX_GPRv_MEMb        ISA_SET: I386
     * SHORT: movsx eax, byte ptr [ecx+0x58ecdc05]               MOVSX r32, r/m8
     * 0f be 81 05 dc ec 58
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 7;	
	vie.inst[0] = 0x0f;
	vie.inst[1] = 0xbe;
	vie.inst[2] = 0x81;
	vie.inst[3] = 0x05;
	vie.inst[4] = 0xdc;
	vie.inst[5] = 0xec;
	vie.inst[6] = 0x58;
	vie.num_valid = 7;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);
    /*
	 * ICLASS: MOVSB              CATEGORY: STRINGOP                EXTENSION: BASE               IFORM: MOVSB             ISA_SET: I86
     * SHORT: movsb byte ptr [di], byte ptr [si]                      MOVSB m8, m8
     * 67 a4                                                    move data from string to string
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 2;	
	vie.inst[0] = 0x67;
	vie.inst[1] = 0xa4;
	vie.num_valid = 2;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	 /*
	 * ICLASS: MOVSW             CATEGORY: STRINGOP             EXTENSION: BASE                 IFORM: MOVSW            ISA_SET: I86
     * SHORT: movsw word ptr [edi], word ptr [esi]                 MOVSW m16/32, m16/32
     * 66 a5                                                move data from string to string
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 2;	
	vie.inst[0] = 0x66;
	vie.inst[1] = 0xa5;
	vie.num_valid = 2;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);
    /*
	 * ICLASS: MOVSD              CATEGORY: STRINGOP              EXTENSION: BASE             IFORM: MOVSD               ISA_SET: I386
     * SHORT: movsd dword ptr [edi], dword ptr [esi]                     MOVSW m32, m32
     * A5                                                    move data from string to string
	 * rip -> ffffffff813d2751
	 * val -> ffffffff8196c0f0
	 * pa -> 0xfee000f0
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	/* RIP-relative is from next instruction */
	vm_regs[VM_REG_GUEST_RIP] = 0xffffffff8046539d + 1;	
	vie.inst[0] = 0xa5;
	vie.num_valid = 1;

    gla = 0;
    err = vmm_decode_instruction(NULL, 0, gla, &vie);
    assert(err == 0);

    mc.addr = 0xfee000f0;
	mc.val  = 0;
	gpa = 0xfee000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

    /* 
	 * ICLASS: OR             CATEGORY: LOGICAL           EXTENSION: BASE               IFORM: OR_GPRv_MEMv             ISA_SET: I86
     * SHORT: or ax, word ptr [ecx+0x5dcec58]
     * (rcx = lapic address, 0xff000000)                 OR reg16, r/m16/32
	 * 66 0b 81 05 dc ec 58
	 */

	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0x66;
	vie.inst[1] = 0x0b;
	vie.inst[2] = 0x81;
	vie.inst[3] = 0x05;
	vie.inst[4] = 0xdc;
	vie.inst[5] = 0xec;
	vie.inst[6] = 0x58;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000ff;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000ff;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/* 
	 * ICLASS: OR             CATEGORY: LOGICAL           EXTENSION: BASE               IFORM: OR_GPRv_MEMv             ISA_SET: I86
     * SHORT: or eax, word ptr [ecx+0x5dcec58]
     * (rcx = lapic address, 0xff000000)                  OR reg32, r/m16/32
	 * 0b 81 05 dc ec 58
	 */

	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0x0b;
	vie.inst[1] = 0x81;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.num_valid = 6;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000ff;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000ff;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	   
	 /*
	 * ICLASS: CMP             CATEGORY: BINARY               EXTENSION: BASE                 IFORM: CMP_MEMv_GPRv           ISA_SET: I86
     * SHORT: cmp word ptr [ecx+0x58ecdc05], ax               CMP r/m16/32, reg16   
	 * (rcx = lapic address, 0xff000000)  
	 * 66 39 81 05 dc ec 58
	 */

	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vm_regs[VM_REG_GUEST_RFLAGS]=0xff000000;
	vie.inst[0] = 0x66;
	vie.inst[1] = 0x39;
	vie.inst[2] = 0x81;
	vie.inst[3] = 0x05;
	vie.inst[4] = 0xdc;
	vie.inst[5] = 0xec;
	vie.inst[6] = 0x58;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

    /*
	 * ICLASS: CMP             CATEGORY: BINARY               EXTENSION: BASE                 IFORM: CMP_MEMv_GPRv           ISA_SET: I86
     * SHORT: cmp dword ptr [ecx+0x58ecdc05], eax             CMP r/m16/32, reg32   
	 * (rcx = lapic address, 0xff000000)  
	 * 39 81 05 dc ec 58
	 */
	
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vm_regs[VM_REG_GUEST_RFLAGS]=0xff000000;
	vie.inst[0] = 0x39;
	vie.inst[1] = 0x80;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.num_valid = 6;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);
    
	/*
	 * ICLASS: CMP                CATEGORY: BINARY                 EXTENSION: BASE               IFORM: CMP_GPRv_MEMv       ISA_SET: I86
     * SHORT: cmp ax, word ptr [ecx+0x58ecdc05]                     CMP r16, r/m16/32
     * (rcx = lapic address, 0xff000000)  
	 * 66 3b 81 05 dc ec 58
	 */
	
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vm_regs[VM_REG_GUEST_RFLAGS]=0xff000000;
	vie.inst[0] = 0x66;
	vie.inst[1] = 0x3b;
	vie.inst[2] = 0x81;
	vie.inst[3] = 0x05;
	vie.inst[4] = 0xdc;
	vie.inst[5] = 0xec;
	vie.inst[6] = 0x58;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/*
	 * ICLASS: CMP                CATEGORY: BINARY                 EXTENSION: BASE               IFORM: CMP_GPRv_MEMv       ISA_SET: I86
     * SHORT: cmp eax, dword ptr [ecx+0x58ecdc05]                  CMP r32, r/m16/32
     * (rcx = lapic address, 0xff000000)  
	 * 3b 81 05 dc ec 58
	 */
	
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vm_regs[VM_REG_GUEST_RFLAGS]=0xff000000;
	vie.inst[0] = 0x3b;
	vie.inst[1] = 0x81;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x58;
	vie.num_valid = 6;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);


	/*
	 * ICLASS: BT              CATEGORY: BITBYTE                   EXTENSION: BASE              IFORM: BT_MEMv_IMMb       ISA_SET: I386
     * SHORT: bt word ptr [ecx+0x58ecdc05], 0xff                    BT r/m16, imm8  
     * 0x66, 0x0F, 0xBA, 0xA1, 0x05, 0xDC, 0xEC, 0x58, 0xFF
     */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0xff000000;
	vm_regs[VM_REG_GUEST_RFLAGS]=0xff000000;
	vie.inst[0] = 0x66;
	vie.inst[1] = 0x0f;
	vie.inst[2] = 0xba;
	vie.inst[3] = 0xa1;
	vie.inst[4] = 0x05;
	vie.inst[5] = 0xdc;
	vie.inst[6] = 0xec;
	vie.inst[7] = 0x58;
	vie.inst[8] = 0xff;
	vie.num_valid = 9;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000a1aa;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);s
	assert(err == 0);
	assert(mc.val == 0xa0aa);
    

    /*
	 * ICLASS: BT              CATEGORY: BITBYTE                 EXTENSION: BASE             IFORM: BT_MEMv_IMMb          ISA_SET: I386
     * SHORT: bt dword ptr [ecx+0x58ecdc05], 0xff                 BT r/m32, imm8
     * 0x0F, 0xBA, 0xA1, 0x05, 0xDC, 0xEC, 0x58, 0xFF
     */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0xff000000;
	vm_regs[VM_REG_GUEST_RFLAGS]=0xff000000;
	vie.inst[0] = 0x0f;
	vie.inst[1] = 0xba;
	vie.inst[2] = 0xa1;
	vie.inst[3] = 0x05;
	vie.inst[4] = 0xdc;
	vie.inst[5] = 0xec;
	vie.inst[6] = 0x58;
	vie.inst[7] = 0xff;
	vie.num_valid = 8;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000a1aa;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);s
	assert(err == 0);
	assert(mc.val == 0xa0aa);
    

	
	/*
	 * (rax = lapic address, 0xff000000)
     * mov   $ff,0xf0(%rax)             || mov r/m8, imm8  ( XXX Group 11 extended opcode - not just MOV )
     * 0xc6 0xf0 0x00 0x00 0x00 0xff
     */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0xff000000;
	vm_regs[VM_REG_GUEST_RFLAGS]=0xff000000;
	vie.inst[0] = 0xc6;
	vie.inst[1] = 0xf0;
	vie.inst[2] = 0x00;
	vie.inst[3] = 0x00;
	vie.inst[4] = 0x00;
	vie.inst[5] = 0xff;
	vie.num_valid = 6;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000f0;
	mc.val  = 0x0000a1aa;
	gpa = 0xff0000f0;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xa0aa);

	/*
	 * ICLASS: SUB                 CATEGORY: BINARY               EXTENSION: BASE            IFORM: SUB_GPRv_MEMv        ISA_SET: I86
     * SHORT: sub ax, word ptr [ecx+0x5ecdc05]                     SUB r16, r/m16
	 * 0x66 0x2b 0x81 0x05 0xdc 0xec 0x05 
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0x66;
	vie.inst[1] = 0x2b;
	vie.inst[2] = 0x81;
	vie.inst[3] = 0x05;
	vie.inst[4] = 0xdc;
	vie.inst[5] = 0xec;
	vie.inst[6] = 0x05;
	vie.num_valid = 7;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000ff;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000ff;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);


	/*
	 * ICLASS: SUB                 CATEGORY: BINARY               EXTENSION: BASE            IFORM: SUB_GPRv_MEMv        ISA_SET: I86
     * SHORT: sub eax, dword ptr [ecx+0x5ecdc05]                   SUB r32, r/m32
	 * 0x2b 0x81 0x05 0xdc 0xec 0x05 
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0x2b;
	vie.inst[1] = 0x81;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.inst[4] = 0xec;
	vie.inst[5] = 0x05;
	vie.num_valid = 6;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000ff;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000ff;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/*
	 * ICLASS: STOSB                 CATEGORY: STRINGOP                 EXTENSION: BASE             IFORM: STOSB           ISA_SET: I86
     *  SHORT: stosb byte ptr [edi]                                       STOS m8, r8
	 * 0xAA; 
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0xaa;
	vie.num_valid = 1;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000ff;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000ff;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/*
	 * ICLASS: STOSW                   CATEGORY: STRINGOP               EXTENSION: BASE            IFORM: STOSW            ISA_SET: I86
     * SHORT: stosw word ptr [edi]                                       STOS m16, r16
	 * 0x66 0xAB
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0x66;
	vie.inst[1] = 0xab;
	vie.num_valid = 2;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000ff;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000ff;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/*
	 * ICLASS: STOSD              CATEGORY: STRINGOP                 EXTENSION: BASE               IFORM: STOSD           ISA_SET: I386
     * SHORT: stosd dword ptr [edi]                                   STOS m32, r32
	 * 0xAB; 
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0xab;
	vie.num_valid = 1;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000ff;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000ff;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

    /* 
	 * ICLASS: PUSH              CATEGORY: PUSH                EXTENSION: BASE               IFORM: PUSH_M              ISA_SET: I86
     * SHORT: push dword ptr [ecx+0x5ecdc05]                   PUSH r/m 16/32
     * 0xff 0xb1 0x05 0xdc 0xec 0x05
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0xff;
	vie.inst[1] = 0xb1;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.isnt[4] = 0xec;
	vie.inst[5] = 0x05;
	vie.num_valid = 6;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000ff;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000ff;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);

	/* 
	 * ICLASS: POP               CATEGORY: POP                    EXTENSION: BASE           IFORM: POP_MEMv          ISA_SET: I86
     * SHORT: pop dword ptr [ecx+0x5ecdc05]                       POP r/m 16/32
     * 0x8f 0x81 0x05 0xdc 0xec 0x05
	 */
	memset(&vie, 0, sizeof(struct vie));
	vie.base_register = VM_REG_LAST;
	vie.index_register = VM_REG_LAST;

	vm_regs[VM_REG_GUEST_RAX] = 0x0000aabb;
	vm_regs[VM_REG_GUEST_RCX] = 0xff000000;
	vie.inst[0] = 0x8f;
	vie.inst[1] = 0x81;
	vie.inst[2] = 0x05;
	vie.inst[3] = 0xdc;
	vie.isnt[4] = 0xec;
	vie.inst[5] = 0x05;
	vie.num_valid = 6;

	gla = 0;
	err = vmm_decode_instruction(NULL, 0, gla, &vie);
	assert(err == 0);

	mc.addr = 0xff0000ff;
	mc.val  = 0x0000aa00;
	gpa = 0xff0000ff;
	err = vmm_emulate_instruction(NULL, 0, gpa, &vie,
				      test_mread, test_mwrite,
				      &mc);
	assert(err == 0);
	assert(mc.val == 0xdeadbeef);





   


   

   







	


}
