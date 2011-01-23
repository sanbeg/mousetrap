#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdlib.h>

sig_atomic_t lastsig=0;
void handle(int sig)
{
  lastsig=sig;
}

void run(const char *arg) 
{
  pid_t pid;
  int proc_stat;
  switch(pid=fork()) 
    {
    case -1:
      perror("fork");
      exit(1);
    case 0:
      execl("/usr/bin/xinput", 
	    "xinput", 
	    "set-prop",
	    "SynPS/2 Synaptics TouchPad",
	    "Device Enabled",
	    arg, (char*)0);
      perror("xinput");
      exit(1);
    default:
      waitpid(pid,&proc_stat,0);
      printf ("Reap %d\n", WEXITSTATUS(proc_stat));
      
    }
}

int main (int ac, char ** argv) 
{
  const char *pid_file=geteuid()?"mousetrap.pid":"/var/run/mousetrap.pid";

  //const char *cmd_e="/usr/bin/xinput set-prop 'SynPS/2 Synaptics TouchPad' 'Device Enabled' 1";
  //const char *cmd_d="/usr/bin/xinput set-prop 'SynPS/2 Synaptics TouchPad' 'Device Enabled' 0";
  const char *cmd_d="/usr/bin/synclient TouchPadOff=1";
  const char *cmd_e="/usr/bin/synclient TouchPadOff=0";
  
  int optc, op=0, nextsig=0;
  enum 
  {
    O_NOP=0x0,
    O_KILL=0x1,
    O_START=0x2,
    O_V=0x4,
  };
  
  while ((optc=getopt(ac, argv, "sdekv"))>0) 
    {
      switch(optc)
	{
	case 's':
	  op |= O_START;
	  break;
	case 'k':
	  op |= O_KILL;
	  nextsig=SIGTERM;
	  break;
	case 'e':
	  op |= O_KILL;
	  nextsig=SIGUSR1;
	  break;
	case 'd':
	  op |= O_KILL;
	  nextsig=SIGUSR2;
	  break;
	case 'v':
	  op |= O_V;
	  break;
	  
	default:
	  fprintf (stderr, "Unknown option\n");
	  break;
	}
    }
  
  if (op & O_KILL) 
    {
      FILE * pidf=fopen(pid_file,"r");
      if (! pidf) 
	{
	  perror(pid_file);
	  return 1;
	}
      
      pid_t pid;
      fscanf(pidf,"%d", &pid);
      
      if (op&O_V) printf("%d\n",pid);
      if (kill (pid, nextsig))
	{
	  perror("Failed to signal server");
	}
    }
  if (op & O_START) 
    {
      FILE * pidf=fopen(pid_file,"r");
      if (pidf) 
	{
	  pid_t pid;
	  fscanf(pidf, "%d", &pid);
	  
	  if (! kill (pid, 0))
	    {
	      fprintf (stderr, "Another process exists: %d\n",pid);
	      return 1;
	    }
	  
	  fclose(pidf);
	}

      pidf=fopen(pid_file, "w");
      
      if (! pidf) 
	{
	  perror(pid_file);
	  return 1;
	}
      fprintf(pidf,"%d\n", getpid());
      fclose(pidf);

      //setuid(getuid());

      signal(SIGUSR1, handle);
      signal(SIGUSR2, handle);
      signal(SIGTERM, handle);
      signal(SIGINT, handle);
      
      for(;;) 
	{
	  pause();
	  if (lastsig==SIGUSR1)
	    {
	      if (op&O_V) puts("on");
	      sleep(1);
	      
	      system(cmd_e);
	      //run("1");
	    }
	  else if (lastsig==SIGUSR2) 
	    {
	      if (op&O_V) puts("off");
	      sleep(1);
	      
	      system(cmd_d);
	      //run("0");
	    }
	  else
	    {
	      unlink(pid_file);
	      printf("Exit on signal %d\n", lastsig);
	      break;
	    }
	  
	}
     }
}

