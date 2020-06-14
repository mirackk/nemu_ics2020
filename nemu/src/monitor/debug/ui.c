#include "monitor/expr.h"
#include "monitor/monitor.h"
#include "monitor/watchpoint.h"
#include "nemu.h"
#include <dirent.h> // for c language, file directory operations (use 'man opendir' for more information)
#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <unistd.h> // for c language, get work path (use 'man getcwd' for more information)

static int cmd_pwd(char *);
static int cmd_echo(char *); // define functions

void cpu_exec(uint64_t);
void isa_reg_display();
/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
    static char *line_read = NULL;

    if (line_read)
    {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline("(nemu) ");

    if (line_read && *line_read)
    {
        add_history(line_read);
    }

    return line_read;
}

static int cmd_c(char *args)
{
    cpu_exec(-1);
    return 0;
}

static int cmd_q(char *args)
{
    return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args)
{
    int step = 0;
    char *str = strtok(NULL, " ");
    if (str == NULL)
        cpu_exec(1);
    else
    {
        sscanf(str, "%d", &step);
        if (step <= 0)
            cpu_exec(-1);
        else
            cpu_exec(step);
    }
    return 0;
}

static int cmd_info(char *args)
{
    char *str = strtok(NULL, " ");
    if (strcmp(str, "r") == 0)
        isa_reg_display();
    else if (strcmp(str, "pc") == 0)
    {
        int val = paddr_read(cpu.pc, 4);
        printf("pc\taddress:0x%-8x\tvalue:0x%x\n", cpu.pc, val);
    }
    else if (strcmp(str, "w") == 0)
    {
        print_wp();
    }
    return 0;
}

static int cmd_x(char *args)
{
    int n;
    paddr_t start;
    char *str = strtok(NULL, "");
    char *str_1 = strtok(NULL, "");
    sscanf(str, "%d", &n);
    bool success;
    start = expr(str_1, &success);
    for (int i = 1; i <= n; i++)
    {
        printf("0x%08x: 0x%08x\n", start + (i << 2), paddr_read(start + (i << 2), 4));
    }
    return 0;
}

static int cmd_p(char *args)
{
    bool success = true;
    //printf("test");
    uint32_t ans = expr(args, &success);
    if (success)
        printf("answer = %d,0x%x", ans, ans);
    else
        printf("wrong expr in cmd_p,%d", ans);
    return 0;
}

static int cmd_ls(char *args)
{
    char buf[256];
    if (getcwd(buf, 256) != 0)
        ;
    char *path;
    if (!args)
        path = buf;
    else
        path = args;
    DIR *directory_pointer;
    struct dirent *entry;
    if ((directory_pointer = opendir(path)) == NULL)
    {
        printf("Error open\n");
        return 0;
    }
    else
    {
        while ((entry = readdir(directory_pointer)) != NULL)
        {
            if (entry->d_name[0] == '.')
                continue;
            printf("%s\n", entry->d_name);
        }
    }
    return 0;
}

static int cmd_w(char *args)
{
    WP *p=new_wp(args);
    printf("set new watchpoint %d: %s; val is:0x%x\n", p->NO, p->msg, p->val);
    return 0;
}

static int cmd_d(char *args)
{
    if(args==NULL)
    {
        delete_all_wp();
    }
    else
    {
        char *str = strtok(NULL, " ");
        int no;
        sscanf(str,"%d",&no);
        bool success;
        success = free_wp(no);
        if(success)
            printf("successfully delete\n");
        else
            printf("fail delete\n");
    }
    return 0;
}

static struct
{
    char *name;
    char *description;
    int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display informations about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    /* TODO: Add more commands *
   * you should add more commands as described in our manual
   */
    {"si", "stepping N instructions", cmd_si},
    {"info", "printf the info", cmd_info},
    {"p", "calculate the expr", cmd_p},
    {"x", "scan the memory", cmd_x},
    {"ls", "list the file", cmd_ls},
    {"echo", "Print the characters given by user", cmd_echo}, // add by wuran
    {"pwd", "Print current work path", cmd_pwd},              // add by wuran
    {"w", "set new watchpoint", cmd_w},
    {"d", "delete watchpoint", cmd_d},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0])) // number of commands

static int cmd_help(char *args)
{
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    int i;

    if (arg == NULL)
    {
        /* no argument given */
        for (i = 0; i < NR_CMD; i++)
        {
            printf("\033[1m\033[33m [%s]\033[0m - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    }
    else
    {
        for (i = 0; i < NR_CMD; i++)
        {
            if (strcmp(arg, cmd_table[i].name) == 0)
            {
                printf("\033[1m\033[33m [%s]\033[0m - %s\n", cmd_table[i].name, cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

static int cmd_echo(char *args)
{
    // char * arg = strtok(args, " ");
    if (args != NULL)
        printf("%s\n", args);
    else
        printf("\n");
    return 0;
}

static int cmd_pwd(char *args)
{
    char buf[256];
    if (getcwd(buf, 256) != 0)
        printf("current work path: %s\n", buf);
    return 0;
}

void ui_mainloop(int is_batch_mode)
{
    if (is_batch_mode)
    {
        cmd_c(NULL);
        return;
    }

    for (char *str; (str = rl_gets()) != NULL;)
    {
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL)
        {
            continue;
        }

        /* treat the remaining string as the arguments,
     * which may need further parsing
     */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end)
        {
            args = NULL;
        }

#ifdef HAS_IOE
        extern void sdl_clear_event_queue(void);
        sdl_clear_event_queue();
#endif

        int i;
        for (i = 0; i < NR_CMD; i++)
        {
            if (strcmp(cmd, cmd_table[i].name) == 0)
            {
                if (cmd_table[i].handler(args) < 0)
                {
                    return;
                }
                break;
            }
        }

        if (i == NR_CMD)
        {
            printf("Unknown command '%s'\n", cmd);
        }
    }
}
