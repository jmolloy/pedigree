#include <stdio.h>
#include <stdlib.h>
#define uintptr_t unsigned int

#include "StaticString.h"

typedef StaticString<128> LargeStaticString;
typedef struct
{
  LargeStaticString substitutions[16];
  int nSubstitutions;
  LargeStaticString templateParameters[8];
  int nTemplateParameters;
  int nLevel;
} demangle_t;

#define FAIL 1
#define SUCCESS 0

#define DECLARE(id) static int parse##id(LargeStaticString &, LargeStaticString &, demangle_t &)
DECLARE(MangledName);
DECLARE(Name);
DECLARE(UnscopedName);
DECLARE(UnscopedTemplateName);
DECLARE(NestedName);
DECLARE(TemplatePrefix);
DECLARE(UnqualifiedName);
DECLARE(SourceName);
DECLARE(CvQualifiers);
DECLARE(Type);
DECLARE(BuiltinType);
DECLARE(FunctionType);
DECLARE(BareFunctionType);
DECLARE(ArrayType);
DECLARE(PointerToMemberType);
DECLARE(TemplateParam);
DECLARE(TemplateTemplateParam);
DECLARE(TemplateArgs);
DECLARE(TemplateArg);
DECLARE(Expression);
DECLARE(ExprPrimary);
DECLARE(Substitution);
DECLARE(SeqId);
DECLARE(OperatorName);
DECLARE(CtorDtorName);

static int parsePrefix(LargeStaticString &, LargeStaticString &, demangle_t &, LargeStaticString &);
static int parseNumber(LargeStaticString &, LargeStaticString &, demangle_t &, int &);
static int parseIdentifier(LargeStaticString &, LargeStaticString &, demangle_t &, int &);

#undef DEBUG
#ifdef DEBUG
#define END_SUCCESS(id) do {printf("[%d] Success:\t %s, \t%s -> %s\n", data.nLevel--, id,(const char*)src,(const char*)dest); return SUCCESS;} while(0)
#define END_FAIL(id) do {printf("[%d] Fail:\t %s, \t%s -> %s\n", data.nLevel--, id,(const char*)src,(const char*)dest); return FAIL;} while(0)
#define START(id) do {printf("[%d] Start:\t %s, \t%s -> %s\n", ++data.nLevel, id,(const char*)src,(const char*)dest);} while(0)
#else // DEBUG
#define END_SUCCESS(id) return SUCCESS
#define END_FAIL(id) return FAIL
#define START(id)
#endif

static void addSubstitution(LargeStaticString str, demangle_t &data)
{
  for (int i = 0; i < data.nSubstitutions; i++)
    if (!strcmp(data.substitutions[i], str))
      return;
  data.substitutions[data.nSubstitutions++] = str;
}

/*
<mangled_name> ::= _Z <name>
*/
static int parseMangledName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("MangledName");
  // Does the string begin with _Z?
  if (src[0] != '_' || src[1] != 'Z')
    END_FAIL("MangledName");
  src.stripFirst(2);
  if (parseName(src, dest, data) == SUCCESS)
    END_SUCCESS("MangledName");
  else
    END_FAIL("MangledName");
}

/*
<name> ::= <nested-name>
       ::= <unscoped-name>
       ::= <unscoped-template-name> <template-args>
       ::= <local-name> <--- ignored.
*/
static int parseName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("Name");
  LargeStaticString origsrc = src;
  LargeStaticString origdest = dest;
  // Nested names are the easiest, they start with a 'N'.
  if (src[0] == 'N')
    return parseNestedName(src, dest, data);
  
  // Here we try and parse a template name, with arguments following.
  if (parseUnscopedTemplateName(src, dest, data) == SUCCESS &&
      parseTemplateArgs(src, dest, data) == SUCCESS)
  {
    END_SUCCESS("Name");
  }
  else // Template parseage failed, try normal unscoped name.
  {
    src = origsrc; // Restore src.
    dest = origdest; // Restore dest;
    if (parseUnscopedName(src, dest, data) == SUCCESS)
      END_SUCCESS("Name");
    else
      END_FAIL("Name");
  }
}

/*
<unscoped_name> ::= <unqualified_name>
  ::= St <unqualified_name> // ::std::
*/
static int parseUnscopedName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("UnscopedName");
  if (parseUnqualifiedName(src, dest, data) == SUCCESS)
    END_SUCCESS("UnscopedName");
  else
    END_FAIL("UnscopedName");
}

/*
<unscoped_template_name> ::= <unscoped_name>
                         ::= <substitution>
*/
static int parseUnscopedTemplateName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("UnscopedTemplateName");
  LargeStaticString origsrc = src;
  LargeStaticString origdest = dest;
  
  // Try substitution first.
  if (parseSubstitution(src, dest, data) == SUCCESS)
    END_SUCCESS("UnscopedTemplateName");
  
  src = origsrc;
  dest = origdest;
  
  if (parseUnqualifiedName(src, dest, data) == SUCCESS)
    END_SUCCESS("UnscopedTemplateName");
  else
    END_FAIL("UnscopedTemplateName");
}

/*
<nested_name> ::= N [<CV-qualifiers>] <prefix> <unqualified_name> E
              ::= N [<CV-qualifiers>] <template-prefix> <template_args> E
=============== CHANGED TO
<nested_name> ::= N [<CV-qualifiers>] <prefix> E
*/
static int parseNestedName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("NestedName");
  
  if (src[0] != 'N')
    END_FAIL("NestedName");
  src.stripFirst(1);
  
  // Check CV qualifiers.
  if (src[0] == 'r' || src[0] == 'K' || src[0] == 'V')
  {
    if (parseCvQualifiers(src, dest, data) == FAIL)
      END_FAIL("NestedName");
  }
  
  LargeStaticString thePrefix;
  if (parsePrefix(src, dest, data, thePrefix) == FAIL)
    END_FAIL("NestedName");
  
  if (src[0] != 'E')
    END_FAIL("NestedName");
  src.stripFirst(1);
  
  END_SUCCESS("NestedName");
}

/*
<prefix> ::= <prefix> <unqualified_name>
         ::= <template_prefix> <template_args>
         ::= <template_param>
         ::= # empty
         ::= <substitution>
=============== CHANGED TO
<prefix> ::= <unqualified_name> <prefix>
         ::= <unqualified_name> <template_args> <prefix>
         ::= <template_param>
         ::= # empty
         ::= <substitution>
*/
static int parsePrefix(LargeStaticString &src, LargeStaticString &dest, demangle_t &data, LargeStaticString &thisPrefix)
{
  START("Prefix");
  
  LargeStaticString origsrc = src;
  LargeStaticString origdest = dest;
  
  // Check for a template_param.
//   if (parseTemplateParam(src, dest, data) == SUCCESS)
//     END_SUCCESS("Prefix");
  
  src = origsrc;
  dest = origdest;
  
  // Check for a substitution.
  if (parseSubstitution(src, dest, data) == SUCCESS)
    END_SUCCESS("Prefix");
  
  src = origsrc;
  dest = origdest;
  
  // Check for an unqualified name
  LargeStaticString unqualName;
  if (parseUnqualifiedName(src, unqualName, data) == SUCCESS)
  {
    dest += "::";
    dest += unqualName;
    origsrc = src;
    origdest = dest; // Checkpoint!
    
    thisPrefix += "::";
    thisPrefix += unqualName;
    
    addSubstitution(thisPrefix, data);
    
    // Do we have a template_args?
    unqualName = "";
    if (parseTemplateArgs(src, unqualName, data) == FAIL)
    {
      src = origsrc;
      dest = origdest;
    }
    else
    {
      dest += unqualName;
      thisPrefix += unqualName;
      addSubstitution(thisPrefix, data);
    }
    
    // Recurse.
    if (parsePrefix(src, dest, data, thisPrefix) == SUCCESS)
      END_SUCCESS("Prefix");
    else
      END_FAIL("Prefix");
  }
  
  src = origsrc;
  dest = origdest;
  
  // Empty rule.
  END_SUCCESS("Prefix");
}

/*
<template_prefix> ::= <prefix> <unqualified_name>
                  ::= <template_param>
                  ::= <substitution>
*/
static int parseTemplatePrefix(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
   // Unused.
}

/*
<unqualified_name> ::= <operator_name>
                   ::= <ctor_dtor_name>
                   ::= <source_name>
*/
static int parseUnqualifiedName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("UnqualifiedName");
  LargeStaticString origsrc = src;
  LargeStaticString origdest = dest;
  
  if (parseOperatorName(src, dest, data) == SUCCESS)
    END_SUCCESS("UnqualifiedName");
  
  src = origsrc;
  dest = origdest;
  
  if (parseCtorDtorName(src, dest, data) == SUCCESS)
    END_SUCCESS("UnqualifiedName");
  
  src = origsrc;
  dest = origdest;
  
  if (parseSourceName(src, dest, data) == SUCCESS)
    END_SUCCESS("UnqualifiedName");
  
  END_FAIL("UnqualifiedName");
}

/*
<ctor_dtor_name> ::= C {0,1,2}
                 ::= D {0,1,2}
*/
static int parseCtorDtorName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("CtorDtorName");
  if (src[0] == 'C' &&
      (src[1] == '0' || src[1] == '1' || src[1] == '2'))
  {
    src.stripFirst(2);
    dest += "`ctor'";
  }
  else if (src[0] == 'D' &&
      (src[1] == '0' || src[1] == '1' || src[1] == '2'))
  {
    src.stripFirst(2);
    dest += "`dtor'";
  }
  else
    END_FAIL("CtorDtorName");
  END_SUCCESS("CtorDtorName");
}

/*
<operator-name> ::= nw        # new           
                ::= na        # new[]
                ::= dl        # delete        
                ::= da        # delete[]      
                ::= ps        # + (unary)
                ::= ng        # - (unary)     
                ::= ad        # & (unary)     
                ::= de        # * (unary)     
                ::= co        # ~             
                ::= pl        # +             
                ::= mi        # -             
                ::= ml        # *             
                ::= dv        # /             
                ::= rm        # %             
                ::= an        # &             
                ::= or        # |             
                ::= eo        # ^             
                ::= aS        # =             
                ::= pL        # +=            
                ::= mI        # -=            
                ::= mL        # *=            
                ::= dV        # /=            
                ::= rM        # %=            
                ::= aN        # &=            
                ::= oR        # |=            
                ::= eO        # ^=            
                ::= ls        # <<            
                ::= rs        # >>            
                ::= lS        # <<=           
                ::= rS        # >>=           
                ::= eq        # ==            
                ::= ne        # !=            
                ::= lt        # <             
                ::= gt        # >             
                ::= le        # <=            
                ::= ge        # >=            
                ::= nt        # !             
                ::= aa        # &&            
                ::= oo        # ||            
                ::= pp        # ++            
                ::= mm        # --            
                ::= cm        # ,             
                ::= pm        # ->*           
                ::= pt        # ->            
                ::= cl        # ()            
                ::= ix        # []            
                ::= qu        # ?             
                ::= st        # sizeof (a type)
                ::= sz        # sizeof (an expression)
                ::= cv <type> # (cast)
*/
static int parseOperatorName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("OperatorName");
  END_FAIL("OperatorName");
}

/*
<source_name> ::= <number> <identifier>
*/
static int parseSourceName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("SourceName");
  
  int lval;
  if (parseNumber(src, dest, data, lval) == SUCCESS &&
      lval >= 0 && 
      parseIdentifier(src, dest, data, lval) == SUCCESS)
    END_SUCCESS("SourceName");
  
  END_FAIL("SourceName");
}

/*
<number> ::= [n] <non negative decimal integer>
*/
static int parseNumber(LargeStaticString &src, LargeStaticString &dest, demangle_t &data, int &lval)
{
  START("Number");
  
  // Negative?
  bool bNegative = false;
  if (src[0] == 'n')
  {
    bNegative = true;
    src.stripFirst(1);
  }
  
  // Non numeric?
  if (src[0] < '0' || src[0] > '9')
    END_FAIL("Number");

  int nLength = 0;
  char str[32];
  while (src[nLength] >= '0' && src[nLength] <= '9')
  {
    str[nLength] = src[nLength];
    nLength++;
  }
  str[nLength] = '\0';
  lval = strtoul(str, NULL, 10);
  if (bNegative) lval *= -1;
  
  src.stripFirst(nLength);
  
  END_SUCCESS("Number");
}

/*
<identifier> ::= <unqualified source code identifier>
*/
static int parseIdentifier(LargeStaticString &src, LargeStaticString &dest, demangle_t &data, int &lval)
{
  START("Identifier");
  if (src.length() < lval)
    END_FAIL("Identifier");
  for (int i = 0; i < lval; i++)
    dest += src[i];
  src.stripFirst(lval);
  END_SUCCESS("Identifier");
}

/*
<CV-qualifiers> ::= [r] [V] [K]
*/
static int parseCvQualifiers(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("CvQualifiers");
  bool bParsedOne = false;
  if (src[0] == 'r')
  {
    bParsedOne = true;
    dest += "restrict ";
    src.stripFirst(1);
  }
  if (src[0] == 'V')
  {
    bParsedOne = true;
    dest += "volatile ";
    src.stripFirst(1);
  }
  if (src[0] == 'K')
  {
    bParsedOne = true;
    dest += "const ";
    src.stripFirst(1);
  }
  if (!bParsedOne)
    END_FAIL("CvQualifiers");
  else
    END_SUCCESS("CvQualifiers");
}

/*
<type> ::= <CV-qualifiers> <type>
       ::= P <type>
       ::= R <type>
       ::= <builtin-type>
       ::= <function-type>
       ::= <name> # class-enum-type
       ::= <array-type>
       ::= <pointer-to-member-type>
       ::= <template-param>
       ::= <template-template-param> <template-args>
       ::= <substitution>
*/
static int parseType(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("Type");
  
  // CV-qualifiers?
  if (src[0] == 'r' || src[0] == 'V' || src[0] == 'K')
  {
    if (parseCvQualifiers(src, dest, data) == FAIL)
      END_FAIL("Type");
    if (parseType(src, dest, data) == FAIL)
      END_FAIL("Type");
    END_SUCCESS("Type");
  }
  // Pointer?
  else if (src[0] == 'P')
  {
    src.stripFirst(1);
    if (parseType(src, dest, data) == FAIL)
      END_FAIL("Type");
    dest += "*";
    END_SUCCESS("Type");
  }
  // Reference?
  else if (src[0] == 'R')
  {
    src.stripFirst(1);
    if (parseType(src, dest, data) == FAIL)
      END_FAIL("Type");
    dest += "&";
    END_SUCCESS("Type");
  }
  // Function-type?
  else if (src[0] == 'F')
  {
    if (parseFunctionType(src, dest, data) == FAIL)
      END_FAIL("Type");
    END_SUCCESS("Type");
  }
  // Array type?
  else if (src[0] == 'A')
  {
    if (parseArrayType(src, dest, data) == FAIL)
      END_FAIL("Type");
    END_SUCCESS("Type");
  }
  // Pointer-to-member type?
  else if (src[0] == 'M')
  {
    if (parsePointerToMemberType(src, dest, data) == FAIL)
      END_FAIL("Type");
    END_SUCCESS("Type");
  }
  // Template parameter type?
  else if (src[0] == 'T')
  {
    // Try the template-template-type first, if it fails fall back.
    LargeStaticString origsrc = src;
    LargeStaticString origdest = dest;
    if (parseTemplateTemplateParam(src, dest, data) == SUCCESS &&
        parseTemplateArgs(src, dest, data) == SUCCESS)
      END_SUCCESS("Type");
    
    src = origsrc;
    dest = origdest;
    
    if (parseTemplateParam(src, dest, data) == FAIL)
      END_FAIL("Type");
    
    END_SUCCESS("Type");
  }
  else if (src[0] == 'S')
  {
    if (parseSubstitution(src, dest, data) == FAIL)
      END_FAIL("Type");
    END_SUCCESS("Type");
  }
  else
  {
    // OK then, try a builtin-type.
    LargeStaticString origsrc = src;
    LargeStaticString origdest = dest;
    if (parseBuiltinType(src, dest, data) == SUCCESS)
      END_SUCCESS("Type");
    
    src = origsrc;
    dest = origdest;
    
    LargeStaticString tmp;
    if (parseName(src, tmp, data) == SUCCESS)
    {
      dest += tmp;
      addSubstitution(tmp, data);
      END_SUCCESS("Type");
    }
    
    END_FAIL("Type");
  }
}

/*
<builtin-type> ::= v    # void
               ::= w  # wchar_t
               ::= b  # bool
               ::= c  # char
               ::= a  # signed char
               ::= h  # unsigned char
               ::= s  # short
               ::= t  # unsigned short
               ::= i  # int
               ::= j  # unsigned int
               ::= l  # long
               ::= m  # unsigned long
               ::= x  # long long, __int64
               ::= y  # unsigned long long, __int64
               ::= n  # __int128
               ::= o  # unsigned __int128
               ::= f  # float
               ::= d  # double
               ::= e  # long double, __float80
               ::= g  # __float128
               ::= z  # ellipsis
*/
static int parseBuiltinType(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("BuiltinType");
  switch(src[0])
  {
    case 'v': dest += "void"; break;
    case 'w': dest += "wchar_t"; break;
    case 'b': dest += "bool"; break;
    case 'c': dest += "char"; break;
    case 'a': dest += "char"; break;
    case 'h': dest += "unsigned char"; break;
    case 's': dest += "short"; break;
    case 't': dest += "unsigned short"; break;
    case 'i': dest += "int"; break;
    case 'j': dest += "unsigned int"; break;
    case 'l': dest += "long"; break;
    case 'm': dest += "unsigned long"; break;
    case 'x': dest += "__int64"; break;
    case 'y': dest += "unsigned __int64"; break;
    case 'n': dest += "__int128"; break;
    case 'o': dest += "unsigned __int128"; break;
    case 'f': dest += "float"; break;
    case 'd': dest += "double"; break;
    case 'z': dest += "..."; break;
    default:
      END_FAIL("BuiltinType");
  }
  src.stripFirst(1);
  END_SUCCESS("BuiltinType");
}

/*
<function_type> ::= F [Y] <bare_function_type> E
*/
static int parseFunctionType(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<bare_function_type> ::= <type>+
*/
static int parseBareFunctionType(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<array_type> ::= A <number> _ <type>
             ::= A <expression> _ <type>
*/
static int parseArrayType(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<pointer_to_member_type> ::= M <class type> <member type>
*/
static int parsePointerToMemberType(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<template_param> ::= T_ # first parameter
                 ::= T <parameter-2 non negative number> _
*/
static int parseTemplateParam(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("TemplateParam");
  END_FAIL("TemplateParam");
}

/*
<template_param> ::= <template-param> # first parameter
                 ::= <substitution>
*/
static int parseTemplateTemplateParam(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("TemplateTemplateParam");
  END_FAIL("TemplateTemplateParam");
}

/*
<template_args> ::= I <template_arg>+ E
*/
static int parseTemplateArgs(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("TemplateArgs");
  
  if (src[0] != 'I')
    END_FAIL("TemplateArgs");
  
  src.stripFirst(1);
  bool bIsFirst = true;
  dest += "<";
  do
  {
    if (!bIsFirst)
      dest += ", ";
    bIsFirst = false;
    if (parseTemplateArg(src, dest, data) == FAIL)
      END_FAIL("TemplateArgs");
  } while (src[0] != 'E');
  dest += ">";
  src.stripFirst(1);
  
  END_SUCCESS("TemplateArgs");
}

/*
<template_arg> ::= <type>
               ::= X <expression> E
               ::= <expr-primary>
*/
static int parseTemplateArg(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("TemplateArg");
  LargeStaticString origsrc = src;
  LargeStaticString origdest = dest;
  
  if (parseType(src, dest, data) == SUCCESS)
    END_SUCCESS("TemplateArg");
  
  src = origsrc;
  dest = origdest;
  
  if (parseExpression(src, dest, data) == SUCCESS)
    END_SUCCESS("TemplateArg");
  
  src = origsrc;
  dest = origdest;
  
  if (parseExprPrimary(src, dest, data) == SUCCESS)
    END_SUCCESS("TemplateArg");
  
  END_FAIL("TemplateArg");
}

/*
<expression> ::= <unary operator-name> <expression>
             ::= <binary operator-name> <expression> <expression>
             ::= <trinary operator-name> <expression> <expression> <expression>
             ::= st <type>
             ::= <template-param>
             ::= sr <type> <unqualified-name>                   # dependent name
             ::= sr <type> <unqualified-name> <template-args>   # dependent template-id
             ::= <expr-primary>
*/
static int parseExpression(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("Expression");
  END_FAIL("Expression");
}

/*
<expr-primary> ::= L <type> <value number> E                   # integer literal
               ::= L <type <value float> E                     # floating literal (ignored)
               ::= L <mangled-name> E                           # external name
*/
static int parseExprPrimary(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("ExprPrimary");
  END_FAIL("ExprPrimary");
}

/*
<substitution> ::= S <seq_id> _
               ::= S_
*/
static int parseSubstitution(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
  START("Substitution");
  END_FAIL("Substitution");
}

/*
<seq_id> ::= <base 36 number>
*/
static int parseSeqId(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

int main(char argc, char **argv)
{
  LargeStaticString src = argv[1];
  LargeStaticString dest;
  demangle_t data;
  data.nLevel = 0;
  data.nSubstitutions = 0;
  int code = parseMangledName(src, dest, data);
  if (code == FAIL)
    printf("Failed.\n");
  else
    printf("%s -> %s\n", (const char*)argv[1], (const char*)dest);
  
  printf("Substitutions:\n");
  for (int i = 0; i < data.nSubstitutions; i++)
  {
    if (i == 0)
      printf("\t S_:  %s\n", (const char*)data.substitutions[0]);
    else
      printf("\t S%d_: %s\n", i-1, (const char*)data.substitutions[i]);
  }
  return 0;
}
