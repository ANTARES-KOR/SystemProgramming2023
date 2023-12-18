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

#define INTERVAL 10

void my_tasklet_handler(struct tasklet_struct *tsk);
DECLARE_TASKLET(my_tasklet, my_tasklet_handler);


/**
 * @struct task_info
 * @brief Structure to hold task information
 * 
 * This structure is used to store information about a task, including its process ID (pid),
 * command name (comm), uptime, start time, page global directory (pgd) base, and memory
 * regions for stack, data, code, and heap.
 * 
 * The stack, data, code, and heap regions are represented by their respective start and end
 * addresses in virtual memory (vm_start and vm_end), page global directory (pgd_start and
 * pgd_end), page upper directory (pud_start and pud_end), page middle directory (pmd_start
 * and pmd_end), page table entry (pte_start and pte_end), and physical memory (phys_start
 * and phys_end).
 */
struct task_info {
    pid_t pid;
    char comm[TASK_COMM_LEN];
    int uptime;
    int start_time;
    pgd_t *pgd_base;
    struct {
        unsigned long vm_start, vm_end;
        unsigned long pgd_start, pgd_end;
        unsigned long pud_start, pud_end;
        unsigned long pmd_start, pmd_end;
        unsigned long pte_start, pte_end;
        unsigned long phys_start, phys_end;
    } stack;

    struct {
        unsigned long vm_start, vm_end;
        unsigned long phys_start, phys_end;
    } data, code, heap;
};

// Circular buffer for storing task information
#define MAX_TASKS 5
static struct task_info task_history[MAX_TASKS];
static int current_index = 0;
static int task_count = 0;

/**
 * Adds a task to the task history.
 * 
 * @param task The task to be added.
 */
void add_task_to_history(struct task_struct *task) {
    struct task_info *info;
    struct vm_area_struct *vma;

    if (task == NULL) return;

    // Get the task information
    info = &task_history[current_index];
    info->pid = task->pid;
    strncpy(info->comm, task->comm, TASK_COMM_LEN);
    info->start_time = task->start_time / 1000000000;
    info->uptime = jiffies_to_msecs(jiffies) / 1000 - info->start_time;
    info->pgd_base = task->mm->pgd;

    // Get code segment information
    info->code.vm_start = task->mm->start_code;
    info->code.vm_end = task->mm->end_code;
    // Calculate the page global directory (pgd) offsets for the code segment
    info->code.pgd_start = pgd_offset(task->mm, info->code.vm_start);
    info->code.pgd_end = pgd_offset(task->mm, info->code.vm_end);
    // Calculate the page upper directory (pud) offsets for the code segment
    info->code.pud_start = pud_offset(p4d_offset(info->code.pgd_start, info->code.vm_start), info->code.vm_start);
    info->code.pud_end = pud_offset(p4d_offset(info->code.pgd_end, info->code.vm_end), info->code.vm_end);
    // Calculate the page middle directory (pmd) offsets for the code segment
    info->code.pmd_start = pmd_offset(info->code.pud_start, info->code.vm_start);
    info->code.pmd_end = pmd_offset(info->code.pud_end, info->code.vm_end);
    // Calculate the page table entry (pte) offsets for the code segment
    info->code.pte_start = pte_offset_kernel(info->code.pmd_start, info->code.vm_start);
    info->code.pte_end = pte_offset_kernel(info->code.pmd_end, info->code.vm_end);
    // Calculate the physical addresses of the start and end of the code segment
    info->code.phys_start = virt_to_phys((void *)info->code.vm_start);
    info->code.phys_end = virt_to_phys((void *)info->code.vm_end);

    // Get data segment information
    info->data.vm_start = task->mm->start_data;
    info->data.vm_end = task->mm->end_data;
    info->data.phys_start = virt_to_phys((void *)info->data.vm_start);
    info->data.phys_end = virt_to_phys((void *)info->data.vm_end);

    // Get heap segment information
    info->heap.vm_start = task->mm->start_brk;
    info->heap.vm_end = task->mm->brk;
    info->heap.phys_start = virt_to_phys((void *)info->heap.vm_start);
    info->heap.phys_end = virt_to_phys((void *)info->heap.vm_end);


    // Get stack segment information
    // Declare a variable to keep track of the index in the mm_mt structure
    long unsigned mm_index = 0;
    // Acquire a read lock on the mmap_lock of the task's mm (memory descriptor)
    down_read(&task->mm->mmap_lock);
    // Iterate over each VMA (Virtual Memory Area) in the mm_mt structure of the task's mm
    mt_for_each(&task->mm->mm_mt, vma, mm_index, ULONG_MAX) {
        // Check if the VMA contains the start of the stack
        if (vma->vm_start <= task->mm->start_stack && vma->vm_end >= task->mm->start_stack) {
            // Store the stack segment information in the task_info structure
            // Set the start and end addresses of the stack segment
            info->stack.vm_start = task->mm->start_stack;
            info->stack.vm_end = vma->vm_end;
            info->stack.phys_start = virt_to_phys((void *)info->stack.vm_start);
            info->stack.phys_end = virt_to_phys((void *)info->stack.vm_end);
        }
    }
    // Release the read lock on the mmap_lock of the task's mm
    up_read(&task->mm->mmap_lock);

    current_index = (current_index + 1) % MAX_TASKS;
    task_count++;
}

// This function finds the latest task (process) based on their start time and adds it to the task history.
void find_latest_task(void) {
    struct task_struct *task, *latest_task = NULL;
    unsigned long latest_start_time = 0;

    // Acquire a read lock on the RCU (Read-Copy Update) mechanism to safely iterate over the process list
    rcu_read_lock();

    // Iterate over each process using the for_each_process macro
    for_each_process(task) {
        // Check if the process has a valid memory descriptor and if its start time is greater than the current latest start time
        if (task->mm != NULL && (task->start_time > latest_start_time)) {
            // Update the latest task and latest start time
            latest_task = task;
            latest_start_time = task->start_time;
        }
    }

    // Release the read lock on the RCU mechanism
    rcu_read_unlock();

    // If a latest task was found, add it to the task history
    if (latest_task != NULL) {
        add_task_to_history(latest_task);
    }
}

// This is a timer callback function that is executed periodically.
void timer_callback(struct timer_list *timer) {
    // Schedule the execution of the tasklet
    tasklet_schedule(&my_tasklet);
    // Modify the timer to trigger again after a specific interval
    mod_timer(timer, jiffies + msecs_to_jiffies(INTERVAL * 1000));
}

// This is the handler function for the tasklet.
void my_tasklet_handler(struct tasklet_struct *tsk) {
    // Call the function to find the latest task
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
    seq_printf(m, "Uptime (s): %u\n", jiffies_to_msecs(jiffies) / 1000);
    seq_printf(m, "--------------------------------------------------\n");


    for (i = 0; i < min(task_count, MAX_TASKS); i++) {
        info = &task_history[(current_index + i + max(MAX_TASKS - task_count, 0)) % MAX_TASKS];
        seq_printf(m, "[Trace #%d]\n", i);
        seq_printf(m, "Uptime (s): %llu\n", info->uptime);
        seq_printf(m, "Command: %s\n", info->comm);
        seq_printf(m, "PID: %d\n", info->pid);
        seq_printf(m, "Start time (s): %llu\n", info->start_time);
        seq_printf(m, "PGD base address: %p\n", info->pgd_base);

        seq_printf(m, "Code Area\n");
        seq_printf(m, "- start (virtual): 0x%lx\n", info->code.vm_start);
        seq_printf(m, "- start (PGD): 0x%lx, 0x%lx\n", info->code.pgd_start, pgd_val((pgd_t){info->code.pgd_start}));
        seq_printf(m, "- start (PUD): 0x%lx, 0x%lx\n", info->code.pud_start, pud_val((pud_t){info->code.pud_start}));
        seq_printf(m, "- start (PMD): 0x%lx, 0x%lx\n", info->code.pmd_start, pmd_val((pmd_t){info->code.pmd_start}));
        seq_printf(m, "- start (PTE): 0x%lx, 0x%lx\n", info->code.pte_start, pte_val((pte_t){info->code.pte_start}));
        seq_printf(m, "- start (physical): 0x%lx\n", info->code.phys_start);
        seq_printf(m, "- end (virtual): 0x%lx\n", info->code.vm_end);
        seq_printf(m, "- end (PGD): 0x%lx, 0x%lx\n", info->code.pgd_end, pgd_val((pgd_t){info->code.pgd_end}));
        seq_printf(m, "- end (PUD): 0x%lx, 0x%lx\n", info->code.pud_end, pud_val((pud_t){info->code.pud_end}));
        seq_printf(m, "- end (PMD): 0x%lx, 0x%lx\n", info->code.pmd_end, pmd_val((pmd_t){info->code.pmd_end}));
        seq_printf(m, "- end (PTE): 0x%lx, 0x%lx\n", info->code.pte_end, pte_val((pte_t){info->code.pte_end}));
        seq_printf(m, "- end (physical): 0x%lx\n", info->code.phys_end);

        seq_printf(m, "Data Area\n");
        seq_printf(m, "- start (virtual): 0x%lx\n", info->data.vm_start);
        seq_printf(m, "- start (physical): 0x%lx\n", info->data.phys_start);
        seq_printf(m, "- end (virtual): 0x%lx\n", info->data.vm_end);
        seq_printf(m, "- end (physical): 0x%lx\n", info->data.phys_end);

        seq_printf(m, "Heap Area\n");
        seq_printf(m, "- start (virtual): 0x%lx\n", info->heap.vm_start);
        seq_printf(m, "- start (physical): 0x%lx\n", info->heap.phys_start);
        seq_printf(m, "- end (virtual): 0x%lx\n", info->heap.vm_end);
        seq_printf(m, "- end (physical): 0x%lx\n", info->heap.phys_end);

        seq_printf(m, "Stack Area\n");
        seq_printf(m, "- start (virtual): 0x%lx\n", info->stack.vm_start);
        seq_printf(m, "- start (physical): 0x%lx\n", info->stack.phys_start);
        seq_printf(m, "- end (virtual): 0x%lx\n", info->stack.vm_end);
        seq_printf(m, "- end (physical): 0x%lx\n", info->stack.phys_end);

        seq_printf(m, "--------------------------------------------------\n");
    }


    return 0;
}

static int proc_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_show, NULL);
}

// This structure defines the operations that are supported by the /proc/hw2 file.
// The proc_open field is set to sched_info_proc_open.
// The proc_read field is set to seq_read, which is a function that reads data from a seq_file.
// The proc_lseek field is set to seq_lseek, which is a function that sets the position of the seq_file.
// The proc_release field is set to seq_release, which is a function that releases the resources used by the seq_file.
static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};


static struct timer_list my_timer;

// load module
// This creates /proc/hw2 by calling proc_create.
static int __init my_module_init(void) {
    proc_create(PROC_NAME, 0, NULL, &proc_fops);

    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies);

    printk(KERN_INFO "hw2 module loaded\n");

    return 0;
}

// unload module
// This removes the /proc/hw2 file by calling remove_proc_entry.
static void __exit my_module_exit(void) {

    remove_proc_entry(PROC_NAME, NULL);
    timer_delete(&my_timer);
    tasklet_kill(&my_tasklet);

    printk(KERN_INFO "hw2 module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);