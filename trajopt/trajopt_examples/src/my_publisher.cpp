#include <ros/ros.h>

#include "trajopt_examples/my_msg.h"
#include <geometry_msgs/Point.h>
#include <signal.h>
#include <stdio.h>
#ifndef _WIN32
# include <termios.h>
# include <unistd.h>
#else
# include <windows.h>
#endif



#define KEYCODE_A 0x61
#define KEYCODE_Q 0x71

#define KEYCODE_Z 0x7a
#define KEYCODE_S 0x73

#define KEYCODE_E 0x65
#define KEYCODE_D 0x64

#define KEYCODE_ESCAPE 0x1B


class KeyboardReader
{
public:
  KeyboardReader()
#ifndef _WIN32
    : kfd(0)
#endif
  {
#ifndef _WIN32
    // get the console in raw mode
    tcgetattr(kfd, &cooked);
    struct termios raw;
    memcpy(&raw, &cooked, sizeof(struct termios));
    raw.c_lflag &=~ (ICANON | ECHO);
    // Setting a new line, then end of file
    raw.c_cc[VEOL] = 1;
    raw.c_cc[VEOF] = 2;
    tcsetattr(kfd, TCSANOW, &raw);
#endif
  }
  void readOne(char * c)
  {
#ifndef _WIN32
    int rc = read(kfd, c, 1);
    if (rc < 0)
    {
      throw std::runtime_error("read failed");
    }
#else
    for(;;)
    {
      HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
      INPUT_RECORD buffer;
      DWORD events;
      PeekConsoleInput(handle, &buffer, 1, &events);
      if(events > 0)
      {
        ReadConsoleInput(handle, &buffer, 1, &events);
        if (buffer.Event.KeyEvent.wVirtualKeyCode == VK_LEFT)
        {
          *c = KEYCODE_LEFT;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == VK_UP)
        {
          *c = KEYCODE_UP;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT)
        {
          *c = KEYCODE_RIGHT;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == VK_DOWN)
        {
          *c = KEYCODE_DOWN;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x42)
        {
          *c = KEYCODE_B;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x43)
        {
          *c = KEYCODE_C;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x44)
        {
          *c = KEYCODE_D;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x45)
        {
          *c = KEYCODE_E;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x46)
        {
          *c = KEYCODE_F;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x47)
        {
          *c = KEYCODE_G;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x51)
        {
          *c = KEYCODE_Q;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x52)
        {
          *c = KEYCODE_R;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x54)
        {
          *c = KEYCODE_T;
          return;
        }
        else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x56)
        {
          *c = KEYCODE_V;
          return;
        }
      }
    }
#endif
  }
  void shutdown()
  {
#ifndef _WIN32
    tcsetattr(kfd, TCSANOW, &cooked);
#endif
  }
private:
#ifndef _WIN32
  int kfd;
  struct termios cooked;
#endif
};

KeyboardReader input;

class TeleopTurtle
{
public:
  TeleopTurtle();
  void keyLoop();

  float X,Y,Z;
private:

  
  ros::NodeHandle nh_;
  ros::Publisher pub;
  
};

TeleopTurtle::TeleopTurtle():

  X(0.5),
  Y(0),
  Z(0.5)
{  
  nh_.param("x", X);
  nh_.param("y", Y);
  nh_.param("z", Z);
  pub = nh_.advertise<geometry_msgs::Point>("my_topic", 1);
}

void quit(int sig)
{
  (void)sig;
  input.shutdown();
  ros::shutdown();
  exit(0);
}






int main(int argc, char** argv)
{
  ros::init(argc, argv, "teleop_turtle");
  TeleopTurtle teleop_turtle;



  signal(SIGINT,quit);

  teleop_turtle.keyLoop();
  quit(0);
  
  return(0);
}

void TeleopTurtle::keyLoop()
{
  char c;
  bool dirty=false;

  

  for(;;)
  {
    // get the next event from the keyboard  
    try
    {
      input.readOne(&c);
    }
    catch (const std::runtime_error &)
    {
      perror("read():");
      return;
    }

    ROS_DEBUG("value: 0x%02X\n", c);
  
    switch(c)
    {
      case KEYCODE_A:
        ROS_DEBUG("X+");
        X += 0.01;
        dirty = true;
        break;
      case KEYCODE_Q:
        ROS_DEBUG("X-");
        X += -0.01;
        dirty = true;
        break;
      case KEYCODE_Z:
        ROS_DEBUG("Y+");
        Y += 0.01;
        dirty = true;
        break;
      case KEYCODE_S:
        ROS_DEBUG("Y-");
        Y += -0.01;
        dirty = true;
        break;
      case KEYCODE_E:
        ROS_DEBUG("Z+");
        Z += 0.01;
        dirty = true;
        break;
      case KEYCODE_D:
        ROS_DEBUG("Z-");
        Z += -0.01;
        dirty = true;
        break;  
      case KEYCODE_ESCAPE:
        ROS_DEBUG("quit");
        return;
    }
   

    geometry_msgs::Point msg;
    msg.x = X;
    msg.y = Y;
    msg.z = Z;

    if(dirty ==true)
    {
      pub.publish(msg);    
      dirty=false;
    }
  }


  return;
}
/*
int main(int argc, char **argv)
{
  ros::init(argc, argv, "my_publisher");
  ros::NodeHandle nh;
  ros::Publisher pub = nh.advertise<geometry_msgs::Point>("my_topic", 1);
  ros::Rate loop_rate(1);

  while (ros::ok())
  {
    geometry_msgs::Point msg;
    msg.x=0.5;
    msg.y=0;
    msg.z=0.5;
    pub.publish(msg);
    loop_rate.sleep();
  }
  return 0;
}*/

