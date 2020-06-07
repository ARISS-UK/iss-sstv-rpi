#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <asm/errno.h>

#include "predict.h"

struct tle_t {
  char *elements[2];
};

#define TXT_NORM "\x1B[0m"
#define TXT_RED  "\x1B[31m"
#define TXT_GRN  "\x1B[32m"
#define TXT_YEL  "\x1B[33m"
#define TXT_BLU  "\x1B[34m"
#define TXT_MAG  "\x1B[35m"
#define TXT_CYN  "\x1B[36m"
#define TXT_WHT  "\x1B[37m"

#define OBSERVER_LATITUDE         50.0524
#define OBSERVER_LONGITUDE        -5.1823
#define OBSERVER_ALTITUDE         100

#define RADIANS_FROM_DEGREES(x)   (x*(M_PI/180.0))
#define DEGREES_FROM_RADIANS(x)   (x*(180.0/M_PI))

#define ISS_ELEVATION_THRESHOLD   -3 // degrees elevation

uint64_t timestamp_ms(void)
{
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return ((uint64_t) spec.tv_sec) * 1000 + (((uint64_t) spec.tv_nsec) / 1000000);
}

void timestamp_ms_to_string(char *destination, uint32_t destination_size, uint64_t timestamp_ms)
{
  uint64_t timestamp;
  struct tm tim;

  if(destination_size < 25)
  {
    return;
  }

  timestamp = timestamp_ms / 1000;

  gmtime_r((const time_t *)&timestamp, &tim);

  strftime(destination, 20, "%Y-%m-%d %H:%M:%S", &tim);
  snprintf(destination+19, 6, ".%"PRIu64"Z", (timestamp_ms - (timestamp * 1000)));
}

int tle_load(char *_filename, struct tle_t *_tle)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(_filename, "r");
    if (fp == NULL)
    {
        return -1;
    }

    int n = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        if(read > 50)
        {
            if(_tle->elements[n] != NULL)
            {
                free(_tle->elements[n]);
            }
            _tle->elements[n] = malloc(read);
            strncpy(_tle->elements[n], line, read);
            n++;
            if(n > 1)
            {
                break;
            }
        }
    }

    fclose(fp);
    if (line)
        free(line);
    
    if(n != 2)
    {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
  struct tle_t tle = {
    .elements = { NULL, NULL }
  };
  predict_orbital_elements_t iss_tle;
  struct predict_position iss_orbit;
  predict_observer_t obs;
  struct predict_observation iss_observation;
  struct predict_sgp4 sgp;

  if(argc != 2)
  {
    printf("iss-barrier, barrier for ISS visibility in the next 10 minutes\n\n"
      "Usage:\tiss-barrier <tle filename>\n"
      );
    exit(1);
  }

  /*** Load TLE ***/

  printf("Loading TLE from file..   ");

  if(tle_load(argv[1], &tle) < 0)
  {
    printf(TXT_RED"Error loading TLE from file!"TXT_NORM"\n");
    exit(1);
  }
  printf(TXT_GRN"OK"TXT_NORM"\n");

  /*** Parse TLE ***/

  printf("Parsing ISS TLE..         ");

  if(!predict_parse_tle(&iss_tle, tle.elements[0], tle.elements[1], &sgp, NULL))
  {
    printf(TXT_RED"Error!"TXT_NORM"\n");
    exit(1);
  }
  printf(TXT_GRN"OK"TXT_NORM" (Epoch: 20%01d.%2.2f)\n",
    iss_tle.epoch_year,
    iss_tle.epoch_day
  );

  /*** Create Observer ***/

  predict_create_observer(&obs, "Self", RADIANS_FROM_DEGREES(OBSERVER_LATITUDE), RADIANS_FROM_DEGREES(OBSERVER_LONGITUDE), OBSERVER_ALTITUDE);
  printf("Observer Position:        %+.4f°N, %+.4f°E, %+dm\n", OBSERVER_LATITUDE, OBSERVER_LONGITUDE, OBSERVER_ALTITUDE);

  /*** Run model ***/

  printf("Checking 10 minutes..     ");
  uint64_t predict_timestamp = timestamp_ms();
  uint64_t timestamp_increment;
  for(timestamp_increment = 0; timestamp_increment < 10*60*1000; timestamp_increment+=1000)
  {
    if(predict_orbit(&iss_tle, &iss_orbit, julian_from_timestamp_ms(predict_timestamp + timestamp_increment)) < 0)
    {
      printf(TXT_RED"Error predicting orbit!"TXT_NORM"\n");
      exit(1);
    }
    predict_observe_orbit(&obs, &iss_orbit, &iss_observation);
    if(DEGREES_FROM_RADIANS(iss_observation.elevation) > ISS_ELEVATION_THRESHOLD)
    {
      /* ISS too high right now! */
      char timestamp_string[25];
      timestamp_ms_to_string(timestamp_string, 25, predict_timestamp+timestamp_increment);
      printf(TXT_RED"ISS > %d at %s"TXT_NORM"\n", ISS_ELEVATION_THRESHOLD, timestamp_string);
      exit(1);
    }
  }
  printf(TXT_GRN"OK"TXT_NORM"\n");

  exit(0);
}

