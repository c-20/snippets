#if 0
src=Program.c
out=Program
gcc=C:/PROGRA~1/Git/mingw33/bin/gcc.exe
$gcc -o $out $src
exit 0
#else
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PROGHALFBONE   "C:\\Program"
#define PROGHALFLONE   "/c/Program"
#define PROGHALFDONE   "C:/Program"
#define PROGHALFBTWO   "Files\\"
#define PROGHALFDTWO   "Files/"
#define PROGHALFTWOLEN 6
#define PROGWHOLE      "C:/PROGRA~1/"
#define PROGWHOLELEN   strlen(PROGWHOLE)
#define PROGLONG       PROGHALFDONE  " "  PROGHALFDTWO
#define PROGBLONG      PROGHALFDONE "\\ " PROGHALFDTWO
#define PROGLONGLEN    strlen(PROGLONG)

#define DOCSTRIPBONE   "C:\\Documents"
#define DOCSTRIPLONE   "/c/Documents"
#define DOCSTRIPDONE   "C:/Documents"
#define DOCSTRIPTWO    "and"
#define DOCSTRIPBTHREE "Settings\\"
#define DOCSTRIPDTHREE "Settings/"
#define DOCSTRIPTHREELEN 9
#define DOCSWHOLE      "C:/DOCUME~1/"
#define DOCSWHOLELEN   strlen(DOCSWHOLE)
#define DOCSLONG       DOCSTRIPDONE  " "  DOCSTRIPTWO  " "  DOCSTRIPDTHREE
#define DOCSBLONG      DOCSTRIPDONE "\\ " DOCSTRIPTWO "\\ " DOCSTRIPDTHREE
#define DOCSLONGLEN    strlen(DOCSLONG)

#define SHELLPROGRAM   "/usr/bin/sh.exe"
#define SRCFOLDER      "Git"
#define SHELLWHOLE     PROGWHOLE    SRCFOLDER SHELLPROGRAM
#define SHELLMATCH     PROGHALFDTWO SRCFOLDER SHELLPROGRAM
#define BINARYMATCH    PROGHALFDONE
#define BINEXEMATCH    PROGHALFDONE ".exe"
#define BINARYBMATCH   PROGHALFBONE
#define BINEXEBMATCH   PROGHALFBONE ".exe"
#define GCCFOLDER      "mingw33"
#define MINGWMATCH     "/" GCCFOLDER "/"
#define MINGWMATCHLEN  9
#define MINGWPREFIX    PROGWHOLE   SRCFOLDER MINGWMATCH

int main(int argc, char **argv) {
//  char *const newenvp[1] = { NULL }; // "CC=/mingw33/bin/gcc.exe", 
//  char newargv[1024]; // 1K buffer limit w/o boundary checks !
//  newargv[0] = 's';
//  newargv[1] = 'h';
//  newargv[2] = '\0';
//  newargv[3] = '-';
//  newargv[4] = 'c';
//  newargv[5] = '\0';
//  char *command = &newargv[6];
  char csilence = 0;
  FILE *silence = fopen("C:/Silence", "r");
  if (silence) {
    fclose(silence);
    csilence = 1;
  } // silence output if C:/Silence exists and can be opened for reading
  char command[1024999]; // 1Mish buffer limit w/o boundary checks !
  char stop = '\0'; // hopefully prevent string overrun on read (write should segfault)
  command[0] = '\0';
  strcat(command, "\""); // starting quote, all programs are run as sh scripts
//  sprintf(command, argv[0]);
  // This one says the specified path cannot be found
  //sprintf(command, "/usr/bin/sh.exe -c '");
  // This one instigates substantial loopback feedback
  //sprintf(command, "C:/Program Files/Git/usr/bin/sh.exe -c '");
  // This one surely works
  //sprintf(command, SHELLWHOLE " -c");
  if (!csilence) {
    if (argc < 1) { printf("NO PROGRAM\n"); return 0; }
    else if ((strcmp(BINARYMATCH, argv[0]) != 0) && (strcmp(BINEXEMATCH, argv[0]) != 0) // )
          && (strcmp(BINARYBMATCH, argv[0]) != 0) && (strcmp(BINEXEBMATCH, argv[0]) != 0))
      { printf("THIS (%s) IS NOT C:/Program\n", argv[0]); } // todo: allow PROGRAM.EXE ? nah
    else { printf("THIS IS C:/Program........... %s\n", argv[1]); }
  }
  int argi = 1;
  char isshell = 0;
  if (argc > argi + 1 && strcmp(SHELLMATCH, argv[argi]) == 0 && strcmp("-c", argv[argi + 1]) == 0)
    { argi += 2; isshell = 1; } // printf("Shell SHELL\n"); }
  else if (argc > argi && strncmp(PROGHALFDTWO, argv[argi], PROGHALFTWOLEN) == 0) {
    char *localprogram = &argv[argi][PROGHALFTWOLEN];
    strcat(command, PROGWHOLE);
    strcat(command, localprogram);
    strcat(command, " "); // space before first param
    argi += 1; // this is a Program Files program
    // printf("Shelled PROGRAM\n");
    // do not pass literal quotes ... also do not expect an && (host sh should handle)
  } else if (!csilence) { printf("Program SHELL\n"); } 
  //else { printf("THIS IS NOT SHELL OR A PROGRAM FILES PROGRAM\n"); } // BUT IT WILL BE return 0; }
//  char *commandend = command;
  //if (isshell) { strcat(command, "\""); } // quotes around shell params (should be all one param!)
  //strcat(command, "\""); // add a start quote
  char *nextvar = NULL;
  while (argi < argc) {
    if (nextvar != NULL) { strcat(command, " "); } // spaces between params from input
    nextvar = &argv[argi][0];
//    printf("nextvar: %s\n", nextvar);
    //strcat(command, " "); // automatic space between parameters
    if (argi + 1 < argc &&
        ((strcmp(PROGHALFBONE, argv[argi]) == 0) ||
	 (strcmp(PROGHALFLONE, argv[argi]) == 0) ||
	 (strcmp(PROGHALFDONE, argv[argi]) == 0)  ) &&
	((strncmp(PROGHALFBTWO, argv[argi + 1], PROGHALFTWOLEN) == 0) ||
         (strncmp(PROGHALFDTWO, argv[argi + 1], PROGHALFTWOLEN) == 0) )) {
      nextvar = &argv[argi + 1][PROGHALFTWOLEN];
      strcat(command, PROGWHOLE);
      argi += 1; // replace C:/Program Files/ with C:/PROGRA~1/
    } else if (argi + 2 < argc &&
        ((strcmp(DOCSTRIPBONE, argv[argi]) == 0) ||
	 (strcmp(DOCSTRIPLONE, argv[argi]) == 0) ||
	 (strcmp(DOCSTRIPDONE, argv[argi]) == 0)  ) &&
	(strcmp(DOCSTRIPTWO, argv[argi + 1]) == 0 ) &&
	((strncmp(DOCSTRIPBTHREE, argv[argi + 2], DOCSTRIPTHREELEN) == 0) ||
         (strncmp(DOCSTRIPDTHREE, argv[argi + 2], DOCSTRIPTHREELEN) == 0) )) {
      nextvar = &argv[argi + 2][DOCSTRIPTHREELEN];
      strcat(command, DOCSWHOLE);
      argi += 2; // replace C:/Documents and Settings/ with C:/DOCUME~1/
    } else if (argi < argc && (strncmp(PROGLONG, argv[argi], PROGLONGLEN) == 0)) {
      nextvar = &argv[argi][PROGLONGLEN];
      strcat(command, PROGWHOLE);
    } else if (argi < argc && (strncmp(DOCSLONG, argv[argi], DOCSLONGLEN) == 0)) {
      nextvar = &argv[argi][DOCSLONGLEN];
      strcat(command, DOCSWHOLE);
    } // ^ also replace prefixes (might help with all-as-one-parameter-ness)
    char *acp = &command[0];
    while (*acp != '\0') { acp++; } // assumes valid command string !
//    commandend = acp;
    char *avp = nextvar;
    char lavp = '\0';
    while (*avp != '\0') {
//      if (*avp == '\\') {
//        //lavp = '\\';
//	*avp = '\\';
//	acp++;
//	*acp = '\0';
  //  	printf("BACKSLASH\n");
  //     } else 
  // hopefully can pass \\n ? is this the cause of the segfault? no..
      // maybe important for \\n but may break other literals .... pass as \  no ...
//      if (*avp == '\n') {
//        if (lavp == '\\') { *acp = ' '; acp++; *acp = '\0'; } // one extra space per linebreak
//        else { printf("LINEBREAK IN COMMAND\n"); } // currently allowing tabs et al
//      } else 
      // " came in as \", forward as \" ....
      // \ came in as \\, forward as \\ ....
      if (*avp == '\\') {
        *acp = '\\';
	acp++;
	*acp = '\\';
	acp++;
	*acp = '\0';
      } else if (*avp == '"') {
        //*acp = '\'';
	*acp = '\\';
	acp++;
// READDED::: REMOVED: escaping of quote !!!!! 28-11-2020-22:30
	*acp = '\"';
	acp++;
	*acp = '\0';
      } else if (*avp == '<' || *avp == '>') {
        // quotes seem to get eaten so '<' and '>' need to be passed as literal
	// as well as any other char that needs to be quoted to be passed
	// (unless quotes can be recovered, seemingly they can't) --
	// unless pre-escaped, but if the quote is escaped then < and > become
	// literal again! so " needs to become "\\\" and \\\""
	*acp = '\\';
	acp++;
	*acp = *avp; // '<' or '>'
	acp++;
	*acp = '\0';
      } else if (*avp == ' ') {
   //     *acp = '\\'; // spaces after -c are literal
   //	acp++;
	*acp = ' ';
	acp++;
	*acp = '\0';
        //for now: all spaces, later: keep spaces that are in quotes (?)
        //if (lavp != ' ') { *acp = ' '; acp++; *acp = '\0'; } // one space per many spaces
      } else { // all else is allowed
        *acp = *avp;
        acp++;
        *acp = '\0';
      }
//      commandend = acp; // in case of increment
      lavp = *avp;
      avp++;
    }
    argi++;
  }
  // extra NULL to end the parameter list
//  commandend++; // assuming prior param ended in NUL
//  *commandend = (char)NULL; // presumably this will work, translated as (char)0
  //if (isshell) { strcat(command, "\""); } // add an end quote
  strcat(command, "\""); // add an end quote
  if (!csilence) {
    printf("%s: [%s]\n", (isshell) ? "SHELL" : "PROGRAM", command);
  }
  fflush(stdout); // hopefully can see log ?
  fflush(stderr); // and err ?

  const // required for execve to _spawnve switch
  char *const newargv[] = { "sh", "-c", command, NULL };
  const
  char *const newenvp[] = { // CC="/mingw33/bin/gcc.exe",
//                            "PATH=C:/Progra~1/Git/mingw33/bin"
//			        ":C:/Progra~1/Git/mingw32/bin"
//			        ":C:/Progra~1/Git/usr/bin", NULL };
                            "PATH=/mingw33/bin:/usr/bin", NULL };
// CC needs to be absolute path but PATH needs to be linux short path
// INSTALL needs an absolute path too. anything called by process_begin: CreateProcess()
// THERE IS NO FORK SYSTEM CALL ON WINDOWS

//  pid_t pid;
//  if ((pid = fork()) == -1) {
//    printf("ERROR OF FORK\n");
//    return 0;
//  } else if (pid == 0) {
//  execve(SHELLWHOLE, newargv, newenvp);
//  printf("RETURN NOT EXPECTED AFTER EXECVE\n");
//  fflush(stdout);
  // execve spawns a new thread without waiting for it, so
  // try spawnve instead. actually, _spawnve. _wspawnve later.
  int ret = _spawnve(_P_WAIT, SHELLWHOLE, newargv, newenvp);
  if (ret) { // PS is short for ProgramSHELL
    fflush(stderr); // only flush stderr if nonzero return code
    printf("PS returned error code %d.\n", ret);
  } // stderr may dump after but don't pass an error as a result
  fflush(stdout); // maybe it'll flush itself but no harm to check
  return 0;
//  }
//  FILE *f = popen(command, "r");
//  if (!f) { printf("open fail\n"); }
//  else {
//    while (1) {
//      int ch = fgetc(f);
//      if (ch == EOF) { break; }
//      printf("%c", ch);
//    }
//    pclose(f);
//  }
//  return 0;
}
#endif

