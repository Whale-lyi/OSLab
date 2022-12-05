# Lab3

## 编写 OS 层次的 IO 程序

### 1. 功能要求

#### 1.1 基本功能

1. 从屏幕左上角开始，以白色显示键盘输入的字符。可以输入并显示 a-z,A-Z 和 0-9 字符。
2. 大小写切换包括 Shift 组合键以及大写锁定两种方式。大写锁定后再用 Shift 组合键将会输入小写字母
3. 支持回车键换行。
4. 支持用退格键删除输入内容。
5. 支持空格键和 Tab 键（4 个空格，可以被统一的删除）
6. 每隔 20 秒左右, 清空屏幕。输入的字符重新从屏幕左上角开始显示。
7. 要求有光标显示, 闪烁与否均可, 但⼀定要跟随输入字符的位置变化。
8. 不要求支持屏幕滚动翻页，但输入字符数不应有上限。
9. 不要求支持方向键移动光标。

#### 1.2 查找功能

1. 按 Esc 键进入查找模式，在查找模式中不会清空屏幕。
2. 查找模式输入关键字，被输入的关键字以红色显示
3. 按回⻋后，所有匹配的文本 (区分大小写) 以红色显示，并屏蔽除 Esc 之外任何输入。
4. 再按 Esc 键，之前输⼊的关键字被自动删除，所有文本恢复白颜色, 光标回到正确位置。参见示例。

#### 1.3 附加功能

按下 control + z 组合键可以撤回操作（包含回车和 Tab 和删除），直到初始状态。

### 2. 实现

基础代码使用 Oranges/chapter7/n，已经实现基本功能 2 3 4 7 8 9

#### 2.1 从屏幕左上角开始

- 基本功能 1

- 在`main.c`中加入以下方法

  ```c
  PUBLIC int kernel_main() {
  	...
  	init_clock();
    init_keyboard();
  
  	cleanScreen(); // 清屏
  
  	restart();
  	while(1){}
  }
  
  // 清屏，将显存指针disp_pos指向第一个位置
  PUBLIC void cleanScreen() {
  	disp_pos = 0;
  	for (int i = 0; i < SCREEN_SIZE; i++) {
  		disp_str(" ");
  	}
  	disp_pos = 0;
  }
  ```


#### 2.2 Tab

##### 2.2.1 支持输入

- 在 `tty.c` 的 `in_process` 方法中加入TAB判断

  ```c
  ...
  		case TAB:
  			put_key(p_tty, '\t');
  			break;
  ...
  ```

- 在`console.h`中定义TAB的宽度和颜色

  ```c
  #define TAB_WIDTH 			4
  #define TAB_CHAR_COLOR 		0x2 	/*绿色，随便拿个用不到的颜色*/
  ```

- 在输出的时候判断`\t`，对`console.c`的`out_char()`方法进行修改

  ```c
  case '\t': // TAB输出, 将光标往后移动TAB_WIDTH(4)
      if(p_con->cursor < p_con->original_addr + 
          p_con->v_mem_limit - TAB_WIDTH) {
          for (int i = 0; i < TAB_WIDTH; i++) {
              // 用空格填充
              *p_vmem++ = ' ';
              *p_vmem++ = TAB_CHAR_COLOR; // tab空格的颜色是特殊的
          }
          p_con->cursor += TAB_WIDTH; // 调整光标
      }
      break;
  ```

- 至此已经可以实现TAB的输入了，但是关于删除还没解决，这时候删除一次还只能删除一个空格，TAB要删除四次，并且换行的删除会回到上一行的末尾

##### 2.2.2 支持删除

- 使用栈来记录光标每个走过的位置

- `console.h` 中定义栈

  ```c
  // 新增，记录光标曾在位置
  typedef struct cursor_pos_stack 
  {
  	int ptr; //offset
  	int pos[SCREEN_SIZE];
  }POS_STACK;
  
  
  /* CONSOLE */
  typedef struct s_console
  {
  	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
  	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
  	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
  	unsigned int	cursor;				/* 当前光标位置 */
  	POS_STACK		pos_stack;
  }CONSOLE;
  ```

- 在`console.c/init_screen`中初始化指针

  ```c
  // 初始化 pos_stack 的 ptr 指针
  p_tty->p_console->pos_stack.ptr = 0;
  ```

- `console.c`新增方法来记录和获取光标位置

  ```c
  PRIVATE void push_pos(CONSOLE* p_con, int pos);
  PRIVATE int pop_pos(CONSOLE* p_con);
  
  // 新增，用于记录/获取光标走过的位置
  PRIVATE void push_pos(CONSOLE* p_con, int pos) {
  	p_con->pos_stack.pos[p_con->pos_stack.ptr++] = pos;
  }
  PRIVATE int pop_pos(CONSOLE* p_con) {
  	if(p_con->pos_stack.ptr == 0){
  		return 0; // 不会发生这种情况
  	}else{
  		p_con->pos_stack.ptr--;
  		return p_con->pos_stack.pos[p_con->pos_stack.ptr];
  	}
  }
  ```

- 修改输出 `console.c/out_char()`

  ```c
  switch(ch) {
  	case '\n':
  		if (p_con->cursor < p_con->original_addr +
  		    p_con->v_mem_limit - SCREEN_WIDTH) {
  			push_pos(p_con, p_con->cursor);
  			p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
  				((p_con->cursor - p_con->original_addr) /
  				 SCREEN_WIDTH + 1);
  		}
  		break;
  
  	case '\b':
  		if (p_con->cursor > p_con->original_addr) {
  			// p_con->cursor--;
  			if(mode == 0) {
  				int temp = p_con->cursor; // 当前位置
  				p_con->cursor = pop_pos(p_con); // 光标回到上次的位置
  				for (int i = 0; i < temp - p_con->cursor; i++) {
  					// 两次位置之间用空格填充
  					*(p_vmem- 2 - 2 * i) = ' ';
  					*(p_vmem- 1 - 2 * i) = DEFAULT_CHAR_COLOR;
  				}
  			} else if(mode == 1){
  				if(p_con->cursor != p_con->search_start_pos) {
  					int temp = p_con->cursor; // 当前位置
  					p_con->cursor = pop_pos(p_con); // 光标回到上次的位置
  					for (int i = 0; i < temp - p_con->cursor; i++) {
  						// 两次位置之间用空格填充
  						*(p_vmem- 2 - 2 * i) = ' ';
  						*(p_vmem- 1 - 2 * i) = DEFAULT_CHAR_COLOR;
  					}
  				}
  			}
  		}
  		break;
  
  	case '\t': // TAB输出, 将光标往后移动TAB_WIDTH(4)
  		if(p_con->cursor < p_con->original_addr + 
  			p_con->v_mem_limit - TAB_WIDTH) {
  			for (int i = 0; i < TAB_WIDTH; i++) {
  				// 用空格填充
  				*p_vmem++ = ' ';
  				*p_vmem++ = TAB_CHAR_COLOR; // tab空格的颜色是特殊的
  			}
  			push_pos(p_con, p_con->cursor);
  			p_con->cursor += TAB_WIDTH; // 调整光标
  		}
  		break;
  
  	default:
  		if (p_con->cursor <
  		    p_con->original_addr + p_con->v_mem_limit - 1) {
  			*p_vmem++ = ch;
  			*p_vmem++ = DEFAULT_CHAR_COLOR;
  			push_pos(p_con, p_con->cursor);
  			p_con->cursor++;
  		}
  		break;
  ```

#### 2.3 每隔 20s 清屏

- 直接清屏光标不会复位，在`tty.c`添加初始化屏幕方法

  ```c
  // 新增，初始化所有tty的screen
  PUBLIC void init_all_screen() {
  	TTY *p_tty;
  	for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
  		init_screen(p_tty);
  	}
  	select_console(0);
  }
  ```

- 修改 `main.c/TestA()`

  ```c
  void TestA()
  {
  	int i = 0;
  	while (1) {
  		/* disp_str("A."); */
  		cleanScreen();
  		init_all_screen();
  		milli_delay(200000);
  	}
  }
  ```

- 这样运行会因为特权级报错，将TestA的进程从PROCS转为TASKS，修改`global.c, proc.h`

  ```c
  // global.c
  PUBLIC	TASK	task_table[NR_TASKS] = {
  	{task_tty, STACK_SIZE_TTY, "tty"},
  	{TestA, STACK_SIZE_TESTA, "TestA"}}; // 新增，改为TASKS
  
  PUBLIC  TASK    user_proc_table[NR_PROCS] = {
  	// {TestA, STACK_SIZE_TESTA, "TestA"},
  	{TestB, STACK_SIZE_TESTB, "TestB"},
  	{TestC, STACK_SIZE_TESTC, "TestC"}};
  
  //proc.h
  /* Number of tasks & procs */
  // #define NR_TASKS	1
  // #define NR_PROCS	3
  #define NR_TASKS	2
  #define NR_PROCS	2
  ```

#### 2.4 ESC 切换模式并输出红色

- 记录模式状态，在 `global.c, global.h` 中添加声明

  ```c
  //global.c
  /*
   0：正常模式
   1：搜索模式
   2：ESC+ENTER
  */
  PUBLIC	int		mode;
  
  //global.h
  extern int mode;//标识模式
  ```

- 只有 `mode == 0` 才可以清屏，修改 `main.c/TestA()`

  ```c
  void TestA()
  {
  	int i = 0;
  	while (1) {
  		if (mode == 0) {
  			/* disp_str("A."); */
  			cleanScreen();
  			init_all_screen();
  			milli_delay(200000);
  		} else {
  			milli_delay(10);
  		}
  	}
  }
  ```

- 处理输入的 ESC，修改 `tty.c/in_process`

  ```c
  case ESC:
      // 切换模式
      if (mode == 0) {
          mode = 1;
          // TODO: 记录ESC开始时的位置
      } else if (mode == 1 || mode == 2) {
          mode = 0;
          // TODO: 清除内容
      }
      break;
  ```

- 输出红色，修改`console.c/out_char()`

  ```c
  default:
      if (p_con->cursor <
          p_con->original_addr + p_con->v_mem_limit - 1) {
          *p_vmem++ = ch;
          if (mode == 0) {
              *p_vmem++ = DEFAULT_CHAR_COLOR;
          } else {
              *p_vmem++ = RED;
          }
          push_pos(p_con, p_con->cursor);
          p_con->cursor++;
      }
      break;
  ```

#### 2.5 匹配输入内容

- 需要记录按下 ESC 后输入开始的位置

- 输入的字符可以从显存中获取

- 修改`console.h`，增加 ESC 开始时的位置的变量声明

  ```c
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
  ```

- 修改 `tty.c/in_process`，输入ESC时记录开始位置

  ```c
  case ESC:
      // 切换模式
      if (mode == 0) {
          mode = 1;
          // 记录ESC开始时的位置
          p_tty->p_console->search_start_pos = p_tty->p_console->cursor;
          p_tty->p_console->pos_stack.search_start_ptr = p_tty->p_console->pos_stack.ptr;
      } else if (mode == 1 || mode == 2) {
          mode = 0;
          // TODO: 清除内容
      }
      break;
  ```

- 按下 ENTER 后匹配文本并屏蔽除了 ESC 以外的输入

- 屏蔽输入，修改 `tty.c/in_process()`

  ```c
  // ESC+ENTER下不允许输入
  if (mode == 2) {
      if ((key & MASK_RAW) != ESC) {
          return;
      }
  }
  ```

- 修改 `tty.c/in_process()` 中 ENTER 的处理，添加search方法

  ```c
  case ENTER:
      if (mode == 0) {
          put_key(p_tty, '\n');
      } else if (mode == 1) {
          search(p_tty->p_console);
          mode = 2;
      }
      break;
  ```

- `console.c` 中添加 `search` 方法

  ```c
  PUBLIC void search(CONSOLE *p_con);
  
  // 搜索，并将搜索结果标为红色
  PUBLIC void search(CONSOLE *p_con) {
  	int i, j;
  	int begin, end; // 滑动窗口
  	for (i = 0; i < p_con->search_start_pos * 2; i += 2) { // 遍历原始白色输入
  		begin = end = i; // 初始化窗口为0
  		int found = 1; // 是否匹配
  		// 遍历匹配
  		for (j = p_con->search_start_pos * 2; j < p_con->cursor * 2; j += 2) {
  			if (*((u8*)(V_MEM_BASE + end)) == ' ') { // 如果是空格，特殊处理
  				if (*((u8*)(V_MEM_BASE + j)) != ' ') { // 如果输入的不是空格，直接break
  					found = 0;
  					break;
  				}
  				if (*((u8*)(V_MEM_BASE + end + 1)) == TAB_CHAR_COLOR) { // 如果是TAB
  					if (*((u8*)(V_MEM_BASE + j + 1)) == TAB_CHAR_COLOR) {
  						end += 2;
  					} else {
  						found = 0;
  						break;
  					}
  				} else { // 普通空格
  					if (*((u8*)(V_MEM_BASE + j + 1)) == TAB_CHAR_COLOR) {
  						found = 0;
  						break;
  					} else {
  						end += 2;
  					}
  				}
  			} else if (*((u8*)(V_MEM_BASE + end)) == *((u8*)(V_MEM_BASE + j))) {
  				end += 2;
  			} else {
  				found = 0;
  				break;
  			}
  		}
  		// 如果找到，标红
  		if (found == 1) {
  			for (j = begin; j < end; j += 2) {
  				if (*(u8*)(V_MEM_BASE + j + 1) != TAB_CHAR_COLOR) {
					  *(u8*)(V_MEM_BASE + j + 1) = RED;
				  }
  			}
  		}
  	}
  }
  ```

#### 2.6 再按 ESC 清空输入文本，匹配文本变回白色，恢复光标

- `console.c` 中添加 `exit_esc` 方法

  ```c
  PUBLIC void exit_esc(CONSOLE* p_con);
  
  // 退出ESC模式
  PUBLIC void exit_esc(CONSOLE* p_con){
  	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
  	// 清屏，用空格填充
  	// 删除ESC开始后的红色输入
  	for (int i = 0; i < p_con->cursor - p_con->search_start_pos; i++) { 
  		*(p_vmem- 2 -2 * i) = ' ';
  		*(p_vmem- 1 -2 * i) = DEFAULT_CHAR_COLOR;
  	}
  	// 把ESC前的普通输入还原为白色
  	for(int i = 0; i < p_con->search_start_pos * 2; i += 2){ 
  		if (*(u8*)(V_MEM_BASE + i + 1) != TAB_CHAR_COLOR) {
			  *(u8*)(V_MEM_BASE + i + 1) = DEFAULT_CHAR_COLOR;
		  }
  	}
  	// 复位指针
  	p_con->cursor = p_con->search_start_pos;
  	p_con->pos_stack.ptr = p_con->pos_stack.search_start_ptr;
  	// p_con->out_char_stack.ptr = p_con->out_char_stack.search_start_ptr;
  	flush(p_con); // 更新p_con，这个不能漏
  }
  ```

- 在 `tty.c/in_process()` 中调用

  ```c
  case ESC:
      // 切换模式
      if (mode == 0) {
          mode = 1;
          // 记录ESC开始时的位置
          p_tty->p_console->search_start_pos = p_tty->p_console->cursor;
          p_tty->p_console->pos_stack.search_start_ptr = p_tty->p_console->pos_stack.ptr;
      } else if (mode == 1 || mode == 2) {
          mode = 0;
          // 清除内容
          exit_esc(p_tty->p_console);
      }
      break;
  ```





