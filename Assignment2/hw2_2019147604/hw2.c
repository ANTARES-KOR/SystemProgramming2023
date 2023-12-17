// Reference: https://tldp.org/LDP/lkmpg/2.6/lkmpg.pdf

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/rbtree_augmented.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/version.h>



#define PROC_NAME "hw2"

MODULE_AUTHOR("Kiung Jung");
MODULE_LICENSE("GPL");

int period = 5;
module_param(period, int, 0);

int __count;
struct archive {
    // pid_t pid;
    char comm[100];
    int cnt;

    unsigned long pgd_base_addr;

    unsigned long code_start_vir_addr;
    pgd_t* code_start_pgd_addr;
    unsigned long code_start_pgd_val;
    pud_t* code_start_pud_addr;
    unsigned long code_start_pud_val;
    pmd_t* code_start_pmd_addr;
    unsigned long code_start_pmd_val;
    pte_t* code_start_pte_addr;
    unsigned long code_start_pte_val;
    unsigned long code_start_phy_addr;

    unsigned long code_end_vir_addr;
    pgd_t* code_end_pgd_addr;
    unsigned long code_end_pgd_val;
    pud_t* code_end_pud_addr;
    unsigned long code_end_pud_val;
    pmd_t* code_end_pmd_addr;
    unsigned long code_end_pmd_val;
    pte_t* code_end_pte_addr;
    unsigned long code_end_pte_val;
    unsigned long code_end_phy_addr;

    unsigned long data_start_vir_addr;
    pgd_t* data_start_pgd_addr;
    unsigned long data_start_pgd_val;
    pud_t* data_start_pud_addr;
    unsigned long data_start_pud_val;
    pmd_t* data_start_pmd_addr;
    unsigned long data_start_pmd_val;
    pte_t* data_start_pte_addr;
    unsigned long data_start_pte_val;
    unsigned long data_start_phy_addr;

    unsigned long data_end_vir_addr;
    pgd_t* data_end_pgd_addr;
    unsigned long data_end_pgd_val;
    pud_t* data_end_pud_addr;
    unsigned long data_end_pud_val;
    pmd_t* data_end_pmd_addr;
    unsigned long data_end_pmd_val;
    pte_t* data_end_pte_addr;
    unsigned long data_end_pte_val;
    unsigned long data_end_phy_addr;

    unsigned long heap_start_vir_addr;
    pgd_t* heap_start_pgd_addr;
    unsigned long heap_start_pgd_val;
    pud_t* heap_start_pud_addr;
    unsigned long heap_start_pud_val;
    pmd_t* heap_start_pmd_addr;
    unsigned long heap_start_pmd_val;
    pte_t* heap_start_pte_addr;
    unsigned long heap_start_pte_val;
    unsigned long heap_start_phy_addr;

    unsigned long heap_end_vir_addr;
    pgd_t* heap_end_pgd_addr;
    unsigned long heap_end_pgd_val;
    pud_t* heap_end_pud_addr;
    unsigned long heap_end_pud_val;
    pmd_t* heap_end_pmd_addr;
    unsigned long heap_end_pmd_val;
    pte_t* heap_end_pte_addr;
    unsigned long heap_end_pte_val;
    unsigned long heap_end_phy_addr;

    unsigned long stck_start_vir_addr;
    pgd_t* stck_start_pgd_addr;
    unsigned long stck_start_pgd_val;
    pud_t* stck_start_pud_addr;
    unsigned long stck_start_pud_val;
    pmd_t* stck_start_pmd_addr;
    unsigned long stck_start_pmd_val;
    pte_t* stck_start_pte_addr;
    unsigned long stck_start_pte_val;
    unsigned long stck_start_phy_addr;

    unsigned long stck_end_vir_addr;
    pgd_t* stck_end_pgd_addr;
    unsigned long stck_end_pgd_val;
    pud_t* stck_end_pud_addr;
    unsigned long stck_end_pud_val;
    pmd_t* stck_end_pmd_addr;
    unsigned long stck_end_pmd_val;
    pte_t* stck_end_pte_addr;
    unsigned long stck_end_pte_val;
    unsigned long stck_end_phy_addr;
} arc[70000];
char buf[50];
int i;
int cur_reside_idx = 0;
int cur_reside[1000];

void my_tasklet_function(struct tasklet_struct *data);
DECLARE_TASKLET(my_tasklet, my_tasklet_function);
/**
 * This function is called at the beginning of a sequence.
 * ie, when:
 * − the /proc file is read (first time)
 * − after the function stop (end of sequence)
 *
 */
static void *hello_seq_start(struct seq_file *s, loff_t *pos) {
    static unsigned long counter = 0;
    /* beginning a new sequence ? */
    if (*pos == 0) {
        /* yes => return a non null value to begin the sequence */
        return &counter;
    } else {
        /* no => it's the end of the sequence, return end to stop reading */
        *pos = 0;
        return NULL;
    }
}

/**
 * This function is called after the beginning of a sequence.
 * It's called untill the return is NULL (this ends the sequence).
 *
 */
static void *hello_seq_next(struct seq_file *s, void *v, loff_t *pos) {
    unsigned long *tmp_v = (unsigned long *)v;
    (*tmp_v)++;
    (*pos)++;
    return NULL;
}

/**
 * This function is called at the end of a sequence
 *
 */
static void hello_seq_stop(struct seq_file *s, void *v) {
    /* nothing to do, we use a static value in start() */
}


/**
 * This function is called for each "step" of a sequence
 *
 */

void printf_bar(struct seq_file *s) {
    for(i = 0; i < 80; i++) seq_printf(s, "-");
    seq_printf(s, "\n");
}

static int hello_seq_show(struct seq_file *s, void *v) {
    int cpu_count = num_online_cpus();

    long pid;
    kstrtol(s->file->f_path.dentry->d_iname, 10, &pid);
    // int idx = -1;
    // for(i = 0; i < cur_reside_idx; i++) {
    //     if(pid == cur_reside[i]) idx = i;
    // }

    // struct task_struct *task;
    // for_each_process(task){
    //     if (task->flags & PF_KTHREAD) continue;
    //     struct vm_area_struct *vma = task->mm->mmap;
    //     seq_printf(s, "====> 0x%lx\n", task->mm->start_stack);
    //     while (vma != NULL) {
    //         seq_printf(s, "--> 0x%lx 0x%lx\n", vma->vm_start, vma->vm_end);
    //         vma = vma->vm_next;
    //     }
    //     break;
    // }

    printf_bar(s);
    seq_printf(s, "Student name(ID): Kiung Jung(2019147604)\n");
    seq_printf(s, "Process name(ID): %s(%d)\n", arc[pid].comm, pid);
    seq_printf(s, "Memory info #%d\n", arc[pid].cnt);
    seq_printf(s, "PGD base address: 0x%lx\n", arc[pid].pgd_base_addr);

    printf_bar(s);
    seq_printf(s, "Code Area Start\n");
    seq_printf(s, "- Virtual address: 0x%lx\n", arc[pid].code_start_vir_addr);
    seq_printf(s, "- PGD address, value: 0x%lx, 0x%lx\n", arc[pid].code_start_pgd_addr, arc[pid].code_start_pgd_val);
    seq_printf(s, "- PUD address, value: 0x%lx, 0x%lx\n", arc[pid].code_start_pud_addr, arc[pid].code_start_pud_val);
    seq_printf(s, "- PMD address, value: 0x%lx, 0x%lx\n", arc[pid].code_start_pmd_addr, arc[pid].code_start_pmd_val);
    seq_printf(s, "- PTE address, value: 0x%lx, 0x%lx\n", arc[pid].code_start_pte_addr, arc[pid].code_start_pte_val);
    seq_printf(s, "- Physical address: 0x%lx\n", arc[pid].code_start_phy_addr);
    
    printf_bar(s);
    seq_printf(s, "Code Area End\n");
    seq_printf(s, "- Virtual address: 0x%lx\n", arc[pid].code_end_vir_addr);
    seq_printf(s, "- PGD address, value: 0x%lx, 0x%lx\n", arc[pid].code_end_pgd_addr, arc[pid].code_end_pgd_val);
    seq_printf(s, "- PUD address, value: 0x%lx, 0x%lx\n", arc[pid].code_end_pud_addr, arc[pid].code_end_pud_val);
    seq_printf(s, "- PMD address, value: 0x%lx, 0x%lx\n", arc[pid].code_end_pmd_addr, arc[pid].code_end_pmd_val);
    seq_printf(s, "- PTE address, value: 0x%lx, 0x%lx\n", arc[pid].code_end_pte_addr, arc[pid].code_end_pte_val);
    seq_printf(s, "- Physical address: 0x%lx\n", arc[pid].code_end_phy_addr);

    printf_bar(s);
    seq_printf(s, "Data Area Start\n");
    seq_printf(s, "- Virtual address: 0x%lx\n", arc[pid].data_start_vir_addr);
    seq_printf(s, "- PGD address, value: 0x%lx, 0x%lx\n", arc[pid].data_start_pgd_addr, arc[pid].data_start_pgd_val);
    seq_printf(s, "- PUD address, value: 0x%lx, 0x%lx\n", arc[pid].data_start_pud_addr, arc[pid].data_start_pud_val);
    seq_printf(s, "- PMD address, value: 0x%lx, 0x%lx\n", arc[pid].data_start_pmd_addr, arc[pid].data_start_pmd_val);
    seq_printf(s, "- PTE address, value: 0x%lx, 0x%lx\n", arc[pid].data_start_pte_addr, arc[pid].data_start_pte_val);
    seq_printf(s, "- Physical address: 0x%lx\n", arc[pid].data_start_phy_addr);
    
    printf_bar(s);
    seq_printf(s, "Data Area End\n");
    seq_printf(s, "- Virtual address: 0x%lx\n", arc[pid].data_end_vir_addr);
    seq_printf(s, "- PGD address, value: 0x%lx, 0x%lx\n", arc[pid].data_end_pgd_addr, arc[pid].data_end_pgd_val);
    seq_printf(s, "- PUD address, value: 0x%lx, 0x%lx\n", arc[pid].data_end_pud_addr, arc[pid].data_end_pud_val);
    seq_printf(s, "- PMD address, value: 0x%lx, 0x%lx\n", arc[pid].data_end_pmd_addr, arc[pid].data_end_pmd_val);
    seq_printf(s, "- PTE address, value: 0x%lx, 0x%lx\n", arc[pid].data_end_pte_addr, arc[pid].data_end_pte_val);
    seq_printf(s, "- Physical address: 0x%lx\n", arc[pid].data_end_phy_addr);

    printf_bar(s);
    seq_printf(s, "Heap Area Start\n");
    seq_printf(s, "- Virtual address: 0x%lx\n", arc[pid].heap_start_vir_addr);
    seq_printf(s, "- PGD address, value: 0x%lx, 0x%lx\n", arc[pid].heap_start_pgd_addr, arc[pid].heap_start_pgd_val);
    seq_printf(s, "- PUD address, value: 0x%lx, 0x%lx\n", arc[pid].heap_start_pud_addr, arc[pid].heap_start_pud_val);
    seq_printf(s, "- PMD address, value: 0x%lx, 0x%lx\n", arc[pid].heap_start_pmd_addr, arc[pid].heap_start_pmd_val);
    seq_printf(s, "- PTE address, value: 0x%lx, 0x%lx\n", arc[pid].heap_start_pte_addr, arc[pid].heap_start_pte_val);
    seq_printf(s, "- Physical address: 0x%lx\n", arc[pid].heap_start_phy_addr);
    
    printf_bar(s);
    seq_printf(s, "Heap Area End\n");
    seq_printf(s, "- Virtual address: 0x%lx\n", arc[pid].heap_end_vir_addr);
    seq_printf(s, "- PGD address, value: 0x%lx, 0x%lx\n", arc[pid].heap_end_pgd_addr, arc[pid].heap_end_pgd_val);
    seq_printf(s, "- PUD address, value: 0x%lx, 0x%lx\n", arc[pid].heap_end_pud_addr, arc[pid].heap_end_pud_val);
    seq_printf(s, "- PMD address, value: 0x%lx, 0x%lx\n", arc[pid].heap_end_pmd_addr, arc[pid].heap_end_pmd_val);
    seq_printf(s, "- PTE address, value: 0x%lx, 0x%lx\n", arc[pid].heap_end_pte_addr, arc[pid].heap_end_pte_val);
    seq_printf(s, "- Physical address: 0x%lx\n", arc[pid].heap_end_phy_addr);

     printf_bar(s);
    seq_printf(s, "Stack Area Start\n");
    seq_printf(s, "- Virtual address: 0x%lx\n", arc[pid].stck_start_vir_addr);
    seq_printf(s, "- PGD address, value: 0x%lx, 0x%lx\n", arc[pid].stck_start_pgd_addr, arc[pid].stck_start_pgd_val);
    seq_printf(s, "- PUD address, value: 0x%lx, 0x%lx\n", arc[pid].stck_start_pud_addr, arc[pid].stck_start_pud_val);
    seq_printf(s, "- PMD address, value: 0x%lx, 0x%lx\n", arc[pid].stck_start_pmd_addr, arc[pid].stck_start_pmd_val);
    seq_printf(s, "- PTE address, value: 0x%lx, 0x%lx\n", arc[pid].stck_start_pte_addr, arc[pid].stck_start_pte_val);
    seq_printf(s, "- Physical address: 0x%lx\n", arc[pid].stck_start_phy_addr);
    
    printf_bar(s);
    seq_printf(s, "Stack Area End\n");
    seq_printf(s, "- Virtual address: 0x%lx\n", arc[pid].stck_end_vir_addr);
    seq_printf(s, "- PGD address, value: 0x%lx, 0x%lx\n", arc[pid].stck_end_pgd_addr, arc[pid].stck_end_pgd_val);
    seq_printf(s, "- PUD address, value: 0x%lx, 0x%lx\n", arc[pid].stck_end_pud_addr, arc[pid].stck_end_pud_val);
    seq_printf(s, "- PMD address, value: 0x%lx, 0x%lx\n", arc[pid].stck_end_pmd_addr, arc[pid].stck_end_pmd_val);
    seq_printf(s, "- PTE address, value: 0x%lx, 0x%lx\n", arc[pid].stck_end_pte_addr, arc[pid].stck_end_pte_val);
    seq_printf(s, "- Physical address: 0x%lx\n", arc[pid].stck_end_phy_addr);
    return 0;
}

static struct seq_operations hello_seq_ops = {
    .start = hello_seq_start,
    .next = hello_seq_next,
    .stop = hello_seq_stop,
    .show = hello_seq_show};

static int hello_proc_open(struct inode *inode, struct file *file) {
    return seq_open(file, &hello_seq_ops);
}

static const struct proc_ops hello_file_ops = {
    .proc_open = hello_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release};

struct proc_dir_entry *parent;

void my_tasklet_function(struct tasklet_struct *data) {
    for (i = 0; i < cur_reside_idx; i++) {
        sprintf(buf, "%d", cur_reside[i]);
        remove_proc_entry(buf, parent);
    }
    cur_reside_idx = 0;

    struct task_struct *task;
    for_each_process(task) {
        if (task->flags & PF_KTHREAD) continue;
        pid_t pid = task->pid;
        sprintf(buf, "%d", pid);
        proc_create(buf, 0, parent, &hello_file_ops);
        cur_reside[cur_reside_idx++] = pid;

        arc[pid].cnt = __count;
        strcpy(arc[pid].comm, task->comm);
        arc[pid].pgd_base_addr = task->mm->pgd;

        arc[pid].code_start_vir_addr = task->mm->start_code;
        arc[pid].code_start_pgd_addr = pgd_offset(task->mm, arc[pid].code_start_vir_addr);
        arc[pid].code_start_pud_addr = pud_offset(p4d_offset(arc[pid].code_start_pgd_addr, arc[pid].code_start_vir_addr), arc[pid].code_start_vir_addr);
        arc[pid].code_start_pmd_addr = pmd_offset(arc[pid].code_start_pud_addr, arc[pid].code_start_vir_addr);
        arc[pid].code_start_pte_addr = pte_offset_map(arc[pid].code_start_pmd_addr, arc[pid].code_start_vir_addr);
        arc[pid].code_start_pgd_val = pgd_val(*arc[pid].code_start_pgd_addr);
        arc[pid].code_start_pud_val = pud_val(*arc[pid].code_start_pud_addr);
        arc[pid].code_start_pmd_val = pmd_val(*arc[pid].code_start_pmd_addr);
        arc[pid].code_start_pte_val = pte_val(*arc[pid].code_start_pte_addr);
        arc[pid].code_start_phy_addr = (arc[pid].code_start_pte_val & ~4095) + (arc[pid].code_start_vir_addr & 4095);

        arc[pid].data_start_vir_addr = task->mm->start_data;
        arc[pid].data_start_pgd_addr = pgd_offset(task->mm, arc[pid].data_start_vir_addr);
        arc[pid].data_start_pud_addr = pud_offset(p4d_offset(arc[pid].data_start_pgd_addr, arc[pid].data_start_vir_addr), arc[pid].data_start_vir_addr);
        arc[pid].data_start_pmd_addr = pmd_offset(arc[pid].data_start_pud_addr, arc[pid].data_start_vir_addr);
        arc[pid].data_start_pte_addr = pte_offset_map(arc[pid].data_start_pmd_addr, arc[pid].data_start_vir_addr);
        arc[pid].data_start_pgd_val = pgd_val(*arc[pid].data_start_pgd_addr);
        arc[pid].data_start_pud_val = pud_val(*arc[pid].data_start_pud_addr);
        arc[pid].data_start_pmd_val = pmd_val(*arc[pid].data_start_pmd_addr);
        arc[pid].data_start_pte_val = pte_val(*arc[pid].data_start_pte_addr);
        arc[pid].data_start_phy_addr = (arc[pid].data_start_pte_val & ~4095) + (arc[pid].data_start_vir_addr & 4095);

        arc[pid].heap_start_vir_addr = task->mm->start_brk;
        arc[pid].heap_start_pgd_addr = pgd_offset(task->mm, arc[pid].heap_start_vir_addr);
        arc[pid].heap_start_pud_addr = pud_offset(p4d_offset(arc[pid].heap_start_pgd_addr, arc[pid].heap_start_vir_addr), arc[pid].heap_start_vir_addr);
        arc[pid].heap_start_pmd_addr = pmd_offset(arc[pid].heap_start_pud_addr, arc[pid].heap_start_vir_addr);
        arc[pid].heap_start_pte_addr = pte_offset_map(arc[pid].heap_start_pmd_addr, arc[pid].heap_start_vir_addr);
        arc[pid].heap_start_pgd_val = pgd_val(*arc[pid].heap_start_pgd_addr);
        arc[pid].heap_start_pud_val = pud_val(*arc[pid].heap_start_pud_addr);
        arc[pid].heap_start_pmd_val = pmd_val(*arc[pid].heap_start_pmd_addr);
        arc[pid].heap_start_pte_val = pte_val(*arc[pid].heap_start_pte_addr);
        arc[pid].heap_start_phy_addr = (arc[pid].heap_start_pte_val & ~4095) + (arc[pid].heap_start_vir_addr & 4095);


        arc[pid].code_end_vir_addr = task->mm->end_code;
        arc[pid].code_end_pgd_addr = pgd_offset(task->mm, arc[pid].code_end_vir_addr);
        arc[pid].code_end_pud_addr = pud_offset(p4d_offset(arc[pid].code_end_pgd_addr, arc[pid].code_end_vir_addr), arc[pid].code_end_vir_addr);
        arc[pid].code_end_pmd_addr = pmd_offset(arc[pid].code_end_pud_addr, arc[pid].code_end_vir_addr);
        arc[pid].code_end_pte_addr = pte_offset_map(arc[pid].code_end_pmd_addr, arc[pid].code_end_vir_addr);
        arc[pid].code_end_pgd_val = pgd_val(*arc[pid].code_end_pgd_addr);
        arc[pid].code_end_pud_val = pud_val(*arc[pid].code_end_pud_addr);
        arc[pid].code_end_pmd_val = pmd_val(*arc[pid].code_end_pmd_addr);
        arc[pid].code_end_pte_val = pte_val(*arc[pid].code_end_pte_addr);
        arc[pid].code_end_phy_addr = (arc[pid].code_end_pte_val & ~4095) + (arc[pid].code_end_vir_addr & 4095);

        arc[pid].data_end_vir_addr = task->mm->end_data;
        arc[pid].data_end_pgd_addr = pgd_offset(task->mm, arc[pid].data_end_vir_addr);
        arc[pid].data_end_pud_addr = pud_offset(p4d_offset(arc[pid].data_end_pgd_addr, arc[pid].data_end_vir_addr), arc[pid].data_end_vir_addr);
        arc[pid].data_end_pmd_addr = pmd_offset(arc[pid].data_end_pud_addr, arc[pid].data_end_vir_addr);
        arc[pid].data_end_pte_addr = pte_offset_map(arc[pid].data_end_pmd_addr, arc[pid].data_end_vir_addr);
        arc[pid].data_end_pgd_val = pgd_val(*arc[pid].data_end_pgd_addr);
        arc[pid].data_end_pud_val = pud_val(*arc[pid].data_end_pud_addr);
        arc[pid].data_end_pmd_val = pmd_val(*arc[pid].data_end_pmd_addr);
        arc[pid].data_end_pte_val = pte_val(*arc[pid].data_end_pte_addr);
        arc[pid].data_end_phy_addr = (arc[pid].data_end_pte_val & ~4095) + (arc[pid].data_end_vir_addr & 4095);

        arc[pid].heap_end_vir_addr = task->mm->brk;
        arc[pid].heap_end_pgd_addr = pgd_offset(task->mm, arc[pid].heap_end_vir_addr);
        arc[pid].heap_end_pud_addr = pud_offset(p4d_offset(arc[pid].heap_end_pgd_addr, arc[pid].heap_end_vir_addr), arc[pid].heap_end_vir_addr);
        arc[pid].heap_end_pmd_addr = pmd_offset(arc[pid].heap_end_pud_addr, arc[pid].heap_end_vir_addr);
        arc[pid].heap_end_pte_addr = pte_offset_map(arc[pid].heap_end_pmd_addr, arc[pid].heap_end_vir_addr);
        arc[pid].heap_end_pgd_val = pgd_val(*arc[pid].heap_end_pgd_addr);
        arc[pid].heap_end_pud_val = pud_val(*arc[pid].heap_end_pud_addr);
        arc[pid].heap_end_pmd_val = pmd_val(*arc[pid].heap_end_pmd_addr);
        arc[pid].heap_end_pte_val = pte_val(*arc[pid].heap_end_pte_addr);
        arc[pid].heap_end_phy_addr = (arc[pid].heap_end_pte_val & ~4095) + (arc[pid].heap_end_vir_addr & 4095);

        
        struct vm_area_struct *vma = task->mm->mmap;
        while (vma->vm_end < task->mm->start_stack) {
		    vma = vma->vm_next;
	    }
        unsigned long stack_start = vma->vm_start;
        unsigned long stack_end = vma->vm_end;
        if (task->mm->start_stack < vma->vm_start) {
            stack_start = task->mm->start_stack; 
            stack_end = task->mm->start_stack;
        }

        arc[pid].stck_start_vir_addr = stack_start;
        arc[pid].stck_start_pgd_addr = pgd_offset(task->mm, arc[pid].stck_start_vir_addr);
        arc[pid].stck_start_pud_addr = pud_offset(p4d_offset(arc[pid].stck_start_pgd_addr, arc[pid].stck_start_vir_addr), arc[pid].stck_start_vir_addr);
        arc[pid].stck_start_pmd_addr = pmd_offset(arc[pid].stck_start_pud_addr, arc[pid].stck_start_vir_addr);
        arc[pid].stck_start_pte_addr = pte_offset_map(arc[pid].stck_start_pmd_addr, arc[pid].stck_start_vir_addr);
        arc[pid].stck_start_pgd_val = pgd_val(*arc[pid].stck_start_pgd_addr);
        arc[pid].stck_start_pud_val = pud_val(*arc[pid].stck_start_pud_addr);
        arc[pid].stck_start_pmd_val = pmd_val(*arc[pid].stck_start_pmd_addr);
        arc[pid].stck_start_pte_val = pte_val(*arc[pid].stck_start_pte_addr);
        arc[pid].stck_start_phy_addr = (arc[pid].stck_start_pte_val & ~4095) + (arc[pid].stck_start_vir_addr & 4095);

        arc[pid].stck_end_vir_addr = stack_end;
        arc[pid].stck_end_pgd_addr = pgd_offset(task->mm, arc[pid].stck_end_vir_addr);
        arc[pid].stck_end_pud_addr = pud_offset(p4d_offset(arc[pid].stck_end_pgd_addr, arc[pid].stck_end_vir_addr), arc[pid].stck_end_vir_addr);
        arc[pid].stck_end_pmd_addr = pmd_offset(arc[pid].stck_end_pud_addr, arc[pid].stck_end_vir_addr);
        arc[pid].stck_end_pte_addr = pte_offset_map(arc[pid].stck_end_pmd_addr, arc[pid].stck_end_vir_addr);
        arc[pid].stck_end_pgd_val = pgd_val(*arc[pid].stck_end_pgd_addr);
        arc[pid].stck_end_pud_val = pud_val(*arc[pid].stck_end_pud_addr);
        arc[pid].stck_end_pmd_val = pmd_val(*arc[pid].stck_end_pmd_addr);
        arc[pid].stck_end_pte_val = pte_val(*arc[pid].stck_end_pte_addr);
        arc[pid].stck_end_phy_addr = (arc[pid].stck_end_pte_val & ~4095) + (arc[pid].stck_end_vir_addr & 4095);
    }

    __count++;
}

static struct timer_list my_timer;
void timer_callback(struct timer_list *timer) {
    tasklet_schedule(&my_tasklet);
    mod_timer(timer, jiffies + period * HZ);
}

static int __init hello_init(void) {
    parent = proc_mkdir(PROC_NAME, NULL);
    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies);
    return 0;
}

static void __exit hello_exit(void) {
    tasklet_kill(&my_tasklet);
    del_timer(&my_timer);
    for (i = 0; i < cur_reside_idx; i++) {
        sprintf(buf, "%d", cur_reside[i]);
        remove_proc_entry(buf, parent);
    }
    remove_proc_entry(PROC_NAME, NULL);
}

module_init(hello_init);
module_exit(hello_exit);