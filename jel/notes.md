// by default terminal is in cooked/canonical mode ( input sent only when user presses ENTER) , so use backspace to fix error
  // raw mode is when each keypress is sent one by one
  // right now we are in canonical so we can make it so that pressing q and then ENTER quits the program

- in order to disable canonical mode , we can also use the ICANON flag as a c_lflag

- struct termios raw - basically a struct that is used to control the parameters of a terminal ( here , we call and refer it as raw )
  mostly has these members:
  tcflag_t c_iflag: Input modes (e.g., handling of break conditions, parity errors, flow control).
  tcflag_t c_oflag: Output modes (e.g., post-processing, newline translation, delays).
  tcflag_t c_cflag: Control modes (e.g., character size, stop bits, modem control, hardware flow control).
  tcflag_t c_lflag: Local modes (e.g., enabling signals, canonical mode, character echoing).

- tcsetattr(filedes,when,termios struct) -> function to set the attributes
we also have tcgetattr(filedes,termios struct)

  -here we use TCSAFLUSH for when -> basically makes changes to the struct after all the output is written and then discards the input

- iscntrl() -> test whether something is a control char(nonprintable chars)

-//ISIG is for Ctrl-C,Z
  //IEXTEN is for V,O
ICRNL -> Ctrl-M
ioctl() -> input output control function
TIOCGWINSZ-> terminal ioctl , get window size
