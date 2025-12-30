/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey{
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP, 
  ARROW_DOWN
};

/*** data ***/
struct editorConfig{
  int cx,cy;
  int screenrows;
  int screencols;

  struct termios orig_termios; //acts as the template struct to use (global variable)
};
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

int editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO,&c,1)) != 1){
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  if(c == '\x1b'){
    char seq[3];

    if (read(STDIN_FILENO,&seq[0],1) != 1) 
      return '\x1b';
    if (read(STDIN_FILENO,&seq[1],1) != 1)
      return '\x1b';

    if (seq[0] == '['){
      switch (seq[1]){
        case 'A':
          return 'w';
        case 'B':
          return 's';
        case 'C':
          return 'd';
        case 'D':
          return 'a';
      }
    }

    return '\x1b';
  }
  else{
  return c;
  }
}

int getCursorPosition(int *rows,int *cols){
  char buf[32];
  unsigned int i = 0;

  if(write(STDOUT_FILENO,"\x1b[6n",4) != 4) 
    return -1;

  while(i<sizeof(buf)-1){
    if(read(STDIN_FILENO,&buf[i],1) != 1)
      break;
    if(buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';
  
  if (buf[0] !='\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2],"%d;%d",rows,cols)!=2)
    return -1;

  return 0;
}

int getWindowSize(int *rows, int *cols){
  struct winsize ws;

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B",12) != 12) 
      return -1;
    return getCursorPosition(rows,cols);

    } else{
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** append buffer ***/
struct abuf{
  char *b;
  int len;
};

#define ABUF_INIT {NULL,0} // acts as constructor for abuf type

void abAppend(struct abuf *ab, const char *s,int len){
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) 
    return;
  memcpy(&new[ab->len],s,len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab){
  free(ab->b);
}

/*** output ***/

void editorDrawRows(struct abuf *ab){
  int y;
  for(y=0;y<E.screenrows;y++){
    if(y==E.screenrows/3){
      char welcome[80];
      int welcomelen = snprintf(welcome,sizeof(welcome),
                                "Jel");
      if (welcomelen > E.screencols)
        welcomelen = E.screencols;
      //logic to center the message
      int padding = (E.screencols - welcomelen) / 2;
      if (padding){
        abAppend(&ab,"%",1);
        padding--;
      }
      while(padding--)
        abAppend(&ab,"",1);
      abAppend(&ab,welcome,welcomelen);
    }
    else{
      abAppend(&ab,"%",1);
    }
    abAppend(&ab, "\x1b[K",3);
    if (y < E.screenrows -1){
      abAppend(ab,"\r\n", 2);
    }
  }
}

void editorRefreshScreen(){
  struct abuf ab = ABUF_INIT;
  
  abAppend(&ab,"\x1b[?25l",6);
  abAppend(&ab,"\x1b[H",3);

  editorDrawRows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf),"\x1b[%d;%dH", E.cy+1,E.cx+1); //use 1-index
  abAppend(&ab,buf,strlen(buf));

  abAppend(&ab,"\x1b[?25h",6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}                                    

/*** input ***/

void editorMoveCursor(int key){
  switch(key){
    case ARROW_UP:
      E.cy--;
      break;
    case ARROW_LEFT:
      E.cx--;
      break;
    case ARROW_DOWN:
      E.cy++;
      break;
    case ARROW_RIGHT:
      E.cx++;
      break;
  }
}

void editorProcessKeypress(){
  int c = editorReadKey();

  switch(c){
    case CTRL_KEY('q'):
      write(STDOUT_FILENO,"\x1b[2J",4);
      write(STDOUT_FILENO,"\x1b[H",3);
      exit(0);
      break;

    case ARROW_UP:
    case ARROW_LEFT:
    case ARROW_DOWN:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;
  }
}

/*** init ***/

void initEditor(){
  E.cx = 0;
  E.cy = 0;

  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}
int main(){
  enableRawMode();
  initEditor();

  while (1){
    editorProcessKeypress();
    editorRefreshScreen();
  }

  return 0;
}
