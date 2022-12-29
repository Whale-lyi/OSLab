# Lab1

**环境：Ubuntu 16.04 32位**

## Hello OS

### 任务描述

选择任意平台，参考讲义搭建NASM+Bochs实验平台，在该实验平台上汇编 boot.asm 并用Bochs虚拟机运行，显示“Hello OS world!”。最终运行结果如下图所示：

![image-20221107135713307](https://whale-picture.oss-cn-hangzhou.aliyuncs.com/img/image-20221107135713307.png)

### 使用说明

```bash
# 安装bochs和gui库
$ sudo apt-get install bochs bochs-sdl
# 创建软盘fd
$ bximage
# 安装nasm
$ sudo apt-get install nasm
# 生成boot.bin
$ nasm boot.asm -o boot.bin
# 使用dd命令将boot.bin写入软盘中
$ dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
# 启动bochs
$ bochs –f bochsrc
```

## 整数加法乘法

### 任务描述

使用汇编语言NASM实现大整数的加法和乘法。给出整数 $x$, $y$，计算 $x + y$ 或 $x \times y$ 的值。

- 输⼊格式：`<input_x><operator><input_y>`，两两之间没有空格，`<input_x>`和`<input_y>`分别代表 $x$ 和 $y$，其中$-10^{20} \le x, y \le 10^{20}$；`<operator>`包含加法运算符 + 和乘法运算符 * ，若操作符后是负数，负号直接跟在操作符后；输入 q 结束程序；
- 输出格式：计算的结果；输出结果后继续接受下⼀个输⼊；
- 至少处理1种非法输入情况（如无效操作符，缺少操作数等），保证程序不崩溃。

### 使用说明

注：没有实现正数与负数间的加法

```bash
$ nasm -f elf32 main.asm
$ gcc -o main main.o
$ ./main
```

