#include "time.h"

struct tm *_GetLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return &timeinfo;
  timeinfo.tm_min += 1;
  return &timeinfo;
}

void split(String t)
{
  std::vector<int> hours;
  char* cToken;
  t.toCharArray(cToken, t.length());
  char *token = strtok(cToken, ":");

  while (token)
  {
    token = strtok(NULL, ":");
    hours.push_back(atoi(token));
  }

  for (int h: hours)
    Serial.print(h + " ");

}

bool isClose(struct tm *pillTime, struct tm *current)
{
  unsigned int pillTimeMins = pillTime->tm_hour * 60 + pillTime->tm_min;
  unsigned int currentTimeMins = current->tm_hour * 60 + current->tm_min;
  if (abs(currentTimeMins - pillTimeMins) <= 30)
    return true;
  return false;
}
