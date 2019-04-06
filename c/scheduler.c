/*
Job scheduler that allows full customization
over scheduling abilities and formatting.
Not intended as a cron replacement, but
to allow full customization of scheduling
behavior.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

// Maximum line length (for memory allocation when reading the file)
#define LINE_LEN 1000
#define PATH_LEN 100

// Function to write a slice of a string `str` into memory at the address of `buffer`,
// slicing from index `start` to index `end`
void slice_str(const char *str, char *buffer, size_t start, size_t end);

// Main
int main(int argc, char *argv[])
{
  if !(argc <= 2)
  {
    printf("Usage: cron2 [schedtabpath]");
    exit(EXIT_FAILURE);
  }

  pid_t pid, sid;

  // Fork the child process and check return for success
  pid = fork();
  if (pid < 0)
  {
    // TODO add logging on failure
    exit(EXIT_FAILURE);
  }

  if (pid > 0)
  {
    exit(EXIT_SUCCESS);
  }

  // Change the file mode mask so we can write to a log
  umask(0);

  // Create an SID for the child process and check for validity
  sid = setsid();
  if (sid < 0)
  {
    // TODO add logging on failure
    exit(EXIT_FAILURE);
  }

  // Change to a directory we know exists (i.e., root)
  if ((chdir("/")) < 0)
  {
    // TODO log failure
    exit(EXIT_FAILURE);
  }

  // Close file descriptors
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  // Read in crontab file from $HOME/repos/data_team/fa_duct_tape/fa_jobs.txt
  // File pointer
  FILE *fp = NULL;

  // System independent path to file
  char *filepath;
  filepath = malloc(PATH_LEN * sizeof(char));

  // Point to user specified .schedtab file if given, otherwise default to $HOME
  if (argc == 2)
  {
    sprintf(filepath, "%s", argv[1]);
  }
  else
  {
    sprintf(filepath, "%s/.schedtab", getenv("HOME"));
  }

  // Begin our forever loop
  while (1)
  {
    fp = fopen(filepath, "r");

    if (fp != NULL)
    {
      // Line buffer
      char buffer[LINE_LEN];

      // Read line by line
      while(fgets(buffer, LINE_LEN, fp) != NULL)
      {
        // Array to hold the schedule
        char sched[14];

        // Skip the line if it's commented
        if (buffer[0] != '#')
        {

          // Copy the line (because strtok alters in place)
          char bcpy[LINE_LEN];
          strcpy(bcpy, buffer);

          // Split the input line on whitespace and store into time variables
          char *pch;
          pch = strtok(bcpy, " ");
          int i = 0;
          char minute[6], hr[6], day[6], mth[6], wkday[4];
          while (i < 5)
          {
            if (i == 0)
            {
              strcpy(minute, pch);
            }
            else if (i == 1)
            {
              strcpy(hr, pch);
            }
            else if (i == 2)
            {
              strcpy(day, pch);
            }
            else if (i == 3)
            {
              strcpy(mth, pch);
            }
            else if (i == 4)
            {
              strcpy(wkday, pch);
            }
            pch = strtok(NULL, " ");
            i++;
          }

          // Get the scheduled command
          int j = 0, count = 0;
          while (count < 5)
          {
            if (buffer[j] == ' ')
            {
              count++;
            }
            j++;
          }
          char command[LINE_LEN];
          slice_str(buffer, command, j, strlen(buffer));

          // Get the current time and date
          time_t theTime = time(NULL);
          struct tm *aTime = localtime(&theTime);
          int currMinute = aTime->tm_min;
          int currHr = aTime->tm_hour;
          int currDay = aTime->tm_mday;
          int currMth = aTime->tm_mon + 1; // localtime() returns months in the range 0-11
          int currWkday = aTime->tm_wday;

          char *cPos;
          // Split minute on "-" in case ranges are provided
          char lowerMinute[3], upperMinute[3];
          cPos = strchr(minute, '-');
          if (cPos != NULL)
          {
            strcpy(lowerMinute, strtok(minute, "-"));
            strcpy(upperMinute, strtok(NULL, "-"));
          }
          else
          {
            if (strcmp(minute, "*") == 0)
            {
              strcpy(lowerMinute, "0");
              strcpy(upperMinute, "59");
            }
            else
            {
              strcpy(lowerMinute, minute);
              strcpy(upperMinute, minute);
            }
          }

          // And hour...
          char lowerHr[3], upperHr[3];
          cPos = strchr(hr, '-');
          if (cPos != NULL)
          {
            strcpy(lowerHr, strtok(hr, "-"));
            strcpy(upperHr, strtok(NULL, "-"));
          }
          else
          {
            if (strcmp(hr, "*") == 0)
            {
              strcpy(lowerHr, "0");
              strcpy(upperHr, "23");
            }
            else
            {
              strcpy(lowerHr, hr);
              strcpy(upperHr, hr);
            }
          }

          // And day...
          char lowerDay[3], upperDay[3];
          cPos = strchr(day, '-');
          if (cPos != NULL)
          {
            strcpy(lowerDay, strtok(day, "-"));
            strcpy(upperDay, strtok(NULL, "-"));
          }
          else
          {
            if (strcmp(day, "*") == 0)
            {
              strcpy(lowerDay, "1");
              strcpy(upperDay, "31");
            }
            else
            {
              strcpy(lowerDay, day);
              strcpy(upperDay, day);
            }
          }

          // And month...
          char lowerMth[3], upperMth[3];
          cPos = strchr(mth, '-');
          if (cPos != NULL)
          {
            strcpy(lowerMth, strtok(mth, "-"));
            strcpy(upperMth, strtok(NULL, "-"));
          }
          else
          {
            if (strcmp(mth, "*") == 0)
            {
              strcpy(lowerMth, "1");
              strcpy(upperMth, "12");
            }
            else
            {
              strcpy(lowerMth, mth);
              strcpy(upperMth, mth);
            }
          }

          // And finally weekday...
          char lowerWkday[3], upperWkday[3];
          cPos = strchr(wkday, '-');
          if (cPos != NULL)
          {
            strcpy(lowerWkday, strtok(wkday, "-"));
            strcpy(upperWkday, strtok(NULL, "-"));
          }
          else
          {
            if (strcmp(wkday, "*") == 0)
            {
              strcpy(lowerWkday, "0");
              strcpy(upperWkday, "6");
            }
            else
            {
              strcpy(lowerWkday, wkday);
              strcpy(upperWkday, wkday);
            }
          }

          // Loop over all ranges of possible times
          for (int mthLoop = atoi(lowerMth); mthLoop <= atoi(upperMth); mthLoop++)
          {
            if (currMth == mthLoop)
            {
              for (int dayLoop = atoi(lowerDay); dayLoop <= atoi(upperDay); dayLoop++)
              {
                if (currDay == dayLoop)
                {
                  for (int wkdayLoop = atoi(lowerWkday); wkdayLoop <= atoi(upperWkday); wkdayLoop++)
                  {
                    if (currWkday == wkdayLoop)
                    {
                      for (int hrLoop = atoi(lowerHr); hrLoop <= atoi(upperHr); hrLoop++)
                      {
                        if (currHr == hrLoop)
                        {
                          for (int minuteLoop = atoi(lowerMinute); minuteLoop <= atoi(upperMinute); minuteLoop++)
                          {
                            if (currMinute == minuteLoop)
                            {
                              // Fork grandchild process that can execvp the job
                              pid_t pid2, sid2;
                              pid2 = fork();
                              if (pid2 == 0)
                              {
                                sid2 = setsid();

                                // Copy and split the command into constituent parts
                                char *comargv[COM_ARGS], *comch, comcpy[LINE_LEN];
                                strcpy(comcpy, command);
                                comch = strtok(comcpy, " ");
                                int k = 0;
                                while (comch != NULL)
                                {
                                    comargv[k] = pch;
                                    comch = strtok(NULL, " ");
                                    k++;
                                }

                                // Fill the remaining comargv values with NULL
                                while (k < COM_ARGS)
                                {
                                    comargv[k] = NULL;
                                    k++;
                                }

                                execvp(comargv[0], comargv);
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    free(filepath);
    sleep(60);
  }

  exit(EXIT_SUCCESS);
}

void slice_str(const char *str, char *buffer, size_t start, size_t end)
{
  size_t j = 0;
  size_t i;
  for (i = start; i <= end; ++i) {
    buffer[j++] = str[i];
  }
  buffer[j] = 0;
}
