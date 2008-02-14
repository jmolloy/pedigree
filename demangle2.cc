#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
  char name[128]; // Function name.
  char params[32][64]; // Parameters.
  int nParams;
} symbol_t;

const char *parseNamespace(const char *src, char *dest);
const char *parseFunction(const char *src, char *dest);
const char *parseTemplate(const char *src, char *dest);

const char *getNumber(const char *src, int &n)
{
  char pStr[32];
  int len = 0;
  while (*src >= '0' && *src <= '9')
  {
    pStr[len++] = *src++;
  }
  pStr[len] = '\0';
  
  n = atoi(pStr);
  return src;
}

const char *parseNamespace(const char *src, char *dest)
{
  src++; // Step over 'N'.

  dest[0] = '\0';
  
  while (*src)
  {
    if (*src == 'E')
    {
      src ++;
      break;
    }
    else if (*src == 'C')
    {
      // If we are not the first namespace/class, add a '::'.
      if (dest[0] != '\0')
        strcat(dest, "::");
      strcat(dest, "`ctor'");
      src += 2; // Get rid of "C1".
    }
    else if (*src == 'D')
    {
      // If we are not the first namespace/class, add a '::'.
      if (dest[0] != '\0')
        strcat(dest, "::");
      strcat(dest, "`dtor'");
      src += 2; // Get rid of "D1".
    }
    else if (*src == 'I')
    {
      char pStr[128];
      src = parseTemplate(src, pStr);
      strcat(dest, pStr);
    }
    else if (*src >= '0' && *src <= '9')
    {
      int n;
      src = getNumber(src, n);
      
      // If we are not the first namespace/class, add a '::'.
      if (dest[0] != '\0')
        strcat(dest, "::");
      
      strncat(dest, src, n);
      src += n;
    }
    else 
    {
      break;
    }
  }
  return src;
}

const char *parseFunction(const char *src, char *dest)
{
  dest[0] = '\0';
  
  int n;
  src = getNumber(src, n);
  strncat(dest, src, n);
  src += n;
  
  if (*src == 'I')
  {
    char pStr[128];
    src = parseTemplate(src, pStr);
    src++; // Skip over 'E'.
    strcat(dest, pStr);
  }

  return src;
}

const char *parseTemplate(const char *src, char *dest)
{
  src++; // Step over 'I'.
  
  dest[0] = '\0';
  
  bool bIsPointer = false;
  bool bIsThisPointer = false;
  while (*src)
  {
    bIsThisPointer = false;
    if (*src == 'E')
    {
      src++;
      break;
    }
    else
    {
      if (dest[0] == '\0')
        strcat(dest, "<");
      else if (!bIsPointer)
        strcat(dest, ", ");
      if (*src == 'N')
      {
        char pStr[128];
        src = parseNamespace(src, pStr);
        strcat(dest, pStr);
      }
      else if (*src == 'i') {src++; strcat(dest, "int");}
      else if (*src == 'j') {src++; strcat(dest, "unsigned int");}
      else if (*src == 'c') {src++; strcat(dest, "char");}
      else if (*src == 'h') {src++; strcat(dest, "unsigned char");}
      else if (*src == 's') {src++; strcat(dest, "short");}
      else if (*src == 't') {src++; strcat(dest, "unsigned short");}
      else if (*src == 'b') {src++; strcat(dest, "bool");}
      else if (*src == 'P') {src++; bIsPointer = bIsThisPointer = true;}
      else {break;}
      
      if (bIsPointer && !bIsThisPointer)
      {
        strcat(dest, " *");
        bIsPointer = false;
      }
    }
  }
  
  strcat(dest, ">");
  
  return src;
}

void parseParameters(const char *src, symbol_t *sym)
{
  int param = -1;
  char *dest;
  bool bIsPointer = false;
  bool bIsThisPointer = false;
  while (*src)
  {
    if (!bIsThisPointer)
    {
      param++;
      sym->params[param][0] = '\0';
      dest = sym->params[param];
    }
    bIsThisPointer = false;
    if (*src == 'I') {char pStr[128]; src = parseTemplate(src, pStr); strcat(dest, pStr);}
    else if (*src == 'i') {src++; strcat(dest, "int");}
    else if (*src == 'j') {src++; strcat(dest, "unsigned int");}
    else if (*src == 'c') {src++; strcat(dest, "char");}
    else if (*src == 'h') {src++; strcat(dest, "unsigned char");}
    else if (*src == 's') {src++; strcat(dest, "short");}
    else if (*src == 't') {src++; strcat(dest, "unsigned short");}
    else if (*src == 'b') {src++; strcat(dest, "bool");}
    else if (*src == 'K') {src++; strcat(dest, "const "); bIsThisPointer = true;}
    else if (*src == 'P') {src++; bIsPointer = bIsThisPointer = true;}
    else if (*src == 'T')
    {
      char pStr[32];
      pStr[0] = '\0';
      int nStr = 0;
      while (*src != '_')
        pStr[nStr++] = *src++;
      src++;
      strcat(dest, pStr);
    }
    else if (*src >= '0' && *src <= '9')
    {
      char *pStr[128];
      int n;
      src = getNumber(src, n);
      strncat(dest, src, n);
      src += n;
      // If the next character is an 'I', this is templated so don't increment 'param'.
      if (*src == 'I')
        bIsThisPointer = true;
    }
    else {break;}

    if (bIsPointer && !bIsThisPointer)
    {
      strcat(dest, " *");
      bIsPointer = false;
    }
    
  }
  sym->nParams = param+1;
}

void demangle(const char *src, symbol_t *sym)
{
  // All C++ symbols start with '_Z'.
  if (src[0] != '_' || src[1] != 'Z')
  {
    strcpy(sym->name, src);
    return;
  }
  
  src += 2; // Skip over _Z.
  
  // Namespace?
  if (*src == 'N')
    src = parseNamespace(src, sym->name);
  else
    src = parseFunction(src, sym->name);
  
  parseParameters(src, sym);
}

int main(char argc, char **argv)
{
  symbol_t sym;
  demangle(argv[1], &sym);
  printf("%s(", sym.name);
  for (int i = 0; i < sym.nParams; i++)
  {
    if (i != 0)
      printf (", ");
    printf("%s", sym.params[i]);
  }
  printf(")\n");
}
