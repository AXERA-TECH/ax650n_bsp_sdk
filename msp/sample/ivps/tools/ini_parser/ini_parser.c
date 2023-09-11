/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include "ini_parser.h"

/*---------------------------- Defines -------------------------------------*/
#define ASCIILINESZ (1024)
#define INI_INVALID_KEY ((char *)-1)

/*---------------------------------------------------------------------------
                        Private to this module
 ---------------------------------------------------------------------------*/
/**
 * This enum stores the status for each parsed line (internal use only).
 */
typedef enum _line_status_
{
    LINE_UNPROCESSED,
    LINE_ERROR,
    LINE_EMPTY,
    LINE_COMMENT,
    LINE_SECTION,
    LINE_VALUE
} line_status;

char *sprint_fmt(char *dst, int len, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(dst, len, format, args); /* do check return value */
    va_end(args);
    return dst;
}

char *strcat_unfmt(char *s, const char *s1, const char *s2)
{
    s[0] = '\0';
    if (strlen(s1) > 0)
    {
        strcat(s, s1);
    }
    if (strlen(s2) > 0)
    {
        strcat(s, s2);
    }
    return s;
}

char *strcat_fmt(char *s, const char *s1, const char *format, ...)
{
    va_list args;
    const int len = 256;
    char s2[len];

    va_start(args, format);
    vsnprintf(s2, len, format, args); /* do check return value */
    va_end(args);

    s[0] = '\0';
    if (strlen(s1) > 0)
    {
        strcat(s, s1);
    }
    if (strlen(s2) > 0)
    {
        strcat(s, s2);
    }

    return s;
}

void print_arr_f32(AX_F32 *arr, int ele_num, const char *arr_name)
{
    int i;
    const int num_each_line = 10;
    char key[256];

    printf("\t %-40s :", strcat_fmt(key, arr_name, " (%d)", ele_num));
    for (i = 0; i < ele_num; i++)
    {
        if ((i > 0) && (0 == i % num_each_line))
        {
            printf("\n\t %-40s  ", "");
        }
        printf(" %f", arr[i]);
    }
    printf("\n");
}

void print_arr_s32(AX_S32 *arr, int ele_num, const char *arr_name)
{
    int i;
    const int num_each_line = 10;
    char key[256];

    printf("\t %-40s :", strcat_fmt(key, arr_name, " (%d)", ele_num));
    for (i = 0; i < ele_num; i++)
    {
        if ((i > 0) && (0 == i % num_each_line))
        {
            printf("\n\t %-40s  ", "");
        }
        printf(" %d", arr[i]);
    }
    printf("\n");
}

void print_arr_u8(AX_U8 *arr, int ele_num, const char *arr_name)
{
    int i;
    const int num_each_line = 10;
    char key[256];

    printf("\t %-40s :", strcat_fmt(key, arr_name, " (%d)", ele_num));
    for (i = 0; i < ele_num; i++)
    {
        if ((i > 0) && (0 == i % num_each_line))
        {
            printf("\n\t %-40s  ", "");
        }
        printf(" %x", arr[i]);
    }
    printf("\n");
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Convert a string to lowercase.
  @param    in   String to convert.
  @param    out Output buffer.
  @param    len Size of the out buffer.
  @return   ptr to the out buffer or NULL if an error occured.

  This function convert a string into lowercase.
  At most len - 1 elements of the input string will be converted.
 */
/*--------------------------------------------------------------------------*/
static const char *strlwc(const char *in, char *out, unsigned len)
{
    unsigned i;

    if (in == NULL || out == NULL || len == 0)
        return NULL;
    i = 0;
    while (i < len - 1 && in[i] != '\0')
    {
        out[i] = (char)tolower((int)in[i]);
        i++;
    }
    out[i] = '\0';
    return out;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Remove blanks at the beginning and the end of a string.
  @param    str  String to parse and alter.
  @return   unsigned New size of the string.
 */
/*--------------------------------------------------------------------------*/
unsigned strstrip(char *s)
{
    char *last = NULL;
    char *dest = s;

    if (s == NULL)
        return 0;

    last = s + strlen(s);
    while (isspace((int)*s) && *s)
        s++;
    while (last > s)
    {
        if (!isspace((int)*(last - 1)))
            break;
        last--;
    }
    *last = (char)0;

    memmove(dest, s, last - s + 1);
    return last - s;
}

unsigned strstrip_char(char *s, char c)
{
    char *last = NULL;
    char *dest = s;

    if (s == NULL)
        return 0;

    last = s + strlen(s);
    while (*s == c && *s)
        s++;
    while (last > s)
    {
        if (!(c == *(last - 1)))
            break;
        last--;
    }
    *last = (char)0;

    memmove(dest, s, last - s + 1);
    return last - s;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Default error callback for iniparser: wraps `fprintf(stderr, ...)`.
 */
/*--------------------------------------------------------------------------*/
static int default_error_callback(const char *format, ...)
{
    int ret;
    va_list argptr;
    va_start(argptr, format);
    ret = vfprintf(stderr, format, argptr);
    va_end(argptr);
    return ret;
}

static int (*iniparser_error_callback)(const char *, ...) = default_error_callback;

/*-------------------------------------------------------------------------*/
/**
  @brief    Configure a function to receive the error messages.
  @param    errback  Function to call.

  By default, the error will be printed on stderr. If a null pointer is passed
  as errback the error callback will be switched back to default.
 */
/*--------------------------------------------------------------------------*/
void iniparser_set_error_callback(int (*errback)(const char *, ...))
{
    if (errback)
    {
        iniparser_error_callback = errback;
    }
    else
    {
        iniparser_error_callback = default_error_callback;
    }
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get number of sections in a dictionary
  @param    d   Dictionary to examine
  @return   int Number of sections found in dictionary

  This function returns the number of sections found in a dictionary.
  The test to recognize sections is done on the string stored in the
  dictionary: a section name is given as "section" whereas a key is
  stored as "section:key", thus the test looks for entries that do not
  contain a colon.

  This clearly fails in the case a section name contains a colon, but
  this should simply be avoided.

  This function returns -1 in case of error.
 */
/*--------------------------------------------------------------------------*/
int iniparser_getnsec(const INI_DICT *d)
{
    int i;
    int nsec;

    if (d == NULL)
        return -1;
    nsec = 0;
    for (i = 0; i < d->size; i++)
    {
        if (d->key[i] == NULL)
            continue;
        if (strchr(d->key[i], ':') == NULL)
        {
            nsec++;
        }
    }
    return nsec;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get name for section n in a dictionary.
  @param    d   Dictionary to examine
  @param    n   Section number (from 0 to nsec-1).
  @return   Pointer to char string

  This function locates the n-th section in a dictionary and returns
  its name as a pointer to a string statically allocated inside the
  dictionary. Do not free or modify the returned string!

  This function returns NULL in case of error.
 */
/*--------------------------------------------------------------------------*/
const char *iniparser_getsecname(const INI_DICT *d, int n)
{
    int i;
    int foundsec;

    if (d == NULL || n < 0)
        return NULL;
    foundsec = 0;
    for (i = 0; i < d->size; i++)
    {
        if (d->key[i] == NULL)
            continue;
        if (strchr(d->key[i], ':') == NULL)
        {
            foundsec++;
            if (foundsec > n)
                break;
        }
    }
    if (foundsec <= n)
    {
        return NULL;
    }
    return d->key[i];
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Dump a dictionary to an opened file pointer.
  @param    d   Dictionary to dump.
  @param    f   Opened file pointer to dump to.
  @return   void

  This function prints out the contents of a dictionary, one element by
  line, onto the provided file pointer. It is OK to specify @c stderr
  or @c stdout as output files. This function is meant for debugging
  purposes mostly.
 */
/*--------------------------------------------------------------------------*/
void iniparser_dump(const INI_DICT *d, FILE *f)
{
    int i;

    if (d == NULL || f == NULL)
        return;
    for (i = 0; i < d->size; i++)
    {
        if (d->key[i] == NULL)
            continue;
        if (d->val[i] != NULL)
        {
            fprintf(f, "[%s]=[%s]\n", d->key[i], d->val[i]);
        }
        else
        {
            fprintf(f, "[%s]=UNDEF\n", d->key[i]);
        }
    }
    return;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Save a dictionary to a loadable ini file
  @param    d   Dictionary to dump
  @param    f   Opened file pointer to dump to
  @return   void

  This function dumps a given dictionary into a loadable ini file.
  It is Ok to specify @c stderr or @c stdout as output files.
 */
/*--------------------------------------------------------------------------*/
void iniparser_dump_ini(const INI_DICT *d, FILE *f)
{
    int i;
    int nsec;
    const char *secname;

    if (d == NULL || f == NULL)
        return;

    nsec = iniparser_getnsec(d);
    if (nsec < 1)
    {
        /* No section in file: dump all keys as they are */
        for (i = 0; i < d->size; i++)
        {
            if (d->key[i] == NULL)
                continue;
            fprintf(f, "%s = %s\n", d->key[i], d->val[i]);
        }
        return;
    }
    for (i = 0; i < nsec; i++)
    {
        secname = iniparser_getsecname(d, i);
        iniparser_dumpsection_ini(d, secname, f);
    }
    fprintf(f, "\n");
    return;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Save a dictionary section to a loadable ini file
  @param    d   Dictionary to dump
  @param    s   Section name of dictionary to dump
  @param    f   Opened file pointer to dump to
  @return   void

  This function dumps a given section of a given dictionary into a loadable ini
  file.  It is Ok to specify @c stderr or @c stdout as output files.
 */
/*--------------------------------------------------------------------------*/
void iniparser_dumpsection_ini(const INI_DICT *d, const char *s, FILE *f)
{
    int j;
    char keym[ASCIILINESZ + 1];
    int seclen;

    if (d == NULL || f == NULL)
        return;
    if (!iniparser_find_entry(d, s))
        return;

    seclen = (int)strlen(s);
    fprintf(f, "\n[%s]\n", s);
    sprintf(keym, "%s:", s);
    for (j = 0; j < d->size; j++)
    {
        if (d->key[j] == NULL)
            continue;
        if (!strncmp(d->key[j], keym, seclen + 1))
        {
            fprintf(f,
                    "%-30s = %s\n",
                    d->key[j] + seclen + 1,
                    d->val[j] ? d->val[j] : "");
        }
    }
    fprintf(f, "\n");
    return;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the number of keys in a section of a dictionary.
  @param    d   Dictionary to examine
  @param    s   Section name of dictionary to examine
  @return   Number of keys in section
 */
/*--------------------------------------------------------------------------*/
int iniparser_getsecnkeys(const INI_DICT *d, const char *s)
{
    int seclen, nkeys;
    char keym[ASCIILINESZ + 1];
    int j;

    nkeys = 0;

    if (d == NULL)
        return nkeys;
    if (!iniparser_find_entry(d, s))
        return nkeys;

    seclen = (int)strlen(s);
    strlwc(s, keym, sizeof(keym));
    keym[seclen] = ':';

    for (j = 0; j < d->size; j++)
    {
        if (d->key[j] == NULL)
            continue;
        if (!strncmp(d->key[j], keym, seclen + 1))
            nkeys++;
    }

    return nkeys;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the number of keys in a section of a dictionary.
  @param    d    Dictionary to examine
  @param    s    Section name of dictionary to examine
  @param    keys Already allocated array to store the keys in
  @return   The pointer passed as `keys` argument or NULL in case of error

  This function queries a dictionary and finds all keys in a given section.
  The keys argument should be an array of pointers which size has been
  determined by calling `iniparser_getsecnkeys` function prior to this one.

  Each pointer in the returned char pointer-to-pointer is pointing to
  a string allocated in the dictionary; do not free or modify them.
 */
/*--------------------------------------------------------------------------*/
const char **iniparser_getseckeys(const INI_DICT *d, const char *s, const char **keys)
{
    int i, j, seclen;
    char keym[ASCIILINESZ + 1];

    if (d == NULL || keys == NULL)
        return NULL;
    if (!iniparser_find_entry(d, s))
        return NULL;

    seclen = (int)strlen(s);
    strlwc(s, keym, sizeof(keym));
    keym[seclen] = ':';

    i = 0;

    for (j = 0; j < d->size; j++)
    {
        if (d->key[j] == NULL)
            continue;
        if (!strncmp(d->key[j], keym, seclen + 1))
        {
            keys[i] = d->key[j];
            i++;
        }
    }

    return keys;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key
  @param    d       Dictionary to search
  @param    key     Key string to look for
  @param    def     Default value to return if key not found.
  @return   pointer to statically allocated character string

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the pointer passed as 'def' is returned.
  The returned char pointer is pointing to a string allocated in
  the dictionary, do not free or modify it.
 */
/*--------------------------------------------------------------------------*/
const char *ini_get_string(const INI_DICT *d, const char *key, const char *def)
{
    const char *lc_key;
    const char *sval;
    char tmp_str[ASCIILINESZ + 1];

    if (d == NULL || key == NULL)
        return def;

    lc_key = strlwc(key, tmp_str, sizeof(tmp_str));
    sval = dictionary_get(d, lc_key, def);
    return sval;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key, convert to an long int
  @param    d Dictionary to search
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   long integer

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.

  Supported values for integers include the usual C notation
  so decimal, octal (starting with 0) and hexadecimal (starting with 0x)
  are supported. Examples:

  "42"      ->  42
  "042"     ->  34 (octal -> decimal)
  "0x42"    ->  66 (hexa  -> decimal)

  Warning: the conversion may overflow in various ways. Conversion is
  totally outsourced to strtol(), see the associated man page for overflow
  handling.

  Credits: Thanks to A. Becker for suggesting strtol()
 */
/*--------------------------------------------------------------------------*/
long int iniparser_getlongint(const INI_DICT *d, const char *key, long int notfound)
{
    const char *str;

    str = ini_get_string(d, key, INI_INVALID_KEY);
    if (str == INI_INVALID_KEY)
        return notfound;
    return strtol(str, NULL, 0);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key, convert to an int
  @param    d Dictionary to search
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   integer

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.

  Supported values for integers include the usual C notation
  so decimal, octal (starting with 0) and hexadecimal (starting with 0x)
  are supported. Examples:

  "42"      ->  42
  "042"     ->  34 (octal -> decimal)
  "0x42"    ->  66 (hexa  -> decimal)

  Warning: the conversion may overflow in various ways. Conversion is
  totally outsourced to strtol(), see the associated man page for overflow
  handling.

  Credits: Thanks to A. Becker for suggesting strtol()
 */
/*--------------------------------------------------------------------------*/
int ini_getint(const INI_DICT *d, const char *key, int notfound)
{
    return (int)iniparser_getlongint(d, key, notfound);
}

void arr_f32_from_str(AX_F32 *arr, int ele_num, const char *str)
{
    char *str_cp = xstrdup(str);
    const char deli[2] = ",";

    char *ele_str;
    int ele_id;

    strstrip_char(str_cp, '[');
    strstrip_char(str_cp, ']');

    ele_id = 0;
    ele_str = strtok(str_cp, deli);
    while (ele_str != NULL)
    {
        strstrip(ele_str);

        if (ele_id < ele_num)
        {
            arr[ele_id] = atof(ele_str);
        }

        ele_str = strtok(NULL, deli);
        ele_id++;
    }

    AX_ASSERT(ele_id == ele_num, "%s %d: Failed to get matrix from str: %s, ele_id: %d, ele_num: %d \n",
              __FILENAME__, __LINE__, str, ele_id, ele_num);
    AX_FREE(str_cp, "%s %d: Warning Pt str_cp already free !\n", __FILENAME__, __LINE__);
}

void arr_int_from_str(int *arr, int ele_num, const char *str)
{
    char *str_cp = xstrdup(str);
    const char deli[2] = ",";

    char *ele_str;
    int ele_id;

    strstrip_char(str_cp, '[');
    strstrip_char(str_cp, ']');

    ele_id = 0;
    ele_str = strtok(str_cp, deli);
    while (ele_str != NULL)
    {
        strstrip(ele_str);

        if (ele_id < ele_num)
        {
            arr[ele_id] = atoi(ele_str);
        }

        ele_str = strtok(NULL, deli);
        ele_id++;
    }

    AX_ASSERT(ele_id == ele_num, "%s %d: Failed to get arr from str: %s, ele_id: %d, ele_num: %d \n",
              __FILENAME__, __LINE__, str, ele_id, ele_num);
    AX_FREE(str_cp, "%s %d: Warning Pt str_cp already free !\n", __FILENAME__, __LINE__);
}

void ini_get_arrint(int *val, int ele_num, const INI_DICT *d, const char *key, int notfound)
{
    const char *str;

    str = ini_get_string(d, key, INI_INVALID_KEY);
    if (str == INI_INVALID_KEY)
    {
        for (int i = 0; i < ele_num; i++)
            val[i] = notfound;
    }
    else
    {
        arr_int_from_str(val, ele_num, str);
    }
}

void ini_get_arrf32(AX_F32 *val, int ele_num, const INI_DICT *d, const char *key, AX_F32 notfound)
{
    const char *str;

    str = ini_get_string(d, key, INI_INVALID_KEY);
    if (str == INI_INVALID_KEY)
    {
        for (int i = 0; i < ele_num; i++)
            val[i] = notfound;
    }
    else
    {
        arr_f32_from_str(val, ele_num, str);
    }
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key, convert to a double
  @param    d Dictionary to search
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   double

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.
 */
/*--------------------------------------------------------------------------*/
double ini_getdouble(const INI_DICT *d, const char *key, double notfound)
{
    const char *str;

    str = ini_get_string(d, key, INI_INVALID_KEY);
    if (str == INI_INVALID_KEY)
        return notfound;
    return atof(str);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key, convert to a boolean
  @param    d Dictionary to search
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   integer

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.

  A true boolean is found if one of the following is matched:

  - A string starting with 'y'
  - A string starting with 'Y'
  - A string starting with 't'
  - A string starting with 'T'
  - A string starting with '1'

  A false boolean is found if one of the following is matched:

  - A string starting with 'n'
  - A string starting with 'N'
  - A string starting with 'f'
  - A string starting with 'F'
  - A string starting with '0'

  The notfound value returned if no boolean is identified, does not
  necessarily have to be 0 or 1.
 */
/*--------------------------------------------------------------------------*/
int iniparser_getboolean(const INI_DICT *d, const char *key, int notfound)
{
    int ret;
    const char *c;

    c = ini_get_string(d, key, INI_INVALID_KEY);
    if (c == INI_INVALID_KEY)
        return notfound;
    if (c[0] == 'y' || c[0] == 'Y' || c[0] == '1' || c[0] == 't' || c[0] == 'T')
    {
        ret = 1;
    }
    else if (c[0] == 'n' || c[0] == 'N' || c[0] == '0' || c[0] == 'f' || c[0] == 'F')
    {
        ret = 0;
    }
    else
    {
        ret = notfound;
    }
    return ret;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Finds out if a given entry exists in a dictionary
  @param    ini     Dictionary to search
  @param    entry   Name of the entry to look for
  @return   integer 1 if entry exists, 0 otherwise

  Finds out if a given entry exists in the dictionary. Since sections
  are stored as keys with NULL associated values, this is the only way
  of querying for the presence of sections in a dictionary.
 */
/*--------------------------------------------------------------------------*/
int iniparser_find_entry(const INI_DICT *ini, const char *entry)
{
    int found = 0;
    if (ini_get_string(ini, entry, INI_INVALID_KEY) != INI_INVALID_KEY)
    {
        found = 1;
    }
    return found;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Set an entry in a dictionary.
  @param    ini     Dictionary to modify.
  @param    entry   Entry to modify (entry name)
  @param    val     New value to associate to the entry.
  @return   int 0 if Ok, -1 otherwise.

  If the given entry can be found in the dictionary, it is modified to
  contain the provided value. If it cannot be found, the entry is created.
  It is Ok to set val to NULL.
 */
/*--------------------------------------------------------------------------*/
int iniparser_set(INI_DICT *ini, const char *entry, const char *val)
{
    char tmp_str[ASCIILINESZ + 1];
    return dictionary_set(ini, strlwc(entry, tmp_str, sizeof(tmp_str)), val);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete an entry in a dictionary
  @param    ini     Dictionary to modify
  @param    entry   Entry to delete (entry name)
  @return   void

  If the given entry can be found, it is deleted from the dictionary.
 */
/*--------------------------------------------------------------------------*/
void iniparser_unset(INI_DICT *ini, const char *entry)
{
    char tmp_str[ASCIILINESZ + 1];
    dictionary_unset(ini, strlwc(entry, tmp_str, sizeof(tmp_str)));
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Load a single line from an INI file
  @param    input_line  Input line, may be concatenated multi-line input
  @param    section     Output space to store section
  @param    key         Output space to store key
  @param    value       Output space to store value
  @return   line_status value
 */
/*--------------------------------------------------------------------------*/
static line_status iniparser_line(const char *input_line,
                                  char *section, char *key, char *value)
{
    line_status sta;
    char *line = NULL;
    size_t len;

    line = xstrdup(input_line);
    len = strstrip(line);

    sta = LINE_UNPROCESSED;
    if (len < 1)
    {
        /* Empty line */
        sta = LINE_EMPTY;
    }
    else if (line[0] == '#' || line[0] == ';')
    {
        /* Comment line */
        sta = LINE_COMMENT;
    }
    else if (line[0] == '[' && line[len - 1] == ']')
    {
        /* Section name */
        sscanf(line, "[%[^]]", section);
        strstrip(section);
        strlwc(section, section, len);
        sta = LINE_SECTION;
    }
    else if (sscanf(line, "%[^=] = \"%[^\"]\"", key, value) == 2 || sscanf(line, "%[^=] = '%[^\']'", key, value) == 2)
    {
        /* Usual key=value with quotes, with or without comments */
        strstrip(key);
        strlwc(key, key, len);
        /* Don't strip spaces from values surrounded with quotes */
        sta = LINE_VALUE;
    }
    else if (sscanf(line, "%[^=] = %[^;#]", key, value) == 2)
    {
        /* Usual key=value without quotes, with or without comments */
        strstrip(key);
        strlwc(key, key, len);
        strstrip(value);
        /*
         * sscanf cannot handle '' or "" as empty values
         * this is done here
         */
        if (!strcmp(value, "\"\"") || (!strcmp(value, "''")))
        {
            value[0] = 0;
        }
        sta = LINE_VALUE;
    }
    else if (sscanf(line, "%[^=] = %[;#]", key, value) == 2 || sscanf(line, "%[^=] %[=]", key, value) == 2)
    {
        /*
         * Special cases:
         * key=
         * key=;
         * key=#
         */
        strstrip(key);
        strlwc(key, key, len);
        value[0] = 0;
        sta = LINE_VALUE;
    }
    else
    {
        /* Generate syntax error */
        sta = LINE_ERROR;
    }

    free(line);
    return sta;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Parse an ini file and return an allocated dictionary object
  @param    ininame Name of the ini file to read.
  @return   Pointer to newly allocated dictionary

  This is the parser for ini files. This function is called, providing
  the name of the file to be read. It returns a dictionary object that
  should not be accessed directly, but through accessor functions
  instead.

  The returned dictionary must be freed using iniparser_freedict().
 */
/*--------------------------------------------------------------------------*/
INI_DICT *iniparser_load(const char *ininame)
{
    FILE *in;

    char line[ASCIILINESZ + 1];
    char section[ASCIILINESZ + 1];
    char key[ASCIILINESZ + 1];
    char tmp[(ASCIILINESZ * 2) + 2];
    char val[ASCIILINESZ + 1];

    int last = 0;
    int len;
    int lineno = 0;
    int errs = 0;
    int mem_err = 0;

    INI_DICT *dict;

    if ((in = fopen(ininame, "r")) == NULL)
    {
        iniparser_error_callback("iniparser: cannot open %s\n", ininame);
        return NULL;
    }

    dict = dictionary_new(0);
    if (!dict)
    {
        fclose(in);
        return NULL;
    }

    memset(line, 0, ASCIILINESZ);
    memset(section, 0, ASCIILINESZ);
    memset(key, 0, ASCIILINESZ);
    memset(val, 0, ASCIILINESZ);
    last = 0;

    while (fgets(line + last, ASCIILINESZ - last, in) != NULL)
    {
        lineno++;
        len = (int)strlen(line) - 1;
        if (len <= 0)
            continue;

        /* Safety check against buffer overflows */
        if (line[len] != '\n' && !feof(in))
        {
            iniparser_error_callback("iniparser: input line too long in %s (%d)\n", ininame, lineno);
            dictionary_del(dict);
            fclose(in);
            return NULL;
        }

        /* Get rid of \n and spaces at end of line */
        while ((len >= 0) &&
               ((line[len] == '\n') || (isspace(line[len]))))
        {
            line[len] = 0;
            len--;
        }
        if (len < 0)
        { /* Line was entirely \n and/or spaces */
            len = 0;
        }

        /* Detect multi-line */
        if (line[len] == '\\')
        {
            /* Multi-line value */
            last = len;
            continue;
        }
        else
        {
            last = 0;
        }

        switch (iniparser_line(line, section, key, val))
        {
        case LINE_EMPTY:
        case LINE_COMMENT:
            break;

        case LINE_SECTION:
            mem_err = dictionary_set(dict, section, NULL);
            break;

        case LINE_VALUE:
            sprintf(tmp, "%s:%s", section, key);
            mem_err = dictionary_set(dict, tmp, val);
            break;

        case LINE_ERROR:
            iniparser_error_callback("iniparser: syntax error in %s (%d):\n-> %s\n",
                                     ininame, lineno, line);
            errs++;
            break;

        default:
            break;
        }
        memset(line, 0, ASCIILINESZ);
        last = 0;
        if (mem_err < 0)
        {
            iniparser_error_callback("iniparser: memory allocation failure\n");
            break;
        }
    }
    if (errs)
    {
        dictionary_del(dict);
        dict = NULL;
    }
    fclose(in);
    return dict;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Free all memory associated to an ini dictionary
  @param    d Dictionary to free
  @return   void

  Free all memory associated to an ini dictionary.
  It is mandatory to call this function before the dictionary object
  gets out of the current context.
 */
/*--------------------------------------------------------------------------*/
void iniparser_freedict(INI_DICT *d)
{
    dictionary_del(d);
}
/*--------------------------------------------------------------------------*/
/**
linux platform has no _splitpath and filepath operation
if not win32, define here
**/
#ifndef WIN32
void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext)
{
    char *p_whole_name;

    drive[0] = '\0';
    if (NULL == path)
    {
        dir[0] = '\0';
        fname[0] = '\0';
        ext[0] = '\0';
        return;
    }

    if ('/' == path[strlen(path)])
    {
        strcpy(dir, path);
        fname[0] = '\0';
        ext[0] = '\0';
        return;
    }

    p_whole_name = (char *)rindex(path, '/');
    if (NULL != p_whole_name)
    {
        p_whole_name++;
        _split_whole_name(p_whole_name, fname, ext);

        snprintf(dir, p_whole_name - path, "%s", path);
    }
    else
    {
        _split_whole_name(path, fname, ext);
        dir[0] = '\0';
    }
}

void _split_whole_name(const char *whole_name, char *fname, char *ext)
{
    char *p_ext;

    p_ext = (char *)rindex(whole_name, '.');
    if (NULL != p_ext)
    {
        strcpy(ext, p_ext);
        snprintf(fname, p_ext - whole_name + 1, "%s", whole_name);
    }
    else
    {
        ext[0] = '\0';
        strcpy(fname, whole_name);
    }
}
#endif
