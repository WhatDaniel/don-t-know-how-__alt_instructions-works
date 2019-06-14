\linux-3.10.0-693.2.2.el7\arch\x86\tools\relocs.c

static const char * const sym_regex_kernel[S_NSYMTYPES] = {
/*
 * Following symbols have been audited. There values are constant and do
 * not change if bzImage is loaded at a different physical address than
 * the address for which it has been compiled. Don't warn user about
 * absolute relocations present w.r.t these symbols.
 */
	[S_ABS] =
	"^(xen_irq_disable_direct_reloc$|"
	"xen_save_fl_direct_reloc$|"
	"VDSO|"
	"__crc_)",

/*
 * These symbols are known to be relative, even if the linker marks them
 * as absolute (typically defined outside any section in the linker script.)
 */
	[S_REL] =
	"^(__init_(begin|end)|"
	"__x86_cpu_dev_(start|end)|"
	"(__parainstructions|__alt_instructions)(|_end)|"
	"(__iommu_table|__apicdrivers|__smp_locks)(|_end)|"
	"__(start|end)_pci_.*|"
	"__(start|end)_builtin_fw|"
	"__(start|stop)___ksymtab(|_gpl|_unused|_unused_gpl|_gpl_future)|"
	"__(start|stop)___kcrctab(|_gpl|_unused|_unused_gpl|_gpl_future)|"
	"__(start|stop)___param|"
	"__(start|stop)___modver|"
	"__(start|stop)___bug_table|"
	"__tracedata_(start|end)|"
	"__(start|stop)_notes|"
	"__end_rodata|"
	"__initramfs_start|"
	"(jiffies|jiffies_64)|"
#if ELF_BITS == 64
	"__per_cpu_load|"
	"init_per_cpu__.*|"
	"__end_rodata_hpage_align|"
	"__vvar_page|"
#endif
	"_end)$"
};


\linux-3.10.0-693.2.2.el7\arch\x86\kernel\alternative.c

extern struct alt_instr __alt_instructions[], __alt_instructions_end[];
extern s32 __smp_locks[], __smp_locks_end[];
void *text_poke_early(void *addr, const void *opcode, size_t len);

/* Replace instructions with better alternatives for this CPU type.
   This runs before SMP is initialized to avoid SMP problems with
   self modifying code. This implies that asymmetric systems where
   APs have less capabilities than the boot processor are not handled.
   Tough. Make sure you disable such features by hand. */

void __init_or_module apply_alternatives(struct alt_instr *start,
					 struct alt_instr *end)
{
	struct alt_instr *a;
	u8 *instr, *replacement;
	u8 insnbuf[MAX_PATCH_LEN];

	DPRINTK("%s: alt table %p -> %p\n", __func__, start, end);
	/*
	 * The scan order should be from start to end. A later scanned
	 * alternative code can overwrite a previous scanned alternative code.
	 * Some kernel functions (e.g. memcpy, memset, etc) use this order to
	 * patch code.
	 *
	 * So be careful if you want to change the scan order to any other
	 * order.
	 */
	for (a = start; a < end; a++) {
		instr = (u8 *)&a->instr_offset + a->instr_offset;
		replacement = (u8 *)&a->repl_offset + a->repl_offset;
		BUG_ON(a->replacementlen > a->instrlen);
		BUG_ON(a->instrlen > sizeof(insnbuf));
		BUG_ON(a->cpuid >= (NCAPINTS + NBUGINTS) * 32);
		if (!boot_cpu_has(a->cpuid))
			continue;

		memcpy(insnbuf, replacement, a->replacementlen);

		/* 0xe8 is a relative jump; fix the offset. */
		if (*insnbuf == 0xe8 && a->replacementlen == 5)
		    *(s32 *)(insnbuf + 1) += replacement - instr;

		add_nops(insnbuf + a->replacementlen,
			 a->instrlen - a->replacementlen);

		text_poke_early(instr, insnbuf, a->instrlen);
	}
}