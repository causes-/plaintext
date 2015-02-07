#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>

#include "util.h"

static char *stylefile = "./style.html";

bool endsto(char *str, int slen, char *str2) {
	if (slen)
		str += slen - strlen(str2);
	else
		str += strlen(str) - strlen(str2);
	return !strncmp(str, str2, strlen(str2));
}

static char *strurlstr(char *str) {
	char *url;

	while (true) {
		url = strstr(str, "http://");
		if (!url)
			url = strstr(str, "https://");
		if (url)
			if (url > str && url[-1] != ' ' && url[-1] != '\n') {
				str = url + strcspn(url, " \n\0");
				continue;
			}
		break;
	}
	return url;
}

int linkurls(char **str) {
	int oldlen, newlen;
	size_t retlen;
	ptrdiff_t sharedlen;
	char *p, *p2;
	char *ret, *r;
	char *linkfmt = "<a href=\"%.*s\">%.*s</a>";
	char *picfmt = "<a href=\"%.*s\"><img src=\"%.*s\"></a>";
	char *fmt;

	if (!strurlstr(*str))
		return 0;

	retlen = strlen(*str);

	for (p = *str; (p2 = strurlstr(p)); p = p2 + oldlen) {
		oldlen = strcspn(p2, " \n\0");
		if (endsto(p2, oldlen, ".png") || endsto(p2, oldlen, ".jpg"))
			fmt = picfmt;
		else
			fmt = linkfmt;
		newlen = snprintf(NULL, 0, fmt, oldlen, p2, oldlen, p2);
		retlen += newlen - oldlen;
	}

	ret = emalloc(retlen + 1);

	for (p = *str, r = ret; (p2 = strurlstr(p)); p = p2 + oldlen) {
		oldlen = strcspn(p2, " \n\0");
		if (endsto(p2, oldlen, ".png") || endsto(p2, oldlen, ".jpg"))
			fmt = picfmt;
		else
			fmt = linkfmt;
		newlen = snprintf(NULL, 0, fmt, oldlen, p2, oldlen, p2);
		sharedlen = p2 - p;
		memcpy(r, p, sharedlen);
		r += sharedlen;
		snprintf(r, newlen + 1, fmt, oldlen, p2, oldlen, p2);
		r += newlen;
	}

	strcpy(r, p);
	free(*str);
	*str = ret;
	return retlen + 1;
}

char *getpage(char *filename) {
	char *p;

	p = filename + strlen(filename);
	while (*--p != '/');
	return p + 1;
}

void printmenu(char *filename) {
	DIR *d;
	struct dirent *dir;
	int len;

	d = opendir(".");
	if (!strcmp(getpage(filename), "index.pt"))
		printf("<a href=\"index.pt\" class=\"activefirst\">index</a>");
	else
		printf("<a href=\"index.pt\" class=\"first\">index</a>");

	while ((dir = readdir(d))) {
		len = strlen(dir->d_name);
		if (endsto(dir->d_name, 0, ".pt")) {
			if (!strcmp(dir->d_name, "index.pt"))
				continue;
			if (!strcmp(getpage(filename), dir->d_name)) {
				printf("<a href=\"%s\" class=\"active\">%.*s</a>",
						dir->d_name, len - 3, dir->d_name);
			} else {
				printf("<a href=\"%s\">%.*s</a>",
						dir->d_name, len - 3, dir->d_name);
			}
		}
	}
	putchar('\n');

	closedir(d);
}

bool isdate(char *str) {
	char *p;
	p = str;
	while (*p && !isalpha(*p))
		p++;
	return !*p;
}

bool isheader(char *str) {
	if (str[0] == '\0')
		return false;
	if (str[strlen(str) - 1] == '.')
		return false;
	if (strstr(str, "http://") || strstr(str, "https://"))
		return false;
	if (strlen(str) > 32)
		return false;
	if (isdate(str))
		return false;
	return true;
}

int main(int argc, char **argv) {
	int y;
	FILE *fp, *stylefp;
	char *line = NULL;
	size_t size = 0;
	ssize_t len;
	char *p;
	bool code;
	int heading;

	if (argc != 2)
		eprintf("usage: plaintext <file>");

	stylefp = efopen(stylefile, "r");

	while ((len = getline(&line, &size, stylefp)) != -1) {
		if (!strcmp(line, "$HTTP_HOST\n"))
			printf("%s\n", getenv("HTTP_HOST"));
		else if (!strcmp(line, "$PAGE\n"))
			printf("%s\n", getpage(argv[1]));
		else if (!strcmp(line, "$PAGES\n"))
			printmenu(argv[1]);
		else if (!strcmp(line, "$CONTENT\n"))
			break;
		else
			printf("%s", line);
	}

	heading = 2;
	code = false;
	fp = efopen(argv[1], "r");
	for (y = 0; (len = getline(&line, &size, fp)) != -1; y++) {
		size = linkurls(&line);
		p = line;
		if (p[len - 1] == '\n')
			p[len - 1] = '\0';
		else if (p[strlen(p) - 1] == '\n')
			p[strlen(p) - 1] = '\0';

		if (!code && !strncmp(p, "$ ", 2)) {
			puts("<pre>");
			code = 1;
		} else if (code && !!strncmp(p, "$ ", 2)) {
			puts("</pre>");
			code = 0;
		}

		if (!heading && isheader(p))
			heading = 3;
		if (heading && !code)
			printf("<h%d>", heading);
		else if (!strncmp(p, "* ", 2)) {
			printf("<li>");
			p += 2;
		}

		printf("%s", p);

		if (heading && !code)
			printf("</h%d>", heading);
		if (!code && !heading)
			printf("<br />");
		putchar('\n');

		heading = 0;
		if (p[0] == '\0')
			heading = 2;
	}
	if (code)
		puts("</pre>");
	fclose(fp);

	while ((len = getline(&line, &size, stylefp)) != -1)
		printf("%s", line);

	free(line);
	fclose(stylefp);
	return EXIT_SUCCESS;
}
