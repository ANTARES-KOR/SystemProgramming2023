```c
extern pud_t level3_kernel_pgt[512];
extern pud_t level3_ident_pgt[512];
extern pmd_t level2_kernel_pgt[512];
extern pmd_t level2_fixmap_pgt[512];
extern pmd_t level2_ident_pgt[512];
extern pgd_t init_level4_pgt[];
```

```c
struct mm_struct;

void set_pte_vaddr_pud(pud_t *pud_page, unsigned long vaddr, pte_t new_pte);
```

```c
/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */

/*
 * Level 4 access.
 */
#define pgd_page_vaddr(pgd)						\
	((unsigned long)__va((unsigned long)pgd_val((pgd)) & PTE_PFN_MASK))
#define pgd_page(pgd)		(pfn_to_page(pgd_val((pgd)) >> PAGE_SHIFT))
#define pgd_present(pgd) (pgd_val(pgd) & _PAGE_PRESENT)
static inline int pgd_large(pgd_t pgd) { return 0; }
#define mk_kernel_pgd(address) __pgd((address) | _KERNPG_TABLE)

/* PUD - Level3 access */
/* to find an entry in a page-table-directory. */
#define pud_page_vaddr(pud)						\
	((unsigned long)__va(pud_val((pud)) & PHYSICAL_PAGE_MASK))
#define pud_page(pud)	(pfn_to_page(pud_val((pud)) >> PAGE_SHIFT))
#define pud_index(address) (((address) >> PUD_SHIFT) & (PTRS_PER_PUD - 1))
#define pud_offset(pgd, address)					\
	((pud_t *)pgd_page_vaddr(*(pgd)) + pud_index((address)))
#define pud_present(pud) (pud_val((pud)) & _PAGE_PRESENT)

static inline int pud_large(pud_t pte)
{
	return (pud_val(pte) & (_PAGE_PSE | _PAGE_PRESENT)) ==
		(_PAGE_PSE | _PAGE_PRESENT);
}

/* PMD  - Level 2 access */
#define pmd_page_vaddr(pmd) ((unsigned long) __va(pmd_val((pmd)) & PTE_PFN_MASK))
#define pmd_page(pmd)		(pfn_to_page(pmd_val((pmd)) >> PAGE_SHIFT))

#define pmd_index(address) (((address) >> PMD_SHIFT) & (PTRS_PER_PMD - 1))
#define pmd_offset(dir, address) ((pmd_t *)pud_page_vaddr(*(dir)) + \
				  pmd_index(address))
#define pmd_none(x)	(!pmd_val((x)))
#define pmd_present(x)	(pmd_val((x)) & _PAGE_PRESENT)
#define pfn_pmd(nr, prot) (__pmd(((nr) << PAGE_SHIFT) | pgprot_val((prot))))
#define pmd_pfn(x)  ((pmd_val((x)) & __PHYSICAL_MASK) >> PAGE_SHIFT)

#define pte_to_pgoff(pte) ((pte_val((pte)) & PHYSICAL_PAGE_MASK) >> PAGE_SHIFT)
#define pgoff_to_pte(off) ((pte_t) { .pte = ((off) << PAGE_SHIFT) |	\
					    _PAGE_FILE })
#define PTE_FILE_MAX_BITS __PHYSICAL_MASK_SHIFT

/* PTE - Level 1 access. */

/* page, protection -> pte */
#define mk_pte(page, pgprot)	pfn_pte(page_to_pfn((page)), (pgprot))

#define pte_index(address) (((address) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))
#define pte_offset_kernel(dir, address) ((pte_t *) pmd_page_vaddr(*(dir)) + \
					 pte_index((address)))
```

```c
/* x86-64 always has all page tables mapped. */
#define pte_offset_map(dir, address) pte_offset_kernel((dir), (address))
#define pte_offset_map_nested(dir, address) pte_offset_kernel((dir), (address))
#define pte_unmap(pte) /* NOP */
#define pte_unmap_nested(pte) /* NOP */
```
