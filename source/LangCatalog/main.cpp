#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct language 
{
  std::string lang;
  std::string designer;
  int date;
};

int main()
{
  std::ifstream fs{};
  fs.open("languages.txt", std::fstream::in);

  if (!fs.is_open())
  {
    std::cerr << "languages.txt could not be opened";
    return EXIT_FAILURE;
  }

  std::vector<language> languageArray{};
  std::string str{};

  while (getline(fs, str))
  {
    language lan{};
    std::istringstream strStrm{str};
    strStrm >> lan.lang;

    std::string temp{};
    bool designerEmpty{ true };
    while(strStrm >> temp)
    {
      char* s{};
      int num{ static_cast<int>(std::strtol(temp.c_str(), &s, 10)) };
      if (*s)
      {
        lan.designer += designerEmpty ? temp : " " + temp;
        designerEmpty = false;
      }
      else
      {
        lan.date = num;
      }
    }

    languageArray.push_back(lan);
  }

  for (const language& lan : languageArray)
  {
    std::cout << lan.lang << "," << lan.designer << "," << lan.date << std::endl;
  }

  return EXIT_SUCCESS;
}