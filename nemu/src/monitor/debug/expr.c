#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h> // for c languare, regularized expressions
#include <sys/types.h>

uint32_t isa_reg_str2val(const char *, bool *);

enum
{
    TK_NOTYPE = 256,
    TK_EQ = 257,
    /* TODO: Add more token types */
    TK_UNEQ,
    TK_REG,
    TK_TEN,
    TK_SIXTEEN,
    TK_POINTER
};

static struct rule
{
    char *regex;
    int token_type;
    int pr; //优先级
} rules[] = {

    /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

    {" +", TK_NOTYPE, 100}, // spaces
    {"\\*", '*', 5},
    {"\\/", '/', 5},
    {"\\+", '+', 4},
    {"\\-", '-', 4},
    {"==", TK_EQ, 3}, // equal
    {"!=", TK_UNEQ, 3},
    {"&&", '&', 2},
    {"\\|\\|", '|', 1},
    {"!", '!', 6},
    {"\\$[a-zA-Z]+", TK_REG, 100},
    {"0[xX][0-9A-Fa-f]+", TK_SIXTEEN, 100},
    {"[0-9]+", TK_TEN, 100},
    //{"[a-zA-Z]",TK_LETTER}
    {"\\(", '(', 7},
    {"\\)", ')', 7}};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX] = {}; // regex_t store number of regexs

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i++)
    {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED); //re is result
        // regcomp(regex_t *preg, const char * regex, int cflags)，description of function regcomp
        if (ret != 0)
        {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}

typedef struct token
{
    int type;
    char str[32];
    int pr;
} Token;

static Token tokens[1024] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0; // number of regex tokens
    printf("%s", e);

    while (e[position] != '\0')
    {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++)
        {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
            {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);
                position += substr_len;

                /* TODO: Now a new token is recognized with rules[i]. Add codes
		   * to record the token in the array `tokens'. For certain types
		   * of tokens, some extra actions should be performed.
		   */
                if (rules[i].token_type == TK_NOTYPE)
                    continue;
                switch (rules[i].token_type)
                {
                case '+':
                case '-':
                case '*':
                case '/':
                case TK_EQ:
                case TK_UNEQ:
                case '&':
                case '|':
                case '!':
                case TK_REG:
                case TK_SIXTEEN:
                case TK_TEN:
                case '(':
                case ')':
                {
                    tokens[nr_token].type = rules[i].token_type;
                    tokens[nr_token].pr = rules[i].pr;
                    memset(tokens[nr_token].str, 0, sizeof(tokens[nr_token].str));
                    strncpy(tokens[nr_token].str, substr_start, substr_len);
                    nr_token++;
                }
                break;
                default:
                    assert(0);
                }
                break;
            }
        }
        if (i == NR_REGEX)
        {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }
    return true;
}

bool check_parentheses(int p, int q)
{
    if (!((tokens[p].type == '(') && (tokens[q].type == ')')))
        return false;
    int k = 0;
    for (int i = p + 1; i < q; i++)
    {
        if (tokens[i].type == '(')
            k++;
        if (tokens[i].type == ')')
            k--;
        if (k < 0)
            return false;
    }
    if (k == 0)
        return true;
    else
        return false;
}

uint32_t find_main_op(int p, int q) // find the main op of a string
{
    int op = p;
    int pth = 0; // mark the parentheses
    for (int i = p; i < q; i++)
    {
	//printf("now main op is:%d;and pr is:%d",op);        
	if (tokens[i].type == '(')
            pth++;
        else if (tokens[i].type == ')')
            pth--;
        else if (pth == 0)
        {
            if (tokens[i].type == TK_POINTER)
                continue;
            if (tokens[i].type == '!')
                continue;
            if (tokens[i].type == '+')
            {
                if (tokens[op].pr < 4)
                    ;
                else
                    op = i;
            }
            else if (tokens[i].type == '-') //deal with numbers < 0
            {
                if (i == p||tokens[i - 1].type == '+' || tokens[i - 1].type == '-' || tokens[i - 1].type == '*' || tokens[i - 1].type == '/' || tokens[i - 1].type == '&' || tokens[i - 1].type == '|' || tokens[i - 1].type == '!' || tokens[i - 1].type == TK_EQ || tokens[i - 1].type == TK_UNEQ)
                {
                    tokens[i].pr = 6;
                    continue;
                }
                if (tokens[op].pr < 4)
                    ;
                else
                    op = i;
            }
            else if (tokens[i].type == '*' || tokens[i].type == '/')
            {
		//printf("now op.pr is:%d;i.pr is:%d\n",tokens[op].pr,tokens[i].pr);                
		if (tokens[op].pr < 5)
                    ;
                else
                    op = i;
            }
            if (tokens[i].type == TK_EQ || tokens[i].type == TK_UNEQ)
            {
                if (tokens[op].pr < 3)
                    ;
                else
                    op = i;
            }
            if (tokens[i].type == '&')
            {
                if (tokens[op].pr < 2)
                    ;
                else
                    op = i;
            }
            if (tokens[i].type == '|')
            {
                if (tokens[op].pr < 1)
                    ;
                else
                    op = i;
            }
        }
    }
    return op;
}

//uint32_t find_main_op(int,int);

uint32_t eval(int start, int end, bool *success)
{
    //printf("%d %d\n", start, end);
    if (start > end)
    {
        Log("please check you expression\n");
        //##这里展示Log函数的使用方法
        assert(0);
        //##这里展示assert函数的使用方法
        /* Bad expression */
    }
    else if (start == end)
    {
        /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
        int num;
        if (tokens[start].type == TK_SIXTEEN)
            sscanf(tokens[start].str, "%x", &num);
        if (tokens[start].type == TK_TEN)
            sscanf(tokens[start].str, "%d", &num);
        //printf("get number: %d\n",num);
        if (tokens[start].type == TK_REG)
        {
            bool success1;
            uint32_t regn = isa_reg_str2val(tokens[start].str, &success1);
            if (!success1)
            {
                printf("wrong register\n");
                assert(0);
            }
            return regn;
        }
        return num;
    }
    else if (check_parentheses(start, end) == true)
    {
        return eval(start + 1, end - 1, success);
    }
    else
    {
        /* We should do more things here. */
        //类似二分法的写法
        uint32_t op = find_main_op(start, end);
        printf("main op is in:%d\n", op);
        //printf("%s\n", tokens[op].str);
        if (op == start && tokens[op].type == TK_POINTER)
            return paddr_read(eval(op + 1, end, success), 4);
        if (op == start && tokens[op].type == '-')
            return -eval(op + 1, end, success);
        if (op == start && tokens[op].type == '!')
            return !eval(op + 1, end, success);
        uint32_t val1 = eval(start, op - 1, success);
        uint32_t val2 = eval(op + 1, end, success);
        switch (tokens[op].type)
        {
        case '+':
            return val1 + val2;
        case '-':
            return val1 - val2;
        case '*':
            return val1 * val2;
        case '/':
            if (val2 == 0)
            {
                printf("this expr is illegal with /0");
                *success = false;
                return 2147483647;
            }
            return val1 / val2;
        case TK_EQ:
            return val1 == val2;
        case TK_UNEQ:
            return val1 != val2;
        case '&':
            return val1 && val2;
        case '|':
            return val1 || val2;
        default:
            assert(0);
        }
    }
}

void ispointer()
{
    for (int i = 0; i < nr_token; i++)
        if (tokens[i].type == '*' && (i == 0 || tokens[i - 1].type == '+' || tokens[i - 1].type == '-' || tokens[i - 1].type == '*' || tokens[i - 1].type == '/' || tokens[i - 1].type == '&' || tokens[i - 1].type == '|' || tokens[i - 1].type == '!' || tokens[i - 1].type == TK_EQ || tokens[i - 1].type == TK_UNEQ))
        {
            tokens[i].type = TK_POINTER;
            tokens[i].pr = 6;
        }
}

unsigned expr(char *e, bool *success)
{
    if (!make_token(e))
    {
        *success = false;
        //printf("quesion in making tokens");
    }
    ispointer();
    /* TODO: Insert codes to evaluate the expression. */
    int p = 0, q = nr_token - 1;
    //for(int i=0;i<nr_token;i++)printf("%s",tokens[i].str);
    //printf("\n");
    *success = true;
    uint32_t ans = eval(p, q, success);
    //printf("answer is:%d\n", ans);
    if (*success)
        return ans;
    else
        return 0;
}

