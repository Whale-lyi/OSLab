
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80
#define TAB_WIDTH			4

#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */
#define TAB_CHAR_COLOR 		0x2 	/*绿色，随便拿个用不到的颜色*/
#define ENTER_CHAR_COLOR 	0x1 	/*蓝色*/

// 新增，记录光标曾在位置
typedef struct cursor_pos_stack 
{
	int ptr; //offset
	int pos[SCREEN_SIZE];
	int search_start_ptr; // ESC模式开始时候的ptr位置
}POS_STACK;


/* CONSOLE */
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;				/* 当前光标位置 */
	unsigned int 	search_start_pos;	/*新增：ESC模式开始时候的cursor位置*/
	POS_STACK		pos_stack;
}CONSOLE;

#endif /* _ORANGES_CONSOLE_H_ */
