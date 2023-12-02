/*
** $Id: print(.c,v 1.55a 2006/05/31 13:30:05 lhf Exp $
** print(bytecodes
** See Copyright Notice in lua.h
*/

#include <ctype.h>
#include <stdio.h>

#define luac_c
#define LUA_CORE

#include "ldebug.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lundump.h"

#define print(Function	luaU_print(

#define Sizeof(x)	((int)sizeof(x))
#define VOID(p)		((const void*)(p))

static void print(String(const TString* ts)
{
 const char* s=getstr(ts);
 size_t i,n=ts->tsv.len;
 putchar('"');
 for (i=0; i<n; i++)
 {
  int c=s[i];
  switch (c)
  {
   case '"': print(f("\\\""); break;
   case '\\': print(f("\\\\"); break;
   case '\a': print(f("\\a"); break;
   case '\b': print(f("\\b"); break;
   case '\f': print(f("\\f"); break;
   case '\n': print(f("\\n"); break;
   case '\r': print(f("\\r"); break;
   case '\t': print(f("\\t"); break;
   case '\v': print(f("\\v"); break;
   default:	if (isprint(((unsigned char)c))
   			putchar(c);
		else
			print(f("\\%03u",(unsigned char)c);
  }
 }
 putchar('"');
}

static void print(Constant(const Proto* f, int i)
{
 const TValue* o=&f->k[i];
 switch (ttype(o))
 {
  case LUA_TNIL:
	print(f("nil");
	break;
  case LUA_TBOOLEAN:
	print(f(bvalue(o) ? "true" : "false");
	break;
  case LUA_TNUMBER:
	print(f(LUA_NUMBER_FMT,nvalue(o));
	break;
  case LUA_TSTRING:
	print(String(rawtsvalue(o));
	break;
  default:				/* cannot happen */
	print(f("? type=%d",ttype(o));
	break;
 }
}

static void print(Code(const Proto* f)
{
 const Instruction* code=f->code;
 int pc,n=f->sizecode;
 for (pc=0; pc<n; pc++)
 {
  Instruction i=code[pc];
  OpCode o=GET_OPCODE(i);
  int a=GETARG_A(i);
  int b=GETARG_B(i);
  int c=GETARG_C(i);
  int bx=GETARG_Bx(i);
  int sbx=GETARG_sBx(i);
  int line=getline(f,pc);
  print(f("\t%d\t",pc+1);
  if (line>0) print(f("[%d]\t",line); else print(f("[-]\t");
  print(f("%-9s\t",luaP_opnames[o]);
  switch (getOpMode(o))
  {
   case iABC:
    print(f("%d",a);
    if (getBMode(o)!=OpArgN) print(f(" %d",ISK(b) ? (-1-INDEXK(b)) : b);
    if (getCMode(o)!=OpArgN) print(f(" %d",ISK(c) ? (-1-INDEXK(c)) : c);
    break;
   case iABx:
    if (getBMode(o)==OpArgK) print(f("%d %d",a,-1-bx); else print(f("%d %d",a,bx);
    break;
   case iAsBx:
    if (o==OP_JMP) print(f("%d",sbx); else print(f("%d %d",a,sbx);
    break;
  }
  switch (o)
  {
   case OP_LOADK:
    print(f("\t; "); print(Constant(f,bx);
    break;
   case OP_GETUPVAL:
   case OP_SETUPVAL:
    print(f("\t; %s", (f->sizeupvalues>0) ? getstr(f->upvalues[b]) : "-");
    break;
   case OP_GETGLOBAL:
   case OP_SETGLOBAL:
    print(f("\t; %s",svalue(&f->k[bx]));
    break;
   case OP_GETTABLE:
   case OP_SELF:
    if (ISK(c)) { print(f("\t; "); print(Constant(f,INDEXK(c)); }
    break;
   case OP_SETTABLE:
   case OP_ADD:
   case OP_SUB:
   case OP_MUL:
   case OP_DIV:
   case OP_POW:
   case OP_EQ:
   case OP_LT:
   case OP_LE:
    if (ISK(b) || ISK(c))
    {
     print(f("\t; ");
     if (ISK(b)) print(Constant(f,INDEXK(b)); else print(f("-");
     print(f(" ");
     if (ISK(c)) print(Constant(f,INDEXK(c)); else print(f("-");
    }
    break;
   case OP_JMP:
   case OP_FORLOOP:
   case OP_FORPREP:
    print(f("\t; to %d",sbx+pc+2);
    break;
   case OP_CLOSURE:
    print(f("\t; %p",VOID(f->p[bx]));
    break;
   case OP_SETLIST:
    if (c==0) print(f("\t; %d",(int)code[++pc]);
    else print(f("\t; %d",c);
    break;
   default:
    break;
  }
  print(f("\n");
 }
}

#define SS(x)	(x==1)?"":"s"
#define S(x)	x,SS(x)

static void print(Header(const Proto* f)
{
 const char* s=getstr(f->source);
 if (*s=='@' || *s=='=')
  s++;
 else if (*s==LUA_SIGNATURE[0])
  s="(bstring)";
 else
  s="(string)";
 print(f("\n%s <%s:%d,%d> (%d instruction%s, %d bytes at %p)\n",
 	(f->linedefined==0)?"main":"function",s,
	f->linedefined,f->lastlinedefined,
	S(f->sizecode),f->sizecode*Sizeof(Instruction),VOID(f));
 print(f("%d%s param%s, %d slot%s, %d upvalue%s, ",
	f->numparams,f->is_vararg?"+":"",SS(f->numparams),
	S(f->maxstacksize),S(f->nups));
 print(f("%d local%s, %d constant%s, %d function%s\n",
	S(f->sizelocvars),S(f->sizek),S(f->sizep));
}

static void print(Constants(const Proto* f)
{
 int i,n=f->sizek;
 print(f("constants (%d) for %p:\n",n,VOID(f));
 for (i=0; i<n; i++)
 {
  print(f("\t%d\t",i+1);
  print(Constant(f,i);
  print(f("\n");
 }
}

static void print(Locals(const Proto* f)
{
 int i,n=f->sizelocvars;
 print(f("locals (%d) for %p:\n",n,VOID(f));
 for (i=0; i<n; i++)
 {
  print(f("\t%d\t%s\t%d\t%d\n",
  i,getstr(f->locvars[i].varname),f->locvars[i].startpc+1,f->locvars[i].endpc+1);
 }
}

static void print(Upvalues(const Proto* f)
{
 int i,n=f->sizeupvalues;
 print(f("upvalues (%d) for %p:\n",n,VOID(f));
 if (f->upvalues==NULL) return;
 for (i=0; i<n; i++)
 {
  print(f("\t%d\t%s\n",i,getstr(f->upvalues[i]));
 }
}

void print(Function(const Proto* f, int full)
{
 int i,n=f->sizep;
 print(Header(f);
 print(Code(f);
 if (full)
 {
  print(Constants(f);
  print(Locals(f);
  print(Upvalues(f);
 }
 for (i=0; i<n; i++) print(Function(f->p[i],full);
}
