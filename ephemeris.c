#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <libnova/solar.h>
#include <libnova/lunar.h>
#include <libnova/julian_day.h>
#include <libnova/transform.h>
#include <libnova/utility.h>

/*
  Return solar and lunar rise, transit and set times for this year,
  previous year and the following year (i.e. if query takes place
  in 2011, respond with data for 2010, 2011 and 2012). Also include
  sun and moon altitude at their transits &c.
*/

/*
  Default location (Kotka, Finland) is used if none is provided.
*/
#define DEFAULT_LNG 26.94663
#define DEFAULT_LAT 60.46636

#define HTTP_HEADER "Content-Type: application/json;charset=utf-8\n\n"

/*
  The following file is generated at build time (see Makefile). It
  includes MD5 checksum of this file (ephemeris.c), and is used to
  implement caching.
 */
#include "fingerprint.h"

char * jsondate(struct ln_date* date, char * buf);

int main(int argc, char **argv)
{
  double req_lat = DEFAULT_LAT, req_lng = DEFAULT_LNG;

  char ds[1024];

  double jd, start, end;
  struct ln_lnlat_posn observer;
  struct ln_date date, rise, set, transit;
  struct ln_rst_time rst;
  struct ln_equ_posn equ;
  struct ln_helio_posn pos;
  struct ln_hrz_posn hrz;
  struct ln_rect_posn rect;

  int i;

  char filename[PATH_MAX+1];
  FILE *fp;
  int n;
  char buf[1024];

  char * query = getenv("QUERY_STRING"); 
  char * pair;
  char * key;
  double value;

  if(strlen(query) > 0) {
    pair = strtok(query,"&");
    while(pair) {
      key = (char *)malloc(strlen(pair)+1);
      sscanf(pair, "%[^=]=%lf", key, &value);
      if(!strcmp(key, "lat")) {
	req_lat = value;
      } else if(!strcmp(key, "lng")) {
	req_lng = value;
      }
      free(key);
      pair = strtok((char *)0,"&");
    }    
  }

  observer.lat = req_lat;
  observer.lng = req_lng;

  jd = ln_get_julian_from_sys();  
  ln_get_date(jd, &date);

  date.years -= 1;
  date.months = 1;
  date.days = 1;
  date.days -= 1;
  /*date.hours = 12;
  date.minutes = 0;
  date.seconds = 0.0;*/
  start = ln_get_julian_day(&date);

  date.years += 2;
  date.months = 12;
  date.days = 31;
  date.days -= 1;
  end = ln_get_julian_day(&date);

  jd = start;

  puts(HTTP_HEADER);

  sprintf(filename, "/tmp/ephemeris-%s", fingerprint);
  mkdir(filename, 0700);
  sprintf(filename, "/tmp/ephemeris-%s/%f-%f", fingerprint, req_lat, req_lng);
  fp = fopen(filename, "w");
  if(fp == NULL) {
    return 1;
  }

  fprintf(fp, "[\n");

  for(i=(int)start; i < (int)end+1; i++) {

    ln_get_solar_rst(jd, &observer, &rst);
    ln_get_date(rst.rise, &rise);
    ln_get_date(rst.set, &set);
    ln_get_date(rst.transit, &transit);
      
    jd += 1.0;

    fprintf(fp, "\t{\n");

    ln_get_date(jd, &date);    
    jsondate(&date, ds);
    fprintf(fp, "\t\t\"now\": { %s },\n", ds);
    
    fprintf(fp, "\t\t\"sun\": {\n");

    ln_get_solar_equ_coords(jd, &equ);
    ln_get_hrz_from_equ(&equ, &observer, jd, &hrz);

    fprintf(fp, "\t\t\t\"alt\": %f,\n", hrz.alt);

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

    /*
      Note: -> 180, new moon, -> 0, full moon.
      See also: http://www.usno.navy.mil/USNO/astronomical-applications/astronomical-information-center/phases-percent-moon 
     */
    jsondate(&date, ds);    
    fprintf(fp, "\t\tmoon: { \"phase\": %f }\n", ln_get_lunar_phase(jd));

    fprintf(fp, "\t}%c\n", i < ((int)end) ? ',' : ' ' );
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
  sprintf(buf, "\"year\": %d, \"month\": %d, \"day\": %d, \"hour\": %d, \"minute\": %d, \"second\": %f", date->years, date->months, date->days, date->hours, date->minutes, date->seconds);

  return buf;
}
