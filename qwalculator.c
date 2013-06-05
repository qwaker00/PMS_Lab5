#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
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
 
static int char_dev[4] = {0, 0, 0, 0};
static char * char_names[4] = {NULL, NULL, NULL, NULL};

static long var1 = 0;
static long var2 = 0;
static char op = '+';

static ssize_t read_var1(struct file* fil,
                        char* buffer,
                        size_t length,
                        loff_t* offset)
{    
    long len;
    len = sprintf(buffer, "%ld\n", var1);
    return len;
}

static ssize_t write_var1(struct file* fil,
                         const char* buffer,
                         size_t count,
                         loff_t* offset)
{
    char ubuffer[512];

    if (count > 511) count = 511;
    if (copy_from_user(ubuffer, buffer, count)) {
        printk(KERN_ERR "Failed to write to char dev\n");
        return -EFAULT;
    }
    ubuffer[count] = 0;
    if (sscanf(ubuffer, "%ld", &*(long*)var1) != 1) {
        printk(KERN_ERR "Long value expected\n");
        return -EFAULT;
    }

    return count;
}

static ssize_t read_var2(struct file* fil,
                        char* buffer,
                        size_t length,
                        loff_t* offset)
{    
    long len;
    len = sprintf(buffer, "%ld\n", var2);
    return len;
}

static ssize_t write_var2(struct file* fil,
                         const char* buffer,
                         size_t count,
                         loff_t* offset)
{
    char ubuffer[512];

    if (count > 511) count = 511;
    if (copy_from_user(ubuffer, buffer, count)) {
        printk(KERN_ERR "Failed to write to char dev\n");
        return -EFAULT;
    }
    ubuffer[count] = 0;
    if (sscanf(ubuffer, "%ld", &*(long*)var2) != 1) {
        printk(KERN_ERR "Long value expected\n");
        return -EFAULT;
    }

    return count;
}

static size_t read_op(struct file* fil,
                      char* buffer,
                      size_t length,
                      loff_t* offset)
{    
    long len;
    len = sprintf(buffer, "%c\n", op);
    return len;
}

static size_t write_op(struct file* fil,
                       const char* buffer,
                       size_t count,
                       loff_t* offset)
{
    char ubuffer[512], new_op;

    if (count > 511) count = 511;
    if (copy_from_user(ubuffer, buffer, count)) {
        printk(KERN_ERR "Failed to write to char dev\n");
        return -EFAULT;
    }
    new_op = ubuffer[0];
    if (strchr("+-/*", new_op) == NULL) {
        printk(KERN_ERR "One of '+', '-', '/', '*' expected\n");
        return -EFAULT;
    }
    op = new_op;

    return count;
}

static size_t read_result(struct file* fil,
                          char *buffer,
                          size_t length,
                          loff_t* offset)
{
    long len = 0;
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
    struct file_operations fops[4];
    memset(fops, 0, sizeof(fops));

    printk(KERN_INFO "Trying to create proc files:\n");

    char_names[0] = inp1;
    char_names[1] = inp2;
    char_names[2] = inp3;
    char_names[3] = out;

    fops[0].read = (void*)read_var1;
    fops[0].write = (void*)write_var1;
    fops[1].read = (void*)read_var2;
    fops[1].write = (void*)write_var2;
    fops[2].read = (void*)read_op;
    fops[2].write = (void*)write_op;
    fops[3].read = (void*)read_result;
   
    for (i = 0; i < 4; ++i) {
        char_dev[i] = register_chrdev(0, char_names[i], &fops[i]);
        if (char_dev[i] < 0) {
            printk(KERN_ERR "Failed to create /dev/%s", char_names[i]);
            return -ENOMEM;
        } else {
            printk(KERN_INFO "/dev/%s\n", char_names[i]);
        }
    }
    return 0;
}

static void __exit cleanup(void)
{
    int i;

    for (i = 0; i < 4; ++i) 
        if (char_dev[i] != 0) {
            unregister_chrdev(char_dev[i], char_names[i]);
        }

    printk(KERN_ALERT "char files removed\n");
}

module_init(init);
module_exit(cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("idevice");

