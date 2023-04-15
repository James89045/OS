#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/utsname.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <asm/processor.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/vmstat.h>
#include "kfetch.h"
#define cpu_data(cpu)		(&per_cpu(cpu_info, cpu))
#define DEVICE_NAME "kfetch"
#define SUCCESS 0

int init_module(void);
void cleanup_module(void);
static int kfetch_open(struct inode *, struct file *);
static int kfetch_release(struct inode *, struct file *);
static ssize_t kfetch_read(struct file *, char *, size_t, loff_t *);
static ssize_t kfetch_write(struct file *, const char *, size_t, loff_t *);
int uname(struct utsname *buf);
static char kfetch_buf[4096]; 
static int Major;               /* Major number assigned to our device driver */   
static int Device_Open = 0;  
static struct class *cls;
int mask_info;
//static ssize_t read_num_processes(struct file *file, char __user *buf, size_t count, loff_t *ppos);

const static struct file_operations kfetch_ops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,
    .write   = kfetch_write,
    .open    = kfetch_open,
    .release = kfetch_release
};

/*
 * This function is called when the module is loaded
 */
int init_module(void) {
    Major = register_chrdev(0, DEVICE_NAME, &kfetch_ops);
    pr_info("this is  test\n");
    if (Major < 0) {
        pr_alert("Registering char device failed with %d\n", Major);
        return Major;
    }

    pr_info("I was assigned major number %d.\n", Major);

    cls = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(cls, NULL, MKDEV(Major, 0), NULL, DEVICE_NAME);

    pr_info("Device created on /dev/%s\n", DEVICE_NAME);
    

    return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void) {

    device_destroy(cls, MKDEV(Major, 0));
    class_destroy(cls);
    /*
     * Unregister the device
     */
    unregister_chrdev(Major, DEVICE_NAME);
}



static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    
    char tmp[100];
    int check_array[6] = {0};
    char kernel[100];
    char cpu[100];
    char cpus[100];
    char mem[100];
    char procs[100];
    char uptimes[100];
    sprintf(tmp, "                    %s\n", utsname()->nodename);
    strcat(kfetch_buf, tmp);
    //struct utsname info;
    /* fetching the information */
    sprintf(tmp, "        .-.         ------------\n");
   
    char *info_array[6];
    strcat(kfetch_buf, tmp);
    
    char penguin[6][100] = {"       (.. |        ", 
                           "       <>  |        ", 
                           "      / --- \\       ", 
                           "     ( |   | |      ", 
                           "   |\\_)___/\\)/\\     ", 
                           "  <__)------(__/    "};
                           
    //printk("%s\n", penguin[0]);
    
    sprintf(kernel, "Kernel: %s\n", utsname()->release);//kernel name
    sprintf(cpu, "CPU: %s\n", cpu_data(smp_processor_id())->x86_model_id);//cpu name
    //printk("%s\n", cpu);
    sprintf(cpus, "CPUs: %d / %d\n", smp_processor_id(), nr_cpu_ids);//cpu number

    struct sysinfo info;
    si_meminfo(&info);
    unsigned long total_ram = info.totalram;
    unsigned long free_ram = info.freeram;
    total_ram /= 1024;
    total_ram *= PAGE_SIZE;
    free_ram /= 1024 ;
    free_ram *= PAGE_SIZE;
    sprintf(mem, "Mem: %d / %d MB\n", (int)free_ram/1024, (int)total_ram/1024);//memory info

    s64  uptime;
    uptime = ktime_divns(ktime_get_coarse_boottime(), NSEC_PER_SEC);
    sprintf(uptimes, "Uptime: %lld mins\n", uptime/60);//uptime
    //printk("%s", uptimes);
    struct task_struct* task_list;
    int thread_counter = 0;
    for_each_process(task_list) {
        thread_counter += task_list->signal->nr_threads;
    }
    
    sprintf(procs, "Proc: %d\n", thread_counter);//process number
    //printk("%s", procs);
    //copy information
    info_array[0] = kernel;
    info_array[1] = cpu;
    info_array[2] = cpus;
    info_array[3] = mem;
    info_array[4] = procs;
    info_array[5] = uptimes;
    //printk("%s", info_array[0]);

    if (mask_info & KFETCH_RELEASE ) {
        check_array[0] = 1;
    }
    if (mask_info & KFETCH_CPU_MODEL) {
        check_array[1] = 1;
    }
    if (mask_info & KFETCH_NUM_CPUS) {
        check_array[2] = 1;
    }
    if (mask_info & KFETCH_MEM) {
        check_array[3] = 1;
    }
    if (mask_info & KFETCH_NUM_PROCS) {
        check_array[4] = 1;
    }
    if (mask_info & KFETCH_UPTIME) {
        check_array[5] = 1;
    }
    for (int i=0; i<6; i++) {

        for (int j=0; j<6; j++) {
            if (check_array[j] == 1) {
                strcat(penguin[i], info_array[j]);                                                                 
                check_array[j] = 0;
                strcat(kfetch_buf, penguin[i]);
                break;
            }
            if (j==5) {
                strcat(penguin[i], "\n");
                strcat(kfetch_buf, penguin[i]);
            }
        }
    }

    int len = strlen(kfetch_buf);

    if (copy_to_user(buffer, kfetch_buf, len)) {
        pr_alert("Failed to copy data to user");
        return 0;
    }

    /* cleaning up */
    return len;
}

static ssize_t kfetch_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    
    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }

    /* setting the information mask */
    return SUCCESS;
}

static int kfetch_open(struct inode *inode, struct file *file)
{

    if (Device_Open)
        return -EBUSY;

    try_module_get(THIS_MODULE);

    return SUCCESS;
}
/*
 * Called when a process closes the device file.
 */
static int kfetch_release(struct inode *inode, struct file *file)
{

    module_put(THIS_MODULE);
    return SUCCESS;
}
MODULE_LICENSE("GPL");