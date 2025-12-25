/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)
/*** data ***/
struct editorConfig{
  int screenrows;
  int screencols;
  struct termios orig_termios; //acts as the template struct to use (global variable)

}
struct editorConfig E;

/*** terminal ***/

//error handling , perror looks at 'errno' to get context
void die(const char*s){
  write(STDOUT_FILENO,"\x1b[2J",4); // to clear the screen on exit
  write(STDOUT_FILENO,"\x1b[H",3);

  perror(s);
  exit(1);
}
void disableRawMode(){
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) 
    die("tcsetattr");
}
void enableRawMode(){
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) 
    die("tcsetattr");
  atexit(disableRawMode);

  struct termios raw = E.orig_termios; // use default struct again 
  raw.c_lflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO,TCSAFLUSH, &raw) == -1) die("tcsetattr"); 
}

char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO,&c,1)) != 1){
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  return c;
}

int getWindowSize(){
  struct winsize ws;

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
    return -1;
  } else{
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}
/*** output ***/

void editorDrawRows(){
  int y;
  for(y=0;y<E.screenrows;y++){
    write(STDOUT_FILENO,"%\r\n",3);
  }
}

void editorRefreshScreen(){
  write(STDOUT_FILENO,"\x1b[2J",4);//x1b [ J -> 27 followed by [ , then J to clear screen , 2 for entire screen , this is from VT100 escape sequences
  write(STDOUT_FILENO,"\x1b[H",3); // the [ is used for escape chars, H is for positioning the cursor ,

  editorDrawRows();
  write(STDOUT_FILENO,"\x1b[H",3);
}                                    

/*** input ***/

void editorProcessKeypress(){
  char c = editorReadKey();

  switch(c){
    case CTRL_KEY('q'):
      exit(0);
      break;
  }
}

/*** init ***/

void initEditor(){
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize")
}
int main(){
  enableRawMode();
  initEditor();
  while (1){
    editorProcessKeypress();
  }

  return 0;
}
