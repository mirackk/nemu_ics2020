#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// this should be enough
static char buf[65536] = "";
int cnt;
int len;

// TODO: implement these functions: choose, gen_rand_op, gen_num, gen_rand_expr
static inline uint32_t choose(uint32_t n)
{
    return rand() % n;
}

static inline uint32_t gen_num()
{
    return (uint32_t)rand() % 10000000;
}

static inline void gen_rand_expr()
{
    if (cnt > 1000 || len > 10000)// control lenth
    {
        return;
    }
    switch (choose(5))
    {
    case 0:
        sprintf(buf + len++, "(");
        cnt++;
        gen_rand_expr();
        sprintf(buf + len++, ")");
        cnt++;
        break;
    case 1:
        {uint32_t num = gen_num();
        //int x = choose(2);
        sprintf(buf + len, "%u", num);
        len = strlen(buf);
        cnt++;}
        break;
    case 2:
        {uint32_t x = choose(10);
        for (int i = 0; i < x; i++)
            sprintf(buf + len++, " ");
        cnt++;
        gen_rand_expr();}
        break;
    case 3:
        gen_rand_expr();
        switch (choose(8))
        {
        case 0:
            sprintf(buf + len++, "+");
            break;
        case 1:
            sprintf(buf + len++, "-");
            break;
        case 2:
            sprintf(buf + len++, "*");
            break;
        case 3:
            sprintf(buf + len++, "/");
            break;
        case 4:
            sprintf(buf + len, "==");
            len += 2;
            break;
        case 5:
            sprintf(buf + len, "!=");
            len += 2;
            break;
        case 6:
            sprintf(buf + len, "&&");
            len += 2;
            break;
        case 7:
            sprintf(buf + len, "||");
            len += 2;
            break;
        }
        cnt++;
        gen_rand_expr();
        break;
    case 4:
        sprintf(buf + len++, "!");
        cnt++;
        gen_rand_expr();
    }
    return;
}
// TODO: if necessary, try to re-implement main function for better generation of expression

static char code_buf[65536];
static char *code_format =
    "#include <stdio.h>\n"
    "int main() { "
    "  unsigned result = %s; "
    "  printf(\"%%u\", result); "
    "  return 0; "
    "}";

int main(int argc, char *argv[])
{
    int seed = time(0);
    srand(seed);
    int loop = 1;
    if (argc > 1)
    {
        sscanf(argv[1], "%d", &loop);
    }
    int i;
    for (i = 0; i < loop; i++)
    {
        len = 0;
        cnt = 0;
        memset(buf, 0, sizeof(buf));
        gen_rand_expr();
	//sprintf(buf + len, "\\0");
        sprintf(code_buf, code_format, buf);

        FILE *fp = fopen("/tmp/.code.c", "w");
        assert(fp != NULL);
        fputs(code_buf, fp);
        fclose(fp);

        int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
        if (ret != 0)
            continue;

        fp = popen("/tmp/.expr", "r");
        assert(fp != NULL);

        int result;
        fscanf(fp, "%d", &result);
        pclose(fp);

        printf("%u %s\n", result, buf);
    }
    return 0;
}

