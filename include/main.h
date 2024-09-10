#ifndef MAIN_H
#define MAIN_h

#include <unordered_set>
#include <string>
#include <vector>
#include "button.h"
#include "widget.h"

extern std::unordered_set<std::string> hash_set;
extern std::vector<Button> scanButtons, sideButtons;
extern std::vector<Widget> scanRects, sideRects;

#endif