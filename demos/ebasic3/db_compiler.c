/* db_compiler.c - a simple basic compiler
 *
 * Copyright (c) 2009 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include "db_compiler.h"
#include "db_vmdebug.h"
#include "db_vm.h"

/* local function prototypes */
static void InitCodeBuffer(ParseContext *c);
static void PlaceStrings(ParseContext *c);
static void PlaceSymbols(ParseContext *c);

/* InitCompiler - initialize the compiler and create a parse context */
ParseContext *InitCompiler(System *sys, int imageBufferSize)
{
    ParseContext *c = NULL;
    uint8_t *freeMark;
    ImageHdr *image;

    /* allocate space for the image header */
    if (!(image = (ImageHdr *)AllocateFreeSpace(sys, imageBufferSize)))
        ParseError(c, "insufficient space for image header");
        
    /* mark the current position in free space to allow compiler data structures to be freed */
    freeMark = sys->freeNext;

    /* allocate and initialize the parse context */
    if (!(c = (ParseContext *)AllocateFreeSpace(sys, sizeof(ParseContext))))
        ParseError(c, "insufficient space for parse context\n");
    
    /* initialize the parse context */
    memset(c, 0, sizeof(ParseContext));
    c->sys = sys;
    c->freeMark = freeMark;
    c->image = image;
    c->imageBufferSize = imageBufferSize;
    
    /* return the new parse context */
    return c;
}

/* Compile - compile a program */
ImageHdr *Compile(ParseContext *c)
{
    ImageHdr *image = c->image;
    System *sys = c->sys;
    
    /* setup an error target */
    if (setjmp(c->errorTarget) != 0)
        return NULL;

    /* clear the runtime space */
    InitImageAllocator(c);

    /* use the rest of the free space for the compiler heap */
    c->nextGlobal = sys->freeNext;
    c->nextLocal = sys->freeTop;
    c->heapSize = sys->freeTop - sys->freeNext;
    c->maxHeapUsed = 0;

    /* initialize block nesting table */
    c->btop = (Block *)((char *)c->blockBuf + sizeof(c->blockBuf));
    c->bptr = c->blockBuf - 1;

    /* initialize the code buffer */
    InitCodeBuffer(c);

    /* initialize the string and label tables */
    c->strings = NULL;
    c->labels = NULL;

    /* start in the main code */
    c->codeType = CODE_TYPE_MAIN;

    /* initialize the global symbol table */
    InitSymbolTable(&c->globals);

    /* initialize scanner */
    c->inComment = VMFALSE;
    
    /* get the next line */
    while (GetLine(c)) {
        int tkn;
        if ((tkn = GetToken(c)) != T_EOL)
            ParseStatement(c, tkn);
    }

    /* end the main code with a halt */
    putcbyte(c, OP_HALT);
    
    /* write the main code */
    StartCode(c, CODE_TYPE_MAIN);
    image->mainCode = StoreCode(c);

    {
        int objectDataSize = (uint8_t *)c->imageDataFree - (uint8_t *)c->image;
#if 1
        DumpSymbols(&c->globals, "symbols");
#endif
        VM_printf("Heap: %d, Image: %d\n", c->maxHeapUsed, objectDataSize);
    }

    /* free up the space the compiler was consuming */
    sys->freeNext = c->freeMark;

    /* return the image */
    return image;
}

/* InitCodeBuffer - initialize the code buffer */
static void InitCodeBuffer(ParseContext *c)
{
    c->codeBuf = (uint8_t *)c->imageDataFree;
    c->ctop = (uint8_t *)c->imageDataTop;
    c->cptr = c->codeBuf;
}

VMVALUE *codeStart;

/* StartCode - start a function or method under construction */
void StartCode(ParseContext *c, CodeType type)
{
    /* all methods must precede the main code */
    if (type != CODE_TYPE_MAIN && c->cptr > c->codeBuf)
        ParseError(c, "subroutines and functions must precede the main code");

    /* don't allow nested functions or subroutines (for now anyway) */
    if (type != CODE_TYPE_MAIN && c->codeType != CODE_TYPE_MAIN)
        ParseError(c, "nested subroutines and functions are not supported");

    /* initialize the code object under construction */
    InitSymbolTable(&c->arguments);
    InitSymbolTable(&c->locals);
    c->localOffset = 0;
    c->codeType = type;
    
    /* write the code prolog */
    if (type != CODE_TYPE_MAIN) {
        putcbyte(c, OP_FRAME);
        putcbyte(c, 0);
    }
    
    codeStart = c->imageDataFree;
}

/* StoreCode - store the function or method under construction */
VMVALUE StoreCode(ParseContext *c)
{
    VMVALUE code;
    int codeSize;

    /* check for unterminated blocks */
    switch (CurrentBlockType(c)) {
    case BLOCK_IF:
    case BLOCK_ELSE:
        ParseError(c, "expecting END IF");
    case BLOCK_FOR:
        ParseError(c, "expecting NEXT");
    case BLOCK_DO:
        ParseError(c, "expecting LOOP");
    case BLOCK_NONE:
        break;
    }

    /* fixup the RESERVE instruction at the start of the code */
    if (c->codeType != CODE_TYPE_MAIN) {
        c->codeBuf[1] = c->localOffset;
        putcbyte(c, OP_RETURN);
    }

    /* make sure all referenced labels were defined */
    CheckLabels(c);
    
    if (c->imageDataFree != codeStart)
        printf("code buffer overwrite! was %08x, is %08x\n", (int)codeStart, (int)c->imageDataFree);

    /* determine the code size */
    codeSize = (int)(c->cptr - c->codeBuf);
    
    /* reserve space for the code */
    code = (VMVALUE)c->imageDataFree;
    c->imageDataFree += GetObjSizeInWords(codeSize);

    /* place string literals defined in this function */
    PlaceStrings(c);
    
    /* place global symbols referenced by this function */
    PlaceSymbols(c);
    
#if 1
{
    VM_printf("%s:\n", c->codeSymbol ? c->codeSymbol->name : "<main>");
    DecodeFunction((uint8_t *)code, codeSize);
    DumpSymbols(&c->arguments, "arguments");
    DumpSymbols(&c->locals, "locals");
    VM_printf("\n");
}
#endif

    /* empty the local heap */
    c->nextLocal = c->sys->freeTop;
    InitSymbolTable(&c->arguments);
    InitSymbolTable(&c->locals);
    c->labels = NULL;

    /* reset to compile the next code */
    c->codeType = CODE_TYPE_MAIN;
    
    /* reinitialize the code buffer */
    InitCodeBuffer(c);
    
    /* return the code vector */
    return code;
}

/* PlaceStrings - place any string literals defined in the current function */
static void PlaceStrings(ParseContext *c)
{
    String *str;
    for (str = c->strings; str != NULL; str = str->next) {
        if (str->fixups) {
            str->value = StoreBVector(c, (uint8_t *)str->data, strlen(str->data));
            str->placed = VMTRUE;
            fixup(c, str->fixups, str->value);
            str->fixups = 0;
        }
    }
}

/* AddString - add a string to the string table */
String *AddString(ParseContext *c, char *value)
{
    String *str;
    
    /* check to see if the string is already in the table */
    for (str = c->strings; str != NULL; str = str->next)
        if (strcmp(value, str->data) == 0)
            return str;

    /* allocate the string structure */
    str = (String *)GlobalAlloc(c, sizeof(String) + strlen(value));
    memset(str, 0, sizeof(String));
    strcpy((char *)str->data, value);
    str->next = c->strings;
    c->strings = str;

    /* return the string table entry */
    return str;
}

/* AddStringRef - add a reference to a string */
VMVALUE AddStringRef(String *str, int offset)
{
    int link;

    /* handle strings that have already been placed */
    if (str->placed)
        return str->value;

    /* add a new entry to the fixup list */
    link = str->fixups;
    str->fixups = offset;
    return link;
}

/* AddSymbolRef - add a reference to a symbol */
VMVALUE AddSymbolRef(Symbol *sym, int offset)
{
    int link;

    /* handle strings that have already been placed */
    if (sym->placed)
        return sym->value;

    /* add a new entry to the fixup list */
    link = sym->fixups;
    sym->fixups = offset;
    return link;
}

/* PlaceSymbols - place any global symbols defined in the current function */
static void PlaceSymbols(ParseContext *c)
{
    Symbol *sym;
    for (sym = c->globals.head; sym != NULL; sym = sym->next) {
        if (sym->fixups) {
            sym->value = StoreVector(c, &sym->initialValue, 1);
            sym->placed = VMTRUE;
            fixup(c, sym->fixups, sym->value);
            sym->fixups = 0;
        }
    }
}

/* GlobalAlloc - allocate memory from the global heap */
void *GlobalAlloc(ParseContext *c, size_t size)
{
    void *p;
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (c->nextGlobal + size > c->nextLocal)
        Fatal(c, "insufficient memory");
    p = c->nextGlobal;
    c->nextGlobal += size;
    if (c->heapSize - (c->nextLocal - c->nextGlobal) > c->maxHeapUsed)
        c->maxHeapUsed = c->heapSize - (c->nextLocal - c->nextGlobal);
    return p;
}

/* LocalAlloc - allocate memory from the local heap */
void *LocalAlloc(ParseContext *c, size_t size)
{
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (c->nextLocal + size < c->nextGlobal)
        Fatal(c, "insufficient memory");
    c->nextLocal -= size;
    if (c->heapSize - (c->nextLocal - c->nextGlobal) > c->maxHeapUsed)
        c->maxHeapUsed = c->heapSize - (c->nextLocal - c->nextGlobal);
    return c->nextLocal;
}

/* VM_printf - formatted print */
void VM_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_vprintf(fmt, ap);
    va_end(ap);
}

/* Fatal - report a fatal error and exit */
void Fatal(ParseContext *c, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_printf("error: ");
    VM_vprintf(fmt, ap);
    VM_putchar('\n');
    va_end(ap);
    longjmp(c->errorTarget, 1);
}
