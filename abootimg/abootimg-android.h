#include <assert.h>

FILE *fmemopen (void *buf, size_t size, const char *opentype)
{
FILE *f;

assert(strcmp(opentype, "r") == 0);

f = tmpfile();
fwrite(buf, 1, size, f);
rewind(f);

return f;
}

static ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
     size_t ret = 0;

     if (!lineptr || !n || !stream)
	     return -1;

     if(!*lineptr) {
	     *n = 128;
	     *lineptr = (char*)malloc(*n);
	     if(!*lineptr)
		     return -1;
     }

     while(!feof(stream) && !ferror(stream)) {
	     int c;
	     if(ret == *n) {
	             *n += 128;
		     *lineptr = (char*)realloc(*lineptr, *n);
		     if(!*lineptr) {
			     *n = 0;
			     return -1;
		     }
	     }
	     c = fgetc(stream);
	     if(c == EOF)
		     break;
	     *lineptr[ret++] = c;
	     if(c == '\n')
		     break;
     }
     *lineptr[ret] = 0;
     return ret;
}
