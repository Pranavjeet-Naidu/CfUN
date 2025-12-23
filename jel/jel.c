#include <unistd.h>
#include <termios.h>

void enableRawMode(){
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw); // reads current attributes into a struct
  raw.c_lflag &= ~(ECHO); //ECHO to flush to terminal,disable it 
                          //c_lflag = local flag basically
  tcsetattr(STDIN_FILENO,TCSAFLUSH, &raw); //TCSAFLUSH->when to apply the attribute change
}
int main(){
  enableRawMode();
  char c;
  // by default terminal is in cooked/canonical mode ( input sent only when user presses ENTER) , so use backspace to fix error
  // raw mode is when each keypress is sent one by one 
  // right now we are in canonical so we can make it so that pressing q and then ENTER quits the program 
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
  return 0;
}
