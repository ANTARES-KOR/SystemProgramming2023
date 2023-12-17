```c
static void sync_global_pgds_l5(unsigned long start, unsigned long end)
{
	unsigned long addr;

	for (addr = start; addr <= end; addr = ALIGN(addr + 1, PGDIR_SIZE)) {
		const pgd_t *pgd_ref = pgd_offset_k(addr);
		struct page *page;

		/* Check for overflow */
		if (addr < start)
			break;

		if (pgd_none(*pgd_ref))
			continue;

		spin_lock(&pgd_lock);
		list_for_each_entry(page, &pgd_list, lru) {
			pgd_t *pgd;
			spinlock_t *pgt_lock;

			pgd = (pgd_t *)page_address(page) + pgd_index(addr);
			/* the pgt_lock only for Xen */
			pgt_lock = &pgd_page_get_mm(page)->page_table_lock;
			spin_lock(pgt_lock);

			if (!pgd_none(*pgd_ref) && !pgd_none(*pgd))
				BUG_ON(pgd_page_vaddr(*pgd) != pgd_page_vaddr(*pgd_ref));

			if (pgd_none(*pgd))
				set_pgd(pgd, *pgd_ref);

			spin_unlock(pgt_lock);
		}
		spin_unlock(&pgd_lock);
	}
}

static void sync_global_pgds_l4(unsigned long start, unsigned long end)
{
	unsigned long addr;

	for (addr = start; addr <= end; addr = ALIGN(addr + 1, PGDIR_SIZE)) {
		pgd_t *pgd_ref = pgd_offset_k(addr);
		const p4d_t *p4d_ref;
		struct page *page;

		/*
		 * With folded p4d, pgd_none() is always false, we need to
		 * handle synchronization on p4d level.
		 */
		MAYBE_BUILD_BUG_ON(pgd_none(*pgd_ref));
		p4d_ref = p4d_offset(pgd_ref, addr);

		if (p4d_none(*p4d_ref))
			continue;

		spin_lock(&pgd_lock);
		list_for_each_entry(page, &pgd_list, lru) {
			pgd_t *pgd;
			p4d_t *p4d;
			spinlock_t *pgt_lock;

			pgd = (pgd_t *)page_address(page) + pgd_index(addr);
			p4d = p4d_offset(pgd, addr);
			/* the pgt_lock only for Xen */
			pgt_lock = &pgd_page_get_mm(page)->page_table_lock;
			spin_lock(pgt_lock);

			if (!p4d_none(*p4d_ref) && !p4d_none(*p4d))
				BUG_ON(p4d_pgtable(*p4d)
				       != p4d_pgtable(*p4d_ref));

			if (p4d_none(*p4d))
				set_p4d(p4d, *p4d_ref);

			spin_unlock(pgt_lock);
		}
		spin_unlock(&pgd_lock);
	}
}

/*
 * When memory was added make sure all the processes MM have
 * suitable PGD entries in the local PGD level page.
 */
static void sync_global_pgds(unsigned long start, unsigned long end)
{
	if (pgtable_l5_enabled())
		sync_global_pgds_l5(start, end);
	else
		sync_global_pgds_l4(start, end);
}

```

```c
static p4d_t *fill_p4d(pgd_t *pgd, unsigned long vaddr)
{
	if (pgd_none(*pgd)) {
		p4d_t *p4d = (p4d_t *)spp_getpage();
		pgd_populate(&init_mm, pgd, p4d);
		if (p4d != p4d_offset(pgd, 0))
			printk(KERN_ERR "PAGETABLE BUG #00! %p <-> %p\n",
			       p4d, p4d_offset(pgd, 0));
	}
	return p4d_offset(pgd, vaddr);
}

static pud_t *fill_pud(p4d_t *p4d, unsigned long vaddr)
{
	if (p4d_none(*p4d)) {
		pud_t *pud = (pud_t *)spp_getpage();
		p4d_populate(&init_mm, p4d, pud);
		if (pud != pud_offset(p4d, 0))
			printk(KERN_ERR "PAGETABLE BUG #01! %p <-> %p\n",
			       pud, pud_offset(p4d, 0));
	}
	return pud_offset(p4d, vaddr);
}

static pmd_t *fill_pmd(pud_t *pud, unsigned long vaddr)
{
	if (pud_none(*pud)) {
		pmd_t *pmd = (pmd_t *) spp_getpage();
		pud_populate(&init_mm, pud, pmd);
		if (pmd != pmd_offset(pud, 0))
			printk(KERN_ERR "PAGETABLE BUG #02! %p <-> %p\n",
			       pmd, pmd_offset(pud, 0));
	}
	return pmd_offset(pud, vaddr);
}

static pte_t *fill_pte(pmd_t *pmd, unsigned long vaddr)
{
	if (pmd_none(*pmd)) {
		pte_t *pte = (pte_t *) spp_getpage();
		pmd_populate_kernel(&init_mm, pmd, pte);
		if (pte != pte_offset_kernel(pmd, 0))
			printk(KERN_ERR "PAGETABLE BUG #03!\n");
	}
	return pte_offset_kernel(pmd, vaddr);
}
```

```c
static void __set_pte_vaddr(pud_t *pud, unsigned long vaddr, pte_t new_pte)
{
	pmd_t *pmd = fill_pmd(pud, vaddr);
	pte_t *pte = fill_pte(pmd, vaddr);

	set_pte(pte, new_pte);

	/*
	 * It's enough to flush this one mapping.
	 * (PGE mappings get flushed as well)
	 */
	flush_tlb_one_kernel(vaddr);
}

void set_pte_vaddr_p4d(p4d_t *p4d_page, unsigned long vaddr, pte_t new_pte)
{
	p4d_t *p4d = p4d_page + p4d_index(vaddr);
	pud_t *pud = fill_pud(p4d, vaddr);

	__set_pte_vaddr(pud, vaddr, new_pte);
}

void set_pte_vaddr_pud(pud_t *pud_page, unsigned long vaddr, pte_t new_pte)
{
	pud_t *pud = pud_page + pud_index(vaddr);

	__set_pte_vaddr(pud, vaddr, new_pte);
}

void set_pte_vaddr(unsigned long vaddr, pte_t pteval)
{
	pgd_t *pgd;
	p4d_t *p4d_page;

	pr_debug("set_pte_vaddr %lx to %lx\n", vaddr, native_pte_val(pteval));

	pgd = pgd_offset_k(vaddr);
	if (pgd_none(*pgd)) {
		printk(KERN_ERR
			"PGD FIXMAP MISSING, it should be setup in head.S!\n");
		return;
	}

	p4d_page = p4d_offset(pgd, 0);
	set_pte_vaddr_p4d(p4d_page, vaddr, pteval);
}

pmd_t * __init populate_extra_pmd(unsigned long vaddr)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;

	pgd = pgd_offset_k(vaddr);
	p4d = fill_p4d(pgd, vaddr);
	pud = fill_pud(p4d, vaddr);
	return fill_pmd(pud, vaddr);
}

pte_t * __init populate_extra_pte(unsigned long vaddr)
{
	pmd_t *pmd;

	pmd = populate_extra_pmd(vaddr);
	return fill_pte(pmd, vaddr);
}

/*
 * Create large page table mappings for a range of physical addresses.
 */
static void __init __init_extra_mapping(unsigned long phys, unsigned long size,
					enum page_cache_mode cache)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pgprot_t prot;

	pgprot_val(prot) = pgprot_val(PAGE_KERNEL_LARGE) |
		protval_4k_2_large(cachemode2protval(cache));
	BUG_ON((phys & ~PMD_MASK) || (size & ~PMD_MASK));
	for (; size; phys += PMD_SIZE, size -= PMD_SIZE) {
		pgd = pgd_offset_k((unsigned long)__va(phys));
		if (pgd_none(*pgd)) {
			p4d = (p4d_t *) spp_getpage();
			set_pgd(pgd, __pgd(__pa(p4d) | _KERNPG_TABLE |
						_PAGE_USER));
		}
		p4d = p4d_offset(pgd, (unsigned long)__va(phys));
		if (p4d_none(*p4d)) {
			pud = (pud_t *) spp_getpage();
			set_p4d(p4d, __p4d(__pa(pud) | _KERNPG_TABLE |
						_PAGE_USER));
		}
		pud = pud_offset(p4d, (unsigned long)__va(phys));
		if (pud_none(*pud)) {
			pmd = (pmd_t *) spp_getpage();
			set_pud(pud, __pud(__pa(pmd) | _KERNPG_TABLE |
						_PAGE_USER));
		}
		pmd = pmd_offset(pud, phys);
		BUG_ON(!pmd_none(*pmd));
		set_pmd(pmd, __pmd(phys | pgprot_val(prot)));
	}
}
```
