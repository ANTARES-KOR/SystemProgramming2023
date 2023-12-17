#include <linux/module.h>
#include <linux/sched/signal.h>
#include <linux/timer.h>
#include <linux/sched.h>        // for task_struct
#include <linux/mm_types.h>     // for mm_struct
#include <linux/mm.h>           // for vm_area_struct
#include <linux/jiffies.h>      // for jiffies and time calculations


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hyeokjun, Seo");
MODULE_DESCRIPTION("A module that tracks the most recently started non-kernel task.");

// Tasklet and timer for periodic execution
static struct timer_list my_timer;
static struct task_struct *latest_task = NULL;

void find_latest_task(void) {
    struct task_struct *task;
    unsigned long latest_start_time = 0;
    struct timespec64 uptime;
    unsigned long long start_time_ns;

    rcu_read_lock();
    for_each_process(task) {
        if (task->mm != NULL && (task->start_time > latest_start_time)) {
            latest_task = task;
            latest_start_time = task->start_time;
        }
    }
    rcu_read_unlock();

    if (latest_task != NULL) {
        // Calculate uptime
        start_time_ns = timespec64_to_ns(&latest_task->real_start_time);
        uptime = timespec64_sub(current_kernel_time64(), ns_to_timespec64(start_time_ns));

        // Print task information
        printk(KERN_INFO "PID: %d, Name: %s, Start Time: %llu, Uptime: %lu.%lus\n",
               latest_task->pid, latest_task->comm,
               start_time_ns, (unsigned long)uptime.tv_sec, uptime.tv_nsec / NSEC_PER_SEC);

        // Print memory-related information
        if (latest_task->mm) {
            printk(KERN_INFO "PGD Base Address: %p\n", latest_task->mm->pgd);

            struct vm_area_struct *vma = latest_task->mm->mmap;
            while (vma) {
                if (vma->vm_start <= latest_task->mm->start_code && vma->vm_end >= latest_task->mm->end_code) {
                    printk(KERN_INFO "Code Segment: Virtual [%lx, %lx], Physical [%lx, %lx]\n",
                           vma->vm_start, vma->vm_end, virt_to_phys((void *)vma->vm_start), virt_to_phys((void *)vma->vm_end));
                }
                if (vma->vm_start <= latest_task->mm->start_data && vma->vm_end >= latest_task->mm->end_data) {
                    printk(KERN_INFO "Data Segment: Virtual [%lx, %lx], Physical [%lx, %lx]\n",
                           vma->vm_start, vma->vm_end, virt_to_phys((void *)vma->vm_start), virt_to_phys((void *)vma->vm_end));
                }
                if (vma->vm_start <= latest_task->mm->start_brk && vma->vm_end >= latest_task->mm->brk) {
                    printk(KERN_INFO "Heap Segment: Virtual [%lx, %lx], Physical [%lx, %lx]\n",
                           vma->vm_start, vma->vm_end, virt_to_phys((void *)vma->vm_start), virt_to_phys((void *)vma->vm_end));
                }
                if (vma->vm_start <= latest_task->mm->start_stack && vma->vm_end >= latest_task->mm->start_stack) {
                    printk(KERN_INFO "Stack Segment: Virtual [%lx, %lx], Physical [%lx, %lx]\n",
                           vma->vm_start, vma->vm_end, virt_to_phys((void *)vma->vm_start), virt_to_phys((void *)vma->vm_end));
                }
                vma = vma->vm_next;
            }
        }
    }
}

// Tasklet handler function
void my_tasklet_handler(unsigned long data) {
    find_latest_task();
}

DECLARE_TASKLET(my_tasklet, my_tasklet_handler, 0);

// Timer callback function
void my_timer_callback(struct timer_list *t) {
    tasklet_schedule(&my_tasklet);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(10000)); // Reschedule for next 10 seconds
}

// Module initialization
static int __init my_module_init(void) {
    printk(KERN_INFO "Module initialized\n");

    // Initialize and start the timer
    timer_setup(&my_timer, my_timer_callback, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(10000));
    return 0;
}

// Module cleanup
static void __exit my_module_exit(void) {
    del_timer(&my_timer);
    tasklet_kill(&my_tasklet);
    printk(KERN_INFO "Module removed\n");
}

module_init(my_module_init);
module_exit(my_module_exit);




