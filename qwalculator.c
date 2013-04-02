#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define DRIVER_AUTHOR "Eugene Gritskevich <qwaker.00@gmail.com>"
#define DRIVER_DESC   "Calculator module"

static char* inp1 = "calc_inp1";
static char* inp2 = "calc_inp2";
static char* inp3 = "calc_inp3";
static char* out = "calc_out";

module_param(inp1, charp, 0000);
MODULE_PARM_DESC(inp1, "Input file number 1");
module_param(inp2, charp, 0000);
MODULE_PARM_DESC(inp2, "Input file number 2");
module_param(inp3, charp, 0000);
MODULE_PARM_DESC(inp3, "Input file number 3");
module_param(out, charp, 0000);
MODULE_PARM_DESC(out, "Output file");
 
static struct proc_dir_entry* proc_files[4] = {NULL, NULL, NULL, NULL};
static char * proc_names[4] = {NULL, NULL, NULL, NULL};

static long var1 = 0;
static long var2 = 0;
static char op = '+';

static size_t read_var(char *buffer,
                       char **buffer_location,
                       off_t offset, int buffer_length, int *eof, void *data)
{    
    long len;
    long value = *(long*)data;
    if (offset > 0)
        return 0;
    len = sprintf(buffer, "%ld\n", value);
    return len;
}

static size_t write_var(struct file* file, const char* buffer,
                        unsigned long count, void* data)
{
    char ubuffer[512];

    if (count > 511) count = 511;
    if (copy_from_user(ubuffer, buffer, count)) {
        printk(KERN_ERR "Failed to write to proc fs\n");
        return -EFAULT;
    }
    ubuffer[count] = 0;
    if (sscanf(ubuffer, "%ld", &*(long*)data) != 1) {
        printk(KERN_ERR "Long value expected\n");
        return -EFAULT;
    }

    return count;
}

static size_t read_op(char *buffer,
                      char **buffer_location,
                      off_t offset, int buffer_length, int *eof, void *data)
{    
    long len;
    char value = *(char*)data;
    if (offset > 0)
        return 0;
    len = sprintf(buffer, "%c\n", value);
    return len;
}

static size_t write_op(struct file* file, const char* buffer,
                       unsigned long count, void* data)
{
    char ubuffer[512], new_op;

    if (count > 511) count = 511;
    if (copy_from_user(ubuffer, buffer, count)) {
        printk(KERN_ERR "Failed to write to proc fs\n");
        return -EFAULT;
    }
    new_op = ubuffer[0];
    if (strchr("+-/*", new_op) == NULL) {
        printk(KERN_ERR "One of '+', '-', '/', '*' expected\n");
        return -EFAULT;
    }
    *(char*)data = new_op;

    return count;
}

static size_t read_result(char *buffer,
                          char **buffer_location,
                          off_t offset, int buffer_length, int *eof, void *data)
{
    long len = 0;
    if (offset > 0)
        return 0;

    switch (op) {
        case '+': len = sprintf(buffer, "%ld\n", var1 + var2); break;
        case '-': len = sprintf(buffer, "%ld\n", var1 - var2); break;
        case '*': len = sprintf(buffer, "%ld\n", var1 * var2); break;
        case '/':
            if (var2 == 0)
                len = sprintf(buffer, "Division error\n");else
                len = sprintf(buffer, "%ld\n", var1 / var2);
            break;
    }
    
    return len;
}

static int __init init(void)
{
    int i;

    printk(KERN_INFO "Trying to create proc files:\n");

    proc_names[0] = inp1;
    proc_names[1] = inp2;
    proc_names[2] = inp3;
    proc_names[3] = out;

    for (i = 0; i < 4; ++i) {
        proc_files[i] = create_proc_entry(proc_names[i], 0666, NULL);
        if (proc_files[i] == NULL) {
            printk(KERN_ERR "Failed to create /proc/%s", proc_names[i]);
            return -ENOMEM;
        } else {
            printk(KERN_INFO "/proc/%s\n", proc_names[i]);
        }
    }

    proc_files[0]->read_proc = (void*)read_var;
    proc_files[0]->write_proc = (void*)write_var;
    proc_files[0]->data = &var1;

    proc_files[1]->read_proc = (void*)read_var;
    proc_files[1]->write_proc = (void*)write_var;
    proc_files[1]->data = &var2;

    proc_files[2]->read_proc = (void*)read_op;
    proc_files[2]->write_proc = (void*)write_op;
    proc_files[2]->data = &op;

    proc_files[3]->read_proc = (void*)read_result;
 
    return 0;
}

static void __exit cleanup(void)
{
    int i;

    for (i = 0; i < 4; ++i) 
        if (proc_files[i] != NULL) {
            remove_proc_entry(proc_names[i], NULL);
        }

    printk(KERN_ALERT "proc files removed\n");
}

module_init(init);
module_exit(cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("idevice");

