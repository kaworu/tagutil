const char *
getprogname(void)
{
    extern char *__progname;

	return (__progname);
}
