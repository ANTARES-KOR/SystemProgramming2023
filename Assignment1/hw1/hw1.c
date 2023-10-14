#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>

// Define module process name
#define PROC_NAME "hw1"

MODULE_AUTHOR("Hyeokjun, Seo");
MODULE_LICENSE("GPL");


// loads Global variable from the kernel
extern struct hw1_schedule_info hw1_recent_schedules[20];
extern int hw1_recent_schedules_index;


static int sched_info_proc_show(struct seq_file *s, void *v) {

    seq_printf(s, "[System Programming Assignment #1]\n");
    seq_printf(s, "ID : 2017147581\n");
    seq_printf(s, "Name: Seo, Hyeokjun\n");
    seq_printf(s, "#CPU: %d\n", num_online_cpus());
    seq_printf(s, "------------------------------\n");


    // loads recent 20 schedule info
    // since hw1_recent_schedules_index stores the slot for "next" schedule info,
    // by starting on that index, we can get the 20 schedule information in the ascending order.
    for(int i = 0; i<20; i++) {
        int index = (hw1_recent_schedules_index + i) % 20;
        struct hw1_schedule_info *info = &hw1_recent_schedules[index];

        seq_printf(s, "schedule() trace #%d - CPU #%d\n", i, info->cpu);

        seq_printf(s, "Command: %s\n", info->prev.task_name);
        seq_printf(s, "PID: %d\n", info->prev.pid);
        seq_printf(s, "Priority: %d\n", info->prev.prio);
        seq_printf(s, "Start time (ms): %lu\n", info->prev.creat_time);
        seq_printf(s, "Scheduler: %s\n", info->prev.sched_type);

        seq_printf(s, "->\n");

        seq_printf(s, "Command: %s\n", info->next.task_name);
        seq_printf(s, "PID: %d\n", info->next.pid);
        seq_printf(s, "Priority: %d\n", info->next.prio);
        seq_printf(s, "Start time (ms): %lu\n", info->next.creat_time);
        seq_printf(s, "Scheduler: %s\n", info->next.sched_type);

	seq_printf(s, "------------------------------\n");
    }

    return 0;
}

// This function is called when the /proc/hw1 file is opened.
static int sched_info_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, sched_info_proc_show, NULL);
}


// This structure defines the operations that are supported by the /proc/hw1 file.
// The proc_open field is set to sched_info_proc_open.
// The proc_read field is set to seq_read, which is a function that reads data from a seq_file.
// The proc_lseek field is set to seq_lseek, which is a function that sets the position of the seq_file.
// The proc_release field is set to seq_release, which is a function that releases the resources used by the seq_file.
static const struct proc_ops sched_info_proc_fops = {
    .proc_open = sched_info_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

// load module
// This creates /proc/hw1 by calling proc_create.
static int __init hw1_init(void) {
    proc_create(PROC_NAME, 0, NULL, &sched_info_proc_fops);
    printk(KERN_INFO "hw1 module loaded\n");
    return 0;
}

// unload module
// This removes the /proc/hw1 file by calling remove_proc_entry.
static void __exit hw1_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "hw1 module unloaded\n");
}

module_init(hw1_init);
module_exit(hw1_exit);