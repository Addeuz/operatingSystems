#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>


void process_info_printing(void) {
    struct task_struct* task_list;
    size_t process_counter = 0;

    for_each_process(task_list) {
        pr_info("== %s [%d]\n", task_list->comm, task_list->pid);
        ++process_counter;
    }
    printk(KERN_INFO "== Number of process: %zu\n", process_counter);
}

int init_function(void) {
    printk(KERN_INFO "[ INIT ==\n");

    process_info_printing();

    return 0;
}

void exit_function(void) {
	printk(KERN_INFO "Removing Module\n");
}

/* Registering entry and exit points */
module_init(init_function);
module_exit(exit_function);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("SGG");
