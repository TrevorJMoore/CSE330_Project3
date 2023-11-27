#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/module.h>

// TODO 1: Define your input parameter (pid - int) here
// Then use module_param to pass them from insmod command line. (--Assignment 2)

static int pid;

module_param(pid, int, 0);

static struct hrtimer hr_timer;
//struct task_struct *task = NULL;
// Initialize memory statistics variables
unsigned long total_rss = 0;
unsigned long total_swap = 0;
unsigned long total_wss = 0;




static void parse_vma(void)
{
    struct task_struct *task = NULL;
    struct vm_area_struct *vma = NULL;
    struct mm_struct *mm = NULL;
    

    if(pid > 0){
        task = pid_task(find_vpid(pid), PIDTYPE_PID);
        if(task && task->mm){
            mm = task->mm;
            // TODO 2: mm_struct to initialize the VMA_ITERATOR (-- Assignment 4)
            // Hint: Only one line of code is needed
            VMA_ITERATOR(vmi, mm, 0);

            // TODO 3: Iterate through the VMA linked list with for_each_vma (-- Assignment 4)
            // Hint: Only one line of code is needed
            for_each_vma(vmi, vma) {
            	unsigned long page;
            	for (page = vma->vm_start; page < vma->vm_end; page += PAGE_SIZE) {
            		pgd_t *pgd;
            		p4d_t *p4d;
            		pud_t *pud;
            		pmd_t *pmd;
            		pte_t *pte;
            		
            		pgd = pgd_offset(mm,page);
            		p4d = p4d_offset(pgd, page);
            		pud = pud_offset(p4d, page);
            		pmd = pmd_offset(pud, page);
            		pte = pte_offset_map(pmd, page);
            		
            		if (!pte_none(*pte)) {
            			if (pte_present(*pte)) {
            				if (pte_young(*pte)) {
            					total_wss++;
            					test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *)pte);
            				}
            			total_rss++;
            			} else {
            			total_swap++;
            			}
            		}
            		pte_unmap(pte);
            	}
            }
        }
   }

}

unsigned long timer_interval_ns = 10e9; // 10 sec timer
static struct hrtimer hr_timer;

enum hrtimer_restart timer_callback( struct hrtimer *timer_for_restart )
{
    ktime_t currtime , interval;
    currtime  = ktime_get();
    interval = ktime_set(0,timer_interval_ns);
    hrtimer_forward(timer_for_restart, currtime , interval);
    total_rss = 0;
    total_swap = 0;
    total_wss = 0;
    parse_vma();
    printk("[PID-%i]:[RSS:%lu KB] [Swap:%lu KB] [WSS:%lu KB]\n", pid, total_rss*4, total_swap*4, total_wss*4);
    return HRTIMER_RESTART;
}


int __init memory_init(void){
    printk("CSE330 Project 3 Kernel Module Inserted\n");
    ktime_t ktime;
    ktime = ktime_set( 0, timer_interval_ns );
    hrtimer_init( &hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
    hr_timer.function = &timer_callback;
    hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );
    return 0;
}


void __exit memory_cleanup(void){
    int ret;
    ret = hrtimer_cancel( &hr_timer );
    if (ret) printk("HR Timer cancelled ...\n");
    printk("CSE330 Project 3 Kernel Module Removed\n");
}


module_init(memory_init);
module_exit(memory_cleanup);
MODULE_LICENSE("GPL");
