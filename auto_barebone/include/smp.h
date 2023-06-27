
typedef struct _pcpu {
  u32 lapic_id;
  u32 lapic_ver;
  u32 lapic_base;
  u32 isbsp;
} __attribute__((packed)) PCPU;

u32 smp_getinfo(PCPU *pcpus, u32 *num_pcpus, void *uefi_rsdp);

