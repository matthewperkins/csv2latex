/* 
 * csv2latex.c, copyright © 2002- Benoît Rouits <brouits@free.fr>
 * 
 ********************************************************* 
 * csv2latex translates a .csv file to a LaTex document. *
 *********************************************************
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 only
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301, USA.
 * 
 * see the COPYING file included in the csv2latex package or
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>
#include "version.h"

typedef struct {
	char* tab;	/* actual escapes */
	int size;	/* escape tab len */
} texcape;

typedef struct {
	char block;		/* CSV delimitor if any */
	char sep;		/* CSV separator */
	unsigned int cols;	/* CSV columns */
	unsigned int chars;	/* CSV max data length */
	unsigned int rows;	/* CSV total number of lines */
	char pos;		/* position in cell (align) */
	unsigned int lines;	/* rows per LaTeX tabular */
	unsigned int guess;	/* guess or not the CSV format */
	unsigned int header;	/* put LaTeX document header or not */
	unsigned int bare;	/* only data, no tabular environment*/
	unsigned int red;	/* table reduction level (from 1 to 4)*/
	unsigned int longtable;	/* use package longtable */
	unsigned int escape;	/* escape TeX control chars or not */
	unsigned int repeat;	/* repeat table headers for each LaTeX table section or not */
	unsigned int vlines;	/* insert vertical lines between columns or not */
	unsigned int hlines;	/* insert horizontal lines between rows or not */
	char* clrrow;	/* row graylevel (from 0 to 1) */
	texcape* tex;	/* TeX escapes */
} config;

#define MAXUINT ((unsigned int)(-1))

void rtfm(char* prog) {
   printf("%s translates a csv file to a LaTeX file\n", basename(prog));
   printf("Example: %s january_stats.csv > january_stats.tex\n", basename(prog));
   printf("Usage: %s [--nohead] (LaTeX) no document header: useful for inclusion\n", basename(prog));
   printf("	[--bare] no document header or tabular envir for inculsion\n");
   printf("	[--longtable] (LaTeX) use package longtable: useful for long input\n");
   printf("	[--noescape] (LaTeX) do not escape text: useful for mixed CSV/TeX input\n");
   printf("	[--guess] (CSV) guess separator and block |\n"
		"	[--separator <(c)omma|(s)emicolon|(t)ab|s(p)ace|co(l)on>] (CSV's comma)\n"
		"	[--block <(q)uote|(d)ouble|(n)one>] (CSV) block delimiter (e.g: none)\n");
   printf("	[--lines n] (LaTeX) rows per table: useful for long tabulars\n");
   printf("	[--position <l|c|r>] (LaTeX) text align in cells\n");
   printf("	[--colorrows graylevel] (LaTeX) alternate gray rows (e.g: 0.75)\n");
   printf("	[--reduce level] (LaTeX) reduce table size (e.g: 1)\n");
   printf("	[--repeatheader] (LaTeX) repeat table header (for long tables)\n");
   printf("	[--nohlines] (LaTeX) don't put hline between table rows\n");
   printf("	[--novlines] (LaTeX) don't put vline between columns\n");
   printf("		csv_file.csv\n");
   printf("The \"longtable\" option needs the {longtable} LaTeX package\n");
   printf("The \"colorrows\" option needs the {colortbl} LaTeX package\n");
   printf("The \"reduce\" option needs the {relsize} LaTeX package\n");
   return;
}

config* parseOptions (config* conf, int argc, char **argv) {
/* thx to <vfebvre@lautre.net> */
	int opt;
	int tmp;

#if defined USE_GETOPT
#else
   int longopt_index = 0;
   static struct option long_options[] = {
		{"help",      	0, NULL, 'h'},
		{"guess",     	0, NULL, 'g'},
		{"block",     	1, NULL, 'b'},
		{"lines",     	1, NULL, 'l'},
		{"noescape",  	0, NULL, 'x'},
		{"nohead",    	0, NULL, 'n'},
		{"bare",    	0, NULL, 'm'},
		{"version",   	0, NULL, 'v'},
		{"position",  	1, NULL, 'p'},
		{"separator", 	1, NULL, 's'},
		{"colorrows", 	1, NULL, 'c'},
		{"reduce",    	1, NULL, 'r'},
		{"longtable", 	0, NULL, 't'},
		{"repeatheader",0, NULL, 'e'},
		{"novlines",		0, NULL, 'y'},
		{"nohlines",		0, NULL, 'z'},
		{NULL, 0, NULL, 0} /* marks end-of-list */
	};
#endif
#if defined USE_GETOPT
	while ((opt = getopt (argc, argv, "hvgnxteyz?b:l:p:s:c:r:")) != EOF) {
#else
	while ((opt = getopt_long (argc, argv, "hvgnxteyz?b:l:p:s:c:r:", long_options, &longopt_index)) > 0) {
#endif
		switch (opt) {
			case '?':
			case 'h':
					rtfm (argv[0]);
					exit (EXIT_SUCCESS);
					break;
			case 'g': /* guess the CSV */
					conf->guess=1;
					break;
			case 't': /* use package longtable */ /* thx to <Christof.Bodner@infineon.com> */
					conf->longtable=1;
					break;
			case 'b': /* csv block delimiter */
					if(optarg[0] == 'q')
						conf->block = '\'';
					else if(optarg[0] == 'd')
						conf->block = '"';
					else if(optarg[0] == 'n')
						conf->block = 0; /* no block delimiter */
					break;
			case 'l': /* number of lines per TeX tabulars */
					if(isdigit(optarg[0])) {
						conf->lines=atoi(optarg);
					} else {
						fprintf(stderr,
						"option \"lines\" need a positive integer value\n");
						exit(EXIT_FAILURE);
					}
					break;
			case 'n':
					conf->header=0;
					break;
			case 'm':
					conf->header=0;
					conf->longtable=0;
					conf->bare=1;
					break;
			case 'x':
					conf->escape=0;
					break;
			case 'v': /* version */
					printf ("%s © 2002- Benoît Rouits <brouits@free.fr>\n"
					"\tVersion %s (%s)\n", PACKAGE, VERSION, RELEASE_DATE);
					exit (EXIT_SUCCESS);
					break;
			case 'p': /* LaTeX position in cell */
					conf->pos=optarg[0]; /* position char in cell */
					break;
			case 's': /* csv block separator */
					if(optarg[0] == 'c')
						conf->sep = ',';
					else if(optarg[0] == 's')
						conf->sep = ';';
					else if(optarg[0] == 't')
						conf->sep = '\t';
					else if(optarg[0] == 'p')
						conf->sep = ' ';
					else if(optarg[0] == 'l')
						conf->sep = ':';
					break;
			case 'c': /* color rows (thx to <jcorso@cse.Buffalo.EDU>) */
					if(isdigit(optarg[0])) {
						conf->clrrow = (char*)malloc(strlen(optarg)+1);
						strcpy(conf->clrrow,optarg);
					} else {
						fprintf(stderr,
						"option \"colorrows\" needs a real value between 0 and 1\n");
						exit(EXIT_FAILURE);
					}
					break;
			case 'r': /* reduce table size (original idea thx to <boaz.gezer@gmail.com>) */
		
					if(isdigit(optarg[0])) {
						tmp = atoi(optarg);
						conf->red=(tmp>4)?4:(tmp<0)?0:tmp; /* [1-4] */
					} else {
						fprintf(stderr,
						"option \"reduce\" needs an integer value between 1 and 4\n");
						exit(EXIT_FAILURE);
					}
					break;
			case 'e': /*repeat table header for each table section*/
					conf->repeat=1;
					break;
			case 'y': /*don't draw vlines between columns*/
					conf->vlines=0;
					break;
			case 'z': /*don't draw hlines between rows*/
					conf->hlines=0;
					break;
		}
	}
	return conf;
}
int guessCSV(config* conf, FILE* in) {
/* guess the block delimiter and the csv separator */
	int token;
	
	token=getc(in); /* first char is block delimiter */
	if(token == EOF) {
		fprintf(stderr,"ERROR: emtpy file ?\n");
		return(-1);
	} else if (ispunct(token) || token == ' ') {
	       	/* found first block delimiter, act this way */
		conf->block=token;
		fprintf(stderr,"Guessed '%c' as Block Delimiter\n",
				conf->block);
		/* stream file while token is printable data */
		while((token=getc(in)) != conf->block &&
				token != '\n' &&
				token != EOF)
		{/* getc has been done */}
		if(token == conf->block){
			/* second delimiter : next is separator */
			conf->sep=getc(in);
			fprintf(stderr,"Guessed '%c' as Separator\n",
					conf->sep);
			return(0);
		}else{
			return (-1); /* what else ? */
		}
	}else{ /* no block delimiter, act this way */
		conf->block=0;
		fprintf(stderr,"Guessed No Block Delimiter\n");
		/* stream file while input is not a control char */
		while(isalnum((token=getc(in))) && 
				token != '\n' && 
				token != EOF)
		{/* getc has been done */}
		/* guess CSV separator */
		if(ispunct(token) || token == '\t' || token == ' '){
			conf->sep=token;
			fprintf(stderr,"Guessed %c as Separator\n", conf->sep);
			return(0);
		} else { /* did not found any separator */
			fprintf(stderr,"ERROR: Did not guess any Separator!\n");
			return(-1);
		}
	}
	return(0);
}

void getMaximums(config* conf, FILE* in)  {
/* gets the number of cols and chars of a csv file assuming a separator */
	int token=0;
	unsigned int curcol=0;
	unsigned int curchar=0;
	unsigned int inblock=0;
	/* init */
	conf->chars=0;
	conf->cols=0;
	conf->rows=0;

	while (token != EOF) {
		token=getc(in);
		
		/* EOF ? */
		if (token == EOF) {
			continue;
		}

		/* decide the maximums */
		if (token == '\n') {
			curcol++;
			conf->cols=(conf->cols<curcol)?curcol:conf->cols;
			conf->chars=(conf->chars<curchar)?curchar:conf->chars;
			conf->rows++;
			curcol=0;
			curchar=0;
			inblock=0; /* reset block state */
			continue;
		}

		/* enter/quit a block */
		if (conf->block && token == conf->block) {
			inblock=!inblock;
			continue;
		}

		/* count cols in current line */
		if (token == conf->sep && ((conf->block && !inblock) || !conf->block)) {
			curcol++;
			continue;
		}

		/* count chars in current cell */
		if (token != conf->block && ((conf->block && inblock) || !conf->block)) {
			curchar++;
			continue;
		}
	}
	return;
}

void doTeXsub(config* conf, char newsep, FILE* in, FILE* out) {
/* substitutes CSV sep by LaTeX newsep and some TeX code */
	int token=0;
	int max;
	int numcols;
	unsigned int lines;
	int inblock=0;
	int csvrows;
	int firstrow=1;
	int nosep=0;
	int token1=0;
	int token2=0;
	char headerrow[1000];
	headerrow[0]='\0';

	max=numcols=conf->cols;
	csvrows=conf->rows;
	/* choose infinity when conf->lines is 0 */
	lines=(conf->lines)?conf->lines:MAXUINT;

	while (token!=EOF) {
		token2=token1;	/* second last character, used for detection of quotation marks */
		token1=token;	/* last character, used for detection of quotation marks */
		token=getc(in);
		
		/* EOF ? */
		if (token == EOF) {
			continue;
		}

		/* new line ? */
		if (token == '\n') {
			inblock = 0; /* close block if any */
			/* fill empty cols if any */
			while (numcols > 1) {
				putc(newsep,out);
				numcols--;
			}
			if (!(firstrow && (conf->longtable && conf->repeat))) {
				fprintf(out,"\\\\\n"); /* TeX new line */
			} else { /* first row and repeat and longtable */
				fprintf(out, "%s\\\\\n", headerrow);
			}
			if(conf->hlines) {
				fprintf(out,"\\hline\n"); /* TeX draw hline */
			}
			if (firstrow && (conf->longtable && conf->repeat)) {
				fprintf(out, "%s", "\\endhead\n");
			}
			if(firstrow && conf->repeat) {
				char tmp[12];
				sprintf(tmp,(conf->hlines?"\\\\\n\\hline":"\\\\\n"));
				strcat(headerrow,tmp);
			}
			firstrow=0;
			numcols=max; /* reset numcols */
			lines--;
			csvrows--;
			/* put a colored row or not (alternate) */
			if (conf->clrrow && (lines % 2)) {
				fprintf(out,"\\colorrow ");
			}
			/* if the LaTeX tabular is full create a new one, except if no more row or this is a long table */
			if (!lines && csvrows && !conf->longtable) {
				fprintf(out,"\\end{tabular}\n");
				fprintf(out, "\\newline\n");
				fprintf(out,"\\begin{tabular}{");
				if(conf->vlines) {
					putc('|',out);
				}
				while(numcols--) {
					putc(conf->pos, out);
					if(conf->vlines) {
						putc('|',out);
					}
				}
				fprintf(out, "}\n");
				if(conf->hlines) {
					fprintf(out, "\\hline\n");
				}
				if(conf->repeat && !conf->longtable) {
					fprintf(out,"%s",headerrow);
					putc('\n',out);
				}
				numcols=max;
				lines=(conf->lines)?conf->lines:MAXUINT;
			}
				/* else end of CSV data */
			continue;
		}

		/* if commas in cells */
		/* thx to <florian@heinze.at> */
		if (token == '\"'){
			if (nosep==0)
				nosep=1;
			else 
				nosep=0;
		}
		
		/* new column ? */
		if ((token == conf->sep && ((conf->block && !inblock) || !conf->block)) && nosep == 0) {			
			if (!(firstrow && (conf->longtable && conf->repeat)))
				putc(newsep,out);
			numcols--;
			if(firstrow && conf->repeat)
			{
				char tmp[2];
				tmp[0]=newsep;
				tmp[1]='\0';
				strcat(headerrow,tmp);
			}
			continue;
		}

		/* enter/quit a block ? */
		if (conf->block && token == conf->block) {
			inblock=!inblock;
			continue;
		}

		/* data ? */
		if ((token != conf->block && ((conf->block && inblock) || !conf->block)) && ((token=='\"' && token1=='\"' && token2=='\"') || token!='\"')) {
			/* look for special TeX char to escape */
			/* FIXME: put all that into a subroutine */
			int i=0;
			if (conf->escape)
				for (i=0;i < conf->tex->size; i++) {
					if (token == conf->tex->tab[i]) {
						switch (token) {
							case '\\':
								fprintf(out, "\\textbackslash{}");
								if(firstrow && conf->repeat)
								{
									char tmp[17];
									sprintf(tmp,"\\textbackslash{}");
									strcat(headerrow,tmp);
								}
								break;
							default:
								fprintf(out, "\\%c", token);
								if(firstrow && conf->repeat)
								{
									char tmp[3];
									tmp[0]='\\';
									tmp[1]=token;
									tmp[2]='\0';
									strcat(headerrow,tmp);
								}
								break;
						}
						break; /* there was some escaping */
					}
				}
			/* or print raw char */
			if ( (i >= conf->tex->size) || (!conf->escape) ) {
				if (!(firstrow && (conf->longtable && conf->repeat)))
					putc(token, out); /* do not print the header twice */
				if(firstrow && conf->repeat)
				{
					char tmp[2];
					tmp[0]=token;
					tmp[1]='\0';
					strcat(headerrow,tmp);
				}
			}
			continue;
		}
		/* do nothing if unexpected char: just loop */
	}
	return;
}

void doTeXdoc(config* conf, FILE* in, FILE* out) {
/* prepares the LaTeX tabular layout */
	int maxcols;
	int numcols;
	char* relsize[5] = {"0", "0.5", "1", "2", "4"}; /* LaTeX relsize good values */
	char* tabcolsep[5] = {"0", "0.05", "0.1", "0.2", "0.4"}; /* LaTeX tabcolsep good values */

	numcols=maxcols=conf->cols;
	if(conf->header){
		fprintf(out, "\\documentclass[a4paper]{article}\n");
		fprintf(out, "\\usepackage[T1]{fontenc}\n");
		fprintf(out, "\\usepackage[latin1]{inputenc}\n");
		if (conf->red){
			fprintf(out,"\\usepackage{relsize}\n");
		}
		if (conf->clrrow){
			fprintf(out,"\\usepackage{colortbl}\n");
		}
		if (conf->longtable){
			fprintf(out,"\\usepackage{longtable}\n");
		}
		fprintf(out, "\\begin{document}\n");
	}
	if (conf->clrrow){
		fprintf(out,"\\def\\colorrow{\\rowcolor[gray]{%s}}\n",
				conf->clrrow);
	}
	if (conf->red){
		fprintf(out,"\\relsize{-%s}\n", relsize[conf->red]);
		fprintf(out,"\\addtolength\\tabcolsep{-%sem}\n", tabcolsep[conf->red]);
	}
  	if (!conf->bare){
	  if (conf->longtable){
	    fprintf(out, "\\begin{longtable}{");
	    if(conf->vlines){
	      putc('|',out);
	    }
	  }
	  else{
	    fprintf(out, "\\begin{tabular}{");
	    if(conf->vlines){
	      putc('|',out);
	    }
	  }
	  while(numcols--)
	    {
	      fprintf(out, "%c",conf->pos); /* position in cell */
	      if(conf->vlines)
		putc('|',out);
	    }
	  fprintf(out, "}\n");
	}	
	if(conf->hlines)
	  fprintf(out, "\\hline\n");
	doTeXsub(conf, '&', in, out); /* & is LaTeX separator */

	if (!conf->bare){
	  if (conf->longtable) {
	    fprintf(out, "\\end{longtable}\n");
	  }
	  else {
	    fprintf(out, "\\end{tabular}\n");
	  }
	}
	if (conf->red){
	  fprintf(out,"\\addtolength\\tabcolsep{+%sem}\n", tabcolsep[conf->red]);
	  fprintf(out,"\\relsize{+%s}\n", relsize[conf->red]);
	}
	if(conf->header){
	  fprintf(out, "\\end{document}\n");
	}
	return;
}
		
 int main (int argc, char **argv) {
   FILE* fp;
   config* conf;

   extern int optind, opterr, optopt;

   if(argc == 1){
     rtfm(argv[0]);
     exit(EXIT_SUCCESS);
   }
   conf=(config*)malloc(sizeof(config));
   /* defaults (ensure init): */
   conf->cols=1;        /* CSV: if getMaximums fails */
   conf->rows=0;         /* CSV: must be 0 */
   conf->chars=0;       /* CSV: must be 0 */
   conf->pos='l';       /* usual; LaTeX */
   conf->lines=40;      /* usual; LaTeX */
   conf->guess=0;       /* usual */
   conf->sep=',';       /* default; csv */
   conf->block=0;       /* default; csv */
   conf->header=1;      /* usual; LaTeX */
   conf->escape=1;      /* usual; LaTeX */
   conf->clrrow=NULL;   /* default; LaTeX */
   conf->red=0;         /* default; LaTeX */
   conf->longtable=0;   /* default; without package longtable */
   conf->repeat=0;		/* default; do not repeat the header row */
   conf->vlines=1;		/* default; draw lines between columns */
   conf->hlines=1;		/* default; draw lines between rows */
	
   /* TeX charaters to escape */
   conf->tex=(texcape*)malloc(sizeof(texcape));
   conf->tex->tab = "\\_#$%^&{}~";
   conf->tex->size = 10;
	
   conf=parseOptions(conf, argc, argv);
   fp=fopen(argv[optind],"r");
   if (!fp){
     fprintf(stderr,"Can't open file %s\n", argv[optind]);
     exit(EXIT_FAILURE);
   }
   if(conf->guess){
     if(guessCSV(conf, fp)){
       fprintf(stderr,"Please run again by using -- delimiter (if any) and --separator\n");
       exit(EXIT_FAILURE);
     }
     rewind(fp);
   }
   getMaximums(conf, fp);
   rewind(fp);
	doTeXdoc(conf, fp, stdout);
	free(conf->tex);
	if (conf->clrrow) free(conf->clrrow); conf->clrrow=NULL;
	free(conf);
	fclose(fp);
	return 0;
}
