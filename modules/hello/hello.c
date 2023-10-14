#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define PROC_NAME "hello"

MODULE_AUTHOR("Hyeokjun, Seo");
MODULE_LICENSE("GPL");

// This function is called when the sequence is started.
// It initializes a counter to 0 and returns a pointer to the counter.
static void *hello_seq_start(struct seq_file*s, loff_t *pos)
{
    static unsigned long counter = 0;
    if (*pos == 0)
    {
        return &counter;
    }
    else
    {
        *pos = 0;
        return NULL;
    }
}

// This function is called to advance the sequence to the next element.
// It increments the counter and returns NULL.
static void *hello_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    unsigned long *tmp_v = (unsigned long *)v;
    (*tmp_v)++;
    (*pos)++;
    return NULL;
}

// This function is called when the sequence is finished.
// It does nothing.
static void hello_seq_stop(struct seq_file *s, void *v) {
    // Nothing to do
}

// This function is called to generate the output for the current element of the sequence.
// It prints "World!" to the seq_file and returns 0.
static int hello_seq_show(struct seq_file *s, void *v) {
    seq_printf(s, "World!\n");
    return 0;
}

// This structure defines the operations that are supported by the /proc/hello file.
static struct seq_operations hello_seq_ops = {
    .start = hello_seq_start,
    .next = hello_seq_next,
    .stop = hello_seq_stop,
    .show = hello_seq_show
};

// This function is called when the /proc/hello file is opened.
// It calls seq_open to initialize the seq_file and returns the result.
static int hello_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &hello_seq_ops);
}

// This structure defines the operations that are supported by the /proc/hello file.
// The proc_open field is set to hello_proc_open.
// The proc_read field is set to seq_read, which is a function that reads data from a seq_file.
// The proc_lseek field is set to seq_lseek, which is a function that sets the position of the seq_file.
// The proc_release field is set to seq_release, which is a function that releases the resources used by the seq_file.
static struct proc_ops hello_proc_ops = {
    .proc_open = hello_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};

// This function is called when the module is loaded.
// It creates the /proc/hello file by calling proc_create.
static int __init hello_init(void)
{
    proc_create(PROC_NAME, 0, NULL, &hello_proc_ops);
    return 0;
}

// This function is called when the module is unloaded.
// It removes the /proc/hello file by calling remove_proc_entry.
static void __exit hello_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
}

module_init(hello_init);
module_exit(hello_exit);