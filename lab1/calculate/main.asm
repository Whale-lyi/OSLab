%define MAX_L   22  ; 数字最长21位, 再加上符号
%define MUL_MAX 42  ; 数字最长41位, 再加上符号
%define INF     46  ; 22*2 + 操作号 + 换行
%define LF      10
%define SPACE   32
%define P       43
%define N       45
%define ZERO    48
%define QUIT    113

section .data
    input_msg:  db  "Please input the expression: ", 0h
    error_msg:  db  "Invalid Operator!", 0h

section .bss
    input_exp:  resb    INF
    first_num:  resb    MAX_L
    second_num: resb    MAX_L
    first_len:  resb    1
    second_len: resb    1
    operator:   resb    1
    add_res:    resb    MAX_L
    mul_res:    resb    MUL_MAX
    mul_res_head:   resd    1
    mul_res_tail:   resd    1
    

section .text
global main
main:
    mov     ebp, esp    ; for correct debugging
    mov     eax, input_msg
    call    print_string    ; 打印提示输入信息
    call    print_lf      ; 打印换行
    call    read_exp    ; 读取输入的表达式
    
    mov     ecx, input_exp

    cmp     byte[ecx], QUIT
    jz      quit

    mov     ebx, operator   ; 操作符
    mov     eax, first_num
    call    get_number
    mov     eax, second_num
    call    get_number

    cmp     byte[operator], 43   ; +
    jz      add_or_sub
    cmp     byte[operator], 42   ; *
    jz      do_mul

    finish_routine:
    call    print_lf
    jmp     main

add_or_sub:
    mov     al, byte[first_num]
    add     al, byte[second_num]
    cmp     al, 88  ;异号
    jz      do_sub
do_add:
    mov     dh, byte[first_len]
    mov     dl, byte[second_len]
    mov     esi, first_num
    mov     edi, second_num
    mov     eax, add_res
    mov     bh, 0   ; bh作为进位标识符
    cmp     dh, dl
    jnb      .add_loop     ; op1 >= op2  即l1 >= l2时跳转, 保证num1最长
    mov     esi, second_num     ; switch
    mov     edi, first_num
    mov     dl, byte[first_len]
    mov     dh, byte[second_len]
    .add_loop:
        cmp     dh, 1   ; while(l1 > 1)
        jna     .check_cf   ; op1 <= op2    l1 <= 1时跳转
        ; 先计算短的数这次要加多少,如果已经到数字开头的符号则将temp, 即要加的数置0
        movzx   ecx, dl
        add     ecx, edi
        sub     ecx, 1
        mov     bl, byte[ecx]      ; char temp = *(num2 + l2 - 1), 获取num2最低位
        
        cmp     bl, byte[second_num]    ; if(temp == *num2) 判断短的数是否已经到最开始的符号
        jz      .set_zero   ; temp = 48 (zero)
        jnz     .sub_length   ; else l2--
            .set_zero:
                mov     bl, ZERO    ; 已经到符号位了, 把这次要加的数设为48, 即字符0
                jmp     .add_start
            .sub_length:
                sub     dl, 1   ;没到符号位, l2--，往前移一位
        .add_start:
        movzx   ecx, dh
        add     ecx, esi
        sub     ecx, 1
        add     bl, byte[ecx]      ; temp += *(num1 + l1 - 1)，即num2这次要加的位再加上num1这次要加的位
        mov     byte[eax], bl   ; *res = temp，将该位的加完结果存到eax指向的地方，即add_res
        ; 判断上次加法是否有进位
        cmp     bh, 1
        jnz     .if_have_cf     ; 没有进位, 跳转到判断这次加法是否有进位
        add     byte[eax], 1    ; 有进位,结果+1
        .if_have_cf:
        cmp     byte[eax], 106      ; 判断加后是否大于9
        jnb     .have_cf            ; 有进位, 减去ascii中的'10'
        jb      .not_have_cf        ; 没有, 减去ascii中的'0'
            .have_cf:
                mov     bh, 1       ; 进位设为1
                sub     byte[eax], 58   ; 结果减去减去ascii中的'10'
                jmp     .change_condition
            .not_have_cf:
                mov     bh, 0       ; 进位清空
                sub     byte[eax], 48   ; 减去ascii中的'0'
        .change_condition:  ; 改变循环的条件
        add     eax, 1  ; res++, 存放结果的地址+1, 下次循环存到下一位
        sub     dh, 1   ; l1--
        jmp     .add_loop
    .check_cf:  ; 判断最后有没有进位
        cmp     bh, 1
        jne     .add_symbol
        mov     byte[eax], 49   ; 字符1
        add     eax, 1  ; res++
    .add_symbol:    ; 添加结果的符号
        cmp     byte[esi], N    ; 和负号比较
        jne     .sub_res
        mov     byte[eax], N    ; 添加负号
        call    print_add_res
        .sub_res:
            sub     eax, 1  ; res--
    call    print_add_res

print_add_res:
    cmp     eax, add_res    ; while(res >= res1)
    jb      finish_routine
    push    eax
    mov     cl, byte[eax]   ; 将字符本身移到eax
    movzx   eax, cl
    call    print_char  ; 字符在eax中
    pop     eax
    sub     eax, 1  ; res--
    jmp     print_add_res
        

do_mul:
    mov     dh, byte[first_len]
    mov     dl, byte[second_len]
    mov     esi, first_num
    mov     edi, second_num

    cmp     byte[esi+1], ZERO
    jz      print_zero
    cmp     byte[edi+1], ZERO
    jz      print_zero

    mov     ecx, mul_res
    call    mem_set     ; 将存放乘法结果的空间清空
    mov     dword[mul_res_tail], ecx    ;记录每轮写入乘法结果的位置的指针
    cmp     dh, dl
    jnb     .mul_loop     ; op1 < op2  l1 < l2, 保证num1最长
    mov     esi, second_num     ; switch
    mov     edi, first_num
    mov     dl, byte[first_len]
    mov     dh, byte[second_len]
    mov     byte[second_len], dl
    mov     byte[first_len], dh
    .mul_loop:
        cmp     dl, 1
        jna     .print_symbol   ; while(l2 > 1)
        mov     ecx, dword[mul_res_tail]     ; 更新写入mul_res的位置
        .next_loop:
            cmp     dh, 1
            jna     .update_param   ; while(l1 > 1)，不符合直接跳转到更新外层循环条件的地方

            push    esi     ; 保存用到的寄存器数据
            push    ecx
            movzx   ecx, dh
            add     esi, ecx
            sub     esi, 1
            mov     al, byte[esi]
            sub     al, ZERO    ; char n1 = *(num1 + l1 - 1) - '0'，获取num1这次要计算的位，注意以纯数字存储
            pop     ecx
            pop     esi

            push    edi     ; 保存用到的寄存器数据
            push    ecx
            movzx   ecx, dl
            add     edi, ecx
            sub     edi, 1
            mov     bl, byte[edi]
            sub     bl, ZERO    ; char n2 = *(num2 + l2 - 1)-'0'，获取num2这次要计算的位，注意以纯数字存储
            pop     ecx
            pop     edi

            mul     bl      ; n1*n2, 结果存放在ax
            add     byte[ecx], al   ; *res += n1 * n2; 因为上一轮乘法可能会有进位，所以用加法

            cmp     byte[ecx], 9    ; 判断是否有进位
            ja      .overflow   ; if(*res > 9) 有进位
            jna     .not_overflow
                .overflow:
                    movzx   ax, byte[ecx]
                    mov     bl, 10
                    div     bl      ; *res / 10, 商保存在al, 余数保存在ah
                    mov     byte[ecx], ah   ; 余数存储在结果中
                    add     ecx, 1  ; 结果地址进一位
                    add     byte[ecx], al   ; 加上进位
                    mov     dword[mul_res_head], ecx    ; res_head = res，用来记录乘法结果的头在哪
                    jmp     .update_len
                .not_overflow:
                    mov     dword[mul_res_head], ecx    ; res_head = res
                    add     ecx, 1
            .update_len:
                sub     dh, 1   ; l1--
            jmp     .next_loop
        .update_param:  ; num2的一位已经乘完整个num1
            sub     dl, 1   ; l2--
            add     dword[mul_res_tail], 1      ; res_p++，每轮乘法存储结果的位置都要往前进一位
            mov     dh, byte[first_len]     ; 重新给l1赋值
        jmp     .mul_loop
    .print_symbol:
        mov     dh, byte[first_num]
        add     dh, byte[second_num]
        cmp     dh, 88  ; 43+45 异号
        jne     .print_mul_res
        mov     eax, N
        call    print_char
    .print_mul_res:
        mov     edx, mul_res
        mov     ecx, dword[mul_res_head]    ; 乘法结果的头
        .loop_mul_print:
        cmp     ecx, edx    ; while(res_head >= res1)
        jb      finish_routine

        movzx   eax, byte[ecx]
        add     eax, ZERO   ; 加上字符0
        call    print_char  ; 字符在eax中

        sub     ecx, 1  ; res_head--
        jmp     .loop_mul_print

do_sub:    
    jmp     finish_routine


mem_set:    ;将乘法结果清空
    mov     eax, mul_res
    mov     bl, MUL_MAX
    .mem_set_loop:
        cmp     bl, 1
        jb      .men_set_ret
        mov     byte[eax], 0
        add     eax, 1
        sub     bl, 1
        jmp     .mem_set_loop
    .men_set_ret:
        ret

print_string:      ; 字符串地址保存在eax
    ; 保存寄存器数据
    push    edx
    push    ecx
    push    ebx
    push    eax

    mov     ecx, eax    ; 原字符串
    call    strlen
    mov     edx, eax    ; 字符串长度
    mov     eax, 4      ; write
    mov     ebx, 1      ; std_out
    int     80h

    pop     eax
    pop     ebx
    pop     ecx
    pop     edx
    ret
    
print_char:      ; 字符保存在eax
    ; 保存寄存器数据
    push    edx
    push    ecx
    push    ebx
    push    eax
  
    mov     eax, 4      ; write
    mov     ebx, 1      ; std_out
    mov     ecx, esp    ; 字符指针
    mov     edx, 1    ; 字符长度
    int     80h 
    
    pop     eax
    pop     ebx
    pop     ecx
    pop     edx
    ret

print_lf:
    push    eax
    mov     eax, LF
    call    print_char
    pop     eax
    ret

print_zero:
    push    eax
    mov     eax, ZERO
    call    print_char
    pop     eax
    jmp     finish_routine

strlen:     ; 初始字符串存储在eax, 结果长度也存储在eax
    push    ebx
    mov     ebx, eax
    .nextchar:
        cmp     byte[eax], 0
        jz    	.finished
        inc     eax
        jmp     .nextchar
    .finished:
        sub     eax, ebx
        pop     ebx
        ret

read_exp:
    push    edx
    push    ecx
    push    ebx
    push    eax

    mov     eax, 3
    mov     ebx, 0
    mov     ecx, input_exp
    mov     edx, INF
    int     80h  
    
    pop     eax
    pop     ebx
    pop     ecx
    pop     edx
        
    ret

get_number:      ;eax负责存放解析的数字, ebx存放操作符, ecx为表达式地址
    cmp     byte[ecx], N    ; 判断是否是负数
    jz      .next_char
    mov     byte[eax], P    ; 正数添加正号
    inc     eax
    .next_char:
        cmp     byte[ecx], 43   ; +
        jz      .add_finish
        cmp     byte[ecx], 42   ; *
        jz      .mul_finish
        cmp     byte[ecx], 47   ; /
        jz      .error_finish
        cmp     byte[ecx], 10   ; \n
        jz      .lf_finish
        mov     dl, byte[ecx]
        mov     byte[eax], dl
        inc     eax
        inc     ecx
        jmp     .next_char
    .mul_finish:
        inc     ecx
        mov     byte[ebx], 42
        mov     byte[eax], 0h   ; 数字以\0结尾
        mov     eax, first_num  ; 计算长度
        call    strlen
        mov     ebx, first_len
        mov     byte[ebx], al

        ret
    .add_finish:
        inc     ecx
        mov     byte[ebx], 43
        mov     byte[eax], 0h   ; 数字以\0结尾
        mov     eax, first_num  ; 计算长度
        call    strlen
        mov     ebx, first_len
        mov     byte[ebx], al

        ret
    .lf_finish:
        mov     byte[eax], 0h   ; 数字以\0结尾
        mov     eax, second_num  ; 计算长度
        call    strlen
        mov     ebx, second_len
        mov     byte[ebx], al

        ret
    .error_finish:
        mov     eax, error_msg
        call    print_string
        jmp     finish_routine

quit:
	mov 	eax, 1
	mov		ebx, 0
	int		80h

