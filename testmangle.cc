#include <stdio.h>
#define uintptr_t unsigned int

#include "StaticString.h"

typedef StaticString<128> LargeStaticString;
typedef struct
{
  LargeStaticString substitutions[16];
  int nSubstitutions;
  LargeStaticString templateParameters[8];
  int nTemplateParameters;
} demangle_t;

/*
<mangled_name> :: _Z <name>
*/
static int parseMangledName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<name> ::= <nested-name>
       ::= <unscoped-name>
       ::= <unscoped-template-name> <template-args>
       ::= <local-name> <--- ignored.
*/
static int parseName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<unscoped_name> ::= <unqualified_name>
                ::= St <unqualified_name>
*/
static int parseUnscopedName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<unscoped_template_name> ::= <unscoped_name>
                         ::= <substitution>
*/
static int parseUnscopedTemplateName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<nested_name> ::= N [<CV-qualifiers>] <prefix> <unqualified_name> E
              ::= N [<CV-qualifiers>] <template-prefix> <template-args> E
*/
static int parseNestedName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<prefix> ::= <prefix> <unqualified_name>
         ::= <template_prefix> <template_args>
         ::= <template_param>
         ::= # empty
         ::= <substitution>
*/
static int parsePrefix(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<template_prefix> ::= <prefix> <unqualified_name>
                  ::= <template_param>
                  ::= <substitution>
*/
static int parseTemplatePrefix(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<unqualified_name> ::= <operator_name>
                   ::= <ctor_dtor_name>
                   ::= <source_name>
*/
static int parseUnqualifiedName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<source_name> ::= <number> <identifier>
*/
static int parseSourceName(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<number> ::= [n] <non negative decimal integer>
*/
static int parseNumber(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<identifier> ::= <unqualified source code identifier>
*/
static int parseIdentifier(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

/*
<CV-qualifiers> ::= [r] [V] [K]
*/
static int parseCvQualifiers(LargeStaticString &src, LargeStaticString &dest, demangle_t &data)
{
}

int main(char argc, char **argv)
{
  return 0;
}
