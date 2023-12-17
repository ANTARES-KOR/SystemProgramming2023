#include <linux/module.h>
#include <linux/sched/signal.h>
#include <linux/timer.h>
#include <linux/sched.h>        // for task_struct
#include <linux/mm_types.h>     // for mm_struct
#include <linux/mm.h>           // for vm_area_struct
#include <linux/jiffies.h>      // for jiffies and time calculations
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/maple_tree.h>


#define PROC_NAME "hw2"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hyeokjun, Seo");
MODULE_DESCRIPTION("A module that tracks the most recently started non-kernel task.");

#define INTERVAL 5

void my_tasklet_handler(struct tasklet_struct *tsk);
DECLARE_TASKLET(my_tasklet, my_tasklet_handler);


// Structure to hold task information
struct task_info {
    pid_t pid;
    char comm[TASK_COMM_LEN];
    u64 start_time_ns;
    pgd_t *pgd_base;
    struct {
        unsigned long vm_start, vm_end;
        unsigned long pgd_start, pgd_end;
        unsigned long pud_start, pud_end;
        unsigned long pmd_start, pmd_end;
        unsigned long pte_start, pte_end;
        unsigned long phys_start, phys_end;
    } stack, data, code, heap;
};

// Circular buffer for storing task information
#define MAX_TASKS 5
static struct task_info task_history[MAX_TASKS];
static int current_index = 0;
static int task_count = 0;


// Function to add task information to the buffer
void add_task_to_history(struct task_struct *task) {
    struct task_info *info;
    struct vm_area_struct *vma;

    if (task == NULL) return;

    info = &task_history[current_index];
    info->pid = task->pid;
    strncpy(info->comm, task->comm, TASK_COMM_LEN);
    info->start_time_ns = task->start_time;
    info->pgd_base = task->mm->pgd;

    long unsigned mm_index = 0;
    down_read(&task->mm->mmap_lock);
    mt_for_each(&task->mm->mm_mt, vma, mm_index, ULONG_MAX) {
        if (vma->vm_start <= task->mm->start_code && vma->vm_end >= task->mm->end_code) {
            info->code.vm_start = task->mm->start_code;
            info->code.vm_end = task->mm->end_code;
            info->code.pgd_start = pgd_offset(task->mm, info->code.vm_start);
            info->code.pgd_end = pgd_offset(task->mm, info->code.vm_end);
            info->code.pud_start = pud_offset(p4d_offset(task->mm->pgd, info->code.vm_start), info->code.vm_start);
            info->code.pud_end = pud_offset(p4d_offset(task->mm->pgd, info->code.vm_end), info->code.vm_end);
            info->code.pmd_start = pmd_offset(info->code.pud_start, info->code.vm_start);
            info->code.pmd_end = pmd_offset(info->code.pud_end, info->code.vm_end);
            info->code.pte_start = pte_offset_kernel(info->code.pmd_start, info->code.vm_start);
            info->code.pte_end = pte_offset_kernel(info->code.pmd_end, info->code.vm_end);
            info->code.phys_start = virt_to_phys((void *)info->code.vm_start);
            info->code.phys_end = virt_to_phys((void *)info->code.vm_end);
        }
        if (vma->vm_start <= task->mm->start_data && vma->vm_end >= task->mm->end_data) {
            info->data.vm_start = task->mm->start_data;
            info->data.vm_end = task->mm->end_data;
            info->data.pgd_start = pgd_offset(task->mm, info->data.vm_start);
            info->data.pgd_end = pgd_offset(task->mm, info->data.vm_end);
            info->data.pud_start = pud_offset(p4d_offset(task->mm->pgd, info->data.vm_start), info->data.vm_start);
            info->data.pud_end = pud_offset(p4d_offset(task->mm->pgd, info->data.vm_end), info->data.vm_end);
            info->data.pmd_start = pmd_offset(info->data.pud_start, info->data.vm_start);
            info->data.pmd_end = pmd_offset(info->data.pud_end, info->data.vm_end);
            info->data.pte_start = pte_offset_kernel(info->data.pmd_start, info->data.vm_start);
            info->data.pte_end = pte_offset_kernel(info->data.pmd_end, info->data.vm_end);
            info->data.phys_start = virt_to_phys((void *)info->data.vm_start);
            info->data.phys_end = virt_to_phys((void *)info->data.vm_end);
        }
        if (vma->vm_start <= task->mm->start_brk && vma->vm_end >= task->mm->brk) {
            info->heap.vm_start = task->mm->start_brk;
            info->heap.vm_end = task->mm->brk;
            info->heap.pgd_start = pgd_offset(task->mm, info->heap.vm_start);
            info->heap.pgd_end = pgd_offset(task->mm, info->heap.vm_end);
            info->heap.pud_start = pud_offset(p4d_offset(task->mm->pgd, info->heap.vm_start), info->heap.vm_start);
            info->heap.pud_end = pud_offset(p4d_offset(task->mm->pgd, info->heap.vm_end), info->heap.vm_end);
            info->heap.pmd_start = pmd_offset(info->heap.pud_start, info->heap.vm_start);
            info->heap.pmd_end = pmd_offset(info->heap.pud_end, info->heap.vm_end);
            info->heap.pte_start = pte_offset_kernel(info->heap.pmd_start, info->heap.vm_start);
            info->heap.pte_end = pte_offset_kernel(info->heap.pmd_end, info->heap.vm_end);
            info->heap.phys_start = virt_to_phys((void *)info->heap.vm_start);
            info->heap.phys_end = virt_to_phys((void *)info->heap.vm_end);
        }
        // if (vma->vm_start <= task->mm->start_stack && vma->vm_end >= task->mm->start_stack) {
        //     info->stack.vm_start = task->mm->start_stack;
        //     info->stack.vm_end = vma->vm_end;
        //     info->stack.pgd_start = pgd_offset(task->mm, info->stack.vm_start);
        //     info->stack.pgd_end = pgd_offset(task->mm, info->stack.vm_end);
        //     info->stack.pud_start = pud_offset(p4d_offset(task->mm->pgd, info->stack.vm_start), info->stack.vm_start);
        //     info->stack.pud_end = pud_offset(p4d_offset(task->mm->pgd, info->stack.vm_end), info->stack.vm_end);
        //     info->stack.pmd_start = pmd_offset(info->stack.pud_start, info->stack.vm_start);
        //     info->stack.pmd_end = pmd_offset(info->stack.pud_end, info->stack.vm_end);
        //     info->stack.pte_start = pte_offset_kernel(info->stack.pmd_start, info->stack.vm_start);
        //     info->stack.pte_end = pte_offset_kernel(info->stack.pmd_end, info->stack.vm_end);
        //     info->stack.phys_start = virt_to_phys((void *)info->stack.vm_start);
        //     info->stack.phys_end = virt_to_phys((void *)info->stack.vm_end);
        // }
    }
    up_read(&task->mm->mmap_lock);

    current_index = (current_index + 1) % MAX_TASKS;
    task_count++;
}

// finds the latest task and adds it to the buffer
void find_latest_task(void) {
    struct task_struct *task, *latest_task = NULL;
    unsigned long latest_start_time = 0;

    rcu_read_lock();
    for_each_process(task) {
        if (task->mm != NULL && (task->start_time > latest_start_time)) {
            latest_task = task;
            latest_start_time = task->start_time;
        }
    }
    rcu_read_unlock();

    if (latest_task != NULL) {
        add_task_to_history(latest_task);
    }
}

// timer for periodic execution
static struct timer_list my_timer;
void timer_callback(struct timer_list *timer) {
    tasklet_schedule(&my_tasklet);
    mod_timer(timer, jiffies + msecs_to_jiffies(INTERVAL * 1000));
}

// Tasklet handler function
void my_tasklet_handler(struct tasklet_struct *tsk) {
    find_latest_task();
}

static int proc_show(struct seq_file *m, void *v) {
    

    // print the task information in the buffer
    // from the oldest to the latest

    int i;
    struct task_info *info;

    seq_printf(m, "[System Programming Assignment #2]\n");
    seq_printf(m, "ID : 2017147581\n");
    seq_printf(m, "Name: Seo, Hyeokjun\n");
    seq_printf(m, "Uptime(s): %u\n", jiffies_to_msecs(jiffies) / 1000);
    seq_printf(m, "--------------------------------------------------\n");


    for (i = 0; i < min(task_count, MAX_TASKS); i++) {
        info = &task_history[(current_index + i + max(MAX_TASKS - task_count, 0)) % MAX_TASKS];
        seq_printf(m, "[Trace #%d]\n", i);
        seq_printf(m, "Command: %s\n", info->comm);
        seq_printf(m, "PID: %d\n", info->pid);
        seq_printf(m, "Start time (s): %llu\n", info->start_time_ns / 1000000000);
        seq_printf(m, "PGD base address: %p\n", info->pgd_base);
        seq_printf(m, "Code Area\n");
        seq_printf(m, "- start (virtual): %lx\n", info->code.vm_start);
        seq_printf(m, "- start (PGD): %lx\n", info->code.pgd_start);
        seq_printf(m, "- start (PUD): %lx\n", info->code.pud_start);
        seq_printf(m, "- start (PMD): %lx\n", info->code.pmd_start);
        seq_printf(m, "- start (PTE): %lx\n", info->code.pte_start);
        seq_printf(m, "- start (physical): %lx\n", info->code.phys_start);
        seq_printf(m, "- end (virtual): %lx\n", info->code.vm_end);
        seq_printf(m, "- end (PGD): %lx\n", info->code.pgd_end);
        seq_printf(m, "- end (PUD): %lx\n", info->code.pud_end);
        seq_printf(m, "- end (PMD): %lx\n", info->code.pmd_end);
        seq_printf(m, "- end (PTE): %lx\n", info->code.pte_end);
        seq_printf(m, "- end (physical): %lx\n", info->code.phys_end);
        seq_printf(m, "Data Area\n");
        seq_printf(m, "- start (virtual): %lx\n", info->data.vm_start);
        seq_printf(m, "- start (physical): %lx\n", info->data.phys_start);
        seq_printf(m, "- end (virtual): %lx\n", info->data.vm_end);
        seq_printf(m, "- end (physical): %lx\n", info->data.phys_end);
        seq_printf(m, "Heap Area\n");
        seq_printf(m, "- start (virtual): %lx\n", info->heap.vm_start);
        seq_printf(m, "- start (physical): %lx\n", info->heap.phys_start);
        seq_printf(m, "- end (virtual): %lx\n", info->heap.vm_end);
        seq_printf(m, "- end (physical): %lx\n", info->heap.phys_end);
        // seq_printf(m, "Stack Area\n");
        // seq_printf(m, "- start (virtual): %lx\n", info->stack.vm_start);
        // seq_printf(m, "- start (physical): %lx\n", info->stack.phys_start);
        // seq_printf(m, "- end (virtual): %lx\n", info->stack.vm_end);
        // seq_printf(m, "- end (physical): %lx\n", info->stack.phys_end);
        seq_printf(m, "--------------------------------------------------\n");
    }


    return 0;
}

static int proc_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init my_module_init(void) {
    proc_create(PROC_NAME, 0, NULL, &proc_fops);

    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies);

    printk(KERN_INFO "hw2 module loaded\n");

    return 0;
}

static void __exit my_module_exit(void) {

    remove_proc_entry(PROC_NAME, NULL);
    timer_delete(&my_timer);
    tasklet_kill(&my_tasklet);

    printk(KERN_INFO "hw2 module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);