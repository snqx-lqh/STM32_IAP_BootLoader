/**
  ******************************************************************************
  * @file    MiniCli.c 
  * @author  少年潜行(snqx-lgh)
  * @version V 1.0
  * @date    2025/11/15
  * @brief   一个简单的用于串口调试的miniShell，仅支持基本的操作功能。
  *          删除插入，历史数据，完成用户处理部分的代码输入，就可以使用了。
  *          在循环中，调用函数 shell_task() 即可，这是一个死循环
  ******************************************************************************
  * @attention
  *
  *
  * <h2><center>&copy; Copyright {Year} LQH,China</center></h2>
  ******************************************************************************
  */
#include "MiniCli.h"

/*------------------- 用户处理部分 start -------------------*/
#include "usart.h"
//#include "CommandTask/CommandTask.h"

/**
  * @brief   读取一个字节
  * @param   c 读取到的字节
  * @retval  0 成功 -1 失败
 **/
static int8_t cli_read_byte(char *c)
{
	// 非阻塞模式
	while( __HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) == RESET);
	*c = (uint8_t)(huart1.Instance->DR & 0xFF);
    return 0;
}

/**
  * @brief   写入一个字节
  * @param   c 写入的字节
  * @retval  void
 **/
static void cli_write_byte(const char c)
{
	HAL_UART_Transmit(&huart1, &c, 1, 10);
}

/**
  * @brief   打印所有命令
  * @param   argc: 参数个数
  * @param   argv: 参数
  * @retval  void
 **/
void cmd_help(int argc, char **argv);

/**
  * @brief   指令操作表 内容分别是 命令名、对应操作函数指针、简易说明
  * @retval  void
 **/
const shell_cmd_t cmd_table[] = {
    {"help",   cmd_help,   "show all cmd"},
//    {"burn",   Do_burn ,   "burn code to flash"},
//    {"flash",  Do_flash,   "flash write and read"},
//    {"loadx",  Do_loadx,   "load code to DDR"},
//    {"go",     Do_go,      "run Addr code"},
//    {"led",     Do_led,    "led on/off"},
    
};

/*------------------- 用户处理部分 end -------------------*/

/**
  * @brief    串口写字符串
  * @param    str 串口写的字符串
  * @retval   void
 **/
void cli_write_str(const char * const str)
{
	char *p_temp = (char *)str;

	while(*p_temp)
		cli_write_byte(*p_temp++);
}

void mini_cli_run_cmd(char *cmdline);

#define CMD_COUNT (sizeof(cmd_table) / sizeof(cmd_table[0]))

static char history[HIST_MAX][SHELL_BUF_SIZE]; // 历史命令
static int  hist_count = 0;                    // 已存历史条数
static int  hist_index = -1;                   // 当前查看的位置（-1 表示不在历史中）
static char cmd_buf[SHELL_BUF_SIZE];           // 指令缓冲
static int  index  = 0;                        // 实际字符数
static int  cursor = 0;                        // 光标位置

/**
  * @brief    历史指令更新方式
  * @param    buf 需要显示的历史字符
  * @retval   void
 **/
void history_redraw_line(const char *buf)
{
    int buf_num = strlen(buf);
    
    // 移动光标到正确位置
    for(int i = 0;i < cursor;i++)
        cli_write_byte('\b');
    for(int i = 0;i < index;i++)
        cli_write_byte(' ');
    for(int i = 0;i < index;i++)
        cli_write_byte('\b');
    cli_write_str(buf);
    index = cursor = strlen(buf); 
}

/**
  * @brief    命令行循环
  * @retval   void
 **/
void mini_cli_loop(void)
{
    char c;

    cli_write_str(USER_NAME);
    while (1)
    {
        if (cli_read_byte(&c) == 0)
        {
            if (c == 0x1B)     // ESC
            {
                char c1, c2;
                if (cli_read_byte(&c1) == 0 && cli_read_byte(&c2) == 0)
                {
                    if (c1 == '[')
                    {
                        // ↑ 历史上一条
                        if (c2 == 'A')
                        {
                            if (hist_count > 0)
                            {
                                if (hist_index < hist_count - 1)
                                    hist_index++;

                                strcpy(cmd_buf, history[hist_count - 1 - hist_index]);
                                history_redraw_line(cmd_buf);
                            }
                        }
                        // ↓ 历史下一条
                        else if (c2 == 'B')
                        {
                            if (hist_index > 0)
                            {
                                hist_index--;
                                strcpy(cmd_buf, history[hist_count - 1 - hist_index]);
                            }
                            else
                            {
                                hist_index = -1;
                                cmd_buf[0] = '\0';
                            }

                            history_redraw_line(cmd_buf);
                        }
                        // → 光标右移
                        else if (c2 == 'C')
                        {
                            if (cursor < index)
                            {
                                cli_write_byte(cmd_buf[cursor]);
                                cursor++;
                            }
                        }
                        // ← 光标左移
                        else if (c2 == 'D')
                        {
                            if (cursor > 0)
                            {
                                cli_write_byte('\b');
                                cursor--;
                            }
                        }
                    }
                }
                continue;
            }
            
            if (c == '\r' || c == '\n')
            {
                cli_write_str("\r\n");
                cmd_buf[index] = '\0';

                if (index > 0)
                {
                    // 保存历史
                    if (hist_count < HIST_MAX)
                        strcpy(history[hist_count++], cmd_buf);
                    else
                    {
                        for (int i = 1; i < HIST_MAX; i++)
                            strcpy(history[i - 1], history[i]);
                        strcpy(history[HIST_MAX - 1], cmd_buf);
                    }

                    hist_index = -1;

                    mini_cli_run_cmd(cmd_buf);
                }

                index = cursor = 0;
                cmd_buf[0] = '\0';

                cli_write_str(USER_NAME);
                continue;
            }
            
            if (c == '\b')
            {
                // 左侧往前删
                if(cursor <= 0)
                    continue;
                for (int i = cursor - 1; i < index - 1; i++)
                    cmd_buf[i] = cmd_buf[i + 1];
                index--;
                cursor--;
                cmd_buf[index] = '\0';
                
                cli_write_byte('\b');
                cli_write_str(&cmd_buf[cursor]);
                cli_write_byte(' ');

                // 移动光标到正确位置
                int pos = strlen(cmd_buf) - cursor + 1;
                while (pos--)
                    cli_write_byte('\b');
                continue;
            }

            // ------------------------------
            // 普通字符插入
            // ------------------------------
            if (index < SHELL_BUF_SIZE - 1 )
            {
                if (cursor < index)
                {
                    for (int i = index; i > cursor; i--)
                        cmd_buf[i] = cmd_buf[i - 1];
                    cmd_buf[cursor] = c;
                }
                else
                    cmd_buf[index] = c;

                index++;
                cursor++;

                cmd_buf[index] = '\0';
                cli_write_str(&cmd_buf[cursor-1]);

                // 移动光标到正确位置
                int pos = strlen(cmd_buf) - cursor;
                while (pos--)
                    cli_write_byte('\b');
            }
        }
    }
}

void mini_cli_run_cmd(char *cmdline)
{
    char *argv[8] = {0};
    memset(argv, 0, sizeof(argv));
    int argc = 0;
      
    char *token = strtok(cmdline, " ");
 
    while (token && argc < 8)
    {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    } 
    if (argc == 0)
        return; 

    for (int i = 0; i < CMD_COUNT; i++)
    {
        if (strcmp(argv[0], cmd_table[i].name) == 0)
        {
            cmd_table[i].func(argc, argv);
            return;
        }
    }
    printf("unknow cmd: %s\r\n", argv[0]);
}


void cmd_help(int argc, char **argv)
{
    printf("right cmd:\r\n");
    for (int i = 0; i < CMD_COUNT; i++)
    {
        printf("  %s : %s\r\n", cmd_table[i].name, cmd_table[i].help);
    }
}
