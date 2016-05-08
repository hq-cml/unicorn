/**
 *  __    __   __   __   __    ______   ______   .______    __   __  
 * |  |  |  | |  \ |  | |  |  /      | /  __  \  |   _  \  |  \ |  | 
 * |  |  |  | |   \|  | |  | |  ,----'|  |  |  | |  |_)  | |   \|  | 
 * |  |  |  | |  . `  | |  | |  |     |  |  |  | |      /  |  . `  | 
 * |  `--'  | |  |\   | |  | |  `----.|  `--'  | |  |\  \-.|  |\   | 
 *  \______/  |__| \__| |__|  \______| \______/  | _| `.__||__| \__| 
 *
 *     Filename:  Unc_so.h 
 * 
 *  Description:  A dynamic library implemention. 
 * 
 *      Version:  1.0.0
 * 
 *       Author:  HQ 
 *
 **/
#include "unc_core.h"
 
/**
 * ����: ���ݲ���sym���������˳����������غ�������ȫ��so��
 * ����: @phandle����̬������ָ��
 *       @sym��smd_symbol_t����
 *       @filename��so�ļ�λ��
 * ע��:
 *      1. ����sym���飬�ֱ��ʼ��ȫ��so�еĸ�������
 * ����: �ɹ���0 ʧ�ܣ�-x
 **/
int unc_load_so(void **phandle, unc_so_symbol_t *sym, const char *filename) 
{
    char    *error;
    int     i = 0;
    
    *phandle = dlopen(filename, RTLD_NOW);
    if ((error = dlerror()) != NULL) 
    {
        fprintf(stderr, "dlopen so file failed:%s\n", error);
        return UNC_ERR;
    }

    while (sym[i].sym_name) 
    {
        if (sym[i].no_error) 
        {
            //������dlsym�Ƿ�ʧ�ܣ���ʧ����ֻ��ӡ������Ϣ 
            *(void **)(sym[i].sym_ptr) = dlsym(*phandle, sym[i].sym_name);
            dlerror();
        } 
        else 
        {
            //��dlsymʧ�ܣ���رվ�����ͷ���Դ
            *(void **)(sym[i].sym_ptr) = dlsym(*phandle, sym[i].sym_name);
            if ((error = dlerror()) != NULL) 
            { 
                dlclose(*phandle); 
                *phandle = NULL;
                return UNC_ERR;
            }          
        }
        ++i;
    }

    return UNC_OK;
}

/*
 * ����: close ���
 */
void unc_unload_so(void **phandle) 
{
    if (*phandle != NULL) 
    {
        dlclose(*phandle);
        *phandle = NULL;
    }
}

#ifdef __UNC_SO_TEST_MAIN__

//��̬�����ͣ���Ա�Ƕ�̬������ĺ���ԭ��
typedef struct so_func_struct 
{
    int (*handle_init)(const void *data, int proc_type);
    int (*handle_fini)(const void *data, int proc_type);
    int (*handle_task)(const void *data);
} so_func_t;

so_func_t so;

//�붯̬��һһ��Ӧ�ı�����飬������ÿ����̬�⺯���ľ�����Ϣ
unc_so_symbol_t syms[] = 
{
    {"handle_init", (void **)&so.handle_init, 1},
    {"handle_fini", (void **)&so.handle_fini, 1},
    {"handle_task", (void **)&so.handle_task, 0},
    {NULL, NULL, 0}
};

int main(int argc, char *argv[]) 
{
    void * handle;

    if (argc < 2) 
    {
        fprintf(stderr, "Invalid arguments\n");
        exit(1);
    }

    if (unc_load_so(&handle, syms, argv[1]) < 0) 
    {
        fprintf(stderr, "load so file failed\n");
        exit(1);
    }

    if (so.handle_init) 
    {
        so.handle_init("handle_init", 0);
    }

    so.handle_task("handle_task");

    if (so.handle_fini) 
    {
        so.handle_fini("handle_fnit", 1);
    }

    unc_unload_so(&handle);
    exit(0);
}
#endif /* __UNC_SO_TEST_MAIN__ */

