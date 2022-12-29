
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"


/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	for (int i = 0; i < NR_TASKS; i++) {
		proc_table[i].ticks = 1;
		proc_table[i].priority = 1;
		proc_table[i].wake_tick = 0;
		proc_table[i].status = 2;
		proc_table[i].isBlocked = 0;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	cleanScreen(); // 清屏

	restart();

	while(1){}
}

// 添加清屏, 将显存指针指向第一个位置
PUBLIC void cleanScreen() {
	disp_pos = 0;
	for (int i = 0; i < 80 * 25; i++) {
		disp_str(" ");
	}
	disp_pos = 0;

	// 初始化变量
	readerNum = 0;
	writerNum = 0;
}

void NormalA() {
	milli_delay(200);
	int n = 0;
	while (TRUE) {
		if (n++ < 20) {
			if(n < 10) {
				char tmp[4] = {n + '0', ' ', ' ', '\0'};
				print(tmp);
			} else {
				char tmp[4] = {(n / 10) + '0', (n % 10) + '0', ' ', '\0'};
				print(tmp);
			}
			for (int i = 1; i < NR_TASKS; i++) {
				int status = proc_table[i].status;
				if (status == 0) {
					print("X ");
				} else if (status == 1) {
					print("O ");
				} else if (status == 2) {
					print("Z ");
				}
			}
			print("\n");
			milli_delay(TIME_SLICE);
			// sleep(TIME_SLICE);
		}
	}
}

void ReaderB() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0;
		READER(2);
		p_proc_ready->status = 2;
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE);
	}
}

void ReaderC() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0;
		READER(3);
		p_proc_ready->status = 2;
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE * 1);
	}
}

void ReaderD() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0;
		READER(3);
		p_proc_ready->status = 2;
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE);
	}
}

void WriterE() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0;
		WRITER(3);
		p_proc_ready->status = 2;
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE);
	}
}

void WriterF() {
	// milli_delay(TIME_SLICE);
	while (TRUE) {
		p_proc_ready->status = 0;
		WRITER(4);
		p_proc_ready->status = 2;
		// sleep(TIME_SLICE);
		milli_delay(TIME_SLICE);
	}
}
