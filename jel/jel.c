/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>


/*** data ***/

struct termios orig_termios; //acts as the template struct to use (global variable)

/*** terminal ***/

//error handling , perror looks at 'errno' to get context
void die(const char*s){
  perror(s);
  exit(1);
}
void disableRawMode(){
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) die("tcsetattr");
}
void enableRawMode(){
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcsetattr");
  atexit(disableRawMode);

  struct termios raw = orig_termios; // use default struct again 
  raw.c_lflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO,TCSAFLUSH, &raw) == -1) die("tcsetattr"); 
}


/*** init ***/

int main(){
  enableRawMode();

  while (1){
    char c = '\0';
    if (read(STDIN_FILENO,&c,1) == -1 && errno != EAGAIN) die("read");
    if (iscntrl(c)){
      printf("%d\r\n",c);
    }else{
      printf("%d ('%c')\r\n", c, c);
    }
    if (c == 'q') break;
  }

  return 0;
}
