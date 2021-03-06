#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <math.h>
#include <libnova/solar.h>
#include <libnova/lunar.h>
#include <libnova/julian_day.h>
#include <libnova/transform.h>
#include <libnova/utility.h>

/*
  Return rise, transit and set times of the sun for previous,
  current and next month. Also include current and at-transit
  sun altitude, as well as current moon phase and altitude.
*/

/* Default location (Etu-Töölö, Helsinki, Finland) is used if none is provided. */
#define DEFAULT_LNG 24.92365
#define DEFAULT_LAT 60.17406

/*
  The following file is generated at build time (see Makefile). It
  includes MD5 checksum of this file (i.e. "ephemeris.c"), and is
  used in caching.
 */
#include "fingerprint.h"

/* Files older than MAX_AGE seconds are not served from cache. */
#define MAX_AGE (60)
#define CACHE (0)

char * jsondate(struct ln_date* date, char * buf);

int main(int argc, char **argv)
{
  double req_lat = DEFAULT_LAT, req_lng = DEFAULT_LNG;

  char ds[1024];

  double jd, start, end, tmp;
  struct ln_lnlat_posn observer;
  struct ln_date date, rise, set, transit;
  struct ln_rst_time rst;
  struct ln_equ_posn equ;
  struct ln_helio_posn pos;
  struct ln_hrz_posn hrz;
  double moonphase, prevmoonphase;

  int i;

  char filename[PATH_MAX+1];
  FILE *fp;
  struct stat filestat;
  struct timeval now;
  int n;
  char buf[1024];

  char * query = getenv("QUERY_STRING"); 
  char * pair;
  char * key;
  double value;

  if(query && strlen(query) > 0) {
    pair = strtok(query, "&");
    while(pair) {
      key = (char *)malloc(strlen(pair)+1);
      sscanf(pair, "%[^=]=%lf", key, &value);
      if(!strcmp(key, "lat")) {
	req_lat = value;
      } else if(!strcmp(key, "lng")) {
	req_lng = value;
      }
      free(key);
      pair = strtok((char *)0, "&");
    }    
  }

  observer.lat = req_lat;
  observer.lng = req_lng;

  if(CACHE) {
    sprintf(filename, "/tmp/ephemeris-%s/%f-%f", fingerprint, req_lat, req_lng);
    if(stat(filename, &filestat) == 0 && (fp = fopen(filename, "r")) != NULL) {
      gettimeofday(&now, NULL);
      if((now.tv_sec - filestat.st_mtime) > MAX_AGE) {
	fclose(fp);
      } else {
	while((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
	  fwrite(buf, 1, n, stdout);
	}
	fclose(fp);
	return 0;
      }
    }
  }
  
  sprintf(filename, "/tmp/ephemeris-%s", fingerprint);
  mkdir(filename, 0700);
  sprintf(filename, "/tmp/ephemeris-%s/%f-%f", fingerprint, req_lat, req_lng);
  if((fp = fopen(filename, "w")) == NULL) {
    return 1;
  }

  jd = ln_get_julian_from_sys();

  ln_get_date(jd, &date);

  date.months -= 1;
  date.days = 1;
  start = ln_get_julian_day(&date);

  jd = start;

  date.months += 3;
  date.days = 1;
  end = ln_get_julian_day(&date) - 2.0;

  fprintf(fp, "[\n");

  for(i=(int)start; i <= (int)end+1; i++) {
    ln_get_date(jd, &date);    
    
    ln_get_solar_rst(jd, &observer, &rst);
    ln_get_date(rst.rise, &rise);

    if(rise.days != date.days) {
      ln_get_solar_rst(jd-1.0, &observer, &rst);
    }

    ln_get_date(rst.rise, &rise);
    ln_get_date(rst.set, &set);
    ln_get_date(rst.transit, &transit);

    fprintf(fp, "\t{\n");

    ln_get_date(jd, &date);    
    jsondate(&date, ds);
    fprintf(fp, "\t\t\"now\": { %s },\n", ds);
    
    fprintf(fp, "\t\t\"sun\": {\n");

    ln_get_solar_equ_coords(jd-0.00001, &equ);
    ln_get_hrz_from_equ(&equ, &observer, jd-0.00001, &hrz);
    tmp = hrz.alt;

    ln_get_solar_equ_coords(jd, &equ);
    ln_get_hrz_from_equ(&equ, &observer, jd, &hrz);

    fprintf(fp, "\t\t\t\"alt\": %f, \"rising\": %s,\n", hrz.alt, (hrz.alt-tmp) > 0 ? "true" : "false");

    jsondate(&rise, ds);
    fprintf(fp, "\t\t\t\"rise\": { %s },\n", ds);

    ln_get_solar_equ_coords(rst.transit, &equ);
    ln_get_solar_geom_coords(rst.transit, &pos);
    ln_get_hrz_from_equ(&equ, &observer, rst.transit, &hrz);
    jsondate(&transit, ds);
    fprintf(fp, "\t\t\t\"transit\": { %s, \"alt\": %f },\n", ds, hrz.alt);

    jsondate(&set, ds);
    fprintf(fp, "\t\t\t\"set\": { %s }\n", ds);

    fprintf(fp, "\t\t},\n");

    /*  0° Full moon
       45° Waning gibbous
       90° Last quarter
      135° Waning crescent
      180° New moon
      135° Waxing crescent
       90° First quarter
       45° Waxing gibbous
        0° Full moon

      http://www.usno.navy.mil/USNO/astronomical-applications/astronomical-information-center/phases-percent-moon 
      http://en.wikipedia.org/wiki/Phase_angle_(astronomy)
    */

    prevmoonphase = ln_get_lunar_phase(jd-0.00001);
    moonphase = ln_get_lunar_phase(jd);

    ln_get_lunar_equ_coords(jd-0.00001, &equ);
    ln_get_hrz_from_equ(&equ, &observer, jd-0.00001, &hrz);
    tmp = hrz.alt;

    ln_get_lunar_equ_coords(jd, &equ);
    ln_get_hrz_from_equ(&equ, &observer, jd, &hrz);

    fprintf(fp, "\t\t\"moon\": { \"phase\": %f, \"waxing\": %s, \"alt\": %f, \"rising\": %s }\n", moonphase, (moonphase-prevmoonphase) < 0 ? "true" : "false", hrz.alt, (hrz.alt-tmp) > 0 ? "true" : "false");

    fprintf(fp, "\t}%c\n", i < ((int)end+1) ? ',' : ' ' );

    jd += 1.0;
  }

  fprintf(fp, "]");

  fclose(fp);

  if((fp = fopen(filename, "r")) == NULL) {
    return 1;
  }
  while((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
    fwrite(buf, 1, n, stdout);
  }
  fclose(fp);

  return 0;
}
 
char * jsondate(struct ln_date* date, char * buf)
{
  double jd = ln_get_julian_day(date);
  
  sprintf(buf, "\"jd\": %f, \"year\": %d, \"month\": %d, \"day\": %d, \"hour\": %d, \"minute\": %d, \"second\": %f", jd, date->years, date->months, date->days, date->hours, date->minutes, date->seconds);

  return buf;
}
