#ifndef PROM_H
#define PROM_H

struct prom_args
{
  const char *service;
  int nargs;
  int nret;
  void *args[10];
};

typedef void *prom_handle;

typedef int (*prom_entry) (prom_args *);
void prom_init(prom_entry pe);

void *call_prom(const char *service, int nargs, int nret, void *a1=0, void *a2=0, void *a3=0, void *a4=0);

void *prom_finddevice(const char *dev);
int prom_exit();
int prom_getprop(prom_handle dev, char *name, void *buf, int len);
void prom_putchar(char c);
int prom_get_chosen(char *name, void *mem, int len);
#endif
