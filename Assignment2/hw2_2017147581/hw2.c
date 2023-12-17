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

int interval = 10;


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
        unsigned long phys_start, phys_end;
    } stack, data, code, heap;
};

// Circular buffer for storing task information
#define MAX_TASKS 5
static struct task_info task_history[MAX_TASKS];
static int current_index = 0;

// Function to add task information to the buffer
void add_task_to_history(struct task_struct *task) {
    struct task_info *info;
    struct vm_area_struct *vma;

    if (task == NULL) return;

    info = &task_history[current_index];
    info->pid = task->pid;
    strncpy(info->comm, task->comm, TASK_COMM_LEN);
    info->start_time_ns = &task->start_time;
    info->pgd_base = task->mm->pgd;

    down_read(&task->mm->mmap_lock);
    mt_for_each(task->mm->mm_mt, vma) {
        if (vma->vm_start <= task->mm->start_code && vma->vm_end >= task->mm->end_code) {
            info->code.vm_start = vma->vm_start;
            info->code.vm_end = vma->vm_end;
            info->code.phys_start = virt_to_phys((void *)vma->vm_start);
            info->code.phys_end = virt_to_phys((void *)vma->vm_end);
        }
        if (vma->vm_start <= task->mm->start_data && vma->vm_end >= task->mm->end_data) {
            info->data.vm_start = vma->vm_start;
            info->data.vm_end = vma->vm_end;
            info->data.phys_start = virt_to_phys((void *)vma->vm_start);
            info->data.phys_end = virt_to_phys((void *)vma->vm_end);
        }
        if (vma->vm_start <= task->mm->start_brk && vma->vm_end >= task->mm->brk) {
            info->heap.vm_start = vma->vm_start;
            info->heap.vm_end = vma->vm_end;
            info->heap.phys_start = virt_to_phys((void *)vma->vm_start);
            info->heap.phys_end = virt_to_phys((void *)vma->vm_end);
        }
        if (vma->vm_start <= task->mm->start_stack && vma->vm_end >= task->mm->start_stack) {
            info->stack.vm_start = vma->vm_start;
            info->stack.vm_end = vma->vm_end;
            info->stack.phys_start = virt_to_phys((void *)vma->vm_start);
            info->stack.phys_end = virt_to_phys((void *)vma->vm_end);
        }
    }
    up_read(&task->mm->mmap_lock);

    current_index = (current_index + 1) % MAX_TASKS;
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
    mod_timer(timer, jiffies + msecs_to_jiffies(interval * 1000));
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

    for (i = 0; i < MAX_TASKS; i++) {
        info = &task_history[(current_index + i) % MAX_TASKS];
        seq_printf(m, "PID: %d\n", info->pid);
        seq_printf(m, "COMM: %s\n", info->comm);
        seq_printf(m, "START_TIME: %llu\n", info->start_time_ns);
        seq_printf(m, "PGD_BASE: %p\n", info->pgd_base);
        seq_printf(m, "CODE: %lx-%lx\n", info->code.vm_start, info->code.vm_end);
        seq_printf(m, "DATA: %lx-%lx\n", info->data.vm_start, info->data.vm_end);
        seq_printf(m, "HEAP: %lx-%lx\n", info->heap.vm_start, info->heap.vm_end);
        seq_printf(m, "STACK: %lx-%lx\n", info->stack.vm_start, info->stack.vm_end);
        seq_printf(m, "\n");
    }


    return 0;
}

static int proc_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_fops = {
    .owner = THIS_MODULE,
    .open = proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

struct proc_dir_entry *proc_entry;

static int __init my_module_init(void) {
    proc_entry = proc_create(PROC_NAME, 0, NULL, &proc_fops);
    if (!proc_entry) {
        return -ENOMEM;
    }

    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies);

    return 0;
}

static void __exit my_module_exit(void) {

    proc_remove(proc_entry);
    timer_delete(&my_timer);
    tasklet_kill(&my_tasklet);

}

module_init(my_module_init);
module_exit(my_module_exit);