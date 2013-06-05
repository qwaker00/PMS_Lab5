#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>

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

static char buf[512];

static long var1 = 0;
static long var2 = 0;
static char op = '+';

#define SUCCESS 0

static ssize_t read_var1(struct file* fil,
                        char* buffer,
                        size_t length,
                        loff_t* offset)
{    
    long len;
    static int finished = 0;
 
    printk(KERN_INFO "read var 1\n");

    if (finished) {
        finished = 0;
        return 0;
    }
    finished = 1;

    len = sprintf(buf, "%ld\n", var1);
    if (copy_to_user(buffer, buf, len)) {
        return -EFAULT;
    }
    return len;
}

static ssize_t write_var1(struct file* fil,
                         const char* buffer,
                         size_t count,
                         loff_t* offset)
{
    printk(KERN_INFO "write var 1\n");
 
    if (count > 511) count = 511;
    if (copy_from_user(buf, buffer, count)) {
        printk(KERN_ERR "Failed to write to char dev\n");
        return -EFAULT;
    }
    buf[count] = 0;
    if (sscanf(buf, "%ld", &var1) != 1) {
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
    static int finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }
    finished = 1;

    len = sprintf(buf, "%ld\n", var2);
    if (copy_to_user(buffer, buf, len)) {
        return -EFAULT;
    }
    return len;
}

static ssize_t write_var2(struct file* fil,
                         const char* buffer,
                         size_t count,
                         loff_t* offset)
{
    if (count > 511) count = 511;
    if (copy_from_user(buf, buffer, count)) {
        printk(KERN_ERR "Failed to write to char dev\n");
        return -EFAULT;
    }
    buf[count] = 0;
    if (sscanf(buf, "%ld", &var2) != 1) {
        printk(KERN_ERR "Long value expected\n");
        return -EFAULT;
    }

    return count;
}

static ssize_t read_op(struct file* fil,
                       char* buffer,
                       size_t length,
                       loff_t* offset)
{   
    static int finished = 0;
    char nl = '\n';

    if (finished) {
        finished = 0;
        return 0;
    }
    finished = 1;

    if (copy_to_user(buffer, &op, 1)) {
        return -EFAULT;
    }
    if (copy_to_user(buffer + 1, &nl, 1)) {
        return -EFAULT;
    }
    return 2;
}

static ssize_t write_op(struct file* fil,
                        const char* buffer,
                        size_t count,
                        loff_t* offset)
{
    char new_op;

    if (count > 511) count = 511;
    if (copy_from_user(buf, buffer, count)) {
        printk(KERN_ERR "Failed to write to char dev\n");
        return -EFAULT;
    }
    new_op = buf[0];
    if (strchr("+-/*", new_op) == NULL) {
        printk(KERN_ERR "One of '+', '-', '/', '*' expected\n");
        return -EFAULT;
    }
    op = new_op;

    return count;
}


static int read_result(struct seq_file *m, void *v)
{
    switch (op) {
        case '+': seq_printf(m, "%ld\n", var1 + var2); break;
        case '-': seq_printf(m, "%ld\n", var1 - var2); break;
        case '*': seq_printf(m, "%ld\n", var1 * var2); break;
        case '/':
            if (var2 == 0)
                seq_printf(m, "Division error\n");else
                seq_printf(m, "%ld\n", var1 / var2);
            break;
    }
    return 0;
}

static int open_result(struct inode *inode, struct file *file)
{
    return single_open(file, read_result, NULL);
}
 

static int fs_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "fs_open\n");
    try_module_get(THIS_MODULE);
    return SUCCESS;
}

static int fs_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "fs_release\n");
    module_put(THIS_MODULE);
    return SUCCESS;
}

struct file_operations fops[4] = {
    {
        .owner = THIS_MODULE,
        .open = fs_open,
        .release = fs_release,
        .write = write_var1,
        .read = read_var1,
    },
    {
        .owner = THIS_MODULE,
        .open = fs_open,
        .release = fs_release,
        .write = write_var2,
        .read = read_var2,
    },
    {
        .owner = THIS_MODULE,
        .open = fs_open,
        .release = fs_release,
        .write = write_op,
        .read = read_op,
    },
    {
        .owner = THIS_MODULE,
        .open = open_result,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = single_release,
    }
};
 

static int __init init( void )
{
    int i;

    char_names[0] = inp1;
    char_names[1] = inp2;
    char_names[2] = inp3;
    char_names[3] = out;
 
    printk( KERN_ALERT "qwalculator module loaded\n");
    for (i = 0; i < 4; ++i) {
        char_dev[i] = register_chrdev(0, char_names[i], &fops[i]);
        printk(KERN_INFO "%s = %d\n", char_names[i], char_dev[i]);
    }

    return 0;
}

static void __exit cleanup( void )
{
    int i;
    for (i = 0; i < 4; ++i) {
        unregister_chrdev(char_dev[i], char_names[i]);
    }
    printk(KERN_ALERT "qwalculator module is unloaded\n");
}

module_init(init);
module_exit(cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("idevice");

