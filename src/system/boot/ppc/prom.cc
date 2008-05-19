#include "prom.h"

prom_entry prom;
void *prom_chosen;
void *prom_stdout;
void *prom_screen;

void *call_prom(const char *service, int nargs, int nret, void *a1, void *a2, void *a3, void *a4)
{
  prom_args pa;
  pa.service = service;
  
  pa.nargs = nargs;
  pa.nret = nret;
  pa.args[0] = a1;
  pa.args[1] = a2;
  pa.args[2] = a3;
  pa.args[3] = a4;
  for (int i = 4; i < 10; i++)
    pa.args[i] = 0;
  
  prom (&pa);
  if (nret > 0)
    return pa.args[nargs];
  else
    return 0;
}

int prom_get_chosen(char *name, void *mem, int len)
{
  return prom_getprop(prom_chosen, name, mem, len);
}

void prom_init(prom_entry pe)
{
  prom = pe;

  prom_chosen = prom_finddevice("/chosen");
  if (prom_chosen == (void *)-1)
    prom_exit();
  if (prom_get_chosen ("stdout", &prom_stdout, sizeof(prom_stdout)) <= 0)
    prom_exit();
  if (prom_get_chosen ("screen", &prom_screen, sizeof(prom_screen)) <= 0)
    prom_exit();
}

void *prom_finddevice(const char *dev)
{
  return call_prom("finddevice", 1, 1, (void*)dev);
}

int prom_exit()
{
  call_prom("exit", 0, 0);
}

int prom_getprop(void *dev, char *name, void *buf, int len)
{
  return (int)call_prom ("getprop", 4, 1, dev, (void*)name, buf, (void*)len);
}

void prom_putchar(char c)
{
  if (c == '\n')
    call_prom ("write", 3, 1, prom_stdout, (void*)"\r\n", (void*)2);
  else
    call_prom ("write", 3, 1, prom_stdout, (void*)&c, (void*)1);
}
