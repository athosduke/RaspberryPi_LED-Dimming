#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include "import_registers.h"
#include "gpio.h"


struct io_peripherals
{
  uint8_t              unused[0x200000];
  struct gpio_register gpio;
};

struct thread_parameter
{
  volatile struct gpio_register * gpio;
  int                             pin;
};

struct thread_char
{
	int input;
};

int  Tstep = 50;  /* PWM time resolution, number used for usleep(Tstep) */ 

int quit = 0;
int red = 0;
int green = 0;
int blue = 0;
int orange = 0;

int get_pressed_key(void)
{
  struct termios  original_attributes;
  struct termios  modified_attributes;
  int             ch;

  tcgetattr( STDIN_FILENO, &original_attributes );
  modified_attributes = original_attributes;
  modified_attributes.c_lflag &= ~(ICANON | ECHO);
  modified_attributes.c_cc[VMIN] = 1;
  modified_attributes.c_cc[VTIME] = 0;
  tcsetattr( STDIN_FILENO, TCSANOW, &modified_attributes );

  ch = getchar();

  tcsetattr( STDIN_FILENO, TCSANOW, &original_attributes );

  return ch;
}
	
void *checkkey (void *arg)
{
	struct thread_char * parameter = (struct thread_char *)arg;
	do
    {
      switch (get_pressed_key())
      {
        case 'q':
          quit = 1;
	  break;
	
	case 'r':
	  if (red != 0)
	    red = 0;
	  else if (red == 0)
	    red = 1;
	  break;
	  
	case 'g':
	  if (green != 0)
	    green = 0;
	  else if (green == 0)
	    green = 1;
	  break;
	  
	case 'b':
	  if (blue != 0)
	    blue = 0;
	  else if (blue == 0)
	    blue = 1;
	  break;
	  
	case 'o':
	  if (orange != 0)
	    orange = 0;
	  else if (orange == 0)
	    orange = 1;
	  break;
		 
	default:
	  quit = 0;
	  break;
      }
    } while (quit == 0);
}
	
void DimLevUnit(int Level, int pin, volatile struct gpio_register *gpio)
{
  if (quit != 0)
	  pthread_exit(NULL);
  int ONcount, OFFcount;

  ONcount = Level;
  OFFcount = 100 - Level;

  /* create the output pin signal duty cycle, same as Level */
  GPIO_SET( gpio, pin ); /* ON LED at GPIO 18 */
  while (ONcount > 0)
  {
    usleep( Tstep );
    ONcount = ONcount - 1;
  }
  GPIO_CLR( gpio, pin ); /* OFF LED at GPIO 18 */
  while (OFFcount > 0)
  {
    usleep( Tstep );
    OFFcount = OFFcount - 1;
  }
}

void *ThreadSW( void * arg )
{
  int                       iterations; /* used to limit the number of dimming iterations */
  int                       Timeu;      /* dimming repetition of each level */
  int                       DLevel;     /* dimming level as duty cycle, 0 to 100 percent */
  int                       cmax;
  int                       clevel;
  struct thread_parameter * parameter = (struct thread_parameter *)arg;

  
  clevel = 3;
  cmax = 100;

  for (iterations = 0; iterations < 100; iterations++)
  {
    DLevel = 0;  /* dim up, sweep the light level from 0 to 100 */
    while(DLevel<cmax)
    {
      Timeu = clevel;   /* repeat the dimming level for 5 times if Tlevel = 5 */
      while(Timeu>0)
      {
        DimLevUnit(DLevel, parameter->pin, parameter->gpio);
        Timeu = Timeu - 1;
      }
      if (((parameter->pin == 18)&&(red == 0)) || 
	 ((parameter->pin == 19)&&(green == 0)) ||
	 ((parameter->pin == 22)&&(blue == 0)) ||
	 ((parameter->pin == 23)&&(orange == 0)) )
	DLevel = DLevel +1;
    }

    DLevel = cmax;  /* dim down, sweep the light level from 100 to 0 */
    while(DLevel>0)
    {
      Timeu = clevel;   /* repeat the dimming level for 5 times if Tlevel = 5 */
      while(Timeu>0)
      {
        DimLevUnit(DLevel, parameter->pin, parameter->gpio);
        Timeu = Timeu - 1;
      }
      DLevel = DLevel - 1;
    }
  }

  return 0;
}

int main( void )
{
  volatile struct io_peripherals *io;
  pthread_t                       thread18_handle;
  pthread_t                       thread19_handle;
  pthread_t                       thread22_handle;
  pthread_t                       thread23_handle;
  pthread_t                       thread_flag;
  struct thread_parameter         thread18_parameter;
  struct thread_parameter         thread19_parameter;
  struct thread_parameter         thread22_parameter;
  struct thread_parameter         thread23_parameter;
  struct thread_char              thread_flagpara;

  io = import_registers();
  if (io != NULL)
  {
    /* print where the I/O memory was actually mapped to */
    printf( "mem at 0x%8.8X\n", (unsigned long)io );
    printf( "press 'r''g''b''o'to stop or resume red/green/blue/orange light\n");
    printf( "press 'q' to quit the program\n");
    printf( "any other key will be ignored\n");
    /* set the pin function to GPIO for GPIO12, GPIO13, GPIO18, GPIO19 */
    io->gpio.GPFSEL2.field.FSEL2 = GPFSEL_OUTPUT;
    io->gpio.GPFSEL2.field.FSEL3 = GPFSEL_OUTPUT;
    io->gpio.GPFSEL1.field.FSEL8 = GPFSEL_OUTPUT;
    io->gpio.GPFSEL1.field.FSEL9 = GPFSEL_OUTPUT;

#if 0
    Thread18( (void *)io );
#else
    thread18_parameter.pin = 18;
    thread18_parameter.gpio = &(io->gpio);
    thread19_parameter.pin = 19;
    thread19_parameter.gpio = &(io->gpio);
    thread22_parameter.pin = 22;
    thread22_parameter.gpio = &(io->gpio);
    thread23_parameter.pin = 23;
    thread23_parameter.gpio = &(io->gpio);
	  thread_flagpara.input = 0;
	
    pthread_create( &thread18_handle, 0, ThreadSW, (void *)&thread18_parameter );
    pthread_create( &thread19_handle, 0, ThreadSW, (void *)&thread19_parameter );
    pthread_create( &thread22_handle, 0, ThreadSW, (void *)&thread22_parameter );
    pthread_create( &thread23_handle, 0, ThreadSW, (void *)&thread23_parameter );
	pthread_create( &thread_flag, 0 , checkkey, (void *)&thread_flagpara);
	pthread_join( thread_flag, 0 );
    pthread_join( thread18_handle, 0 );
    pthread_join( thread19_handle, 0 );
    pthread_join( thread22_handle, 0 );
    pthread_join( thread23_handle, 0 );
#endif

    io->gpio.GPFSEL2.field.FSEL2 = GPFSEL_INPUT;
    io->gpio.GPFSEL2.field.FSEL3 = GPFSEL_INPUT;
    io->gpio.GPFSEL1.field.FSEL8 = GPFSEL_INPUT;
    io->gpio.GPFSEL1.field.FSEL9 = GPFSEL_INPUT;
	printf("  Dimming light levels done. \n \n");

  }
  else
  {
    ; /* warning message already issued */
  }

  return 0;
}

