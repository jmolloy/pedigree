#ifndef INTERFACE_H
#define INTERFACE_H

class Interface
{
public:
  Interface();
  ~Interface();

  virtual void start(char argc, char **argv) = 0;
};

#endif
