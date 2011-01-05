#include <stdio.h>
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
  XXX IMPLEMENT CHACHING. Max age should be at the end of current year.
  Header format: "Expires: Sat, 31 Dec 2011 23:59:59 GMT"
 */

char * jsondate(struct ln_date* date, char * buf);

int main(int argc, char **argv)
{
  double req_lat = 0, req_lng = 0;

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

  if(0) {

  } else {
    req_lat = DEFAULT_LAT;
    req_lng = DEFAULT_LNG;
  }

  observer.lat = req_lat;
  observer.lng = req_lng;

  jd = ln_get_julian_from_sys();  
  ln_get_date(jd, &date);

  date.years -= 2;
  date.months = 12;
  date.days = 31;
  date.hours = 12;
  date.minutes = 0;
  date.seconds = 0.0;
  start = ln_get_julian_day(&date);

  date.years += 3;
  date.months = 12;
  date.days = 30;
  end = ln_get_julian_day(&date);

  jd = start;

  puts(HTTP_HEADER);

  printf("[\n");

  /* XXX implement caching of results */

  for(i=(int)start; i < (int)end+1; i++) {

    ln_get_solar_rst(jd, &observer, &rst);
    ln_get_date(rst.rise, &rise);
    ln_get_date(rst.set, &set);
    ln_get_date(rst.transit, &transit);

    jd += 1.0;

    ln_get_date(jd, &date);

    printf("\t{\n");

    jsondate(&date, ds);
    printf("\t\t\"date\": %s,\n", ds);

    printf("\t\t\"sun\": {\n");

    jsondate(&rise, ds);
    printf("\t\t\t\"rise\": %s,\n", ds);
    jsondate(&transit, ds);
    printf("\t\t\t\"transit\": %s,\n", ds);
    jsondate(&set, ds);
    printf("\t\t\t\"set\": %s,\n", ds);

    ln_get_solar_equ_coords(rst.transit, &equ);
    ln_get_solar_geom_coords(rst.transit, &pos);

    ln_get_hrz_from_equ(&equ, &observer, rst.transit, &hrz);

    printf("\t\t\t\"alt\": %f,\"az\": %f\n", hrz.alt, hrz.az);

    printf("\t\t},\n");

    printf("\t\t\"moon\": {\n");

    /*ln_get_lunar_equ_coords (jd, &equ);
      ln_get_lunar_geo_posn(jd, &rect, 0.0005);*/

    /*ln_get_hrz_from_equ(&equ, &observer, rst.transit, &hrz);*/

    printf("\t\t\t\"phase\": %f\n", ln_get_lunar_phase(jd)); /*, rect.X, rect.Y, rect.Z); */ /* , hrz.alt, hrz.az); */

    /*
      Note: -> 180 new moon, -> 0 full moon.
      See also: http://www.usno.navy.mil/USNO/astronomical-applications/astronomical-information-center/phases-percent-moon 
     */

    printf("\t\t}\n");

    printf("\t}%c\n", i < ((int)end) ? ',' : ' ' );
  }

  printf("]");

  return 0;
}

char * jsondate(struct ln_date* date, char * buf)
{
  sprintf(buf, "{\"year\":%d,\"month\":%d,\"day\":%d,\"hour\":%d,\"minute\":%d,\"second\":%f}", date->years, date->months, date->days, date->hours, date->minutes, date->seconds);

  return buf;
}
