#include "monitor/expr.h"
#include "monitor/watchpoint.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};

// TODO: try to re-organize, you can abort head and free_ pointer while just use static int index
static WP *head, *free_ = NULL;

void init_wp_pool()
{
    int i;
    for (i = 0; i < NR_WP; i++)
    {
        wp_pool[i].NO = i + 1;
        wp_pool[i].next = &wp_pool[i + 1];
    }
    wp_pool[NR_WP - 1].next = NULL;
    head = NULL;
    free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP *new_wp(char *s)
{
    if (s == NULL)
    {
        printf("no expressions");
        return NULL;
    }
    if (head == NULL)
    {
        head = free_;
        free_ = free_->next;
        head->next = NULL;
        strcpy(head->msg, s);
        bool success;
        uint32_t result = expr(s, &success);
        if (!success)
        {
            printf("invalid expr");
            return NULL;
        }
        head->val = result;
        return head;
    }
    else
    {
        if (free_ == NULL)
        {
            printf("no available watchpointer");
            return NULL;
        }
        else
        {
            //cut in from head
            WP *p = free_;
            free_ = free_->next;
            p->next = head;
            head = p;
            strcpy(head->msg, s);
            bool success;
            uint32_t result = expr(s, &success);
            if (!success)
            {
                printf("invalid expr");
                return NULL;
            }
            head->val = result;
            return head;
        }
    }
    //return free_;
}

bool free_wp(int no)
{
    WP *p, *pre;
    p = head, pre = p;
    if (head == NULL)
    {
        printf("no watchpointer");
        return false;
    }
    if (head->NO == no)
    {
        head = head->next;
        p->next = free_;
        free_ = p;
        printf("watchpoint %d is deleted", p->NO);
        return true;
    }
    else
    {
        while (p != NULL && p->NO != no)
        {
            pre = p;
            p = p->next;
        }
        if (p->NO == no)
        {
            pre->next = p->next;
            p->next = free_;
            free_ = p;
            printf("watchpoint %d is deleted", p->NO);
            return true;
        }
        else
        {
            printf("no such NO match watchpointer");
            return false;
        }
    }
    printf("haven't do anything");
    return false;
}

void print_wp()
{
    if (head == NULL)
    {
        printf("No watchpoints!\n");
        return;
    }
    printf("NUM\tVALUE\tWHAT\n");
    for (WP *p = head; p != NULL; p = p->next)
    {
        printf("%d\t%u\t%s\n", p->NO, p->val, p->msg);
    }
    return;
}

void delete_all_wp()
{
    if (head == NULL)
    {
        printf("No watchpoints!\n");
        return;
    }
    for (WP *p = head; p != NULL; p = p->next)
    {
        p->next = free_;
        free_ = p;
        printf("delete: %d\t%u\t%s\n", p->NO, p->val, p->msg);
    }
    return;
}

int checkpoint()
{
    WP *p = head;
    int flg = 0;
    uint32_t result;
    if (p != NULL)
    {
        for (; p != NULL; p = p->next)
        {
            bool success;
            result = expr(p->msg, &success);
            if (result != p->val)
            {
                printf("change happen at %d, expr:%s\n", p->NO, p->msg);
                printf("old value is:0x%x\n", p->val);
                printf("new value is:0x%x\n", result);
                p->val = result;
                flg = 1;
            }
        }
    }
    return flg;
}
